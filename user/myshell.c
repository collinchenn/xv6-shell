#include "kernel/types.h"
#include "user/user.h"

#define IB_LEN 1024
#define MAX_ARGS 32
static char input_buffer[IB_LEN];

int parse_input(char *line, char *argv[]) {
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

  while (*p != '\0') {
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

int main(int argc, char *argv[]) {
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

    // Create a fork of the current process
    int pid = fork();
    if (pid < 0) {
      fprintf(2, "error forking\n");
      continue;
    }

    // if fork is the child, execute process
    if (pid == 0) {
      if (exec(parsed_args[0], parsed_args) < 0) {
        fprintf(2, "error finding syscall\n");
        exit(1);
      }
    // wait for process to finish
    } else {
      wait(0);
    }
  }
}

