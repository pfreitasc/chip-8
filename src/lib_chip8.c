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
  chip8->PC = RAM_PROGRAM_START;

  //clearing variables
  chip8->I = 0;
  chip8->SP = 0;
  chip8->opcode = 0;
  chip8->key = 0;
  chip8->delay_timer = 0;
  chip8->sound_timer = 0;
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
  printf("Iniciando loop infinito.\n");
  while (1){
    //fetch stage
    chip8->opcode = chip8->ram[chip8->PC];
    chip8->opcode = (chip8->opcode)<<8;
    chip8->opcode = (chip8->opcode) | chip8->ram[(chip8->PC) + 1];

    unsigned short current_opcode = chip8->opcode;
    unsigned short opcode_nibble1 = chip8->opcode & 0xF000;
    unsigned short opcode_nibble4 = chip8->opcode & 0x000F;
    unsigned short opcode_byte2 = chip8->opcode & 0x00FF;

    chip8->PC = chip8->PC + 2;

    //decode & execute stage
    printf("addr: %#04X, opcode: %#04X, instruction: ", chip8->PC, chip8->opcode);
    switch (opcode_nibble1) {
      case 0x0000:
        switch (opcode_byte2) {
          case 0x00E0: //00E0 - clear screen
            printf("00E0");
          break;

          case 0x00EE: //00EE - return from subroutine
            printf("00EE");
          break;

          default: //doesn't exist
            printf("Doesn't exist");
          break;
        }
      break;
      
      case 0x1000: //1NNN - jump to address
        printf("1NNN");
      break;

      case 0x2000: //2NNN - jump to subroutine
        printf("2NNN");
      break;

      case 0x3000: //3XNN - skip if different (reg with val)
        printf("3XNN");
      break;

      case 0x4000: //4XNN skip if equals (reg with val)
        printf("4XNN");
      break;

      case 0x5000: //5XY0 skip if equals (reg with reg)
        printf("5XY0");
      break;

      case 0x6000: //6XNN assign (val to reg)
        printf("6XNN");
      break;

      case 0x7000: //7XNN accumulate (val to reg)
        printf("7XNN");
      break;
      
      case 0x8000:
        switch (opcode_nibble4) {
          case 0x0000: //8XY0 assign (reg to reg)
            printf("8XY0");
          break;

          case 0x0001: //8XY1 bitwise OR (reg with reg)
            printf("8XY1");
          break;

          case 0x0002: //8XY2 bitwise AND (reg with reg)
            printf("8XY2");
          break;

          case 0x0003: //8XY3 bitwise XOR (reg with reg)
            printf("8XY3");
          break;

          case 0x0004: //8XY4 accumulate (reg to reg)
            printf("8XY4");
          break;

          case 0x0005: //8XY5 subtract then assign (reg to reg)
            printf("8XY5");
          break;

          case 0x0006: //8XY6 assign then shift right (reg by reg) or shift (reg)
            printf("8XY6");
          break;

          case 0x0007: //8XY7 neg subtract then assign (reg to reg)
            printf("8XY7");
          break;

          case 0x000E: //8XYE 8XY6 assign then shift left (reg by reg) or shift (reg)
            printf("8XYE");
          break;

          default: //doesn't exist
            printf("Doesn't exist");
          break;
        }
      break;

      case 0x9000: //9XY0 skip if equals (reg with reg)
        printf("9XY0");
      break;

      case 0xA000: //ANNN assign to ram pointer
        printf("ANNN");
      break;

      case 0xB000: //BNNN jump to addr + v0
        printf("BNNN");
      break;

      case 0xC000: //CXNN random number AND val
        printf("CXNN");
      break;

      case 0xD000: //DXYN draw sprite
        printf("DXYN");
      break;

      case 0xE000: 
        switch (opcode_nibble4) {
          case 0x000E: //EX9E skip if key not pressed
            printf("EX9E");
          break;

          case 0x0001: //EXA1 skip if key pressed
            printf("EXA1");
          break;

          default: //Doesn't exist
            printf("Doesn't exist");
          break;
        }
      break;

      case 0xF000: 
        switch (opcode_byte2) {
          case 0x0007: //FX07 assign delay timer to reg
            printf("FX07");
          break;

          case 0x000A: //FX0A assign key to reg (wait for keypress)
            printf("FX0A");
          break;

          case 0x0015: //FX15 assign reg to delay timer
            printf("FX15");
          break

          case 0x0018: //FX18 assign reg to sound timer
            printf("FX18");
          break;

          case 0x001E: //FX1E accumulate to ram pointer
            printf("FX1E");
          break;

          case 0x0029: //FX29 assign ram pointer to hex char in reg (in ram region 0~0x50)
            printf("FX29");
          break;

          case 0x0033: //FX33 bcd reg
            printf("FX33");
          break;

          case 0x0055: //FX55 save regs to ram
            printf("FX55");
          break;

          case 0x0065: //FX65 load regs from ram
            printf("FX65");
          break;

          default: //doesn't exist
            printf("Doesn't exist");
          break;
        }
      break;

      default: //doesn't exist
        printf("Doesn't exist");
      break;
    }
    printf("\n");
  }
}