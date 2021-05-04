#include <stdlib.h>
#include "target.h"
#include "scope.h"
#include "value.h"
#include "error.h"
#include "lex.h"

const char* integer_size_str[NUM_INTS] = {
   "byte",
   "char",
   "short",
   "int",
   "long",
};
const char* fp_size_str[NUM_FPS] = {
   "float",
   "double",
};
const char* value_type_str[NUM_VALS] = {
   "integer",
   "floating-point number",
   "pointer",
   "void",
};

static bool ptreq(const struct value_type* a, const struct value_type* b) {
   if (a->type != b->type) return false;
   switch (a->type) {
   case VAL_INT:     return (a->integer.is_unsigned == b->integer.is_unsigned) && (a->integer.size == b->integer.size);
   case VAL_FLOAT:   return a->fp.size == b->fp.size;
   case VAL_POINTER: return ptreq(a->pointer.type, b->pointer.type);
   default:          panic("ptreq(): invalid value type '%u'", a->type);
   }
}
void print_value_type(FILE* file, const struct value_type* val) {
   switch (val->type) {
   case VAL_INT:
      if (val->integer.is_unsigned) fputs("unsigned ", file);
      fputs(integer_size_str[val->integer.size], file);
      break;
   case VAL_FLOAT:
      fputs(fp_size_str[val->fp.size], file);
      break;
   case VAL_POINTER:
      print_value_type(file, val->pointer.type);
      fputc('*', file);
      break;
   case VAL_VOID:
      fputs("void", file);
      break;
   default: panic("print_value_type(): invalid value type '%d'", val->type);
   }
   if (val->is_const) fputs(" const", file);
}

void free_value_type(struct value_type* val) {
   switch (val->type) {
   case VAL_POINTER:
      free_value_type(val->pointer.type);
      break;
   default:
      break;
   }
   free(val);
}

static struct value_type* new_vt(void) {
   struct value_type* vt = malloc(sizeof(struct value_type));
   if (!vt) panic("new_vt(): failed to allocate value type");
   else return vt;
}

// const unsigned int* const
struct value_type* parse_value_type(void) {
   bool has_begon = false;
   bool has_signedness = false;
   struct value_type* vt = new_vt();
   vt->type = NUM_VALS;
   vt->integer.is_unsigned = false;
   vt->is_const = false;

   while (lexer_matches(KW_CONST) || lexer_matches(KW_SIGNED) || lexer_matches(KW_UNSIGNED)) {
      const struct token tk = lexer_next();
      switch (tk.type) {
      case KW_CONST:    vt->is_const = true; break;
      case KW_SIGNED:
         vt->type = VAL_INT;
         vt->integer.size = INT_INT;
         vt->integer.is_unsigned = false;
         has_signedness = true;
         break;
      case KW_UNSIGNED:
         vt->type = VAL_INT;
         vt->integer.size = INT_INT;
         vt->integer.is_unsigned = true;
         has_signedness = true;
         break;
      default:          panic("parse_value_type(): invalid token type '%s'", token_type_str[tk.type]);
      }
      if (!has_begon) {
         has_begon = true;
         vt->begin = tk.begin;
      }
   }
   struct token tk = lexer_peek();
   switch (tk.type) {
   case KW_BYTE:
      lexer_skip();
      vt->type = VAL_INT;
      vt->integer.size = INT_BYTE;
      lexer_match(KW_INT);
      break;
   case KW_CHAR:
      lexer_skip();
      vt->type = VAL_INT;
      vt->integer.size = INT_CHAR;
      lexer_match(KW_INT);
      if (!has_signedness) vt->integer.is_unsigned = target_info.unsigned_char;
      break;
   case KW_SHORT:
      lexer_skip();
      vt->type = VAL_INT;
      vt->integer.size = INT_SHORT;
      lexer_match(KW_INT);
      break;
   case KW_LONG:
      lexer_skip();
      vt->type = VAL_INT;
      vt->integer.size = INT_LONG;
      lexer_match(KW_INT);
      break;
   case KW_INT:
      lexer_skip();
      vt->type = VAL_INT;
      vt->integer.size = INT_INT;
      break;
   case KW_FLOAT:
      lexer_skip();
      if (vt->type != NUM_VALS) parse_error(&tk.begin, "invalid combination of signed or unsigned and float");
      vt->type = VAL_FLOAT;
      vt->fp.size = FP_FLOAT;
      break;
   case KW_DOUBLE:
      lexer_skip();
      if (vt->type != NUM_VALS) parse_error(&tk.begin, "invalid combination of signed or unsigned and double");
      vt->type = VAL_FLOAT;
      vt->fp.size = FP_DOUBLE;
      break;
   case KW_VOID:
      lexer_skip();
      if (vt->type != NUM_VALS) parse_error(&tk.begin, "invalid combination of signed or unsigned and void");
      vt->type = VAL_VOID;
      break;
   default: break;
   }
   if (vt->type == NUM_VALS) {
      // no error handling
      free(vt);
      return NULL;
   }
   if (!has_begon) {
      vt->begin = tk.begin;
      has_begon = true;
   }
   vt->end = tk.end;
   while (lexer_matches(KW_CONST)) {
      vt->is_const = true;
      vt->end = lexer_next().end;
   }
   while (lexer_matches(TK_STAR)) {
      const struct token tk2 = lexer_next();
      struct value_type* ptr = new_vt();
      ptr->type = VAL_POINTER;
      ptr->pointer.type = vt;
      ptr->begin = tk2.begin;
      ptr->end = tk2.end;
      while (lexer_match(KW_CONST)) ptr->is_const = true;
      vt = ptr;
   }
   return vt;
}

