/*  ELF Object Hasher
    Copyright (C) 2013 Embecosm Limited and University of Bristol

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

/** @file elfhash.cpp ELF Object Hasher. */
#include "elfhash.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include "libelf.h"
#include "gelf.h"
#include "gcrypt.h"


using namespace mageec;

hashedelf::hashedelf (bool verbose) : verbose(verbose)
{
  data = NULL;
  datasize = 0;
}

#pragma GCC diagnostic ignored "-Wpointer-arith"
int hashedelf::hash (char const *filename)
{
  Elf *elf;
  Elf_Kind kind;
  size_t sheaderindex;
  Elf_Scn *section;
  GElf_Shdr sechdr;
  Elf_Data *elfdata;

  if (elf_version (EV_CURRENT) == EV_NONE)
    return 1;

  int fd = open (filename, O_RDONLY, 0);
  if (fd < 0)
    return 1;

  elf = elf_begin (fd, ELF_C_READ, NULL);
  if (elf == NULL)
    return 1;

  kind = elf_kind (elf);
  if (kind != ELF_K_ELF)
    return 1;

  if (elf_getshdrstrndx (elf, &sheaderindex) != 0)
    return 1;

  section = elf_nextscn (elf, NULL);

  while (section != NULL)
  {
    char *name;

    if (gelf_getshdr(section, &sechdr) != &sechdr)
      return 1;

    name = elf_strptr (elf, sheaderindex, sechdr.sh_name);
    if (name == NULL)
      return 1;

    if (sechdr.sh_flags & SHF_ALLOC)
    {
      /* Add sectionname */
      data = realloc (data, datasize + strlen(name));
      if (data == NULL)
        return 1;
      memcpy((data+datasize), name, strlen(name));
      datasize += strlen(name);

      /* Add size and load address */
      data = realloc(data, datasize + sizeof(sechdr.sh_size)  +
                     sizeof(sechdr.sh_addr));
      if (data == NULL)
        return 1;
      memcpy((data+datasize), &sechdr.sh_size, sizeof(sechdr.sh_size));
      memcpy((data+datasize+sizeof(sechdr.sh_size)), &sechdr.sh_addr,
              sizeof(sechdr.sh_addr));
      datasize += sizeof(sechdr.sh_size) + sizeof(sechdr.sh_addr);

      /* Add section data */
      elfdata = elf_rawdata (section, NULL);
      if (elfdata == NULL)
        return 1;
      if (elfdata->d_size > 0 && elfdata->d_buf != NULL) {
        data = realloc (data, datasize + elfdata->d_size);
        if (data == NULL)
          return 1;
        memcpy((data+datasize), elfdata->d_buf, elfdata->d_size);
        datasize += elfdata->d_size;
      }
    }

    section = elf_nextscn (elf, section);
  }


  return 0;
}

uint64_t hashedelf::getResult()
{
  gcry_md_hd_t handle = NULL;
  gcry_error_t err = 0;
  unsigned char *hash = NULL;
  uint64_t finalhash;

  err = gcry_md_open (&handle, GCRY_MD_SHA256, 0);
  if (err != 0)
    return 0;
  
  gcry_md_write(handle, data, datasize);
  hash = gcry_md_read(handle, GCRY_MD_SHA256);
  if (hash == NULL)
    return 0;

  /* XOR each 64-bit block to generate 64-bit hash */
  finalhash  = (uint64_t)(hash[0] ^ hash[8] ^ hash[16] ^ hash[24]) << 56;
  finalhash |= (uint64_t)(hash[1] ^ hash[9] ^ hash[17] ^ hash[25]) << 48;
  finalhash |= (uint64_t)(hash[2] ^ hash[10] ^ hash[18] ^ hash[26]) << 40;
  finalhash |= (uint64_t)(hash[3] ^ hash[11] ^ hash[19] ^ hash[27]) << 32;
  finalhash |= (uint64_t)(hash[4] ^ hash[12] ^ hash[20] ^ hash[28]) << 24;
  finalhash |= (uint64_t)(hash[5] ^ hash[13] ^ hash[21] ^ hash[29]) << 16;
  finalhash |= (uint64_t)(hash[6] ^ hash[14] ^ hash[22] ^ hash[30]) << 8;
  finalhash |= (uint64_t)(hash[7] ^ hash[15] ^ hash[23] ^ hash[31]);

  return finalhash;
}
