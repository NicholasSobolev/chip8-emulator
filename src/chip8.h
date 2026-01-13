#pragma once
#include <array>
#include <cstdint>
#include <string>

class Chip8 {
public:
  Chip8();
  void load_font_set();
  void load_rom(const std::string &path);
	//void emulate_cycle();

private:
  std::array<uint8_t, 4096> memory{};
  std::array<uint8_t, 16> registers{};
  uint16_t index_register{};
  uint16_t program_counter{0x200};
  uint8_t delay_timer{};
  uint8_t sound_timer{};
  uint8_t stack_pointer{};
  std::array<uint16_t, 16> stack{};
  static constexpr int width = 64;
  static constexpr int height = 32;
  std::array<uint8_t, width * height> gfx{};
  std::array<bool, 16> key_states{};
  uint16_t opcode{};
  bool draw_flag{false};
};
