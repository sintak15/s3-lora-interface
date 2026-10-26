// Touchscreen game frontends. Radio, GPS, web and storage services retain the main loop.
#include "../games/PeanutGbCore.h"
#include "../games/RomFiles.h"
#include "../games/TamaPet.h"
#include "../games/GameBoyInput.h"
#include "../vendor/giraffegotchi/pet.h"
#include "../games/GiraffeSprites.h"

static constexpr const char* GAME_ROM_DIR = "/s3-lora/games/gb";
static constexpr const char* GAME_SAVE_DIR = "/s3-lora/games/saves";
static TamaGame::Pet tamaPet;
static Pet giraffePet;
static uint8_t activePet = 0;
static bool petsLoaded = false;
static uint32_t lastPetTick = 0, lastPetSave = 0, lastGamesRefresh = 0;
static lv_obj_t *petTitle = nullptr, *petStats = nullptr, *petMood = nullptr, *petSprite = nullptr;
static lv_obj_t* tamaAvatar = nullptr;
static lv_obj_t* petDashboard[6] = {};
static lv_obj_t *romList = nullptr, *romStatus = nullptr, *gbCanvas = nullptr, *gbErrorLabel = nullptr;
static lv_obj_t* petCareButtons[5] = {};
static uint8_t *gbRom = nullptr, *gbRam = nullptr;
static lv_color_t* gbPixels = nullptr;
static void* gbCore = nullptr;
static size_t gbRomBytes = 0, gbRamBytes = 0;
static bool gbRunning = false, gbRamDirty = false;
static uint8_t gbTouchKeys = 0xff;
static uint32_t gbTouchReads = 0, gbTouchPresses = 0;
static uint32_t gbLastSave = 0, gbNextFrameUs = 0, gbLastPresentUs = 0;
static uint32_t gbFrames = 0, gbPresented = 0, gbFrameUs = 0, gbPeakFrameUs = 0;
static uint32_t gbFpsWindowMs = 0, gbFpsWindowFrames = 0, gbFpsTenths = 0;
static constexpr uint32_t GB_FRAME_US = 16743; // 70224 clocks at 4.194304 MHz.
static int gbDirtyLeft = GameBoyInput::VIDEO_W, gbDirtyTop = GameBoyInput::VIDEO_H, gbDirtyRight = -1, gbDirtyBottom = -1;
static char gbFileName[65] = "";
static char gbRomNames[32][65] = {};
static unsigned gbRomCount = 0;
static void updateGameBoyTouches(const uint8_t* points, size_t bytes) {
  gbTouchKeys = 0xff;
  if (!points || currentPage != pageGameBoyPlayer) return;
  lv_area_t content; lv_obj_get_content_coords(pageGameBoyPlayer, &content);
  uint8_t mask = GameBoyInput::decode(points, bytes, content.x1, content.y1);
  ++gbTouchReads; if (mask) ++gbTouchPresses;
  gbTouchKeys = ~mask;
}

