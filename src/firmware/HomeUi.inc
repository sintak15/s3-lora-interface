// Home has its own type hierarchy and quiet card palette; other pages retain
// their established layout. All four cards share styling and touch feedback.
static lv_obj_t* homeLabel(lv_obj_t* parent, const char* text, int x, int y, int width, const lv_font_t* font, uint32_t color) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_width(label, width);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  return label;
}

static lv_obj_t* homeCard(const char* title, int x, int y, int width, int height, lv_event_cb_t action, lv_obj_t** caption = nullptr) {
  lv_obj_t* card = lv_btn_create(pageLauncher);
  lv_obj_set_pos(card, x, y); lv_obj_set_size(card, width, height);
  styleDarkObject(card, 0x111E1B, 0xEDF5EF);
  styleDarkBorder(card, 0x263C34);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x203D32), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(card, lv_color_hex(0x638B77), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(card, 0, LV_STATE_PRESSED);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(card, action, LV_EVENT_CLICKED, nullptr);
  auto* label = homeLabel(card, title, 9, 6, width - 37, &lv_font_montserrat_10, 0xA8BCB0);
  if (caption) *caption = label;
  homeLabel(card, LV_SYMBOL_RIGHT, width - 19, 6, 10, LV_FONT_DEFAULT, 0x789B88);
  return card;
}

static void buildHomeUi() {
  lv_obj_set_style_bg_color(pageLauncher, lv_color_hex(0x0B1713), 0);
  lv_obj_set_style_bg_grad_color(pageLauncher, lv_color_hex(0x050A08), 0);
  lv_obj_set_style_bg_grad_dir(pageLauncher, LV_GRAD_DIR_VER, 0);
  homeLabel(pageLauncher, "At a glance", 8, 0, 214, &lv_font_montserrat_22, 0xEDF5EF);
  auto* caption = homeLabel(pageLauncher, "FIELD DASHBOARD", 9, 28, 210, &lv_font_montserrat_10, 0x88AB96);
  lv_obj_set_style_text_letter_space(caption, 1, 0);

  auto* radio = homeCard("RADIO LINK", 6, 48, 218, 62, [](lv_event_t*) { showPage(pageSystemRadio); });
  lblHomeLink = homeLabel(radio, "Waiting", 9, 21, 157, &lv_font_montserrat_22, 0xEDF5EF);
  lblHomeLinkDetail = homeLabel(radio, "Listening for Heltec", 10, 47, 154, &lv_font_montserrat_10, 0xA8BCB0);
  homeLinkIcon = lv_obj_create(radio);
  lv_obj_set_pos(homeLinkIcon, 175, 25); lv_obj_set_size(homeLinkIcon, 28, 28);
  styleDarkObject(homeLinkIcon, 0x20362C); lv_obj_set_style_radius(homeLinkIcon, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(homeLinkIcon, 0, 0); lv_obj_set_style_pad_all(homeLinkIcon, 0, 0);
  lv_obj_clear_flag(homeLinkIcon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  auto* icon = lv_label_create(homeLinkIcon); lv_label_set_text(icon, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(icon, LV_FONT_DEFAULT, 0); lv_obj_center(icon);

  auto* battery = homeCard("BATTERY", 6, 118, 105, 70, [](lv_event_t*) { showPage(pageBattery); });
  lblHomePower = homeLabel(battery, "--", 9, 21, 85, &lv_font_montserrat_22, 0xEDF5EF);
  lblHomePowerDetail = homeLabel(battery, "Checking gauge", 9, 49, 85, &lv_font_montserrat_10, 0xA8BCB0);
  homePowerBar = lv_bar_create(battery); lv_obj_set_pos(homePowerBar, 9, 63); lv_obj_set_size(homePowerBar, 84, 2);
  lv_bar_set_range(homePowerBar, 0, 100); lv_bar_set_value(homePowerBar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(homePowerBar, lv_color_hex(0x2D4137), LV_PART_MAIN);
  lv_obj_set_style_bg_color(homePowerBar, lv_color_hex(0x8DCEAA), LV_PART_INDICATOR);
  lv_obj_clear_flag(homePowerBar, LV_OBJ_FLAG_CLICKABLE);

  auto* mesh = homeCard("NODES", 119, 118, 105, 70, [](lv_event_t*) { refreshNodeList(true); showPage(pageNodes); });
  lblHomeNodes = homeLabel(mesh, "0", 9, 21, 85, &lv_font_montserrat_22, 0xEDF5EF);
  lblHomeGps = homeLabel(mesh, "GPS waiting", 9, 49, 85, &lv_font_montserrat_10, 0xA8BCB0);

  auto* messages = homeCard("Messages", 6, 196, 218, 48, [](lv_event_t*) { showPage(pageOperate); }, &lblHomeMessageTitle);
  lblHomeUnread = homeLabel(messages, "", 123, 6, 67, &lv_font_montserrat_10, 0xA8BCB0);
  lv_obj_set_style_text_align(lblHomeUnread, LV_TEXT_ALIGN_RIGHT, 0);
  lblHomeMessages = homeLabel(messages, "No messages yet", 9, 23, 195, &lv_font_montserrat_14, 0xEDF5EF);
}
