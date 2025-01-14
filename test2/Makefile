#  Copyright (C) 2021 Benjamin Stürz
#  
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

ifeq ($(shell test -e ../bcc && echo y),y)
BCC = ../bcc
BCPP = ../cpp/bcpp
else
BCC = $(shell which bcc)
BCPP = $(patsubst %/bcc,%/bcpp,$(BCC))
endif

BCC_ARCH ?= $(shell $(BCC) -dumparch)
BCC_MACH ?= $(shell $(BCC) -dumpmachine)

BCCFLAGS ?= -O3

BCC_FLAGS := $(BCCFLAGS) -fpath-cpp=$(BCPP) -I ../bcc-include -L ../libbcc -nobccobjs

ifeq ($(BCC_ARCH),x86_64)
BITS=64
ARCH=x86
else ifeq ($(BCC_ARCH),i386)
BITS=32
ARCH=x86
else ifeq ($(BCC_ARCH),riscv32)
BITS=32
ARCH=riscv
QLP=/usr/$(BCC_MACH)/
BCC_FLAGS := $(BCC_FLAGS) -mabi=ilp32d
else ifeq ($(BCC_ARCH),riscv64)
BITS=64
ARCH=riscv
QLP=/usr/$(BCC_MACH)/
BCC_FLAGS := $(BCC_FLAGS) -mabi=lp64
endif

ifneq ($(QEMU_LD_PREFIX),)
	QLP=$(QEMU_LD_PREFIX)
endif

all: test

test.ir: test.c $(BCC)
	$(BCC) -i -o $@ $< $(BCC_FLAGS)

test: test.s
	$(BCC) -o $@ $< $(BCC_FLAGS)

test.s: test.c $(BCC)
	$(BCC) -S -o $@ $< $(BCC_FLAGS)

clean:
	rm -f *.s *.asm *.o test *.ir *.core

run: test
	@QEMU_LD_PREFIX=$(QLP) ./test; echo "Exit code: $$?"

.PHONY: all clean
