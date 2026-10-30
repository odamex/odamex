// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Per-thread render context.
//
//	Split the screen into vertical slices, and render each of them
//	concurrently before compositing the results into the final frame.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <algorithm>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "r_context.h"

#include "c_cvars.h"
#include "doomstat.h"
#include "i_video.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"

EXTERN_CVAR(r_threads)
EXTERN_CVAR(r_showcontexts)

//
// R_RenderThreadCount
//
// When r_threads is 0, it resolves to one thread per hardware thread. The main
// thread counts as one, since it renders slices too.
//
int R_RenderThreadCount()
{
	const int threads = r_threads.asInt();
	if (threads > 0)
		return threads;

	// hardware_concurrency may not know and return a 0.
	return std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, MAXRENDERSLICES);
}

CVAR_FUNC_IMPL(r_threads)
{
	if (var.asInt() == 0)
		PrintFmt(PRINT_HIGH, "Automatic Render Worker Count: rendering with {} threads\n",
		         R_RenderThreadCount());
}

rendercontext_t rctx;

// Slices 1..n-1.
// Slice 0 is rctx (main thread), so the one slice case allocates nothing.
static std::vector<std::unique_ptr<rendercontext_t>> extra_contexts;

static int activeslices = 1;

//
// R_ActiveSlices
//
int R_ActiveSlices()
{
	return activeslices;
}

//
// R_SliceContext
//
rendercontext_t& R_SliceContext(int i)
{
	return i == 0 ? rctx : *extra_contexts[i - 1];
}

//
// R_SyncRenderContexts
//
// Grows the context set to match r_threads.
// Contexts are never released in order to only allocate once.
//
bool R_SyncRenderContexts()
{
	const int want = R_SliceCount();
	const bool grew = want > activeslices;
	while (extra_contexts.size() + 1 < static_cast<size_t>(want))
	{
		extra_contexts.push_back(std::make_unique<rendercontext_t>());
		R_InitVisSprites(*extra_contexts.back());
	}

	activeslices = want;
	return grew;
}

//
// RenderWorkers
//
// One worker thread per slice past the first, since the calling thread
// renders too.
//
// The frame hands them a pass at a time and waits for every slice to finish,
// so within a pass all slices run together and between passes none do.
//
// No slice belongs to a thread. Each thread, the caller included, claims the
// next slice in the pass's order until none are left, so whichever threads
// wake first take the slices at the front of the order, and a thread that
// finishes early takes another.
//
// The slices themselves are fixed, so which thread runs one never changes the image.
//
// The main thread does its serial work in the meantime:
// 
// Portals
// Weapon psprite
// Diagnostic overlays (vid_displayfps, etc)
//
// This is a mishmash of R&R's screen slice MT renderer mixed with
// Doom 3 BFG's MT renderer.
//
// Jury still out as to whether or not this needs a thread to manage HID input
//
class RenderWorkers
{
public:
	~RenderWorkers()
	{
		resize(0);
	}

	void run(int nslices, const std::function<void(int)>& job, const int* order)
	{
		resize(nslices - 1);

		std::unique_lock<std::mutex> lock(m_mutex);
		m_job = &job;
		m_order = order;
		m_count = nslices;
		m_next = 0;
		m_finished = 0;
		m_error = nullptr;

		if (nslices > 1)
			m_wake.notify_all();

		// The calling thread claims slices alongside the workers.
		claim(lock);

		// Wait even if a slice threw an error
		// The workers are still using the job, and the contexts behind it.
		m_done.wait(lock, [this] { return m_finished == m_count; });

		std::exception_ptr error = m_error;
		m_job = nullptr;
		m_order = nullptr;
		m_count = 0;
		m_next = 0;
		m_error = nullptr;
		lock.unlock();

		// An I_Error on a worker has to reach the main thread's handler
		// If we threw immediately it would just exit before Odamex
		// could dump or show the error.
		if (error)
			std::rethrow_exception(error);
	}

private:
	void resize(int nworkers)
	{
		if (static_cast<int>(m_threads.size()) == nworkers)
			return;

		// Shrinking stops everyone and starts over
		if (static_cast<int>(m_threads.size()) > nworkers)
		{
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_stop = true;
			}
			m_wake.notify_all();
			for (std::thread& thread : m_threads)
				thread.join();
			m_threads.clear();
			m_stop = false;
		}

