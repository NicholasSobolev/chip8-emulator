#include "chip8.h"
#include <fstream>
#include <stdexcept>
#include <string>

Chip8::Chip8() = default;

void Chip8::load_font_set() {
  const uint8_t font_set[80] = {
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

  for (int i = 0; i < 80; i++) {
    memory[i] = font_set[i]; // store in interpreter area (0x000 - 0x1FF)
  }
};

void Chip8::load_rom(const std::string &path) {
  std::ifstream rom(
      path,
      std::ios::binary |
          std::ios::ate); // create input stream with pointer at end to get size
  if (!rom) {
    throw std::runtime_error("Failed to open ROM" + path);
  }

  auto size = rom.tellg();
  if (size < 0 || size > (4096 - 0x200)) {
    throw std::runtime_error("ROM file too large or invalid " + path);
  }

  rom.seekg(0,
            std::ios::beg); // move get pointer to beginning of file for reading
  rom.read(reinterpret_cast<char *>(&memory[0x200]),
           size); // expects char buffer so cast uint8_t pointer to char pointer
                  // for reading starting at 0x200 in memory
  if (!rom) {
    throw std::runtime_error("Failed to read ROM file: " + path);
  }
};
