#pragma once

#include <stddef.h>

#ifdef __cplusplus
#include <string>
#include <string_view>

std::string I_ParseOdamexUrl(std::string_view url);

extern "C" {
#endif

char* I_ParseOdamexUrlC(const char *url);

#ifdef __cplusplus
}
#endif
