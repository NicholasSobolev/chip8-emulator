#include "chip8.h"

#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>

#include <iostream>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

Chip8::Chip8() : rng(std::random_device{}()), dist(0, 255) {
  load_font_set();
}
void Chip8::load_font_set() {
  const uint8_t font_set[FONT_SIZE] = {
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

  for (size_t i = 0; i < FONT_SIZE; ++i) {
    memory[i] = font_set[i]; // store in interpreter area (0x000 - 0x1FF)
  }
}

void Chip8::load_rom(const std::string& path) {
  std::ifstream rom(
      path,
      std::ios::binary |
          std::ios::ate); // create input stream with pointer at end to get size
  if (!rom) {
    throw std::runtime_error("Failed to open ROM" + path);
  }

  auto size = rom.tellg();
  if (size < 0 || size > MAX_ROM_SIZE) {
    throw std::runtime_error("ROM file too large or invalid " + path);
  }

  rom.seekg(0,
            std::ios::beg); // move get pointer to beginning of file for reading
  rom.read(reinterpret_cast<char *>(&memory[PROGRAM_START]),
           size); // expects char buffer so cast uint8_t pointer to char pointer
                  // for reading starting at 0x200 in memory
  if (!rom) {
    throw std::runtime_error("Failed to read ROM file: " + path);
  }
}

void Chip8::emulate_cycle() {
  fetch_opcode();
  decode_and_execute();
}

void Chip8::fetch_opcode() {
  opcode = (memory[program_counter] << 8) | memory[program_counter + 1];
  program_counter += 2;
}

void Chip8::decode_and_execute() {
  const uint8_t first_nibble = static_cast<uint8_t>((opcode >> 12) & 0xF);
  const uint8_t x = static_cast<uint8_t>((opcode >> 8) & 0xF);
  const uint8_t y = static_cast<uint8_t>((opcode >> 4) & 0xF);
  const uint8_t n = static_cast<uint8_t>(opcode & 0xF);
  const uint8_t kk = static_cast<uint8_t>(opcode & 0xFF);
  const uint16_t nnn = opcode & 0xFFF;

  switch (first_nibble) {
  case 0x0:
    execute_0x0(opcode);
    break;

  case 0x1: // jump to location nnn
    program_counter = nnn;
    break;

  case 0x2: // call subroutine at nnn
    execute_0x2(nnn);
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
    registers[x] += kk;
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
    uint8_t random = dist(rng);
    registers[x] = random & kk;
    break;
  }

  case 0xD:
    execute_0xD(x, y, n);
    break;
  case 0xE:
    execute_0xE(x, kk);
    break;

  case 0xF:
    execute_0xF(x, kk);
    break;
  }
}

void Chip8::execute_0x0(uint16_t opcode) {
  if (opcode == 0x00E0) {
    gfx.fill(0);
    draw_flag = true;
  } else if (opcode == 0x00EE) {
    if (stack_pointer == 0) {
      throw std::runtime_error("Stack underflow on return");
    }
    program_counter = stack[stack_pointer];
    --stack_pointer;
  }
}

void Chip8::execute_0x2(uint16_t nnn) {
  if (stack_pointer >= 15) {
    throw std::runtime_error("Stack overflow on call");
  }
  ++stack_pointer;
  stack[stack_pointer] = program_counter;
  program_counter = nnn;
}

void Chip8::execute_0xD(uint8_t x, uint8_t y, uint8_t n) {
  const uint8_t x_pos = registers[x] % WIDTH;
  const uint8_t y_pos = registers[y] % HEIGHT;
  const uint8_t height_n = n; // low nibble (how many rows tall the sprite is)

  registers[0xF] = 0; // assume no collision

  for (uint8_t row = 0; row < height_n; ++row) {
    uint8_t sprite = memory[index_register + row]; // store sprite in memory starting at index_register

    for (uint8_t bit = 0; bit < 8; ++bit) {
      if (sprite & (0x80 >> bit)) { // check if pixel is set in sprite
        const uint8_t px = (x_pos + bit) % WIDTH; // updated x/y positions for sprite bit, where WIDTH and HEIGHT considers screen wrap
        const uint8_t py = (y_pos + row) % HEIGHT;
        const std::size_t pixel_index = static_cast<std::size_t>(py) * WIDTH + px;

        if (gfx[pixel_index] == 1) {
          registers[0xF] = 1; // collision found
        }
        gfx[pixel_index] ^= 1; // xor to display
        draw_flag = true;
      }
    }
  }
}

