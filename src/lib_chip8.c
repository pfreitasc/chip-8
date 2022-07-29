#define DISPLAY_SIZE 64*32
#define RAM_SIZE 4096
#define RAM_PROGRAM_START 512
#define MAX_GAME_SIZE 4096-512

#include <stdio.h>
#include <time.h>

typedef struct { 
  unsigned char ram[RAM_SIZE];
  unsigned char display [DISPLAY_SIZE];
  unsigned char V[16]; //all purpose registers
  unsigned short I; //memory address pointer
  unsigned short PC; //program address
  unsigned short opcode; //current opcode
  unsigned char delay_timer; //interpreter runs while > 0
  unsigned char sound_timer; //beeps while > 0
  unsigned short subroutine_stack [16]; //contains information to return from subroutines
  unsigned short SP; //points to top of subroutine stack
  unsigned char key; //current key being pressed on keypad
} Chip8;

//initializes chip8 variables and SDL screen
//input: chip8 object
void Chip8_init(Chip8 *chip8){
  int i, j;

  //clearing RAM
  for (i = 0; i < 4096; i++)
    chip8->ram[i] = 0;
  
  //loading fontset on RAM. ADDR 0x00 to 0x50
  unsigned char fontset[80] =
  { 
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };
  for (i = 0; i < 80; i++)
    chip8->ram[i] = fontset[i];

  //clearing subroutine stack
  for (i = 0; i < 16; i++) {
    chip8->V[i] = 0;
    chip8->subroutine_stack[i] = 0;
  }
  
  //clearing display
  for (i = 0; i < DISPLAY_SIZE; i++)
    chip8->display[i] = 0;

  //setting PC to start of program
  chip8->PC = RAM_PROGRAM_START + 1;

  //clearing registers
  chip8->I = 0;
  chip8->SP = 0;
  chip8->opcode = 0;
  chip8->key = 0;
}

//loads game on chip 8 memory. game file size must be 3896 kb max 
//inputs: chip8 object and file name string
void Chip8_loadGame (Chip8 *chip8, char *filename) {
  FILE *fgame;
  fgame = fopen(filename, "rb");
  
  //checking if file opened
  if (fgame == NULL) {
    printf("Couldn't open the game file: %s\n", filename);
    return;
  }

  fread(&(chip8->ram[RAM_PROGRAM_START]), 1, MAX_GAME_SIZE, fgame);
  fclose(fgame);

  return;
}

void Chip8_interpreter(Chip8 *chip8){
  printf("Iniciando loop infinito.");
  while (1){
    //fetch stage
    chip8->opcode = chip8->ram[chip8->PC];
    chip8->opcode = (chip8->opcode)<<8;
    chip8->opcode = (chip8->opcode) | chip8->ram[(chip8->PC) + 1];

    chip8->PC = chip8->PC + 2;

    //decode & execute stage
    
  }
}