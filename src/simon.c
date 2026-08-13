#include <xc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "simon.h"
#include "delay.h"
#include "lcd.h"

extern void thh_wait_100us(void);

#define LED_RED_LAT    LATBbits.LATB2
#define LED_BLUE_LAT   LATBbits.LATB3
#define LED_YELLOW_LAT LATBbits.LATB6
#define LED_GREEN_LAT  LATBbits.LATB5

#define LED_RED_TRIS   TRISBbits.TRISB2
#define LED_BLUE_TRIS  TRISBbits.TRISB3
#define LED_YELLOW_TRIS TRISBbits.TRISB6
#define LED_GREEN_TRIS TRISBbits.TRISB5

#define BTN_RED_PORT   PORTBbits.RB7
#define BTN_BLUE_PORT  PORTBbits.RB14
#define BTN_YELLOW_PORT PORTBbits.RB15
#define BTN_GREEN_PORT PORTBbits.RB12

#define IR_PIN_PORT PORTBbits.RB4
#define IR_PIN_TRIS TRISBbits.TRISB4

#define BUZ_TRIS TRISBbits.TRISB11
#define BUZ_LAT  LATBbits.LATB11

#define MAX_SEQ 255

static uint8_t seq[MAX_SEQ];
static uint16_t seq_len = 0;

static const unsigned int BASE_PLAY_ON_MS = 400;
static const unsigned int BASE_PLAY_OFF_MS = 160;

static const unsigned int note_freqs[4] = {262, 330, 392, 523};

static uint16_t high_score = 0;
static uint8_t lives = 3;

static void leds_all_off(void);
static void led_set_idx(uint8_t idx, bool on);
static void play_tone_blocking(unsigned int freq_hz, unsigned int dur_ms);
static int read_button_debounced(unsigned int timeout_ms);
static void play_pattern(void);
static void append_random_note(void);
static void wait_for_start_button(void);
static void lcd_update_status(uint16_t level, uint16_t high, bool waiting);
static void start_song(void);
static void end_song(void);
static void indicate_error(void);

static int ir_try_decode_once(uint32_t *out_code, unsigned int timeout_ms)
{
    return 0;
}

static void leds_all_off(void)
{
    LED_RED_LAT = 0;
    LED_BLUE_LAT = 0;
    LED_YELLOW_LAT = 0;
    LED_GREEN_LAT = 0;
}

static void led_set_idx(uint8_t idx, bool on)
{
    switch (idx) {
        case 0: LED_RED_LAT = on ? 1 : 0; break;
        case 1: LED_BLUE_LAT = on ? 1 : 0; break;
        case 2: LED_YELLOW_LAT = on ? 1 : 0; break;
        case 3: LED_GREEN_LAT = on ? 1 : 0; break;
        default: break;
    }
}

static void play_tone_blocking(unsigned int freq_hz, unsigned int dur_ms)
{
    if (freq_hz == 0 || dur_ms == 0) return;

    unsigned long period_us = 1000000UL / (unsigned long)freq_hz;
    if (period_us == 0) return;
    unsigned long half_us = period_us / 2UL;
    unsigned long total_us = (unsigned long)dur_ms * 1000UL;

    unsigned long cycles = total_us / period_us;
    if (cycles == 0) cycles = 1;

    unsigned long half_100us = (half_us + 50UL) / 100UL;
    if (half_100us == 0) half_100us = 1;

    for (unsigned long i = 0; i < cycles; i++) {
        BUZ_LAT = 1;
        for (unsigned long k = 0; k < half_100us; k++) thh_wait_100us();
        BUZ_LAT = 0;
        for (unsigned long k = 0; k < half_100us; k++) thh_wait_100us();
    }
    BUZ_LAT = 0;
}

