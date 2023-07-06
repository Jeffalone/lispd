#include <stdio.h>
#include <stdlib.h>

#define EXIT_SUCCESS 0
#define TRUE 1
#define FALSE 0

#ifdef _WIN32
#include <string.h>
static char buffer[2048];

char *readline(char *prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char *cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

void add_history(char *unused) {}
#else
#include "include/mpc.h"
#include <editline/history.h>
#include <editline/readline.h>

#endif

typedef struct {
  int type;
  union {
    long iVal;
    double fVal;
  } num;
  int error;
} lval;

enum { LVAL_LONG, LVAL_ERR, LVAL_FLOAT };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_long(long x) {
  lval v;
  v.type = LVAL_LONG;
  v.num.iVal = x;
  return v;
}

lval lval_float(double x) {
  lval v;
  v.type = LVAL_FLOAT;
  v.num.fVal = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.error = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_LONG:
    printf("%li", v.num.iVal);
    break;
  case LVAL_FLOAT:
    printf("%f", v.num.fVal);
    break;
  case LVAL_ERR:
    if (v.error == LERR_DIV_ZERO) {
      printf("Error: Division By Zero!");
    }
    if (v.error == LERR_BAD_OP) {
      printf("Error: Invalid Operator!");
    }
    if (v.error == LERR_BAD_NUM) {
      printf("Error: Invalid Number");
    }
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_float_op(lval x, char *operator, lval y) {
  /* handle mixed operation everything will become a double */
  double trueX, trueY;
  if (x.type == LVAL_LONG) {
    trueX = (double)x.num.iVal;
  } else if (x.type == LVAL_FLOAT) {
    trueX = x.num.fVal;
  } else {
    return lval_err(LERR_BAD_NUM);
  }

  if (y.type == LVAL_LONG) {
    trueY = (double)y.num.iVal;
  } else if (y.type == LVAL_FLOAT) {
    trueY = y.num.fVal;
  } else {
    return lval_err(LERR_BAD_NUM);
  }

  if (strcmp(operator, "+") == 0) {
    return lval_float(trueX + trueY);
  }
  if (strcmp(operator, "-") == 0) {
    return lval_float(trueX - trueY);
  }
  if (strcmp(operator, "*") == 0) {
    return lval_float(trueX * trueY);
  }
  if (strcmp(operator, "/") == 0) {
    return trueY == 0 ? lval_err(LERR_DIV_ZERO) : lval_float(trueX / trueY);
  }
  return lval_err(LERR_BAD_OP);
}

lval eval_integer_op(lval x, char *operator, lval y) {
  /* do math */
  if (strcmp(operator, "+") == 0) {
    return lval_long(x.num.iVal + y.num.iVal);
  }
  if (strcmp(operator, "-") == 0) {
    return lval_long(x.num.iVal - y.num.iVal);
  }
  if (strcmp(operator, "*") == 0) {
    return lval_long(x.num.iVal - y.num.iVal);
  }
  if (strcmp(operator, "/") == 0) {
    return y.num.iVal == 0 ? lval_err(LERR_DIV_ZERO)
                           : lval_long(x.num.iVal / y.num.iVal);
  }

  return lval_err(LERR_BAD_OP);
}

lval eval_op(lval x, char *operator, lval y) {
  /* If value is an error pass it back */
  if (x.type == LVAL_ERR) {
    return x;
  }
  if (y.type == LVAL_ERR) {
    return y;
  }
  if (x.type == LVAL_FLOAT || y.type == LVAL_FLOAT) {
    return eval_float_op(x, operator, y);
  } else {
    return eval_integer_op(x, operator, y);
  }
}

lval eval(mpc_ast_t *t) {
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_long(x) : lval_err(LERR_BAD_NUM);
  }

  if (strstr(t->tag, "double")) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lval_float(x) : lval_err(LERR_BAD_NUM);
  }

  char *op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  return x;
}

int main(int argc, char *argv[]) {
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Double = mpc_new("double");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispd = mpc_new("lispd");

  mpca_lang(MPCA_LANG_DEFAULT, "\
number : /-?[0-9]+/ ; \
double : /-?[0-9]+\\.[0-9]+/ ;\
operator: '+' | '-' | '*' | '/' ; \
expr : <double> | <number> | '(' <operator> <expr>+ ')' ; \
lispd : /^/ <operator> <expr>+ /$/ ; \
",
            Number, Double, Operator, Expr, Lispd);

  puts("Lispd Version 0.0.0.0.5");
  puts("Press Ctrl+c to Exit\n");

  while (TRUE) {
    char *input = readline("lispd> ");

    add_history(input);

    if (strcmp(input, "exit") == 0) {
      exit(0);
    }

    mpc_result_t res;
    if (mpc_parse("<stdin>", input, Lispd, &res)) {
      lval result = eval(res.output);
      lval_println(result);
      mpc_ast_delete(res.output);
    } else {
      mpc_err_print(res.error);
      mpc_err_delete(res.error);
    }

    free(input);
  }

  mpc_cleanup(5, Double, Number, Operator, Expr, Lispd);
  return EXIT_SUCCESS;
}