static lv_obj_t* gameText(lv_obj_t* parent, const char* text, int x, int y, int width, const lv_font_t* font = &montserrat_11) {
  auto* label = lv_label_create(parent);
  lv_label_set_text(label, text); lv_obj_set_pos(label, x, y); lv_obj_set_width(label, width);
  lv_obj_set_style_text_font(label, font, 0); lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT), 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); return label;
}
static lv_obj_t* gameButton(lv_obj_t* parent, const char* text, int x, int y, int w, int h, lv_event_cb_t cb) {
  auto* button = makeSmallButton(parent, text, x, y, w, cb);
  lv_obj_set_height(button, h);
  lv_obj_set_style_pad_all(button, 0, 0);
  return button;
}
static void savePets() {
  if (!petsLoaded) return;
  prefs.putBytes("gameTama1", &tamaPet, sizeof(tamaPet));
  const uint8_t state[] = {1, giraffePet.hunger(), giraffePet.thirst(), giraffePet.fun(), giraffePet.hygiene(), giraffePet.poopCount()};
  prefs.putBytes("gameGira1", state, sizeof(state));
  lastPetSave = millis();
}
static void initGames() {
  if (prefs.getBytesLength("gameTama1") == sizeof(tamaPet)) {
    TamaGame::Pet saved; prefs.getBytes("gameTama1", &saved, sizeof(saved));
    if (saved.hunger <= 100 && saved.happiness <= 100 && saved.health <= 100 && saved.energy <= 100 && saved.cleanliness <= 100 && saved.elapsed < 60000) tamaPet = saved;
  }
  uint8_t state[6] = {};
  if (prefs.getBytesLength("gameGira1") == sizeof(state)) {
    prefs.getBytes("gameGira1", state, sizeof(state));
    if (state[0] == 1) giraffePet.load(state[1], state[2], state[3], state[4], state[5]);
  }
  petsLoaded = true; lastPetTick = lastPetSave = millis();
}
static uint8_t gbReadRom(void*, uint32_t address) { return address < gbRomBytes ? gbRom[address] : 0xff; }
static uint8_t gbReadRam(void*, uint32_t address) { return address < gbRamBytes ? gbRam[address] : 0xff; }
static void gbWriteRam(void*, uint32_t address, uint8_t value) {
  if (address < gbRamBytes && gbRam[address] != value) { gbRam[address] = value; gbRamDirty = true; }
}
static void gbDrawLine(void*, const uint8_t* pixels, uint8_t line) {
  static const lv_color_t palette[] = {lv_color_hex(0xe0f0c0), lv_color_hex(0x91ae65), lv_color_hex(0x496547), lv_color_hex(0x142c2b)};
  if (!gbPixels || line >= 144) return;
  unsigned first = GameBoyInput::firstRow(line), last = GameBoyInput::firstRow(line + 1);
  // Scale in internal stack memory, then write sequential bursts to PSRAM.
  lv_color_t row[GameBoyInput::VIDEO_W];
  int left = GameBoyInput::VIDEO_W, right = -1;
  const auto* previous = gbPixels + first * GameBoyInput::VIDEO_W;
  for (unsigned x = 0; x < GameBoyInput::VIDEO_W; ++x) {
    row[x] = palette[pixels[GameBoyInput::sourceX(x)] & 3];
    if (row[x].full != previous[x].full) { left = min(left, (int)x); right = x; }
  }
  if (right < 0) return;
  gbDirtyLeft = min(gbDirtyLeft, left); gbDirtyRight = max(gbDirtyRight, right);
  gbDirtyTop = min(gbDirtyTop, (int)first); gbDirtyBottom = max(gbDirtyBottom, (int)last - 1);
  for (unsigned y = first; y < last; ++y) memcpy(gbPixels + y * GameBoyInput::VIDEO_W, row, sizeof(row));
}
static bool saveGameBoy() {
  if (!gbRamDirty || !gbRamBytes || !gbFileName[0]) return true;
  SdLock lock(50); if (!lock || !sdStorage.available) return false;
  sdCardFilesystem().mkdir("/s3-lora/games"); sdCardFilesystem().mkdir(GAME_SAVE_DIR);
  String path = String(GAME_SAVE_DIR) + "/" + gbFileName + ".sav", temp = path + ".tmp", backup = path + ".bak";
  sdCardFilesystem().remove(temp); File file = sdCardFilesystem().open(temp, FILE_WRITE);
  if (!file) return false;
  bool ok = file.write(gbRam, gbRamBytes) == gbRamBytes; file.flush(); file.close();
  if (!ok) { sdCardFilesystem().remove(temp); return false; }
  sdCardFilesystem().remove(backup);
  const bool hadOld = sdCardFilesystem().exists(path);
  if (hadOld && !sdCardFilesystem().rename(path, backup)) return false;
  if (!sdCardFilesystem().rename(temp, path)) { if (hadOld) sdCardFilesystem().rename(backup, path); return false; }
  sdCardFilesystem().remove(backup); gbRamDirty = false; gbLastSave = millis(); return true;
}
static bool gamesMayLeavePage(lv_obj_t* target) {
  if (pageGameBoyPlayer && currentPage == pageGameBoyPlayer && target != pageGameBoyPlayer) {
    gbRunning = false; gbTouchKeys = 0xff;
    if (!saveGameBoy()) {
      if (gbErrorLabel) { lv_label_set_text(gbErrorLabel, "Save failed. Check SD; try Back again."); lv_obj_clear_flag(gbErrorLabel, LV_OBJ_FLAG_HIDDEN); }
      return false;
    }
  }
  return true;
}
static void releaseGameBoy() {
  gbRunning = false; gbTouchKeys = 0xff;
  if (gbRom) heap_caps_free(gbRom);
  if (gbRam) heap_caps_free(gbRam);
  if (gbCore) heap_caps_free(gbCore);
  gbRom = gbRam = nullptr; gbCore = nullptr; gbRomBytes = gbRamBytes = 0; gbRamDirty = false;
}
static void makeControlsDemo(uint8_t* rom) {
  // Original public-domain homebrew: a scrolling checkerboard, A/B change palette.
  memset(rom, 0, 32768); rom[0x100] = 0xc3; rom[0x101] = 0x50; rom[0x102] = 0x01;
  memcpy(rom + 0x134, "CYD CONTROLS", 12);
  const uint8_t code[] = {0xf3,0x31,0xfe,0xff,0xaf,0xe0,0x40,0x21,0x00,0x80,0x06,0x08,
    0x3e,0xaa,0x22,0xaf,0x22,0x05,0x20,0xf8,0x3e,0xe4,0xe0,0x47,0x3e,0x91,0xe0,0x40,
    0x3e,0x20,0xe0,0x00,0xf0,0x00,0x2f,0xe6,0x0f,0xe0,0x43,
    0x3e,0x10,0xe0,0x00,0xf0,0x00,0xe6,0x03,0xfe,0x03,0x3e,0xe4,0x28,0x02,0x3e,0x1b,0xe0,0x47,0x18,0xe1};
  memcpy(rom + 0x150, code, sizeof(code));
  uint8_t sum = 0; for (unsigned i = 0x134; i <= 0x14c; ++i) sum = uint8_t(sum - rom[i] - 1);
  rom[0x14d] = sum;
}
static bool startGameBoy(const char* name) {
  if (!saveGameBoy()) return false;
  releaseGameBoy(); gbFileName[0] = 0;
  const bool demo = name == nullptr;
  if (!demo && !RomFiles::validName(name)) return false;
  if (demo) {
    gbRomBytes = 32768; gbRom = (uint8_t*)heap_caps_malloc(gbRomBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (gbRom) makeControlsDemo(gbRom);
  } else {
    SdLock lock(100); if (!lock || !sdStorage.available) { lv_label_set_text(romStatus, "SD card unavailable"); return false; }
    File file = sdCardFilesystem().open(String(GAME_ROM_DIR) + "/" + name);
    if (!file || file.isDirectory() || file.size() > RomFiles::MAX_ROM_BYTES || file.size() < 32768) { if (file) file.close(); lv_label_set_text(romStatus, "Invalid ROM file"); return false; }
    gbRomBytes = file.size(); gbRom = (uint8_t*)heap_caps_malloc(gbRomBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (gbRom && file.read(gbRom, gbRomBytes) != gbRomBytes) { heap_caps_free(gbRom); gbRom = nullptr; }
    file.close(); strlcpy(gbFileName, name, sizeof(gbFileName));
  }
  if (!gbRom || !RomFiles::validHeader(gbRom, gbRomBytes)) { releaseGameBoy(); lv_label_set_text(romStatus, "ROM invalid, Color-only, or insufficient PSRAM"); return false; }
  // Registers, WRAM and VRAM are touched on almost every emulated instruction.
  gbCore = heap_caps_calloc(1, gameBoyCoreSize(), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  gbRamBytes = 128 * 1024;
  gbRam = (uint8_t*)heap_caps_malloc(gbRamBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!gbPixels) gbPixels = (lv_color_t*)heap_caps_calloc(GameBoyInput::VIDEO_W * GameBoyInput::VIDEO_H, sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!gbCore || !gbRam || !gbPixels) { releaseGameBoy(); lv_label_set_text(romStatus, "Insufficient game memory"); return false; }
  memset(gbRam, 0xff, gbRamBytes);
  if (gameBoyCoreInit(gbCore, nullptr, gbReadRom, gbReadRam, gbWriteRam, gbDrawLine) != 0) { releaseGameBoy(); lv_label_set_text(romStatus, "Cartridge type unsupported"); return false; }
  gbRamBytes = min(gbRamBytes, gameBoyCoreSaveSize(gbCore));
  if (!demo && gbRamBytes) {
    SdLock lock(100); if (!lock) { releaseGameBoy(); lv_label_set_text(romStatus, "SD busy; retry"); return false; }
    String path = String(GAME_SAVE_DIR) + "/" + name + ".sav";
    if (!sdCardFilesystem().exists(path) && sdCardFilesystem().exists(path + ".bak")) sdCardFilesystem().rename(path + ".bak", path);
    if (sdCardFilesystem().exists(path)) {
      File save = sdCardFilesystem().open(path);
      if (!save || save.size() != gbRamBytes || save.read(gbRam, gbRamBytes) != gbRamBytes) { if (save) save.close(); releaseGameBoy(); lv_label_set_text(romStatus, "Save file unreadable; original preserved"); return false; }
      save.close();
    }
  }
  lv_canvas_set_buffer(gbCanvas, gbPixels, GameBoyInput::VIDEO_W, GameBoyInput::VIDEO_H, LV_IMG_CF_TRUE_COLOR);
  lv_obj_add_flag(gbErrorLabel, LV_OBJ_FLAG_HIDDEN);
  gbFrames = gbPresented = gbFrameUs = gbPeakFrameUs = gbFpsTenths = 0;
  gbFpsWindowFrames = 0; gbFpsWindowMs = gbLastSave = millis();
  gbNextFrameUs = micros(); gbLastPresentUs = gbNextFrameUs - 33333;
  gbTouchKeys = 0xff; gbRunning = true; showPage(pageGameBoyPlayer); return true;
}
static void scanGameRoms() {
  if (!romList) return;
  lv_obj_clean(romList); gbRomCount = 0;
  SdLock lock(50);
  if (!lock || !sdStorage.available) { lv_label_set_text(romStatus, "Insert an SD card for ROMs"); return; }
  File dir = sdCardFilesystem().open(GAME_ROM_DIR);
  if (dir && dir.isDirectory()) {
    for (File entry = dir.openNextFile(); entry && gbRomCount < 32; entry = dir.openNextFile()) {
      const String path = entry.name(); const String name = path.substring(path.lastIndexOf('/') + 1);
      if (!entry.isDirectory() && RomFiles::validName(name.c_str())) strlcpy(gbRomNames[gbRomCount++], name.c_str(), 65);
      entry.close();
    }
    dir.close();
  }
  for (unsigned i = 0; i < gbRomCount; ++i) {
    auto* button = lv_list_add_btn(romList, nullptr, gbRomNames[i]);
    styleDarkObject(button, COLOR_PANEL); lv_obj_set_height(button, 44);
    lv_obj_set_style_text_font(button, UI_FONT_SMALL, 0);
    lv_obj_add_event_cb(button, [](lv_event_t* e) { unsigned i = (uintptr_t)lv_event_get_user_data(e); startGameBoy(gbRomNames[i]); }, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  lv_label_set_text(romStatus, gbRomCount ? "Choose a ROM. Back saves and exits." : "Upload .gb ROMs from Web UI > Games");
}
static void openPet(uint8_t index) { activePet = index; showPage(pagePet); lastGamesRefresh = 0; }
static void petCareAction(unsigned index) {
  if (activePet == 0) tamaPet.action(index);
  else { switch (index) { case 0: giraffePet.feed(); break; case 1: giraffePet.drink(); break; case 2: giraffePet.play(); break; case 3: giraffePet.clean(); break; case 4: giraffePet.read(); break; } }
  savePets(); lastGamesRefresh = 0;
}
static void buildGamesUi() {
  makePageTitle(pageGames, "Games");
  gameButton(pageGames, "Pets", 6, 36, 218, 60, [](lv_event_t*) { showPage(pagePets); });
  gameButton(pageGames, "Game Boy", 6, 108, 218, 60, [](lv_event_t*) { showPage(pageGameBoy); scanGameRoms(); });
  makePageTitle(pagePets, "Pets");
  gameButton(pagePets, "Tamapetchi", 6, 36, 218, 60, [](lv_event_t*) { openPet(0); });
  gameButton(pagePets, "Giraffegotchi", 6, 108, 218, 60, [](lv_event_t*) { openPet(1); });
  const char* dashboard[] = {"Radio", "Battery", "Nodes", "GPS", "Messages", "Map"};
  for (unsigned i = 0; i < 6; ++i) {
    auto* button = gameButton(pagePet, dashboard[i], 4 + (i % 2) * 114, (i / 2) * 33, 108, 30, [](lv_event_t*) {});
    petDashboard[i] = lv_obj_get_child(button, 0);
    lv_obj_set_style_text_font(petDashboard[i], &montserrat_11, 0);
    lv_obj_add_event_cb(button, [](lv_event_t* e) {
      lv_obj_t* targets[] = {pageSystemRadio, pageBattery, pageNodes, pageSystemGps, pageOperate, pageGps};
      unsigned index = (uintptr_t)lv_event_get_user_data(e); if (index == 2) refreshNodeList(true); showPage(targets[index]);
    }, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  petTitle = gameText(pagePet, "Tamapetchi", 4, 102, 220, &montserrat_15);
  petSprite = lv_img_create(pagePet); lv_obj_set_pos(petSprite, 5, 127);
  tamaAvatar = lv_obj_create(pagePet); lv_obj_set_pos(tamaAvatar, 7, 134); lv_obj_set_size(tamaAvatar, 65, 50);
  styleDarkObject(tamaAvatar, 0xB38BE8); lv_obj_set_style_radius(tamaAvatar, 22, 0);
  lv_obj_set_style_pad_all(tamaAvatar, 0, 0); lv_obj_set_style_border_width(tamaAvatar, 0, 0);
  lv_obj_clear_flag(tamaAvatar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  gameText(tamaAvatar, "o o", 13, 6, 48, &lv_font_montserrat_22);
  gameText(tamaAvatar, "  w", 14, 30, 42);
  petStats = gameText(pagePet, "", 83, 125, 141);
  petMood = gameText(pagePet, "", 83, 184, 141);
  gameButton(pagePet, "Feed", 4, 208, 70, 40, [](lv_event_t*) { petCareAction(0); });
  gameButton(pagePet, "Play", 80, 208, 70, 40, [](lv_event_t*) { petCareAction(activePet == 0 ? 1 : 2); });
  gameButton(pagePet, "Care", 156, 208, 70, 40, [](lv_event_t*) { showPage(pagePetCare); lastGamesRefresh = 0; });
  makePageTitle(pagePetCare, "Pet care"); makePageScrollable(pagePetCare);
  for (unsigned i = 0; i < 5; ++i) {
    petCareButtons[i] = gameButton(pagePetCare, "Care", 6, 32 + i * 49, 218, 44, [](lv_event_t*) {});
    lv_obj_add_event_cb(petCareButtons[i], [](lv_event_t* e) { petCareAction((uintptr_t)lv_event_get_user_data(e)); }, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  makePageTitle(pageGameBoy, "Game Boy");
  gameText(pageGameBoy, "Peanut GB", 6, 26, 214, &montserrat_15);
  gameButton(pageGameBoy, "Refresh", 6, 50, 70, 34, [](lv_event_t*) { scanGameRoms(); });
  gameButton(pageGameBoy, "Resume", 82, 50, 68, 34, [](lv_event_t*) { if (gbCore && gbPixels) { lv_canvas_set_buffer(gbCanvas, gbPixels, GameBoyInput::VIDEO_W, GameBoyInput::VIDEO_H, LV_IMG_CF_TRUE_COLOR); gbTouchKeys = 0xff; gbRunning = true; gbNextFrameUs = micros(); gbFpsWindowMs = millis(); gbFpsWindowFrames = gbFrames; showPage(pageGameBoyPlayer); } });
  gameButton(pageGameBoy, "Demo", 156, 50, 68, 34, [](lv_event_t*) { startGameBoy(nullptr); });
  romStatus = gameText(pageGameBoy, "Upload .gb ROMs from Web UI > Games", 6, 89, 216);
  romList = lv_list_create(pageGameBoy); lv_obj_set_pos(romList, 4, 125); lv_obj_set_size(romList, 222, 123); UiScroll::vertical(romList); styleDarkObject(romList, COLOR_BG);
  lv_obj_set_style_pad_all(pageGameBoyPlayer, 0, 0);
  lv_obj_align(pageGameBoyPlayer, LV_ALIGN_TOP_LEFT, 0, STATUS_BAR_H + NAV_BAR_H);
  gbCanvas = lv_canvas_create(pageGameBoyPlayer); lv_obj_set_pos(gbCanvas, 0, 0); lv_obj_set_size(gbCanvas, GameBoyInput::VIDEO_W, GameBoyInput::VIDEO_H);
  for (const auto& key : GameBoyInput::keys) {
    auto* button = gameButton(pageGameBoyPlayer, key.name, key.x, key.y, key.w, key.h, [](lv_event_t*) {});
    lv_obj_set_style_text_font(lv_obj_get_child(button, 0), &montserrat_11, 0);
  }
  gbErrorLabel = gameText(pageGameBoyPlayer, "", 4, 44, 220, &montserrat_15);
  lv_obj_set_style_bg_opa(gbErrorLabel, LV_OPA_COVER, 0); lv_obj_set_style_bg_color(gbErrorLabel, lv_color_hex(COLOR_PANEL), 0);
  lv_obj_add_flag(gbErrorLabel, LV_OBJ_FLAG_HIDDEN);
}
static void clearGamesUiPointers() {
  pageGames = pagePets = pagePet = pagePetCare = pageGameBoy = pageGameBoyPlayer = btnHomeGames = nullptr;
  petTitle = petStats = petMood = petSprite = romList = romStatus = gbCanvas = gbErrorLabel = nullptr;
  tamaAvatar = nullptr;
  for (auto& label : petDashboard) label = nullptr;
  for (auto& button : petCareButtons) button = nullptr;
}
static void advanceGameBoyFrame(bool render) {
  const uint32_t frameStart = micros();
  gameBoyCoreFrameRender(gbCore, gbTouchKeys, render);
  gbFrameUs = micros() - frameStart; gbPeakFrameUs = max(gbPeakFrameUs, gbFrameUs);
  ++gbFrames; gbNextFrameUs += GB_FRAME_US;
}
static void serviceGames() {
  uint32_t now = millis();
  if (petsLoaded) {
    uint32_t elapsed = now - lastPetTick; lastPetTick = now;
    tamaPet.update(elapsed); giraffePet.update(elapsed);
    if (now - lastPetSave >= 60000) savePets();
  }
  if (gbRunning && currentPage == pageGameBoyPlayer) {
    const uint32_t started = micros();
    // Preserve game speed independently of the slower SPI screen. Catch-up is
    // bounded so touch, radio and GPS get another turn between short batches.
    if ((int32_t)(started - gbNextFrameUs) > (int32_t)(6 * GB_FRAME_US)) gbNextFrameUs = started;
    for (unsigned batch = 0; batch < 3 && (int32_t)(micros() - gbNextFrameUs) >= 0; ++batch) {
      const uint32_t frameStart = micros();
      const bool render = frameStart - gbLastPresentUs >= 33333;
      if (render) { gbDirtyLeft = GameBoyInput::VIDEO_W; gbDirtyTop = GameBoyInput::VIDEO_H; gbDirtyRight = gbDirtyBottom = -1; }
      advanceGameBoyFrame(render);
      if (render) {
        ++gbPresented; gbLastPresentUs = frameStart;
        if (gbDirtyRight >= 0) {
          lv_area_t area; lv_obj_get_coords(gbCanvas, &area);
          area.x2 = area.x1 + gbDirtyRight; area.y2 = area.y1 + gbDirtyBottom;
          area.x1 += gbDirtyLeft; area.y1 += gbDirtyTop;
          lv_obj_invalidate_area(gbCanvas, &area);
        }
      }
      if (gameBoyCoreError(gbCore) || micros() - started >= 28000) break;
    }
    if (now - gbFpsWindowMs >= 2000) {
      gbFpsTenths = (gbFrames - gbFpsWindowFrames) * 10000UL / (now - gbFpsWindowMs);
      gbFpsWindowFrames = gbFrames; gbFpsWindowMs = now;
    }
    if (gameBoyCoreError(gbCore)) { gbRunning = false; lv_label_set_text(gbErrorLabel, "Emulation error. Back to ROMs."); lv_obj_clear_flag(gbErrorLabel, LV_OBJ_FLAG_HIDDEN); }
    if (gbRamDirty && now - gbLastSave >= 30000) saveGameBoy();
  }
  if ((currentPage != pagePet && currentPage != pagePetCare) || (lastGamesRefresh && now - lastGamesRefresh < 500)) return;
  lastGamesRefresh = now;
  const char* tamaActions[] = {"Feed", "Play", "Clean", "Sleep / wake", "Heal"};
  const char* giraffeActions[] = {"Feed", "Drink", "Play", "Clean", "Read"};
  for (unsigned i = 0; i < 5; ++i) lv_label_set_text(lv_obj_get_child(petCareButtons[i], 0), activePet == 0 ? tamaActions[i] : giraffeActions[i]);
  if (currentPage != pagePet) return;
  char text[160];
  const char* radioState = !radioResponseSeen ? "Waiting" : strcmp(linkHealthText(), "responsive") == 0 ? "Linked" : strcmp(linkHealthText(), "quiet") == 0 ? "Quiet" : "Offline";
  snprintf(text, sizeof(text), "Radio: %s", radioState); lv_label_set_text(petDashboard[0], text);
  snprintf(text, sizeof(text), "Battery: %u%%", localBattery.percent); lv_label_set_text(petDashboard[1], localBattery.gaugePresent ? text : "Battery: --");
  snprintf(text, sizeof(text), "Nodes: %u", (unsigned)nodeCount); lv_label_set_text(petDashboard[2], text);
  snprintf(text, sizeof(text), "GPS: %lu sats", (unsigned long)localGps.satellites.value()); lv_label_set_text(petDashboard[3], text);
  snprintf(text, sizeof(text), "Messages: %u", (unsigned)(unreadPublic + unreadFamily + unreadDirect)); lv_label_set_text(petDashboard[4], text);
  lv_label_set_text(petTitle, activePet == 0 ? "Tamapetchi" : "Giraffegotchi");
  if (activePet == 0) {
    snprintf(text, sizeof(text), "Food %u   Joy %u\nEnergy %u\nClean %u   HP %u\nAge %lu min", tamaPet.hunger, tamaPet.happiness, tamaPet.energy, tamaPet.cleanliness, tamaPet.health, (unsigned long)tamaPet.age);
    lv_label_set_text(petStats, text); lv_label_set_text(petMood, tamaPet.mood());
    lv_obj_add_flag(petSprite, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(tamaAvatar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_y(tamaAvatar, 134 + (tamaPet.sleeping ? 0 : ((now / 500) % 2) * 3));
    lv_obj_set_style_bg_color(tamaAvatar, lv_color_hex(tamaPet.health < 30 ? 0xC48B91 : 0xB38BE8), 0);
  } else {
    lv_obj_add_flag(tamaAvatar, LV_OBJ_FLAG_HIDDEN);
    snprintf(text, sizeof(text), "Food %u  Water %u\nFun %u  Clean %u\nCleanup: %u", giraffePet.hunger(), giraffePet.thirst(), giraffePet.fun(), giraffePet.hygiene(), giraffePet.poopCount());
    lv_label_set_text(petStats, text);
    const lv_img_dsc_t* pose = &giraffe_happy; const char* mood = "Happy";
    switch (giraffePet.emotion()) {
      case Emotion::Hungry: pose = &giraffe_hungry; mood = "Hungry"; break;
      case Emotion::Sleepy: pose = &giraffe_sleepy; mood = "Sleepy"; break;
      case Emotion::Excited: pose = &giraffe_excited; mood = "Excited"; break;
      case Emotion::Sick: pose = &giraffe_sick; mood = "Needs care"; break;
      case Emotion::Reading: pose = &giraffe_reading; mood = "Reading"; break;
      case Emotion::Thirsty: mood = "Thirsty"; break;
      case Emotion::Bored: mood = "Bored"; break;
      case Emotion::Dirty: mood = "Needs cleaning"; break;
      case Emotion::Sad: mood = "Sad"; break;
      default: break;
    }
    lv_img_set_src(petSprite, pose); lv_obj_clear_flag(petSprite, LV_OBJ_FLAG_HIDDEN); lv_label_set_text(petMood, mood);
  }
}

#ifdef FIRMWARE_RUN_TESTS
static String verifyGamesRuntime() {
  if (gbCore) return "{\"busy\":true}";
  auto* savedPage = currentPage; uint8_t depth = navigationDepth;
  lv_obj_t* stack[8]; memcpy(stack, navigationStack, sizeof(stack));
  bool demo = startGameBoy(nullptr), video = false, input = false, trapped = false, recovered = false;
  bool buttons = demo, multiTouch = false, released = false, layout = false, navigation = false;
  if (demo) {
    lv_obj_update_layout(mainScreen);
    layout = lv_obj_get_y(navBar) == STATUS_BAR_H && lv_obj_get_y(pageGameBoyPlayer) == STATUS_BAR_H + NAV_BAR_H &&
             lv_obj_get_width(gbCanvas) == GameBoyInput::VIDEO_W && lv_obj_get_height(gbCanvas) == GameBoyInput::VIDEO_H;
    auto hash = []() { uint32_t value = 2166136261U; for (unsigned i = 0; i < GameBoyInput::VIDEO_W * GameBoyInput::VIDEO_H; ++i) value = (value ^ gbPixels[i].full) * 16777619U; return value; };
    gameBoyCoreFrame(gbCore, 0xff); gameBoyCoreFrame(gbCore, 0xff);
    uint32_t before = hash(); video = gbPixels[0].full != gbPixels[2].full;
    gameBoyCoreFrame(gbCore, uint8_t(~GAMEBOY_A)); gameBoyCoreFrame(gbCore, uint8_t(~GAMEBOY_A)); input = before != hash();
    lv_area_t content; lv_obj_get_content_coords(pageGameBoyPlayer, &content);
    auto point = [&content](uint8_t* packet, unsigned i, int x, int y) {
      x += content.x1; y += content.y1; unsigned at = 1 + i * 6;
      packet[at] = (x >> 8) & 15; packet[at + 1] = x & 255;
      packet[at + 2] = (y >> 8) & 15; packet[at + 3] = y & 255;
    };
    auto checkJoypad = [](uint8_t mask) {
      gameBoyCoreFrame(gbCore, gbTouchKeys);
      return (gameBoyCoreJoypadRegister(gbCore, 0x10) & 15) == (15 ^ (mask & 15)) &&
             (gameBoyCoreJoypadRegister(gbCore, 0x20) & 15) == (15 ^ (mask >> 4));
    };
    for (const auto& key : GameBoyInput::keys) {
      uint8_t packet[13] = {1}; point(packet, 0, key.x + key.w / 2, key.y + key.h / 2);
      updateGameBoyTouches(packet, sizeof(packet));
      buttons &= uint8_t(~gbTouchKeys) == key.mask && checkJoypad(key.mask);
    }
    uint8_t packet[13] = {2}; point(packet, 0, 60, 178); point(packet, 1, 200, 180);
    updateGameBoyTouches(packet, sizeof(packet)); multiTouch = uint8_t(~gbTouchKeys) == (GAMEBOY_UP | GAMEBOY_A) && checkJoypad(GAMEBOY_UP | GAMEBOY_A);
    packet[1] |= 0x40; updateGameBoyTouches(packet, sizeof(packet)); multiTouch &= uint8_t(~gbTouchKeys) == GAMEBOY_A && checkJoypad(GAMEBOY_A);
    updateGameBoyTouches(nullptr, 0); released = gbTouchKeys == 0xff && checkJoypad(0);
    gbRom[0x150] = 0xd3; gameBoyCoreInit(gbCore, nullptr, gbReadRom, gbReadRam, gbWriteRam, gbDrawLine);
    gameBoyCoreFrame(gbCore, 0xff); trapped = gameBoyCoreError(gbCore) != 0;
    gbRom[0x150] = 0xf3; gameBoyCoreInit(gbCore, nullptr, gbReadRom, gbReadRam, gbWriteRam, gbDrawLine);
    gameBoyCoreFrame(gbCore, 0xff); recovered = gameBoyCoreError(gbCore) == 0;
    lv_event_send(btnNavBack, LV_EVENT_CLICKED, nullptr); lv_obj_update_layout(mainScreen);
    navigation = currentPage != pageGameBoyPlayer && lv_obj_get_y(navBar) == SCREEN_H - NAV_BAR_H;
    gbRunning = true; showPage(pageGameBoyPlayer, false); lv_obj_update_layout(mainScreen);
    navigation &= lv_obj_get_y(navBar) == STATUS_BAR_H;
    lv_event_send(btnNavHome, LV_EVENT_CLICKED, nullptr); lv_obj_update_layout(mainScreen);
    navigation &= currentPage == pageLauncher && lv_obj_get_y(navBar) == SCREEN_H - NAV_BAR_H;
  }
  releaseGameBoy(); showPage(savedPage, false); navigationDepth = depth; memcpy(navigationStack, stack, sizeof(stack));
  TamaGame::Pet tama; tama.action(0); bool care = tama.hunger == 70; tama.update(60000); care &= tama.hunger == 65;
  Pet giraffe(10, 20, 30, 40); giraffe.feed(); care &= giraffe.hunger() == 35 && giraffe.thirst() == 20;
  return String("{\"demo\":") + (demo ? "true" : "false") + ",\"video\":" + (video ? "true" : "false") + ",\"input\":" + (input ? "true" : "false") + ",\"allButtons\":" + (buttons ? "true" : "false") + ",\"multiTouch\":" + (multiTouch ? "true" : "false") + ",\"release\":" + (released ? "true" : "false") + ",\"layout\":" + (layout ? "true" : "false") + ",\"navigation\":" + (navigation ? "true" : "false") + ",\"errorTrapped\":" + (trapped ? "true" : "false") + ",\"recovered\":" + (recovered ? "true" : "false") + ",\"independentPetCare\":" + (care ? "true" : "false") + "}";
}
#endif
