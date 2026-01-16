#include "chip8.h"

#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

Chip8::Chip8() : rng(std::random_device{}()), dist(0, 255) {}

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

  for (int i = 0; i < 80; ++i) {
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

void Chip8::emulate_cycle() {
  // fetch opcode
  opcode = (memory[program_counter] << 8) | memory[program_counter + 1];

  program_counter += 2;

  // decode opcode
  const auto first_nibble = (opcode >> 12) & 0xF;
  const auto x = (opcode >> 8) & 0xF;
  const auto y = (opcode >> 4) & 0xF;
  const auto n = opcode & 0xF;
  const auto kk = opcode & 0xFF;
  const auto nnn = opcode & 0xFFF;

  // decode and execute based on first nibble
  switch (first_nibble) {
  case 0x0:
    if (opcode == 0x00E0) {
      // clear screen
    } else if (opcode == 0x00EE) { // return from a subroutine
      program_counter = stack[stack_pointer];
      --stack_pointer;
    }
    break;

  case 0x1: // jump to location nnn
    program_counter = nnn;
    break;

  case 0x2: // call subroutine at nnn
    ++stack_pointer;
    stack[stack_pointer] = program_counter;
    program_counter = nnn;
    break;

  case 0x3: // skip next instruction if Vx = kk
    if (registers[x] == kk) {
      program_counter += 2;
    }
    break;

  case 0x4: // skip next instruction if Vx != kk
    if (registers[x] != kk) {
      program_counter += 2;
    }
    break;

  case 0x5: // skip next instruction if Vx = Vy
    if (registers[x] == registers[y]) {
      program_counter += 2;
    }
    break;

  case 0x6:
    registers[x] = kk;
    break;

  case 0x7:
    registers[x] = registers[x] + kk;
    break;

  case 0x8:
    execute_0x8(x, y, n);
    break;

  case 0x9:
    if (registers[x] != registers[y]) {
      program_counter += 2;
    }
    break;

  case 0xA:
    index_register = nnn;
    break;

  case 0xB:
    program_counter = nnn + registers[0];
    break;

  case 0xC: {
    auto random = dist(rng);
    registers[x] = random & kk;
    break;
  }

  case 0xD:
    // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF =
    // collision.
    // draw_flag = true;
    //   break;

  case 0xE:
    switch (kk) {
    case 0x9E:
      if (key_states[registers[x]]) {
        program_counter += 2;
      }
      break;

    case 0xA1:
      if (!key_states[registers[x]]) {
        program_counter += 2;
      }
      break;
    }

    break;

  case 0xF:
    execute_0xF(x, kk);
    break;
  }
}

void Chip8::execute_0x8(uint8_t x, uint8_t y, uint8_t n) {
  switch (n) {
  case 0x0:
    registers[x] = registers[y];
    break;
  case 0x1:
    registers[x] |= registers[y];
    break;
  case 0x2:
    registers[x] &= registers[y];
    break;
  case 0x3:
    registers[x] ^= registers[y];
    break;
  case 0x4: {
    uint16_t sum = registers[x] + registers[y];
    registers[0xF] = (sum > 0xFF) ? 1 : 0;
    registers[x] = sum & 0xFF;
    break;
  }
  case 0x5:
    registers[0xF] = (registers[x] > registers[y]) ? 1 : 0;
    registers[x] -= registers[y];
    break;
  case 0x6:
    registers[0xF] = registers[x] & 0x1;
    registers[x] >>= 1;
    break;
  case 0x7:
    registers[0xF] = (registers[y] > registers[x]) ? 1 : 0;
    registers[x] = registers[y] - registers[x];
    break;
  case 0xE:
    registers[0xF] = (registers[x] & 0x80) ? 1 : 0;
    registers[x] <<= 1;
    break;
  default:
    throw std::runtime_error("Unknown 0x8 opcode: 0x" + std::to_string(opcode));
  }
}

void Chip8::execute_0xF(uint8_t x, uint8_t kk) {
  switch (kk) {
  case 0x07:
    registers[x] = delay_timer;
    break;
  case 0x0A: {
    bool key_pressed = false;
    for (int i = 0; i < 16; ++i) {
      if (key_states[i]) {
        registers[x] = i;
        key_pressed = true;
        break;
      }
    }
    if (!key_pressed) {
      program_counter -= 2; // wait for key press
    }
    break;
  }
  case 0x15:
    delay_timer = registers[x];
    break;
  case 0x18:
    sound_timer = registers[x];
    break;
  case 0x1E:
    index_register += registers[x];
    break;
  case 0x29:
    index_register = registers[x] * 5;
    break;
  case 0x33: {
    uint8_t value = registers[x];
    memory[index_register] = value / 100;
    memory[index_register + 1] = (value / 10) % 10;
    memory[index_register + 2] = value % 10;
    break;
  }
  case 0x55:
    for (int i = 0; i <= x; ++i) {
      memory[index_register + i] = registers[i];
    }
    break;
  case 0x65:
    for (int i = 0; i <= x; ++i) {
      registers[i] = memory[index_register + i];
    }
    break;
  default:
    throw std::runtime_error("Unknown 0xF opcode: 0x" + std::to_string(opcode));
  }
}

