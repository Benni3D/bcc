#ifndef FILE_DIR_H
#define FILE_DIR_H
#include <stdbool.h>
#include <stdio.h> 
#include "token.h"

struct directive {
   const char* name;
   bool(*handler)(size_t linenum, const char* line, struct token* tokens, size_t num_tks, FILE* out);
};

struct directive* get_dir(const char* name, size_t len);

// pre-processor directives

bool dir_define(size_t, const char*, struct token*, size_t num_tks, FILE*);

#endif /* FILE_DIR_H */