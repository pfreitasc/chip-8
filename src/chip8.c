#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_SCALE_FACTOR 10
#define RAM_SIZE 4096
#define RAM_PROGRAM_START 512
#define MAX_GAME_SIZE 4096-512
#define CPU_CLOCK_DELAY 0.001 //500Hz (0.002 s) should be enough for most basic games
#define TIMER_DELAY 0.0166667 //60Hz (0.0166667 s) for timers
#define SHIFT_INSTRUCTION 1 //if 1, 8XY6 and 8XYE just shift vx. if 0 first sets vx to vy then shifts
#define JUMP_INSTRUCTION 1 //if 1, BNNN uses v0, else it becomes BXNN, using vx
#define STORE_INSTRUCTION 1 //if 1, does not increment index while storing/loading registers

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <SDL2/SDL.h>

typedef struct { 
  unsigned char ram[RAM_SIZE];
  unsigned char display [SCREEN_WIDTH * SCREEN_HEIGHT];
  unsigned char V[16]; //all purpose registers
  unsigned short I; //memory address pointer
  unsigned short PC; //program address
  unsigned short opcode; //current opcode
  unsigned char delay_timer; //interpreter runs while > 0
  unsigned char sound_timer; //beeps while > 0
  unsigned short subroutine_stack [16]; //contains information to return from subroutines
  unsigned short SP; //points to top of subroutine stack
  unsigned char key; //current key being pressed on keypad
  unsigned char was_key_pressed; //variable that stores if there is currently a key being pressed
  float cycleCounter; //stores time elapsed in s since last 60Hz timing
} Chip8;

//SDL global variables
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

//initializes chip8 variables and SDL screen
//input: chip8 struct
void Chip8_init(Chip8 *chip8){
  int i;

  //starts random number
  srand(time(NULL));

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
  for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    chip8->display[i] = 0;

  //setting PC to start of program
  chip8->PC = RAM_PROGRAM_START;

  //clearing variables
  chip8->I = 0;
  chip8->SP = 0;
  chip8->opcode = 0;
  chip8->key = 16;
  chip8->was_key_pressed = 0;
  chip8->delay_timer = 0;
  chip8->sound_timer = 0;
  chip8->cycleCounter = 0;

  //initializing SDL
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
      printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
      return;
  }
  //initializing SDL global variables
  window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH*SCREEN_SCALE_FACTOR, SCREEN_HEIGHT*SCREEN_SCALE_FACTOR, SDL_WINDOW_SHOWN);
  if(window == NULL) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    return;
  }
  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL) {
    printf("SDL renderer could not be created.\n");
    return;
  }
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
  if (texture == NULL) {
    printf("SDL texture could not be created.\n");
    return;
  }

  //clear SDL screen
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
}

//loads game on chip 8 memory. game file size must be 3896 kb max 
//inputs: chip8 struct and file name string
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

//draws display matrix to sdl screen
//input: chip8 struct
void Chip8_drawDisplay(Chip8 *chip8) {
  SDL_UpdateTexture(texture, NULL, chip8->display, SCREEN_WIDTH * sizeof(unsigned char));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void Chip8_setKey(Chip8 *chip8) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_1:
          chip8->key = 0x1;
        break;

        case SDLK_2:
          chip8->key = 0x2;
        break;
        
        case SDLK_3:
          chip8->key = 0x3;
        break;
        
        case SDLK_4:
          chip8->key = 0xC;
        break;
        
        case SDLK_q:
          chip8->key = 0x4;
        break;

        case SDLK_w:
          chip8->key = 0x5;
        break;
        
        case SDLK_e:
          chip8->key = 0x6;
        break;
        
        case SDLK_r:
          chip8->key = 0xD;
        break;
        
        case SDLK_a:
          chip8->key = 0x7;
        break;
        
        case SDLK_s:
          chip8->key = 0x8;
        break;
        
        case SDLK_d:
          chip8->key = 0x9;
        break;
        
        case SDLK_f:
          chip8->key = 0xE;
        break;
        
        case SDLK_z:
          chip8->key = 0xA;
        break;
        
        case SDLK_x:
          chip8->key = 0x0;
        break;
        
        case SDLK_c:
          chip8->key = 0xB;
        break;
        
        case SDLK_v:
          chip8->key = 0xF;
        break;

        default:
          chip8->key = 0x10;
        break;
      }
    }

    if (event.type == SDL_KEYUP) {
      chip8->key = 16;
    }
  }
}

