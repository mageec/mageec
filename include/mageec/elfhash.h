/*  ELF Object Hasher
    Copyright (C) 2013, 2014 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/** @file elfhash.h ELF Object Hasher. */
#ifndef __ELFHASH_H
#define __ELFHASH_H

#include <cstring>
#include <stdint.h>

namespace mageec
{

class hashedelf
{
    void *data;
    size_t datasize;
public:
    hashedelf ();
    bool ignoredsection (char const *sectionname);
    int hash (char const *filename);
    uint64_t getResult();
};

} // End namespace mageec

#endif