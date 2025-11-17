#include "kernel/types.h"
#include "user/user.h"

#define IB_LEN 1024
#define MAX_ARGS 32
#define MAX_CMD_LEN 512
static char input_buffer[IB_LEN];

// Helper functions

int
parse_input(char *line, char *argv[]) {
  int argc = 0;
  char *p = line;

  // Remove \n from the end
  int length = strlen(line);
  if (length > 0 && line[length - 1] == '\n') {
    line[length - 1] = '\0';
  }

  // Skip leading white lines
  while (*p == ' ') { p++; }

  // If nothing after white spaces
  // return 0, which reprompts
  if (*p == '\0') { return 0; }

  while (*p != '\0' && argc < MAX_ARGS - 1) {
    // Beginning of word
    argv[argc++] = p;

    // Move until end or space
    while (*p != '\0' && *p != ' ') {
      p++;
    }

    // This means it's the last arg
    if (*p == '\0') { break; }

    // Set *p to null terminator so
    // argv[argc-1] is terminated
    *p = '\0';
    p++;

    // Skip whitespaces till start
    // of next word or '\0'
    while (*p == ' ') { p++; }
  }

  // Null terminate argv for exec()
  argv[argc] = 0;
  return argc;
}

int
parse_exit(char *x) {
  // x cannot be null
  // or parse_exit will not be called
  int sign = 1;
  int i = 0;

  if (x[i] == '-') {
    sign = -1;
    i++;
  }

  if (x[i] == '\0') { return -1; }

  for (int j = i; x[j]; j++) {
    if (x[j] < '0' || x[j] > '9') {
      return -1;
    }
  }

  if (sign < 0) {
    x++;
  }
  int x_int = atoi(x);
  printf("xafter atoi: %d", x_int);
  return sign * x_int;
}

int
contains_slash(const char *s) {
  for (; *s; s++) {
    if (*s == '/') {
      return 1;
    }
  }
  return 0;
}

// Main

int
main(int argc, char *argv[]) {
  // Stores the user-passed args
  // ex: ['echo', 'hello', 'world', 0]
  char* parsed_args[MAX_ARGS];

  // Clear the buffer to be safe
  memset(input_buffer, 0, IB_LEN);

  while (1) {
    write(2, "cs143a$ ", 8);

    // Read input
    gets(input_buffer, IB_LEN - 1);

    // Detect EOF/Crtl+D
    if (input_buffer[0] == '\0') {
      printf("\nbye\n");
      exit(0);
    }

    // Parse the input into parsed_args
    // Store argument count in n
    int n = parse_input(input_buffer, parsed_args);
    if (n == 0) { continue; }

// Built-in methods

    if (strcmp(parsed_args[0], "cd") == 0) {
      char* path = parsed_args[1];
      if (*path == 0 || chdir(path) < 0) {
        fprintf(2, "error\n");
      }
      continue;
    }

    if (strcmp(parsed_args[0], "exit") == 0) {
      int status_code;
      if (*parsed_args[1] == 0) {
        status_code = 0;
      } else {
        status_code = parse_exit(parsed_args[1]);
      }
      printf("bye\n");
      printf("status code: %d\n", status_code);
      exit(status_code);
    }

    if (strcmp(parsed_args[0], "about") == 0) {
      printf("MyShell:\n");
      printf("  This shell can predict what you are going to run before\n");
      printf("  you do it. For example, it knew you were gonna call the\n");
      printf("  built-in 'about', and well... here you are.\n\n");
      printf("Author: Collin Chen\n");
      continue;
    }

// Exec commands

    // Create a fork of the current process
    int pid = fork();
    if (pid < 0) {
      fprintf(2, "error\n"); // forking
      continue;
    }

    // If pid is the child, execute process
    if (pid == 0) {
      if (exec(parsed_args[0], parsed_args) < 0) {
        // If exact path fails, see if only
        // the name of the binary file was given
        if (!contains_slash(parsed_args[0])) {
          char fallback[MAX_CMD_LEN];
          int i = 0;

          fallback[i++] = '/';
          for (char *s = parsed_args[0]; *s && i < MAX_CMD_LEN - 1; s++) {
            fallback[i++] = *s;
          }
          fallback[i] = '\0';

          exec(fallback, parsed_args);
        }

        // If this point is reached, can't EXEC
        fprintf(2, "error\n");
        exit(1);
      }
    // Otherwise, pid is parent; wait for child
    } else {
      wait(0);
    }
  }
}
