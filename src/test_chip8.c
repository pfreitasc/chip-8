#include <stdio.h>
#include "chip8.c"

int main(int argc, char *argv[]) {
  Chip8 chip8;

  //The window we'll be rendering to
  SDL_Window* game_window = NULL;
  
  //The surface contained by the window
  SDL_Surface* game_surface = NULL;

  Chip8_init(&chip8, game_window, game_surface);
  Chip8_loadGame(&chip8, "../rom/IBM_Logo.ch8");
  Chip8_interpreterMainLoop(&chip8, game_window, game_surface);
  return 0;
}
