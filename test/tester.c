#include <stdnoreturn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define arraylen(a) (sizeof(a) / sizeof(*(a)))

#define COLOR_BOLD_GREEN   "\033[1;32m"
#define COLOR_BOLD_RED     "\033[1;31m"
#define COLOR_GREEN        "\033[32m"
#define COLOR_RED          "\033[31m"
#define COLOR_DFL          "\033[0m"

#define BCC "../bcc"
#define LINKER "gcc"
#define TEST_SOURCE "/tmp/test.c"
#define TEST_OBJECT "/tmp/test.o"
#define TEST_BINARY "/tmp/test"

struct test_case {
   const char* name;
   bool compiles;
   const char* source;
   const char* output;
   int ret_val;
};

static struct test_case cases[] = {
#include "cases.h"
};

static noreturn void panic(const char* msg, ...) {
   va_list ap;
   va_start(ap, msg);

   fputs("bcc: ", stderr);
   vfprintf(stderr, msg, ap);
   if (errno) {
      fprintf(stderr, ": %s\n", strerror(errno));
   } else fputc('\n', stderr);

   va_end(ap);
   exit(2);
}

static void write_test(const char* s) {
   FILE* file = fopen(TEST_SOURCE, "w");
   if (!file) panic("failed to open '%s'", TEST_SOURCE);
   fwrite(s, 1, strlen(s), file);
   fclose(file);
}
static bool compile_test(void) {
   int ec = system(BCC " -c -o " TEST_OBJECT " " TEST_SOURCE " -O2");
   if (ec < 0 || ec == 127) panic("failed to invoke bcc");
   else if (ec != 0) return false;

   ec = system(LINKER " -o " TEST_BINARY " " TEST_OBJECT);
   if (ec < 0 || ec == 127) panic("failed to invoke linker");
   else return ec == 0;
}

static bool run_test(const struct test_case* test) {
   const char* cause;
   write_test(test->source);
   const bool compiled = compile_test();
   if (compiled != test->compiles) {
      if (compiled) cause = "invalid compilation success";
      else cause = "failed to compile";
      goto failed;
   }

   bool r = true;
   const size_t len = strlen(test->output);
   if (len) {
      FILE* process = popen(TEST_BINARY, "r");
      if (!process) panic("failed to invoke " TEST_BINARY);

      char* buffer = malloc(len + 1);

      ssize_t len_input = fread(buffer, 1, len, process);
      if (len_input < 0) {
         r = false;
         cause = "failed to read input";
      }
      else if ((size_t)len_input != len || memcmp(buffer, test->output, len)) {
         r = false;
         cause = "output did not match";
      }

      free(buffer);
      if (pclose(process) != test->ret_val) {
         r = false;
         cause = "return value did not match";
      }
   } else {
      pid_t pid = fork();
      if (pid == 0) {
         execl(TEST_BINARY, TEST_BINARY, NULL);
         fprintf(stderr, "tester: failed to execl: %s\n", strerror(errno));
         _exit(1);
      } else {
         int wstatus;
         waitpid(pid, &wstatus, 0);
         if (WIFEXITED(wstatus)) {
            r = WEXITSTATUS(wstatus) == test->ret_val;
            if (!r) cause = "return value did not match";
         } else {
            r = false;
            cause = "failed to invoke test";
         }
      }
   }

   if (!r) goto failed;
   printf(COLOR_BOLD_GREEN "TEST '%s' PASSED\n" COLOR_DFL, test->name);
   return true;
failed:
   printf(COLOR_BOLD_RED "TEST '%s' FAILED\n" COLOR_DFL, test->name);
   printf(COLOR_RED "%s.\n", cause);
   return false;
}

static struct test_case* get_case(const char* name) {
   for (size_t i = 0; i < arraylen(cases); ++i) {
      if (!strcmp(name, cases[i].name)) return &cases[i];
   }
   return NULL;
}

int main(int argc, char* argv[]) {
   if (argc > 2) {
      fputs("usage: tester [test]\n", stderr);
      return 1;
   }
   if (argc == 2) {
      const char* name = argv[1];
      struct test_case* t = get_case(name);
      if (!t) {
         printf(COLOR_RED "TEST '%s' NOT FOUND\n" COLOR_DFL, name);
         return 1;
      }
      const bool r = run_test(t);
      return r ? 0 : 2;
   } else {
      const size_t num = arraylen(cases);
      size_t failed = 0;
      for (size_t i = 0; i < num; ++i) {
         if (!run_test(&cases[i])) ++failed;
      }

      printf("%s%zu from %zu passed\n" COLOR_DFL, failed ? COLOR_RED : COLOR_GREEN, num - failed, num);
      return failed ? 2 : 0;
   }
}