struct value_type* common_value_type(const struct value_type* a, const struct value_type* b, bool warn) {
   struct value_type* c = new_vt();
   c->is_const = false;
   if (a->type == b->type) {
      c->type = a->type;
      switch (a->type) {
      case VAL_INT:
         if (a->integer.is_unsigned != b->integer.is_unsigned) {
            if (warn) parse_warn(&a->begin, "performing operation between signed and unsigned");
            c->integer.is_unsigned = true;
         } else c->integer.is_unsigned = a->integer.is_unsigned;
         c->integer.size = my_max(a->integer.size, b->integer.size);
         break;
      case VAL_FLOAT:
         c->fp.size = my_max(a->fp.size, b->fp.size);
         break;
      case VAL_POINTER: parse_error(&a->end, "performing binary operation on pointers");
      default:          panic("common_value_type(): unsupported value type '%d'", a->type);
      }
   } else {
      if (a->type == VAL_INT && b->type == VAL_FLOAT) {
         c->type = VAL_FLOAT;
         c->fp.size = b->fp.size;
      } else if (a->type == VAL_FLOAT && b->type == VAL_INT) {
         c->type = VAL_FLOAT;
         c->fp.size = a->fp.size;
      } else if (a->type == VAL_POINTER && b->type == VAL_INT) {
         c->type = VAL_POINTER;
         c->pointer.type = copy_value_type(a->pointer.type);
      } else if (a->type == VAL_INT && b->type == VAL_POINTER) {
         c->type = VAL_POINTER;
         c->pointer.type = copy_value_type(b->pointer.type);
      } else parse_error(&a->end, "invalid combination of types");
   }
   return c;
}

struct value_type* common_value_type_free(struct value_type* a, struct value_type* b, bool warn) {
   struct value_type* c = common_value_type(a, b, warn);
   free_value_type(a);
   free_value_type(b);
   return c;
}

#define check1or(a, b, c) (((a)->type == (c)) || ((b)->type == (c)))
#define check1and(a, b, c) (((a)->type == (c)) && ((b)->type == (c)))
#define check2(a, b, c, d) ((((a)->type == (c)) && ((b)->type == (d))) || (((a)->type == (d)) && ((b)->type == (c))))

static struct value_type* decay(struct value_type* vt) {
   if (vt->type == VAL_POINTER && vt->pointer.is_array) {
      vt->pointer.is_array = false;
   }
   return vt;
}

