
#include <stdio.h>
#include <stdlib.h>

char *serial = "\x31\x3e\x3d\x26\x31";

void check(char *ptr)
{
  int i = 0;

  while (i < 5){
    if (((ptr[i] - 1) ^ 0x55) != serial[i])
    {
      printf("Print wrong password\n");
      return;
    }
    
    i++;
  }
  
  printf("Good password ! You got it !\n");
}

int main(int ac, char **av)
{
  int ret;

  if (ac != 2)
  {
    printf("First param must be the string password");
    return -1;
  }

  check(av[1]);
}