//timing function for the chip8
void Chip8_tick(Chip8 *chip8) {
  usleep(CPU_CLOCK_DELAY * 1000000);
  chip8->cycleCounter += CPU_CLOCK_DELAY;
  if (chip8->cycleCounter >= 1/60) {
    chip8->cycleCounter = 0;
    chip8->delay_timer -= 1;
    chip8->sound_timer -= 1;
  }
}

//instructions

//00E0: clear screen
void instr_clearScreen(Chip8 *chip8) {
  //clearing display
  int i = 0;
  for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
    chip8->display[i] = 0;
  //draw cleared display
  Chip8_drawDisplay(chip8);
}

//00EE: returns from subroutine
void instr_return(Chip8 *chip8) {
  chip8->PC = chip8->subroutine_stack[chip8->SP];
  chip8->SP -= 1;
}

//1NNN: jump to instruction in address nnn
void instr_jump(Chip8 *chip8) {
  unsigned short nnn = chip8->opcode & 0x0FFF;
  chip8->PC = nnn;
}

//2NNN: call subroutine in address nnn
void instr_callSubroutine(Chip8 *chip8) {
  unsigned short nnn = chip8->opcode & 0x0FFF;
  
  chip8->SP += 1;
  chip8->subroutine_stack[chip8->SP] = chip8->PC;
  chip8->PC = nnn;
}

//3XNN: skips next instruction if vx = nn
void instr_skipEq_vx_nn(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short nn = chip8->opcode & 0x00FF;

  if (chip8->V[x] == nn) {
    chip8->PC += 2;
  }
}

//4XNN: skips next instruction if vx != nn
void instr_skipNEq_vx_nn(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short nn = chip8->opcode & 0x00FF;

  if (chip8->V[x] != nn) {
    chip8->PC += 2;
  }
}

//5XY0: skips next instruction if vx == vy
void instr_skipEq_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  if (chip8->V[x] == chip8->V[y]) {
    chip8->PC += 2;
  }
}

//6XNN: set register vx to the value nn
void instr_set_vx_nn(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short nn = chip8->opcode & 0x00FF;
  
  chip8->V[x] = nn;
}

//7XNN: add value nn to vx
void instr_add_vx_nn(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short nn = chip8->opcode & 0x00FF;
  
  chip8->V[x] = chip8->V[x] + nn;
}

//8XY0: sets vx to value of vy
void instr_set_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  chip8->V[x] = chip8->V[y];
}

//8XY1: vx gets the result of vx OR vy (bitwise operation)
void instr_or_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  chip8->V[x] = chip8->V[x] | chip8->V[y];
}

//8XY2: vx gets the result of vx AND vy (bitwise operation)
void instr_and_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  chip8->V[x] = chip8->V[x] & chip8->V[y];
}

//8XY3: vx gets the result of vx XOR vy (bitwise operation)
void instr_xor_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  chip8->V[x] = chip8->V[x] ^ chip8->V[y];
}

//8XY4: vx gets the result of vx + vy
//vf = 1 if carry
void instr_add_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  unsigned short sum;

  sum = chip8->V[x] + chip8->V[y];
  chip8->V[x] = sum;

  if (sum > 255)
    chip8->V[0xF] = 1;
}

//8XY5: vx gets the result of vx - vy
//vf = 0 if borrows
void instr_sub_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  if (chip8->V[x] > chip8->V[y])
    chip8->V[0xF] = 1;
  else
    chip8->V[0xF] = 0;

  unsigned short sub;
  sub = chip8->V[x] - chip8->V[y];
  chip8->V[x] = sub;
}

