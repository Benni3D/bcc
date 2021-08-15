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

#include <stdarg.h>
#include <stdio.h>
#include "cpp.h"

bool failed = false;

static void do_warn(size_t linenum, const char* msg, va_list ap) {
   fflush(stdout);

   if (console_color) {
      fputs("\033[31;1m", stderr);
   }

   fprintf(stderr, "bcpp: %s:%zu: ", source_name ? source_name : "<source>", linenum + 1);
   if (console_color) {
      fputs("\033[0m", stderr);
   }
   vfprintf(stderr, msg, ap);
   fputc('\n', stderr);
}

void warn(size_t linenum, const char* msg, ...) {
   va_list ap;
   va_start(ap, msg);

   do_warn(linenum, msg, ap);

   va_end(ap);
}

void fail(size_t linenum, const char* msg, ...) {
   va_list ap;
   va_start(ap, msg);

   do_warn(linenum, msg, ap);
   
   va_end(ap);

   failed = true;
}
