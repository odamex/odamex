#pragma once

class OLumpName;
class IWindowSurface;

struct page_image_t
{
	IWindowSurface* surface = nullptr;
	int width = 0;
	int height = 0;
	int display_height = 0;
};

bool D_LoadPageImage(page_image_t& page, const OLumpName& lumpname, bool is_raw);
void D_FreePageImage(page_image_t& page);
void D_DrawPageImage(const page_image_t& page, IWindowSurface* dest_surface, bool clear);
