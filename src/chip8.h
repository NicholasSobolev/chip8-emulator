#pragma once
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include <array>
#include <cstdint>
#include <string>
#include <random>


class Chip8 {
 public:
  static constexpr uint16_t MEMORY_SIZE = 4096;
  static constexpr uint16_t FONT_SIZE = 80;
  static constexpr uint16_t PROGRAM_START = 0x200;
  static constexpr uint16_t MAX_ROM_SIZE = MEMORY_SIZE - PROGRAM_START;
  static constexpr int WIDTH = 64;
  static constexpr int HEIGHT = 32;

  Chip8();
  ~Chip8();

  // Disable copy operations due to pointer members
  Chip8(const Chip8&) = delete;
  Chip8& operator=(const Chip8&) = delete;
  void load_rom(const std::string& path);
  void emulate_cycle();
  void load_font_set();
  void update_timers();
  void play_beep();

  //input handling
  void set_key_down(const int key);
  void set_key_up(const int key);
  static int get_key(SDL_Scancode scancode);

  //display
  std::array<uint8_t, WIDTH * HEIGHT> gfx{};
  bool draw_flag{false};

  //double buffering
  SDL_Texture* front_buffer{};
  SDL_Texture* back_buffer{};
  bool frame_complete{false};

  void init_double_buffer(SDL_Renderer* renderer);
  void set_frame_complete(bool complete);
  void destroy_double_buffer();

 private:
    std::array<uint8_t, MEMORY_SIZE> memory{};
    std::array<uint8_t, 16> registers{};
    uint16_t index_register{};
    uint16_t program_counter{0x200};
    uint8_t delay_timer{};
    uint8_t sound_timer{};
    uint8_t stack_pointer{};
    std::array<uint16_t, 16> stack{};
    std::array<bool, 16> key_states{};

    uint16_t opcode{};

    // font and opcode helpers
    void fetch_opcode();
    void decode_and_execute();
    void execute_0x0(uint16_t opcode);
    void execute_0x2(uint16_t nnn);
    void execute_0xD(uint8_t x, uint8_t y, uint8_t n);
    void execute_0xE(uint8_t x, uint8_t kk);
    void execute_0x8(uint8_t x, uint8_t y, uint8_t n);
    void execute_0xF(uint8_t x, uint8_t kk);

    // RNG for opcode 0xC
    std::mt19937 rng;
    std::uniform_int_distribution<uint8_t> dist;

    // audio
    SDL_AudioStream* audio_stream;
};

