#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <random>

class Chip8 {
public:
    Chip8();
    void load_rom(const std::string &path);
    void emulate_cycle();

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

    // font and opcode helpers
    void load_font_set();
    void execute_0x8(uint8_t x, uint8_t y, uint8_t n);
    void execute_0xF(uint8_t x, uint8_t kk);

    // RNG for opcode 0xC
    std::mt19937 rng;
    std::uniform_int_distribution<uint8_t> dist;
};

