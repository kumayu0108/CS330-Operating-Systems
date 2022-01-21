#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  open("test/1/out/SriLankavNewZealand", O_RDWR | O_CREAT, 0644);
  return 0;
}
