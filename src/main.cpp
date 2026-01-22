#include "chip8.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include <vector>

void render_lcd_style(SDL_Renderer* renderer, SDL_Texture* texture, int width, int height, int scale);

int main(int, char**) {
  const int SCALE = 15;

  Chip8 chip8;
  chip8.load_rom("../roms/Space Invaders [David Winter].ch8");

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* Window = SDL_CreateWindow("CHIP-8 Emulator", Chip8::WIDTH * SCALE, Chip8::HEIGHT * SCALE, 0);
  SDL_Renderer* Renderer = SDL_CreateRenderer(Window, NULL);

  chip8.init_double_buffer(Renderer);

  std::vector<uint32_t> pixels(Chip8::WIDTH * Chip8::HEIGHT);

  const int TARGET_FPS = 60;
  const int CYCLES_PER_FRAME = 5;

  bool IsRunning = true;
  SDL_Event Event;

  Uint64 frame_start = SDL_GetPerformanceCounter();
  Uint64 last_frame_time = frame_start;
  Uint64 accumulator = 0;

  while (IsRunning) {
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 delta_ms = (current_time - last_frame_time) * 1000 / SDL_GetPerformanceFrequency();

    accumulator += delta_ms;
    const int FRAME_DURATION_MS = 1000 / TARGET_FPS;

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

    if (accumulator >= FRAME_DURATION_MS) {
      int cycles_to_execute = static_cast<int>((accumulator / FRAME_DURATION_MS) * CYCLES_PER_FRAME);

      for (int i = 0; i < cycles_to_execute; ++i) {
        chip8.emulate_cycle();
      }

      accumulator %= FRAME_DURATION_MS;
      chip8.update_timers();
      chip8.set_frame_complete(true);

      SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
      SDL_RenderClear(Renderer);

      if (chip8.draw_flag) {
        uint32_t* pixels;
        int pitch;
        SDL_LockTexture(chip8.back_buffer, NULL, reinterpret_cast<void**>(&pixels), &pitch);
        for (std::size_t i = 0; i < chip8.gfx.size(); ++i) {
          uint8_t pixel = chip8.gfx[i];
          pixels[i] = pixel ? 0xFFFFFFFF : 0xFF000000;
        }
        SDL_UnlockTexture(chip8.back_buffer);

        std::swap(chip8.front_buffer, chip8.back_buffer);
        chip8.draw_flag = false;
      }

      render_lcd_style(Renderer, chip8.front_buffer, Chip8::WIDTH, Chip8::HEIGHT, SCALE); //this actually makes it look nice

      SDL_RenderPresent(Renderer);
      last_frame_time = current_time;
    } else {
      SDL_Delay(1);
    }
  }
  
  chip8.destroy_double_buffer();
  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}

void render_lcd_style(SDL_Renderer* renderer, SDL_Texture* texture, int width, int height, int scale) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

  const int pixel_size = scale;
  const int gap = 0;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      SDL_FRect src_rect = {static_cast<float>(x), static_cast<float>(y), 1.0f, 1.0f};
      SDL_FRect dst_rect = {
        static_cast<float>(x * pixel_size + gap),
        static_cast<float>(y * pixel_size + gap),
        static_cast<float>(pixel_size - gap),
        static_cast<float>(pixel_size - gap)
      };
      SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);
    }
  }
}
