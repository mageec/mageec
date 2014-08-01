/*  Generic Blob Hasher
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

/** @file hasher.cpp Generic hasher. */
#include "mageec/mageec.h"
#include "gcrypt.h"

uint64_t mageec::hash_data(const void *data, unsigned long size)
{
  gcry_md_hd_t handle = NULL;
  gcry_error_t err = 0;
  unsigned char *hash = NULL;
  uint64_t finalhash;

  err = gcry_md_open (&handle, GCRY_MD_SHA256, 0);
  if (err != 0)
    return 0;
  
  gcry_md_write(handle, data, size);
  hash = gcry_md_read(handle, GCRY_MD_SHA256);
  if (hash == NULL)
    return 0;

  /* XOR each 64-bit block to generate 64-bit hash */
  finalhash  = static_cast<uint64_t>(hash[0] ^ hash[8] ^ hash[16] ^ hash[24]) << 56;
  finalhash |= static_cast<uint64_t>(hash[1] ^ hash[9] ^ hash[17] ^ hash[25]) << 48;
  finalhash |= static_cast<uint64_t>(hash[2] ^ hash[10] ^ hash[18] ^ hash[26]) << 40;
  finalhash |= static_cast<uint64_t>(hash[3] ^ hash[11] ^ hash[19] ^ hash[27]) << 32;
  finalhash |= static_cast<uint64_t>(hash[4] ^ hash[12] ^ hash[20] ^ hash[28]) << 24;
  finalhash |= static_cast<uint64_t>(hash[5] ^ hash[13] ^ hash[21] ^ hash[29]) << 16;
  finalhash |= static_cast<uint64_t>(hash[6] ^ hash[14] ^ hash[22] ^ hash[30]) << 8;
  finalhash |= static_cast<uint64_t>(hash[7] ^ hash[15] ^ hash[23] ^ hash[31]);

  return finalhash;
}