void Chip8::set_frame_complete(bool complete) {
  frame_complete = complete;
}

void Chip8::execute_0xE(uint8_t x, uint8_t kk) {
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
    registers[x] = static_cast<uint8_t>(sum);
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
    for (std::size_t i = 0; i < key_states.size(); ++i) {
      if (key_states[i]) {
        registers[x] = static_cast<uint8_t>(i);
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
    index_register = (index_register + registers[x]) & 0xFFF;
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
    for (std::size_t i = 0; i <= static_cast<std::size_t>(x); ++i) {
      memory[index_register + i] = registers[i];
    }
    break;
  case 0x65:
    for (std::size_t i = 0; i <= static_cast<std::size_t>(x); ++i) {
      registers[i] = memory[index_register + i];
    }
    break;
  default:
    throw std::runtime_error("Unknown 0xF opcode: 0x" + std::to_string(opcode));
  }
}

void Chip8::update_timers() {
  if (delay_timer > 0) {
    --delay_timer;
  }

  if (sound_timer > 0) {
    --sound_timer;

    if (sound_timer == 0) {
      // TODO: sdl audio
      std::cout << "BEEP!\n";
    }
  }
}

void Chip8::set_key_down(const int key){
	if (key >= 0 && key < 16){
		key_states[static_cast<std::size_t>(key)] = true;
	}
}

void Chip8::set_key_up(const int key){
	if (key >= 0 && key < 16){
		key_states[static_cast<std::size_t>(key)] = false;
	}
}

void Chip8::init_double_buffer(SDL_Renderer* renderer) {
  front_buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  if (!front_buffer) {
    throw std::runtime_error("Failed to create front texture");
  }

  back_buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  if (!back_buffer) {
    SDL_DestroyTexture(front_buffer);
    throw std::runtime_error("Failed to create back texture");
  }

  uint32_t* pixels;
  int pitch;
  SDL_LockTexture(back_buffer, NULL, reinterpret_cast<void**>(&pixels), &pitch);
  std::fill(pixels, pixels + WIDTH * HEIGHT, 0xFF000000);
  SDL_UnlockTexture(back_buffer);
}

void Chip8::destroy_double_buffer() {
  if (back_buffer) {
    SDL_DestroyTexture(back_buffer);
    back_buffer = nullptr;
  }
  if (front_buffer) {
    SDL_DestroyTexture(front_buffer);
    front_buffer = nullptr;
  }
}

int Chip8::get_key(SDL_Scancode scancode){
	struct KeyMapping{
		SDL_Scancode scancode;
		int chip8_key;
	};

	static constexpr KeyMapping key_mappings[] = {
		{SDL_SCANCODE_1, 0x1}, // row 1
		{SDL_SCANCODE_2, 0x2},
    {SDL_SCANCODE_3, 0x3},
    {SDL_SCANCODE_4, 0xC},
    {SDL_SCANCODE_Q, 0x4}, // row 2
    {SDL_SCANCODE_W, 0x5},
    {SDL_SCANCODE_E, 0x6},
    {SDL_SCANCODE_R, 0xD},
    {SDL_SCANCODE_A, 0x7}, // row 3
    {SDL_SCANCODE_S, 0x8},
    {SDL_SCANCODE_D, 0x9},
    {SDL_SCANCODE_F, 0xE},
    {SDL_SCANCODE_Z, 0xA}, // row 4
    {SDL_SCANCODE_X, 0x0},
    {SDL_SCANCODE_C, 0xB},
    {SDL_SCANCODE_V, 0xF}
	};

	for (const auto& mapping : key_mappings){
		if (mapping.scancode == scancode){
			return mapping.chip8_key;
		}
	}

	return -1;
}
