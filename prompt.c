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

int main(int argc, char *argv[]) {
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispd = mpc_new("lispd");

  mpca_lang(MPCA_LANG_DEFAULT, "\
number : /-?[0-9]+/ ; \
operator: '+' | '-' | '*' | '/' ; \
expr : <number> | '(' <operator> <expr>+ ')' ; \
lispd : /^/ <operator> <expr>+ /$/ ; \
",
            Number, Operator, Expr, Lispd);

  puts("Lispd Version 0.0.0.0.2");
  puts("Press Ctrl+c to Exit\n");

  while (TRUE) {
    char *input = readline("lispd> ");

    add_history(input);

    printf("You said: %s\n", input);

    mpc_result_t res;
    if (mpc_parse("<stdin>", input, Lispd, &res)) {
      mpc_ast_print(res.output);
      mpc_ast_delete(res.output);
    } else {
      mpc_err_print(res.error);
      mpc_err_delete(res.error);
    }
    free(input);
  }
  mpc_cleanup(4, Number, Operator, Expr, Lispd);
  return EXIT_SUCCESS;
}