static int read_button_debounced(unsigned int timeout_ms)
{
    const unsigned int DEBOUNCE_MS = 50;
    const unsigned int HOLD_OFF_MS = 100;
    unsigned int elapsed = 0;
    
    while (1)
    {
        if (BTN_RED_PORT == 0) {
            delay_ms_asm(DEBOUNCE_MS);
            if (BTN_RED_PORT == 0) {
                while (BTN_RED_PORT == 0) delay_ms_asm(5);
                delay_ms_asm(HOLD_OFF_MS);
                return 1;
            }
        }
        if (BTN_BLUE_PORT == 0) {
            delay_ms_asm(DEBOUNCE_MS);
            if (BTN_BLUE_PORT == 0) {
                while (BTN_BLUE_PORT == 0) delay_ms_asm(5);
                delay_ms_asm(HOLD_OFF_MS);
                return 2;
            }
        }
        if (BTN_YELLOW_PORT == 0) {
            delay_ms_asm(DEBOUNCE_MS);
            if (BTN_YELLOW_PORT == 0) {
                while (BTN_YELLOW_PORT == 0) delay_ms_asm(5);
                delay_ms_asm(HOLD_OFF_MS);
                return 3;
            }
        }
        if (BTN_GREEN_PORT == 0) {
            delay_ms_asm(DEBOUNCE_MS);
            if (BTN_GREEN_PORT == 0) {
                while (BTN_GREEN_PORT == 0) delay_ms_asm(5);
                delay_ms_asm(HOLD_OFF_MS);
                return 4;
            }
        }

        if (timeout_ms) {
            delay_ms_asm(1);
            elapsed++;
            if (elapsed >= timeout_ms) return 0;
        } else {
            delay_ms_asm(1);
        }
    }
}

static unsigned int compute_play_on_ms(uint16_t level)
{
    unsigned int dec = (level / 5) * 10;
    if (dec > (BASE_PLAY_ON_MS - 120)) dec = BASE_PLAY_ON_MS - 120;
    return BASE_PLAY_ON_MS - dec;
}

static unsigned int compute_play_off_ms(uint16_t level)
{
    unsigned int dec = (level / 5) * 5;
    if (dec > (BASE_PLAY_OFF_MS - 50)) dec = BASE_PLAY_OFF_MS - 50;
    return (BASE_PLAY_OFF_MS > dec) ? (BASE_PLAY_OFF_MS - dec) : 50;
}

static void play_pattern(void)
{
    unsigned int on_ms = compute_play_on_ms(seq_len);
    unsigned int off_ms = compute_play_off_ms(seq_len);

    for (uint16_t i = 0; i < seq_len; i++) {
        uint8_t idx = seq[i] - 1;
        led_set_idx(idx, true);
        delay_ms_asm(10);
        play_tone_blocking(note_freqs[idx], on_ms);
        led_set_idx(idx, false);
        delay_ms_asm(off_ms);

        lcd_update_status(seq_len, high_score, false);
    }
}

static void indicate_error(void)
{
    for (int i = 0; i < 3; i++) {
        play_tone_blocking(196, 120);
        delay_ms_asm(80);
    }
    for (int r = 0; r < 3; r++) {
        LED_RED_LAT = LED_BLUE_LAT = LED_YELLOW_LAT = LED_GREEN_LAT = 1;
        delay_ms_asm(180);
        leds_all_off();
        delay_ms_asm(120);
    }
}

static void start_song(void)
{
    unsigned int tune[] = {440, 523, 659, 0};
    unsigned int dur[]  = {120, 120, 300, 0};

    for (int i = 0; tune[i] != 0; i++) {
        play_tone_blocking(tune[i], dur[i]);
        delay_ms_asm(40);
    }
}

static void end_song(void)
{
    play_tone_blocking(196, 200); delay_ms_asm(80);
    play_tone_blocking(164, 200); delay_ms_asm(80);
    play_tone_blocking(130, 900);
}

static void append_random_note(void)
{
    if (seq_len < MAX_SEQ) {
        seq[seq_len++] = (rand() % 4) + 1;
    } else {
        for (int i = 0; i < MAX_SEQ - 1; i++) seq[i] = seq[i + 1];
        seq[MAX_SEQ - 1] = (rand() % 4) + 1;
    }
}