//8XY6: sets vx to value of vy then shifts vx to the right
//or just shifts vx to the right (see SHIFT_INSTRUCTION)
//vf gets the shifted bit
void instr_shr_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  if (SHIFT_INSTRUCTION == 0)
    chip8->V[x] = chip8->V[y];
  
  if (chip8->V[x] & 0x01 == 1)
    chip8->V[0xF] = 1;
  else
    chip8->V[0xF] = 0;
  
  chip8->V[x] = chip8->V[x] >> 1;
}

//8XY7: vx gets the result of vy - vx
//vf = 0 if borrows
void instr_sub_vy_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  if (chip8->V[x] < chip8->V[y])
    chip8->V[0xF] = 1;
  else
    chip8->V[0xF] = 0;
  
  unsigned short sub;
  sub = chip8->V[y] - chip8->V[x];
  chip8->V[x] = sub;
}

//8XY6: sets vx to value of vy then shifts vx to the left
//or just shifts vx to the left (see SHIFT_INSTRUCTION)
//vf gets the shifted bit
void instr_shl_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  if (SHIFT_INSTRUCTION == 0)
    chip8->V[x] = chip8->V[y];
  
  if (chip8->V[x] & 0x80 == 0x80)
    chip8->V[0xF] = 1;
  else
    chip8->V[0xF] = 0;
  
  chip8->V[x] = chip8->V[x] << 1;
}

//9XY0: skips next instruction if vx != vy
void instr_skipNEq_vx_vy(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  if (chip8->V[x] != chip8->V[y]) {
    chip8->PC += 2;
  }
}

//ANNN: set index to value nnn
void instr_set_i(Chip8 *chip8) {
  unsigned short nnn = chip8->opcode & 0x0FFF;
  chip8->I = nnn; 
}

//BNNN: jump with offset
void instr_jumpOffset(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short nn = chip8->opcode & 0x00FF;
  unsigned short nnn = chip8->opcode & 0x0FFF;

  if (JUMP_INSTRUCTION == 1)
    chip8->PC = nnn + chip8->V[0];
  else
    chip8->PC = nn + chip8->V[x];
}

//CXNN: generates a random number then AND with nn
void instr_rand(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned char nn = chip8->opcode & 0x00FF;

  unsigned char rn = rand();
  chip8->V[x] = rn & nn;
}

//DXYN: draw N pixels tall sprite from memory pointed by the index starting in coordinate (x, y)
//vf = 1 if there is collision
void instr_draw(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned short y = chip8->opcode & 0x00F0;
  y = y >> 4;

  //since there are more coordinates than screen space, must do modulo operation (& in binary) to find the coordinate
  unsigned char vx = chip8->V[x] & 63;
  unsigned char vy = chip8->V[y] & 31;
  unsigned char h = chip8->opcode & 0x000F;

  //reset collision register
  chip8->V[0xF] = 0;

  unsigned char byte;
  unsigned char current_pixel;
  int i, j;
  //i represents the y coordinate
  //j represents the x coordinate
  for (i = 0; i < h; i++) {
    byte = chip8->ram[(chip8->I) + i];
    for (j = 0; j < 8; j++){
      current_pixel = byte & (0x80 >> j);
      //checking if there is a collision
      if (current_pixel != 0x00) {
        if (chip8->display[vx + j + SCREEN_WIDTH*(vy + i)] == 0xFF)
          chip8->V[0xF] = 1;
        chip8->display[vx + j + SCREEN_WIDTH*(vy + i)] = ~chip8->display[vx + j + SCREEN_WIDTH*(vy + i)];
      }
    }
  }
  Chip8_drawDisplay(chip8);
}

//EX9E: skips instruction if key corresponding to vx is pressed
void instr_skipEq_vx_key(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;

  if (chip8->V[x] == chip8->key) {
    chip8->PC += 2;
  }
}

//EXA1: skips instruction if key corresponding to vx is not pressed
void instr_skipNEq_vx_key(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;

  if (chip8->V[x] != chip8->key) {
    chip8->PC += 2;
  }
}

//FX07: vx gets current delay timer
void instr_set_vx_delayTimer(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;

  chip8->V[x] = chip8->delay_timer;
}