struct value_type* get_value_type(struct scope* scope, const struct expression* e) {
   struct value_type* type;
   switch (e->type) {
   case EXPR_PAREN:
      return get_value_type(scope, e->expr);
   case EXPR_INT:
   case EXPR_UINT:
      type = new_vt();
      type->type = VAL_INT;
      type->is_const = false;
      type->integer.is_unsigned = e->type == EXPR_UINT;
      if (type->integer.is_unsigned) {
         type->integer.size = e->uVal > target_info.max_uint ? INT_LONG : INT_INT;
      } else {
         type->integer.size = e->iVal > target_info.max_int ? INT_LONG : INT_INT;
      }
      return type;
   case EXPR_SIZEOF:
      type = new_vt();
      type->type = VAL_INT;
      type->is_const = true;
      type->integer.is_unsigned = true;
      type->integer.size = INT_INT;
      return type;
   case EXPR_FLOAT:
      type = new_vt();
      type->type = VAL_FLOAT;
      type->is_const = true;
      type->fp.size = FP_DOUBLE;
      return type;
   case EXPR_CHAR:
      type = new_vt();
      type->type = VAL_INT;
      type->is_const = true;
      type->begin = e->begin;
      type->end = e->end;
      type->integer.is_unsigned = target_info.unsigned_char;
      type->integer.size = INT_CHAR;
      return type;
   case EXPR_NAME: {
      const struct variable* var = scope_find_var(scope, e->str);
      if (!var) var = func_find_var(scope->func, e->str);
      if (!var) parse_error(&e->begin, "undeclared variable '%s'", e->str);
      return copy_value_type(var->type);
   }
   case EXPR_ADDROF:
   {
      struct value_type* ve = get_value_type(scope, e->expr);
      if (ve->type == VAL_POINTER && ve->pointer.is_array)
         return decay(ve);
      type = new_vt();
      type->type = VAL_POINTER;
      type->is_const = true;
      type->pointer.type = ve;
      return type;
   }
   case EXPR_INDIRECT: {
      struct value_type* tmp = get_value_type(scope, e->expr);
      if (tmp->type != VAL_POINTER) parse_error(&e->expr->begin, "cannot dereference a non-pointer");
      type = copy_value_type(tmp->pointer.type);
      free_value_type(tmp);
      return type;
   }
   case EXPR_STRING:
      type = new_vt();
      type->type = VAL_POINTER;
      type->is_const = true;
      type->pointer.type = new_vt();
      type->pointer.type->type = VAL_INT;
      type->pointer.type->integer.size = INT_CHAR;
      type->pointer.type->integer.is_unsigned = target_info.unsigned_char;
      return type;
   case EXPR_COMMA:
      return get_value_type(scope, e->comma[buf_len(e->comma) - 1]);
   case EXPR_SUFFIX:
      type = get_value_type(scope, e->unary.expr);
      type->is_const = true;
      return type;
   case EXPR_ASSIGN:
   {
      struct value_type* vl = get_value_type(scope, e->assign.left);
      struct value_type* vr = get_value_type(scope, e->assign.right);
      if (vl->is_const)
         parse_error(&e->begin, "assignment to const");
      else if (!is_castable(vr, vl, true))
         parse_error(&e->begin, "incompatible types");
      free_value_type(vr);
      return vl;
   }
   case EXPR_BINARY:
   {
      struct value_type* vl = get_value_type(scope, e->binary.left);
      struct value_type* vr = get_value_type(scope, e->binary.right);
      struct value_type* result;
      if (check1or(vl, vr, VAL_VOID))
         parse_error(&e->binary.op.begin, "binary operation on void");
      switch (e->binary.op.type) {
      case TK_EQEQ:
      case TK_NEQ:
      case TK_GR:
      case TK_GREQ:
      case TK_LE:
      case TK_LEEQ:
         if (vl->type != vr->type) {
            if (check2(vl, vr, VAL_POINTER, VAL_FLOAT))
               parse_error(&e->binary.op.begin, "comparisson between pointer and floating-point");
            else if (check2(vl, vr, VAL_POINTER, VAL_INT))
               parse_warn(&e->binary.op.begin, "comparisson between pointer and integer");
         } else if (check1and(vl, vr, VAL_POINTER)) {
            if (vl->pointer.type->type != vr->pointer.type->type)
               parse_error(&e->binary.op.begin, "comparisson between pointer of different types");
            // TODO: check for the specifics
         }
         result = new_vt();
         result->type = VAL_INT;
         result->integer.size = INT_INT;
         result->integer.is_unsigned = false;
         free_value_type(vl);
         free_value_type(vr);
         return result;
      case TK_PLUS:
         if (check1and(vl, vr, VAL_POINTER))
            parse_error(&e->binary.op.begin, "addition of pointers");
         switch (vl->type) {
         case VAL_INT:
            if (vr->type == VAL_POINTER) return free_value_type(vl), vr;
            else return common_value_type_free(vl, vr, true);
         case VAL_FLOAT:
            if (vr->type == VAL_POINTER)
               parse_error(&e->binary.op.begin, "addition of pointer and floating-point number");
            else return common_value_type_free(vl, vr, true);
         case VAL_POINTER:
            if (vr->type == VAL_INT) return free_value_type(vr), decay(vl);
            else if (vr->type == VAL_FLOAT)
               parse_error(&e->binary.op.begin, "addition of pointer and floating-point number");
            panic("get_value_type(): addition of pointer and '%s'", value_type_str[vr->type]);
         default:
            panic("get_value_type(): unsupported value type '%s'", value_type_str[vl->type]);
         }
      case TK_MINUS:
         switch (vl->type) {
         case VAL_INT:
            if (vr->type == VAL_POINTER)
               parse_error(&e->binary.op.begin, "invalid types");
            else return common_value_type_free(vl, vr, true);
         case VAL_FLOAT:
            if (vr->type == VAL_POINTER)
               parse_error(&e->binary.op.begin, "invalid types");
            else return common_value_type_free(vl, vr, true);
         case VAL_POINTER:
            if (vr->type == VAL_FLOAT)
               parse_error(&e->binary.op.begin, "invalid types");
            else if (vr->type == VAL_INT) return free_value_type(vr), decay(vl);
            else if (vr->type == VAL_POINTER) {
               if (!ptreq(vl->pointer.type, vr->pointer.type))
                  parse_error(&e->binary.op.begin, "incompatible pointer types"); // TODO: maybe error?
               result = new_vt();
               result->type = VAL_INT;
               result->integer.size = target_info.ptrdiff_type;
               result->integer.is_unsigned = false;
               free_value_type(vl);
               free_value_type(vr);
               return result;
            }
            else panic("get_value_type(): unsupported value type '%s'", value_type_str[vr->type]);
         default:
            panic("get_value_type(): unsupported value type '%s'", value_type_str[vl->type]);
         }
      case TK_STAR:
      case TK_SLASH:
      case TK_PERC:
         if (check1or(vl, vr, VAL_POINTER))
            parse_error(&e->binary.op.begin, "invalid use of pointer");
         else return common_value_type_free(vl, vr, true);
      case TK_AMP:
      case TK_PIPE:
      case TK_XOR:
         if (check1or(vl, vr, VAL_FLOAT))
            parse_error(&e->binary.op.begin, "bitwise operation on floating-point number");
         else if (check1or(vl, vr, VAL_POINTER))
            parse_error(&e->binary.op.begin, "bitwise operation on pointer");
         else return common_value_type_free(vl, vr, true);

      default:
         panic("get_value_type(): binary operator '%s' not implemented", token_type_str[e->binary.op.type]);
      }
      panic("get_value_type(): reached unreachable");
   }
      return common_value_type_free(get_value_type(scope, e->binary.left), get_value_type(scope, e->binary.right), true); // TODO
   case EXPR_TERNARY:
      return common_value_type_free(get_value_type(scope, e->ternary.true_case), get_value_type(scope, e->ternary.false_case), true);
   case EXPR_PREFIX:
   case EXPR_UNARY:
      type = get_value_type(scope, e->unary.expr);
      if (e->unary.op.type == TK_MINUS && type->type == VAL_INT && type->integer.is_unsigned)
         parse_warn(&e->begin, "negating an unsigned integer");
      type->is_const = true;
      return type;
   case EXPR_CAST:
   {
      struct value_type* old = get_value_type(scope, e->cast.expr);
      if (!is_castable(old, e->cast.type, false))
         parse_error(&e->begin, "invalid cast");
      free_value_type(old);
      return copy_value_type(e->cast.type);
   }
   case EXPR_FCALL:
      // TODO
      type = new_vt();
      type->is_const = true;
      type->type = VAL_INT;
      type->integer.is_unsigned = false;
      type->integer.size = INT_INT;
      return type;
   default: panic("get_value_type(): unsupported expression '%s'", expr_type_str[e->type]);
   }
}

