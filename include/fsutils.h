#pragma once

/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "types.h"



bool fsEnsureMounted(const char *path);
void fsUnmountAll();
u32 fsMountNandFilesystems();
void fsUnmountNandFilesystems();
bool fsMountSdmc();
bool fsCreateFileWithPath(const char *filepath);
bool fsQuickRead(const char* filepath, void* buff, u32 len, u32 off);
bool fsQuickCreate(const char* filepath, const void *const , u32 len);