//FX0A: blocks execution while waiting for key and stores in vx when inputed
void instr_getKey(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;

  chip8->was_key_pressed = 0;

  if (chip8->key < 16) {
    chip8->V[x] = chip8->key;
    chip8->was_key_pressed = 1;
  }

  if (chip8->was_key_pressed == 0) {
    chip8->PC -= 2;
  }
}

//FX15: delay timer gets vx
void instr_set_delayTimer_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;

  chip8->delay_timer = chip8->V[x];
}

//FX18: sound timer gets vx
void instr_set_soundTimer_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;

  chip8->sound_timer = chip8->V[x];
}

//FX1E: add to index
//vf gets 1 if i exceeds adressing range (0x0FFF)
void instr_add_i_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;

  chip8->I =  chip8->I + chip8->V[x];

  if (chip8->I > 0x0FFF)
    chip8->V[0xF] = 1;
}

//FX29: points i to hex char in last nibble of vx
void instr_hex(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned char vx = chip8->V[x] & 0x0F;

  chip8->I = vx * 5;
}

//FX33: stores BCD value in vx in ram
void instr_store_vx_bcd(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  unsigned char vx = chip8->V[x];

  unsigned char dec100 = vx / 100;
  unsigned char dec10 = (vx / 10) % 10;
  unsigned char dec1 = (vx % 100) % 10;

  chip8->ram[chip8->I] = dec100;
  chip8->ram[(chip8->I) + 1] = dec10;
  chip8->ram[(chip8->I) + 2] = dec1;

}

//FX55: stores from v0 to vx in ram
void instr_store_v0_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  
  int i;
  for (i = 0; i <= x; i++)
    chip8->ram[chip8->I + i] = chip8->V[i];
  if (STORE_INSTRUCTION == 0)
    chip8->I = chip8->I + x + 1;
}

//FX65: load in v0 to vx ram data
void instr_load_v0_vx(Chip8 *chip8) {
  unsigned short x = chip8->opcode & 0x0F00;
  x = x >> 8;
  
  int i;
  for (i = 0; i <= x; i++)
    chip8->V[i] = chip8->ram[chip8->I + i];
  if (STORE_INSTRUCTION == 0)
    chip8->I = chip8->I + x + 1;
}

