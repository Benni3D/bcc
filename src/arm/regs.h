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

#ifndef FILE_ARM_REGS_H
#define FILE_ARM_REGS_H
#include "config.h"

#if BITS == 32
typedef uint32_t uintreg_t;
typedef int32_t  intreg_t;
#define REGSIZE 4

#endif

static const char* regs[5] = { "r0", "r1", "r2", "r3", "r12" };

#define reg(r) ((const char*)((r) < arraylen(regs) ? regs[r] : (panic("register out of range"), NULL)))

#define align_stack_size(sz) ((uintreg_t)(((uintreg_t)(sz) & 7) ? (((uintreg_t)(sz) & ~7) + 8) : (uintreg_t)(sz)))

#endif /* FILE_ARM_REGS_H */
