#pragma once

#include <cstdint>
#include <soc/gpio_num.h>

class S7_Digit
{
public:
    typedef struct config_TAG {
        int active_level;
        int pin_A;
        int pin_B;
        int pin_C;
        int pin_D;
        int pin_E;
        int pin_F;
        int pin_G;
        int pin_DP;
        int pin_COM;

        uint64_t bit_mask() const { return 0
           | (1 << pin_A)
           | (1 << pin_B)
           | (1 << pin_C)
           | (1 << pin_D)
           | (1 << pin_E)
           | (1 << pin_F)
           | (1 << pin_G)
           | (1 << pin_DP);
        }
    }
    digit_config_t;

    static const uint8_t None   = 0;
    static const uint8_t SEG_A  = 1;
    static const uint8_t SEG_B  = 2;
    static const uint8_t SEG_C  = 4;
    static const uint8_t SEG_D  = 8;
    static const uint8_t SEG_E  = 16;
    static const uint8_t SEG_F  = 32;
    static const uint8_t SEG_G  = 64;
    static const uint8_t SEG_DP = 128;

    constexpr static const uint8_t DIGIT[] = {
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
        SEG_B | SEG_C,
        SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,
        SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,
        SEG_F | SEG_G | SEG_B | SEG_C,
        SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,
        SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,
        SEG_A | SEG_B | SEG_C,
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
    };

    enum class Letter {
        A = 0,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
    };

    constexpr const static uint8_t LETTER[] = {
        SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,          // A
        SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,                  // b
        SEG_D | SEG_E | SEG_G,                                  // c
        SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,                  // d
        SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,                  // E
        SEG_A | SEG_E | SEG_F | SEG_G,                          // F
        SEG_A | SEG_C | SEG_D | SEG_E | SEG_F,                  // G
        SEG_C | SEG_E | SEG_F | SEG_G,                          // h
        SEG_C,                                                  // i
        SEG_B | SEG_C | SEG_D | SEG_E,                          // J
        SEG_E | SEG_F | SEG_G,                                  // K
        SEG_D | SEG_E | SEG_F,                                  // L
        SEG_A | SEG_C | SEG_E | SEG_G,                          // m
        SEG_C | SEG_E | SEG_G,                                  // n
        SEG_C | SEG_D | SEG_E | SEG_G,                          // o
        SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,                  // P
        SEG_A | SEG_B | SEG_C | SEG_F | SEG_G,                  // q
        SEG_E | SEG_G,                                          // r
        SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                  // S = 5
        SEG_D | SEG_E | SEG_F | SEG_G,                          // t
        SEG_C | SEG_D | SEG_E,                                  // u
        SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,                  // V
        SEG_B | SEG_D | SEG_F | SEG_G,                          // W
        SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,                  // X
        SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,                  // y
        SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                  // Z = 2
    };

    S7_Digit();

    void SetConfig(digit_config_t config);
    void SetSegments(uint8_t segments);
    void Display_(uint8_t segments);
    void Hide();
    void Refresh();

private:
    digit_config_t m_config;
    uint8_t m_segments = 0;

    int set_active(uint8_t active) {
        return m_config.active_level ? !!active : !active;
    };

};

