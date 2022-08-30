#include <stdio.h>
#include "chip8.c"

int main(int argc, char *argv[]) {
  Chip8 chip8;

  Chip8_init(&chip8);
  Chip8_loadGame(&chip8, "../rom/games/Pong (1 player).ch8");
  //Chip8_loadGame(&chip8, "../rom/programs/Framed MK1 [GV Samways, 1980].ch8");
  Chip8_interpreterMainLoop(&chip8);
  return 0;
}
