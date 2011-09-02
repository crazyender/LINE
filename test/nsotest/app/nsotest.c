#include <stdio.h>

int main(int argv, char *argc[])
{
  printf("Invoking msgbox()\n");
  msgbox("This is a message!");
  printf("Return from msgbox()\n");

  printf("Invoking msgbox2()\n");
  msgbox2("This is the title...", "...and this is the message");
  printf("Return from msgbox2()\n");
  return 0;
}
