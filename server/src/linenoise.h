/* linenoise.h -- VERSION 1.0
 *
 * Guerrilla line editing library against the idea that a line editing lib
 * needs to be 20,000 lines of C code.
 *
 * See linenoise.c for more information.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2023, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <stddef.h> /* For size_t. */

extern const char *linenoiseEditMore;

#define LINENOISE_MAX_FOLDS 16

/* The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct linenoiseState {
    int in_completion;  /* The user pressed TAB and we are now in completion
                         * mode, so input is handled by completeLine(). */
    size_t completion_idx; /* Index of next completion to propose. */
    int ifd;            /* Terminal stdin file descriptor. */
    int ofd;            /* Terminal stdout file descriptor. */
    char *buf;          /* Edited line buffer. */
    size_t buflen;      /* Edited line buffer size. */
    size_t buflen_max;  /* Max buffer size, or 0 if fixed. */
    const char *prompt; /* Prompt to display. */
    size_t plen;        /* Prompt length. */
    size_t pos;         /* Current cursor position. */
    size_t oldpos;      /* Previous refresh cursor position. */
    size_t len;         /* Current edited line length. */
    size_t cols;        /* Number of columns in terminal. */
    size_t oldrows;     /* Rows used by last refrehsed line (multiline mode) */
    int oldrpos;        /* Cursor row from last refresh (for multiline clearing). */
    int history_index;  /* The history index we are currently editing. */
    int fold_count;    /* Number of folded ranges. */
    size_t fold_start[LINENOISE_MAX_FOLDS]; /* Folded range start offsets. */
    size_t fold_end[LINENOISE_MAX_FOLDS];   /* Folded range end offsets. */
};

struct linenoiseCompletions {
  size_t len;
  char **cvec;
};

/* Non blocking API. */
int linenoiseEditStart(linenoiseState *l, char *buf, size_t buflen, const char *prompt);
char *linenoiseEditFeed(linenoiseState *l);
void linenoiseEditStop(linenoiseState *l, bool newline = true);
void linenoiseHide(linenoiseState *l);
void linenoiseShow(linenoiseState *l);

/* Completion API. */
using linenoiseCompletionCallback = void(*)(const char *, linenoiseCompletions *);
using linenoiseHintsCallback = char*(*)(const char *, int *color, int *bold);
using linenoiseFreeHintsCallback = void(*)(void *);
void linenoiseSetCompletionCallback(linenoiseCompletionCallback);
void linenoiseSetHintsCallback(linenoiseHintsCallback);
void linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback);
void linenoiseAddCompletion(linenoiseCompletions *, const char *);

/* History API. */
int linenoiseHistoryAdd(const char *line);
int linenoiseHistorySetMaxLen(int len);

/* Other utilities. */
void linenoiseClearScreen(void);
void linenoiseMaskModeEnable(void);
void linenoiseMaskModeDisable(void);
