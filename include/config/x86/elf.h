//  Copyright (C) 2021 Benjamin Stürz
//  
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef FILE_CONFIG_X86_ELF_H
#define FILE_CONFIG_X86_ELF_H
#include "config.h"

#if BITS == 32
# define GNU_LD_EMULATION "-melf_i386"
#else
# define GNU_LD_EMULATION "-melf_x86_64"
#endif

#endif /* FILE_CONFIG_X86_ELF_H */