struct value_type* copy_value_type(const struct value_type* vt) {
   struct value_type* copy = new_vt();
   copy->type = vt->type;
   copy->begin = vt->begin;
   copy->end = vt->end;
   copy->is_const = vt->is_const;
   switch (vt->type) {
   case VAL_INT:
      copy->integer.size = vt->integer.size;
      copy->integer.is_unsigned = vt->integer.is_unsigned;
      break;
   case VAL_FLOAT:
      copy->fp.size = vt->fp.size;
      break;
   case VAL_POINTER:
      copy->pointer.type = copy_value_type(vt->pointer.type);
      copy->pointer.is_array = vt->pointer.is_array;
      if (vt->pointer.is_array) {
         copy->pointer.array.has_const_size = vt->pointer.array.has_const_size;
         if (copy->pointer.array.has_const_size)
            copy->pointer.array.size = vt->pointer.array.size;
         else copy->pointer.array.dsize = vt->pointer.array.dsize;
      }
      break;
   case VAL_VOID:
      break;
   default: panic("copy_value_type(): unsupported value type '%d'", vt->type);
   }
   return copy;
}

#define warn_implicit(f, t) parse_warn(&old->begin, "implicit conversion from %s to %s", f, t)


bool is_castable(const struct value_type* old, const struct value_type* type, bool implicit) {
   if (old->type == VAL_VOID || type->type == VAL_VOID) return false;
   switch (old->type) {
   case VAL_INT:
      switch (type->type) {
      case VAL_INT:
      case VAL_FLOAT:
         return true;
      case VAL_POINTER:
         return !implicit;
      default: panic("is_castable(): invalid value type '%u'", type->type);
      }
   case VAL_FLOAT:
      switch (type->type) {
      case VAL_INT:
         if (implicit) warn_implicit(fp_size_str[old->fp.size], integer_size_str[type->integer.size]);
         fallthrough;
      case VAL_FLOAT:
         return true;
      case VAL_POINTER:
         return false;
      default: panic("is_castable(): invalid value type '%u'", type->type);
      }
   case VAL_POINTER:
      switch (type->type) {
      case VAL_INT:
         return !implicit;
      case VAL_POINTER:
         if (old->pointer.type->is_const && !type->pointer.type->is_const && implicit)
            warn_implicit("const pointer", "non-const pointer");
         if (old->pointer.type->type == VAL_VOID || type->pointer.type->type == VAL_VOID) return true;
         if (!ptreq(old->pointer.type, type->pointer.type) && implicit)
            parse_warn(&old->begin, "implicit pointer conversion");
         return true;
      default: panic("is_castable(): invalid value type '%u'", type->type);
      }
   default: panic("is_castable(): invalid value type '%u'", type->type);
   }
}
struct value_type* make_array_vt(struct value_type* vt) {
   struct value_type* arr = new_vt();
   arr->type = VAL_POINTER;
   arr->begin = vt->begin;
   arr->pointer.type = vt;
   arr->pointer.is_array = true;
   arr->is_const = true;
   return arr;
}
size_t sizeof_value(const struct value_type* vt) {
   switch (vt->type) {
   case VAL_POINTER:
      if (vt->pointer.is_array && vt->pointer.array.has_const_size)
         return sizeof_value(vt->pointer.type) * vt->pointer.array.size;
      else return target_info.size_pointer;
   case VAL_FLOAT:
      switch (vt->fp.size) {
      case FP_FLOAT:
         return target_info.size_float;
      case FP_DOUBLE:
         return target_info.size_double;
      default:
         panic("sizeof_value(): invalid floating-point size '%s'", fp_size_str[vt->fp.size]);
      }
   case VAL_INT:
      switch (vt->integer.size) {
      case INT_BYTE:
         return target_info.size_byte;
      case INT_CHAR:
         return target_info.size_char;
      case INT_SHORT:
         return target_info.size_short;
      case INT_INT:
         return target_info.size_int;
      case INT_LONG:
         return target_info.size_long;
      default:
         panic("sizeof_value(): invalid integer size '%s'", integer_size_str[vt->integer.size]);
      }
   default:
      panic("sizeof_value(): invalid value type '%s'", value_type_str[vt->type]);
   }
}
