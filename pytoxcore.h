/**
 * pytoxcore
 *
 * Copyright (C) 2015 Anton Batenev <antonbatenev@yandex.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//----------------------------------------------------------------------------------------------
#ifndef _pytoxcore_h_
#define _pytoxcore_h_
//----------------------------------------------------------------------------------------------
#include "pytox.h"
//----------------------------------------------------------------------------------------------
typedef struct {
    int      fd;
    uint64_t offset;
    uint64_t size;
    time_t   checkpoint;
    time_t   timeout;
    uint32_t friend_number;
    uint32_t file_number;
} ToxFile;
//----------------------------------------------------------------------------------------------
typedef struct {
    ToxFile** files;
    size_t    index;
    size_t    count;
} ToxFileBucket;
//----------------------------------------------------------------------------------------------
typedef struct {
    PyObject_HEAD
    Tox*          tox;
    ToxFileBucket send_files;
    ToxFileBucket recv_files;
} ToxCore;
//----------------------------------------------------------------------------------------------
extern PyTypeObject ToxCoreType;
//----------------------------------------------------------------------------------------------
extern PyObject* ToxCoreException;
//----------------------------------------------------------------------------------------------
void ToxCore_install_dict(void);
//----------------------------------------------------------------------------------------------
#endif   // _pytoxcore_h_
//----------------------------------------------------------------------------------------------
