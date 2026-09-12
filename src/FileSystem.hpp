#pragma once

#include "ZunResult.hpp"
#include "inttypes.hpp"
#include <cstdio>

namespace FileSystem
{

FILE *FopenUTF8(const char *filepath, const char *mode);
void CreateDir(const char *path);
u8 *OpenPath(const char *filepath, int isExternalResource);
int WriteDataToFile(const char *path, const void *data, std::size_t size);
}
extern u32 g_LastFileSize;
