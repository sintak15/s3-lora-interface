// Real LVGL objects, exercised by the verification build's USB test-ui command.
static bool runUiScrollTests() {
  bool passed = true;
  auto check = [&passed](const char* label, bool ok) {
    Serial.printf("[test-scroll] %s: %s\n", label, ok ? "PASS" : "FAIL");
    passed &= ok;
  };
  auto* fixture = lv_obj_create(lv_scr_act());
  lv_obj_set_size(fixture, SCREEN_W, SCREEN_H);
  lv_obj_clear_flag(fixture, LV_OBJ_FLAG_SCROLLABLE);
  auto* list = lv_list_create(fixture);
  lv_obj_set_size(list, 210, 100);
  UiScroll::vertical(list);
  auto fill = [list](uint32_t first, int height) {
    for (uint32_t id = first; id < first + 15; ++id) {
      auto* row = lv_obj_create(list);
      lv_obj_set_size(row, 180, height);
      lv_obj_set_user_data(row, (void*)(uintptr_t)id);
      lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_update_layout(list);
  };
  fill(2, 30);
  lv_obj_scroll_to_y(list, 140, LV_ANIM_OFF);
  auto before = UiScroll::capture(list);
  check("List can scroll away from top", before.y > 0 && before.node != 0);
  lv_obj_clean(list);
  fill(1, 30); // A newly heard node sorts above the row being read.
  UiScroll::restore(list, before);
  auto after = UiScroll::capture(list);
  check("Refresh and insertion preserve visible node", after.node == before.node && after.offset == before.offset);
  lv_obj_clean(list);
  fill(1, 60); // Compact/detail changes keep the same node in view.
  UiScroll::restore(list, before);
  after = UiScroll::capture(list);
  check("Row-height changes preserve visible node", after.node == before.node && after.offset == before.offset);
  lv_obj_clean(list);
  fill(100, 30); // The anchor disappears from the filtered set.
  UiScroll::restore(list, before);
  check("Missing anchor retains bounded offset", lv_obj_get_scroll_y(list) == before.y);
  lv_obj_clean(list);
  UiScroll::restore(list, before);
  check("Empty list clamps to top", lv_obj_get_scroll_y(list) == 0);

  auto* text = makeReadonlyText(fixture, 0, 80);
  const char* lines = "one\ntwo\nthree\nfour\nfive\nsix\nseven\neight\nnine\nten\neleven\ntwelve";
  setReadonlyTextStable(text, lines, true);
  lv_obj_scroll_to_y(text, 25, LV_ANIM_OFF);
  lv_coord_t readingY = lv_obj_get_scroll_y(text);
  setReadonlyTextNewest(text, "updated\none\ntwo\nthree\nfour\nfive\nsix\nseven\neight\nnine\nten\neleven\ntwelve", false);
  check("Incoming chat preserves older reading position", readingY > 0 && lv_obj_get_scroll_y(text) == readingY);
  scrollReadonlyTextToBottom(text);
  setReadonlyTextNewest(text, "updated\none\ntwo\nthree\nfour\nfive\nsix\nseven\neight\nnine\nten\neleven\ntwelve\nnewest", false);
  check("Chat follows new messages only at bottom", lv_obj_get_scroll_bottom(text) <= 4);
  auto* button = makeSmallButton(fixture, "Test", 0, 0, 80, [](lv_event_t*) {});
  check("Touch focus cannot pull menus back", !lv_obj_has_flag(button, LV_OBJ_FLAG_SCROLL_ON_FOCUS));
  auto* panel = makePanel(fixture);
  check("Decorative panels pass scrolling to page", !lv_obj_has_flag(panel, LV_OBJ_FLAG_SCROLLABLE));
  check("Lists and text use bounded vertical scrolling",
        lv_obj_get_scroll_dir(list) == LV_DIR_VER && lv_obj_get_scroll_dir(text) == LV_DIR_VER &&
        !lv_obj_has_flag(list, LV_OBJ_FLAG_SCROLL_ELASTIC) && !lv_obj_has_flag(text, LV_OBJ_FLAG_SCROLL_ELASTIC));
  lv_obj_del(fixture);
  return passed;
}
