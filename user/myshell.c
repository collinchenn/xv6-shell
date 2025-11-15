#include "kernel/types.h"
#include "user/user.h"

#define IB_LEN 1024
static char input_buffer[IB_LEN];

int main(int argc, char *argv[]) {
  // clear the buffer
  memset(input_buffer, 0, IB_LEN);

  // print prompt to stderr
  write(2, "dummy$ ", 7);

  // get user input
  gets(input_buffer, IB_LEN - 1);

  // print user input back.
  printf("%s", input_buffer);

  // exit process
  exit(0);
}
