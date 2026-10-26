#ifndef CYD_PEANUT_GB_CORE_H
#define CYD_PEANUT_GB_CORE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uint8_t (*GameBoyRead)(void*, uint32_t);
typedef void (*GameBoyWrite)(void*, uint32_t, uint8_t);
typedef void (*GameBoyLine)(void*, const uint8_t*, uint8_t);
enum GameBoyKey {
  GAMEBOY_A = 0x01, GAMEBOY_B = 0x02, GAMEBOY_SELECT = 0x04, GAMEBOY_START = 0x08,
  GAMEBOY_RIGHT = 0x10, GAMEBOY_LEFT = 0x20, GAMEBOY_UP = 0x40, GAMEBOY_DOWN = 0x80
};
size_t gameBoyCoreSize(void);
int gameBoyCoreInit(void* core, void* context, GameBoyRead rom, GameBoyRead ram, GameBoyWrite write, GameBoyLine line);
void gameBoyCoreFrame(void* core, uint8_t keys);
void gameBoyCoreFrameRender(void* core, uint8_t keys, int render);
size_t gameBoyCoreSaveSize(void* core);
int gameBoyCoreError(void* core);
uint8_t gameBoyCoreJoypadRegister(void* core, uint8_t select);
#ifdef __cplusplus
}
#endif

#endif