		while (static_cast<int>(m_threads.size()) < nworkers)
			m_threads.emplace_back([this] { work(); });
	}

	// Runs slices until the pass has none left to claim.
	// Called with the mutex held, and returns with it held.
	void claim(std::unique_lock<std::mutex>& lock)
	{
		while (m_next < m_count)
		{
			const int n = m_next++;
			const int slice = m_order ? m_order[n] : n;
			const std::function<void(int)>& job = *m_job;
			lock.unlock();

			std::exception_ptr error;
			try
			{
				job(slice);
			}
			catch (...)
			{
				error = std::current_exception();
			}

			lock.lock();
			if (error && !m_error)
				m_error = error;
			if (++m_finished == m_count)
				m_done.notify_one();
		}
	}

	void work()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		for (;;)
		{
			// Unclaimed slices only exist while a pass is running, so a
			// worker that wakes after its pass ended just goes back to sleep.
			m_wake.wait(lock, [this] { return m_stop || m_next < m_count; });
			if (m_stop)
				return;
			claim(lock);
		}
	}

	std::vector<std::thread>         m_threads;
	std::mutex                       m_mutex;
	std::condition_variable          m_wake;
	std::condition_variable          m_done;
	const std::function<void(int)>*  m_job = nullptr;
	const int*                       m_order = nullptr;
	int                              m_count = 0;		// slices in the pass
	int                              m_next = 0;		// next index into the order to claim
	int                              m_finished = 0;
	bool                             m_stop = false;
	std::exception_ptr               m_error;
};

// Destroyed on exit, which joins the idle workers.
static RenderWorkers renderworkers;

//
// R_RunSlices
//
void R_RunSlices(int nslices, const std::function<void(int)>& job, const int* order)
{
	renderworkers.run(nslices, job, order);
}

//
// R_SliceCount
//
// One slice per render thread, limited to >4 columns.
//
int R_SliceCount()
{
	int nslices = std::clamp(R_RenderThreadCount(), 1, MAXRENDERSLICES);

	while (nslices > 1 && (viewwidth >> 4) / nslices < 4)
		nslices--;

	return nslices;
}

//
// R_SliceBounds
//
// Distributes whole 16-pixel column groups evenly with any remainder
// going to the last slice.
//
void R_SliceBounds(int i, int nslices, int& start, int& stop)
{
	const int ngroups = viewwidth >> 4;

	start = (ngroups * i / nslices) << 4;
	stop = (i == nslices - 1) ? viewwidth - 1
	                          : ((ngroups * (i + 1) / nslices) << 4) - 1;
}

//
// R_ShowContexts
//
// Marks the first and last column of every slice.
// (And clears the top/bottom of the slice)
//
void R_ShowContexts()
{
	if (!r_showcontexts)
		return;

	IWindowSurface* surface = R_GetRenderingSurface();
	DCanvas* canvas = surface->getDefaultCanvas();

	const argb_t slicecolor(255, 0, 255);		// pink: nothing else draws it
	const int y1 = viewwindowy;
	const int y2 = viewwindowy + viewheight;

	const int nslices = R_ActiveSlices();

	for (int i = 0; i < nslices; i++)
	{
		int start, stop;
		R_SliceBounds(i, nslices, start, stop);

		start += viewwindowx;
		stop += viewwindowx;

		canvas->Clear(start, y1, start + 1, y2, slicecolor);
		canvas->Clear(stop, y1, stop + 1, y2, slicecolor);
	}
}

VERSION_CONTROL (r_context_cpp, "$Id$")
