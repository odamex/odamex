#pragma once

#include "olumpname.h"

struct page_image_t;

void D_ResetDemoLoop();
void D_DoAdvanceDemoLoop(page_image_t& page, int& pagetic, bool& showAdvisorOverlay);
