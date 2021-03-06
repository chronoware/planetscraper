#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include "events.h"
#include "tick.h"
#include "render.h"
#include "states.h"
#include "colors.h"
#include "stateMenu.h"
#include "stateNewgame.h"
#include "stateGame.h"

#define OPTIONS 4
#define SETTINGS 2

static int choice = 0;
static char options[OPTIONS][16] = {
  "World name:",
  "      Seed:",
  "Create world",
  "Back"
};
static char settings[SETTINGS][32] = {
  "Test", ""
};

uint32_t getSeed(char *s) {
  uint32_t seed = 0xABCDDCBA;

  for(int i=0; i<strlen(s); i++) {
    seed <<= 1;
    seed ^= (s[i] + i);
  }

  return seed;
}

void newgameEvents() {
  SDL_Event e;
  
  while(SDL_PollEvent(&e)) {
    switch(e.type) {
      case SDL_QUIT:
        quit = true;
        break;
      
      case SDL_KEYDOWN:
        switch(e.key.keysym.sym) {
          case SDLK_UP:
            if(--choice == -1) choice = OPTIONS - 1;
            break;

          case SDLK_DOWN:
            if(++choice == OPTIONS) choice = 0;
            break;

          case SDLK_RETURN:
            if(!strcmp(options[choice], "Back")) nextState = &menuState;
            if(!strcmp(options[choice], "Create world") && strlen(settings[0])) {
              char extension[] = ".scrap";
              char *filename = malloc(sizeof(char)*(strlen(settings[0])+strlen(extension)));
              strcpy(filename, settings[0]);
              strcat(filename, extension);
              
              if(fp = fopen(filename, "r")) { // file already exists
                fclose(fp);
                printf("File exists!\n");
                // @TODO: add warning
              } else {
                fp = fopen(filename, "ab");
                worldGen();
                nextState = &gameState;
              }
              free(filename);
            }
            break;

          case SDLK_ESCAPE:
            nextState = &menuState;
            break;

          case SDLK_BACKSPACE:
            if(choice < SETTINGS && strlen(settings[choice])) {
              settings[choice][strlen(settings[choice])-1] = 0;
            }
            break;

          default:
            if(choice < SETTINGS && strlen(settings[choice]) < 31) {
              uint8_t pos = strlen(settings[choice]);
              settings[choice][pos]   = e.key.keysym.sym;
              settings[choice][pos+1] = 0;
            }
            break;
        }
        break;
    }
  }
}

void newgameTick() {
  clearScreen();

  for(int i=0; i<OPTIONS; i++) {
    write(1, SCREEN_H/2 + 5 + i + (i>OPTIONS-3 ? 1 : 0), options[i], choice == i ? COLOR_GREEN : COLOR_WHITE, 0);
  }

  for(int i=0; i<SETTINGS; i++) {
    write(13, SCREEN_H/2 + 5 + i, settings[i], choice == i ? COLOR_GREEN : COLOR_WHITE, 0);
  }
}

void newgameRedraw() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  for(uint32_t i=0; i<SCREEN_W * SCREEN_H * 5; i += 5) {
    if(screen[i] == 0) continue;

    SDL_Rect src;
    src.x = (screen[i]%16)*tileW; src.w = tileW;
    src.y = (screen[i]/16)*tileH; src.h = tileH;

    SDL_Rect dst;
    dst.x = ((i/5)%SCREEN_W)*tileW*zoom; dst.w = tileW*zoom;
    dst.y = ((i/5)/SCREEN_W)*tileH*zoom; dst.h = tileH*zoom;

    uint8_t r = fgRed(screen, i),
            g = fgGreen(screen, i),
            b = fgBlue(screen, i),
            rb = bgRed(screen, i),
            gb = bgGreen(screen, i),
            bb = bgBlue(screen, i);

    SDL_SetRenderDrawColor(renderer, rb, gb, bb, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &dst);
    SDL_SetTextureColorMod(tileset, r, g, b);
    SDL_RenderCopy(renderer, tileset, &src, &dst);
  }
  
  SDL_RenderPresent(renderer);
}

void worldGen() {
  char *signature = "SCRP";
  char *version   = "0001";
  char *worldName = malloc(33*sizeof(char));
  uint32_t seed = getSeed(settings[1]);

  for(uint8_t i=0; i<32; i++) {
    if(i < strlen(settings[0])) worldName[i] = settings[0][i];
    else worldName[i] = 0x20; // space character
  }
  worldName[32] = 0;

  // HEADER, 64B
  fprintf(fp, "%s", signature);
  fprintf(fp, "%s", version);
  fprintf(fp, "%s", worldName);
  fwrite(&seed, sizeof(uint32_t), 1, fp);
  uint16_t camPos = 0x800;
  fwrite(&camPos, sizeof(uint16_t), 1, fp);
  fwrite(&camPos, sizeof(uint16_t), 1, fp);
  for(uint8_t i=0; i<16; i++) putc('0', fp);

  // EMPTY CHUNK LIST
  uint32_t chunksBlockSize = 4;
  fwrite(&chunksBlockSize, sizeof(uint32_t), 1, fp);

  // EMPTY OBJECT LIST
  uint32_t objectBlockSize = 4;
  fwrite(&objectBlockSize, sizeof(uint32_t), 1, fp);
}


State newgameState = {
  .events = newgameEvents,
  .tick   = newgameTick,
  .redraw = newgameRedraw
};