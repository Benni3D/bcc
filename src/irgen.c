#include "error.h"
#include "ir.h"

static ir_node_t* new_node(enum ir_node_type t) {
   ir_node_t* n = malloc(sizeof(ir_node_t));
   if (!n) panic("failed to allocate ir_node");
   n->type = t;
   n->prev = n->next = NULL;
   return n;
}

static ir_reg_t creg = 0;

enum ir_value_size vt2irs(const struct value_type* vt) {
   switch (vt->type) {
   case VAL_INT:
      switch (vt->integer.size) {
      case INT_CHAR:    return IRS_CHAR;
      case INT_BYTE:    return IRS_BYTE;
      case INT_SHORT:   return IRS_SHORT;
      case INT_INT:     return IRS_INT;
      case INT_LONG:    return IRS_LONG;
      default:          panic("vt2irs(): invalid integer value '%d'", vt->integer.size);
      }
   case VAL_FLOAT:      panic("vt2irs(): floating-point values are currently not supported by the IR backend.");
   case VAL_POINTER:    return IRS_PTR;
   default:             panic("vt2irs(): invalid value type '%d'", vt->type);
   }
}

static ir_node_t* ir_expr(struct scope* scope, const struct expression* e) {
   struct value_type* vt = get_value_type(scope, e);
   const enum ir_value_size irs = vt2irs(vt);
   free_value_type(vt);
   ir_node_t* n;
   ir_node_t* tmp;
   switch (e->type) {
   case EXPR_PAREN:  return ir_expr(scope, e->expr);
   case EXPR_UINT:
   case EXPR_INT:
      n = new_node(IR_LOAD);
      n->load.dest = creg++;
      n->load.value = e->uVal;
      n->load.size = irs;
      return n;
   case EXPR_BINARY:
      n = ir_expr(scope, e->binary.left);
      ir_append(n, ir_expr(scope, e->binary.right));
      tmp = new_node(IR_NOP);
      tmp->binary.size = irs;
      tmp->binary.dest = tmp->binary.a = creg - 2;
      tmp->binary.b = creg - 1;
      switch (e->binary.op.type) {
      case TK_PLUS:  tmp->type = IR_IADD; break;
      case TK_MINUS: tmp->type = IR_ISUB; break;
      case TK_STAR:  tmp->type = IR_IMUL; break;
      case TK_SLASH: tmp->type = IR_IDIV; break;
      case TK_PERC:  tmp->type = IR_IMOD; break;
      case TK_GRGR:  tmp->type = IR_ILSL; break;
      case TK_LELE:  tmp->type = IR_ILSR; break;
      default:       panic("ir_expr(): unsupported binary operator '%s'", token_type_str[e->binary.op.type]);
      }
      ir_append(n, tmp);
      --creg;
      return n;
   case EXPR_UNARY:
      n = ir_expr(scope, e->unary.expr);
      if (e->unary.op.type == TK_PLUS) return n;
      tmp = new_node(IR_NOP);
      tmp->unary.size = irs;
      tmp->unary.reg = creg - 1;
      switch (e->unary.op.type) {
      case TK_MINUS: tmp->type = IR_INEG; break;
      case TK_NOT:   tmp->type = IR_BNOT; break;
      case TK_WAVE:  tmp->type = IR_INOT; break;
      default:       panic("ir_expr(): unsupported unary operator '%s'", token_type_str[e->unary.op.type]);
      }
      ir_append(n, tmp);
      return n;
   case EXPR_NAME:
      n = new_node(IR_LOOKUP);
      n->lookup.reg = creg++;
      n->lookup.scope = scope;
      n->lookup.var_idx = scope_find_var_idx(scope, e->str);
      if (n->lookup.var_idx == SIZE_MAX)
         parse_error(&e->begin, "undeclared variable '%s'", e->str);
      return n;
   default:
      panic("ir_expr(): unsupported expression '%s'", expr_type_str[e->type]);
   }
}

ir_node_t* irgen_expr(struct scope* scope, const struct expression* expr) {
   creg = 0;
   return ir_expr(scope, expr);
}

static ir_node_t* ir_stmt(const struct statement* s) {
   ir_node_t* n;
   ir_node_t* tmp;
   switch (s->type) {
   case STMT_NOP: return new_node(IR_NOP);
   case STMT_EXPR:return ir_expr(s->parent, s->expr);
   case STMT_RETURN:
      tmp = new_node(IR_RET);
      if (s->expr) {
         n = ir_expr(s->parent, s->expr);
         tmp->type = IR_IRET;
         tmp->unary.size = IRS_INT; // TODO
         tmp->unary.reg = creg - 1;
         ir_append(n, tmp);
      } else n = tmp;
      return n;
   case STMT_SCOPE:
      n = new_node(IR_BEGIN_SCOPE);
      n->scope = s->scope;
      for (size_t i = 0; i < buf_len(s->scope->body); ++i)
         n = ir_append(n, ir_stmt(s->scope->body[i]));
      tmp = new_node(IR_END_SCOPE);
      tmp->scope = s->scope;
      return ir_append(n, tmp);
   case STMT_VARDECL:
      n = new_node(IR_NOP);
      return n;
      

   default: panic("ir_stmt(): unsupported statement '%s'", stmt_type_str[s->type]);
   }
}

ir_node_t* irgen_stmt(const struct statement* s) {
   creg = 0;
   return ir_stmt(s);
}