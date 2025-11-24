#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define STDERR 2

#define CMD_PROMPT "cs143a$ "
#define END_MSG "bye\n"

#define IB_LEN 1024
#define MAX_ARGS 32 // exec only accepts 32
#define MAX_CMD_LEN 512

static char input_buffer[IB_LEN];
char* parsed_args[MAX_ARGS];

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
parse_redirections(char *argv[], char **in, char **out) {
  int i = 0;
  int j = 0;
  *in = 0;
  *out = 0;

  while (argv[i]) {
    if (strcmp("<", argv[i]) == 0) {
      if (argv[i+1] == 0 || *in) {
        return -1;
      }
      *in = argv[i+1];
      i += 2;
    } else if (strcmp(">", argv[i]) == 0) {
      if (argv[i+1] == 0 || *out) {
        return -1;
      }
      *out = argv[i+1];
      i += 2;
    } else {
      argv[j++] = argv[i++];
    }
  }
  argv[j] = 0;
  return 0;
}

int
parse_exit_status(char *x) {
  // x cannot be null
  // or parse_exit will not be called
  int sign = 1;
  int i = 0;

  if (x[i] == '-') {
    sign = -1;
    i++;
  }

  if ((x[i] == '\0') ||
      (x[i] == '0' && x[i+1])) {
       return -1;
     }

  for (int j = i; x[j]; j++) {
    if (x[j] < '0' || x[j] > '9') {
      return -1;
    }
  }

  if (sign < 0) {
    x++;
  }

  int x_int = atoi(x);
  if (sign == -1 && x_int > 128) {
    return -1;
  }
  if (sign == 1 && x_int > 127) {
    return -1;
  }
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

void
about_cmd(void) {
  printf("Made by Collin\n");
}

void
exit_cmd(void) {
  int status_code;
  if (parsed_args[1] == 0) {
    status_code = 0;
  } else {
    status_code = parse_exit_status(parsed_args[1]);
  }
  printf(END_MSG);
  printf("Returning status code: %d\n", status_code);
  exit(status_code);
}

int
cd_cmd(void) {
  const char* path = parsed_args[1];
  if (!path) {
    path = "/";
  }

  if (chdir(path) < 0) {
    return -1;
  }

  return 0;
}

void
execute_cmd(char *in, char *out) {
  // ----------- Check built-ins ----------
  if (strcmp(parsed_args[0], "cd") == 0) {
    if (cd_cmd() < 0) {
      fprintf(STDERR, "error\n");
    }
    return;
  }

  if (strcmp(parsed_args[0], "about") == 0) {
    if (out) {
      int saved_fd = dup(1);
      if (saved_fd < 0) {
        fprintf(STDERR, "error\n");
        return;
      }

      close(1);
      if (open(out, O_WRONLY | O_CREATE | O_TRUNC) < 0) {
        fprintf(STDERR, "error\n");
        dup(saved_fd);
        close(saved_fd);
        return;
      }

      about_cmd();

      close(1);
      dup(saved_fd);
      close(saved_fd);
    } else {
      about_cmd();
    }
    return;
  }

  if (strcmp(parsed_args[0], "exit") == 0) {
    exit_cmd();
  }

  // ---------- Exec (if not built-in)  ----------
  // First check that input file exists
  if (in) {
    int fd = open(in, O_RDONLY);
    if (fd < 0) {
      fprintf(STDERR, "error\n");
      return;
    }
    close(fd);
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(STDERR, "error\n"); // forking
    return;
  }

  // If pid is the child, execute process
  if (pid == 0) {
    if (in) {
      close(0);
      int fd = open(in, O_RDONLY);
      if (fd < 0) {
        fprintf(STDERR, "error\n");
        exit(-1);
      }
    }

    if (out) {
      close(1);
      int fd = open(out, O_WRONLY | O_CREATE | O_TRUNC);
      if (fd < 0) {
        fprintf(STDERR, "error\n");
        exit(-1);
      }
    }

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
      fprintf(STDERR, "error\n");
      exit(1);
    }
    // Otherwise, pid is parent; wait for child
  } else {
    wait(0);
  }
}

int
main(int argc, char *argv[]) {
  memset(input_buffer, 0, IB_LEN);

  while (1) {
    fprintf(STDERR, CMD_PROMPT);
    gets(input_buffer, IB_LEN - 1);

    // Detect EOF/Crtl+D
    if (input_buffer[0] == '\0') {
      printf("\n");
      printf(END_MSG);
      exit(0);
    }

    if (!parse_input(input_buffer, parsed_args)) {
      continue;
    }

    char *in = 0;
    char *out = 0;

    if (parse_redirections(parsed_args, &in, &out) < 0) {
      fprintf(STDERR, "error\n");
      continue;
    }

    if (parsed_args[0] == 0) {
      fprintf(STDERR, "error\n");
      continue;
    }

    execute_cmd(in, out);
  }
}
