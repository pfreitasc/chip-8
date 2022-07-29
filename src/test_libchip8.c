#include <stdio.h>
//#include <SDL2/SDL.h>
#include "lib_chip8.c"

int main(int argc, char *argv[]) {
  Chip8 chip8;
  Chip8_init(&chip8);
  Chip8_loadGame(&chip8, "../rom/IBM_Logo.ch8");
  Chip8_interpreter(&chip8);
  printf("ok\n");
  return 0;
}