//implementation of the instruction fetch -> decode -> execute loop of the chip 8 interpreter
//input: initialized chip8 struct 
void Chip8_interpreterMainLoop(Chip8 *chip8) {
  printf("Starting loop.\n");
  while (1) {
    //fetch stage
    chip8->opcode = chip8->ram[chip8->PC];
    chip8->opcode = (chip8->opcode)<<8;
    chip8->opcode = (chip8->opcode) | chip8->ram[(chip8->PC) + 1];

    unsigned short opcode_nibble1 = chip8->opcode & 0xF000;
    unsigned short opcode_nibble4 = chip8->opcode & 0x000F;
    unsigned short opcode_byte2 = chip8->opcode & 0x00FF;

    chip8->PC += 2;

    //decode & execute stage
    printf("addr: %#04X, opcode: %#04X, instruction: ", chip8->PC, chip8->opcode);
    switch (opcode_nibble1) {
      case 0x0000:
        switch (opcode_byte2) {
          case 0x00E0: //00E0 - clear screen
            printf("00E0");
            instr_clearScreen(chip8);
          break;

          case 0x00EE: //00EE - return from subroutine
            printf("00EE");
            instr_return(chip8);
          break;

          default: //doesn't exist
            printf("Doesn't exist");
          break;
        }
      break;
      
      case 0x1000: //1NNN - jump to address
        printf("1NNN");
        instr_jump(chip8);
      break;

      case 0x2000: //2NNN - jump to subroutine
        printf("2NNN");
        instr_callSubroutine(chip8);
      break;

      case 0x3000: //3XNN - skip if different (reg with val)
        printf("3XNN");
        instr_skipEq_vx_nn(chip8);
      break;

      case 0x4000: //4XNN skip if equals (reg with val)
        printf("4XNN");
        instr_skipNEq_vx_nn(chip8);
      break;

      case 0x5000: //5XY0 skip if equals (reg with reg)
        printf("5XY0");
        instr_skipEq_vx_vy(chip8);
      break;

      case 0x6000: //6XNN assign (val to reg)
        printf("6XNN");
        instr_set_vx_nn(chip8);
      break;

      case 0x7000: //7XNN accumulate (val to reg)
        printf("7XNN");
        instr_add_vx_nn(chip8);
      break;
      
      case 0x8000:
        switch (opcode_nibble4) {
          case 0x0000: //8XY0 assign (reg to reg)
            printf("8XY0");
            instr_set_vx_vy(chip8);
          break;

          case 0x0001: //8XY1 bitwise OR (reg with reg)
            printf("8XY1");
            instr_or_vx_vy(chip8);
          break;

          case 0x0002: //8XY2 bitwise AND (reg with reg)
            printf("8XY2");
            instr_and_vx_vy(chip8);
          break;

          case 0x0003: //8XY3 bitwise XOR (reg with reg)
            printf("8XY3");
            instr_xor_vx_vy(chip8);
          break;

          case 0x0004: //8XY4 accumulate (reg to reg)
            printf("8XY4");
            instr_add_vx_vy(chip8);
          break;

          case 0x0005: //8XY5 subtract then assign (reg to reg)
            printf("8XY5");
            instr_sub_vx_vy(chip8);
          break;

          case 0x0006: //8XY6 assign then shift right (reg by reg) or shift (reg)
            printf("8XY6");
            instr_shr_vx(chip8);
          break;

          case 0x0007: //8XY7 neg subtract then assign (reg to reg)
            printf("8XY7");
            instr_sub_vy_vx(chip8);
          break;

          case 0x000E: //8XYE 8XY6 assign then shift left (reg by reg) or shift (reg)
            printf("8XYE");
            instr_shl_vx(chip8);
          break;

          default: //doesn't exist
            printf("Doesn't exist");
          break;
        }
      break;

      case 0x9000: //9XY0 skip if different (reg with reg)
        printf("9XY0");
        instr_skipNEq_vx_vy(chip8);
      break;

      case 0xA000: //ANNN assign to ram pointer
        printf("ANNN");
        instr_set_i(chip8);
      break;

      case 0xB000: //BNNN jump to addr + v0
        printf("BNNN");
        instr_jumpOffset(chip8);
      break;

      case 0xC000: //CXNN random number AND val
        printf("CXNN");
        instr_rand(chip8);
      break;

      case 0xD000: //DXYN draw sprite
        printf("DXYN");
        instr_draw(chip8);
      break;

      case 0xE000: 
        switch (opcode_nibble4) {
          case 0x000E: //EX9E skip if key is pressed
            printf("EX9E");
            instr_skipEq_vx_key(chip8);
          break;

          case 0x0001: //EXA1 skip if key not pressed
            printf("EXA1");
            instr_skipNEq_vx_key(chip8);
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
            instr_set_vx_delayTimer(chip8);
          break;

          case 0x000A: //FX0A assign key to reg (wait for keypress)
            printf("FX0A");
            instr_getKey(chip8);
          break;

          case 0x0015: //FX15 assign reg to delay timer
            printf("FX15");
            instr_set_delayTimer_vx(chip8);
          break;

          case 0x0018: //FX18 assign reg to sound timer
            printf("FX18");
            instr_set_soundTimer_vx(chip8);
          break;

          case 0x001E: //FX1E accumulate to ram pointer
            printf("FX1E");
            instr_add_i_vx(chip8);
          break;

          case 0x0029: //FX29 assign ram pointer to hex char in reg (in ram region 0~0x50)
            printf("FX29");
            instr_hex(chip8);
          break;

          case 0x0033: //FX33 bcd reg
            printf("FX33");
            instr_store_vx_bcd(chip8);
          break;

          case 0x0055: //FX55 save regs to ram
            printf("FX55");
            instr_store_v0_vx(chip8);
          break;

          case 0x0065: //FX65 load regs from ram
            printf("FX65");
            instr_load_v0_vx(chip8);
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

    //Store key being pressed
    Chip8_setKey(chip8);

    //Slow system speed to 500Hz
    Chip8_tick(chip8);
  }
}