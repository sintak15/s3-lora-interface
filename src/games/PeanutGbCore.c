#include "PeanutGbCore.h"
#include <setjmp.h>
// This interpreter is CPU-bound; keep the firmware's size optimization elsewhere.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize ("O3")
#endif
#define ENABLE_SOUND 0
#define PEANUT_GB_HIGH_LCD_ACCURACY 0
#include "../vendor/peanut-gb/peanut_gb.h"
_Static_assert(GAMEBOY_A == JOYPAD_A && GAMEBOY_B == JOYPAD_B && GAMEBOY_SELECT == JOYPAD_SELECT && GAMEBOY_START == JOYPAD_START, "action key mapping matches Peanut-GB");
_Static_assert(GAMEBOY_RIGHT == JOYPAD_RIGHT && GAMEBOY_LEFT == JOYPAD_LEFT && GAMEBOY_UP == JOYPAD_UP && GAMEBOY_DOWN == JOYPAD_DOWN, "direction key mapping matches Peanut-GB");
struct adapter {
    struct gb_s gb;
    void* context;
    GameBoyRead rom, ram;
    GameBoyWrite write;
    GameBoyLine line;
    int error;
    jmp_buf trap;
};
static uint8_t read_rom(struct gb_s* gb, const uint_fast32_t address) {
    struct adapter* a = (struct adapter*)gb;
    return a->rom(a->context, address);
}
static uint8_t read_ram(struct gb_s* gb, const uint_fast32_t address) {
    struct adapter* a = (struct adapter*)gb;
    return a->ram(a->context, address);
}
static void write_ram(struct gb_s* gb, const uint_fast32_t address, const uint8_t value) {
    struct adapter* a = (struct adapter*)gb;
    a->write(a->context, address, value);
}
static void error_cb(struct gb_s* gb, const enum gb_error_e error, const uint16_t address) {
    (void)address;
    struct adapter* a = (struct adapter*)gb;
    a->error = (int)error + 1;
    longjmp(a->trap, 1); // Peanut-GB requires error callbacks never to return.
}
static void draw_line(struct gb_s* gb, const uint8_t* pixels, const uint_fast8_t line) {
    struct adapter* a = (struct adapter*)gb;
    a->line(a->context, pixels, line);
}
size_t gameBoyCoreSize(void) { return sizeof(struct adapter); }
int gameBoyCoreInit(void* core, void* context, GameBoyRead rom, GameBoyRead ram, GameBoyWrite write, GameBoyLine line) {
    struct adapter* a = core;
    a->context = context; a->rom = rom; a->ram = ram; a->write = write; a->line = line; a->error = 0;
    if (setjmp(a->trap)) return -1;
    int result = gb_init(&a->gb, read_rom, read_ram, write_ram, error_cb, context);
    if (result == GB_INIT_NO_ERROR) gb_init_lcd(&a->gb, draw_line);
    return result;
}
void gameBoyCoreFrame(void* core, uint8_t keys) {
    gameBoyCoreFrameRender(core, keys, 1);
}
void gameBoyCoreFrameRender(void* core, uint8_t keys, int render) {
    struct adapter* a = core;
    a->gb.direct.joypad = keys;
    // Skipping presentation must still advance every CPU, timer and LCD cycle.
    a->gb.display.lcd_draw_line = render ? draw_line : NULL;
    if (!a->error && setjmp(a->trap) == 0) gb_run_frame(&a->gb);
}
size_t gameBoyCoreSaveSize(void* core) { return gb_get_save_size(&((struct adapter*)core)->gb); }
int gameBoyCoreError(void* core) { return ((struct adapter*)core)->error; }
uint8_t gameBoyCoreJoypadRegister(void* core, uint8_t select) {
    struct adapter* a = core;
    __gb_write(&a->gb, 0xff00, select);
    return __gb_read(&a->gb, 0xff00);
}
