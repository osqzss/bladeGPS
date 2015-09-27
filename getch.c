#include <unistd.h>
#include <termios.h>
#include "getch.h"

char _getch() {
  char c = 0;
  struct termios old;
  tcgetattr(0, &old);
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN]=1;
  old.c_cc[VTIME]=0;
  tcsetattr(0, TCSANOW, &old);
  if (read(0, &c, 1) < 0)
    return 0;
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  tcsetattr(0, TCSADRAIN, &old);
  return c;
}
