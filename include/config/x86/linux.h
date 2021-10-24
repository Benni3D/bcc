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

#ifndef FILE_CONFIG_X86_LINUX_H
#define FILE_CONFIG_X86_LINUX_H
#include "config/x86/elf.h"

#define HAS_INTERPRETER 1

#if LIBC_glibc
# if BITS == 32
#  define GNU_LD_INTERPRETER "/lib/ld-linux.so.2"
# else
#  define GNU_LD_INTERPRETER "/lib64/ld-linux-x86-64.so.2"
# endif

#elif LIBC_musl
# define GNU_LD_INTERPTETER ("/lib/ld-musl-" BCC_FULL_ARCH ".so.1")

#else
# error "Unsupported C library"
#endif

#endif /* FILE_CONFIG_X86_LINUX_H */
