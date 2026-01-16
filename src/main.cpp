#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chip8.h"
#include <vector>

int main(int, char**) {
  Chip8 chip8;
	chip8.load_rom("../roms/IBM Logo.ch8");

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* Window = SDL_CreateWindow("CHIP-8 Emulator", Chip8::WIDTH * 10, Chip8::HEIGHT * 10, 0);
  SDL_Renderer* Renderer = SDL_CreateRenderer(Window, NULL);

  SDL_Texture* Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, Chip8::WIDTH, Chip8::HEIGHT);

  std::vector<uint32_t> pixels(Chip8::WIDTH * Chip8::HEIGHT);

  bool IsRunning = true;
  SDL_Event Event;

  while (IsRunning) {
    while (SDL_PollEvent(&Event)) {
      if (Event.type == SDL_EVENT_QUIT) {
        IsRunning = false;
      } else if (Event.type == SDL_EVENT_KEY_DOWN) {
        int key = chip8.get_key(Event.key.scancode);
        if (key != -1) {
          chip8.set_key_down(key);
        }
      } else if (Event.type == SDL_EVENT_KEY_UP) {
        int key = chip8.get_key(Event.key.scancode);
        if (key != -1) {
          chip8.set_key_up(key);
        }
      }
    }

    SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
    SDL_RenderClear(Renderer);

    if (chip8.draw_flag) {
      for (std::size_t i = 0; i < chip8.gfx.size(); ++i) {
        uint8_t pixel = chip8.gfx[i];
        pixels[i] = pixel ? 0xFFFFFFFF : 0xFF000000;
      }

      SDL_UpdateTexture(Texture, NULL, pixels.data(), Chip8::WIDTH * sizeof(uint32_t));

      SDL_FRect dest_rect;
      dest_rect.x = 0;
      dest_rect.y = 0;
      dest_rect.w = Chip8::WIDTH * 10;
      dest_rect.h = Chip8::HEIGHT * 10;

      SDL_RenderTexture(Renderer, Texture, NULL, &dest_rect);
      chip8.draw_flag = false;
    } else {
      SDL_FRect dest_rect;
      dest_rect.x = 0;
      dest_rect.y = 0;
      dest_rect.w = Chip8::WIDTH * 10;
      dest_rect.h = Chip8::HEIGHT * 10;

      SDL_RenderTexture(Renderer, Texture, NULL, &dest_rect);
    }

    SDL_RenderPresent(Renderer);
  }

  SDL_DestroyTexture(Texture);
  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}