static void lcd_update_status(uint16_t level, uint16_t high, bool waiting)
{
    char buf[16];

    lcd_setCursor(0, 0);
    lcd_printStr("SIMON GAME   ");

    lcd_setCursor(0, 1);
    sprintf(buf, "L: %u       ", (unsigned)level);
    lcd_printStr(buf);

    lcd_setCursor(0, 2);
    sprintf(buf, "HS: %u      ", (unsigned)high);
    lcd_printStr(buf);

    lcd_setCursor(0, 3);
    if (waiting) {
        lcd_printStr("Press GRN     ");
    } else {
        lcd_printStr("              ");
    }
}

static void wait_for_start_button(void)
{
    lcd_update_status(seq_len, high_score, true);

    while (1)
    {
        if (BTN_GREEN_PORT == 0) {
            delay_ms_asm(20);
            if (BTN_GREEN_PORT == 0) {
                while (BTN_GREEN_PORT == 0) delay_ms_asm(10);
                start_song();
                return;
            }
        }
        delay_ms_asm(10);
    }
}

void simon_init(void)
{
    AD1PCFG = 0xFFFF;

    LED_RED_TRIS = 0; 
    LED_BLUE_TRIS = 0; 
    LED_YELLOW_TRIS = 0; 
    LED_GREEN_TRIS = 0;
    
    LED_RED_LAT = 0;
    LED_BLUE_LAT = 0;
    LED_YELLOW_LAT = 0;
    LED_GREEN_LAT = 0;

    BUZ_TRIS = 0; 
    BUZ_LAT = 0;

    TRISBbits.TRISB7  = 1;
    TRISBbits.TRISB14 = 1;
    TRISBbits.TRISB15 = 1;
    TRISBbits.TRISB12 = 1;

    IR_PIN_TRIS = 1;

    T1CON = 0x8000;
    TMR1 = 0;
    
    unsigned int seed_value = 0;
    while (1) {
        seed_value = TMR1;
        if (BTN_RED_PORT == 0 || BTN_BLUE_PORT == 0 || 
            BTN_YELLOW_PORT == 0 || BTN_GREEN_PORT == 0) {
            delay_ms_asm(20);
            if (BTN_RED_PORT == 0 || BTN_BLUE_PORT == 0 || 
                BTN_YELLOW_PORT == 0 || BTN_GREEN_PORT == 0) {
                break;
            }
        }
    }
    
    srand(seed_value);

    seq_len = 0;
    for (int i = 0; i < MAX_SEQ; i++) seq[i] = 0;
    append_random_note();

    lives = 3;
    high_score = 0;
    lcd_update_status(seq_len, high_score, true);
}

void simon_play_game(void)
{
    while (1) {
        wait_for_start_button();

        lcd_update_status(seq_len, high_score, false);

        while (1) {
            lcd_update_status(seq_len, high_score, false);

            play_pattern();

            bool failed = false;
            for (uint16_t step = 0; step < seq_len; step++) {
                int expected = seq[step];

                int pressed = read_button_debounced(5000);

                if (pressed == 0) {
                    indicate_error();
                    failed = true;
                    break;
                }

                led_set_idx(pressed - 1, true);
                delay_ms_asm(10);
                play_tone_blocking(note_freqs[pressed - 1], 200);
                led_set_idx(pressed - 1, false);

                if (pressed != expected) {
                    indicate_error();
                    failed = true;
                    break;
                }
            }

            if (failed) {
                end_song();

                if (seq_len > high_score) {
                    high_score = seq_len;
                }

                seq_len = 0;
                append_random_note();
                lcd_update_status(seq_len, high_score, true);
                break;
            }

            BUZ_LAT = 1; delay_ms_asm(80); BUZ_LAT = 0;
            append_random_note();

            if (seq_len > high_score) {
                high_score = seq_len;
            }

            delay_ms_asm(300);
        }
    }
}
