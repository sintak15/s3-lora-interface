// Home has its own type hierarchy and quiet card palette; other pages retain
// their established layout. All four cards share styling and touch feedback.
static lv_obj_t* homeLabel(lv_obj_t* parent, const char* text, int x, int y, int width, const lv_font_t* font, uint32_t color) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_width(label, width);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_height(label, font->line_height);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  return label;
}

static lv_obj_t* homeCard(const char* title, UiIcons::Id iconId, int x, int y, int width, int height, lv_event_cb_t action, lv_obj_t** caption = nullptr) {
  lv_obj_t* card = lv_btn_create(pageLauncher);
  lv_obj_set_pos(card, x, y); lv_obj_set_size(card, width, height);
  styleDarkObject(card, 0x102238, 0xF4F8FF);
  styleDarkBorder(card, 0x294766);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1C3C60), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(card, lv_color_hex(0x6596C9), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(card, 0, LV_STATE_PRESSED);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(card, action, LV_EVENT_CLICKED, nullptr);
  auto* label = homeLabel(card, title, 31, 6, width - 55, &montserrat_11, COLOR_ACCENT);
  if (caption) *caption = label;
  auto* icon = makeThemeIcon(card,iconId,16);
  lv_obj_set_pos(icon,9,5);
  auto* next = makeThemeIcon(card,UiIcons::Id::Next,16);
  lv_obj_set_pos(next,width-20,5);
  return card;
}

static void buildHomeUi() {
  lv_obj_set_style_bg_color(pageLauncher, lv_color_hex(0x0C1C30), 0);
  lv_obj_set_style_bg_grad_color(pageLauncher, lv_color_hex(0x07111F), 0);
  lv_obj_set_style_bg_grad_dir(pageLauncher, LV_GRAD_DIR_VER, 0);
  homeLabel(pageLauncher, "At a glance", 8, 0, 148, &lv_font_montserrat_22, 0xF4F8FF);
  auto* caption = homeLabel(pageLauncher, "FIELD DASHBOARD", 9, 28, 148, &montserrat_11, 0x85BFFF);
  btnHomeGames = makeSmallButton(pageLauncher, "Games", 164, 0, 60, [](lv_event_t*) { showPage(pageGames); });
  lv_obj_set_height(btnHomeGames, 44);
  lv_obj_set_style_text_letter_space(caption, 1, 0);

  auto* radio = homeCard("RADIO LINK", UiIcons::Id::Radio, 6, 48, 218, 62, [](lv_event_t*) { showPage(pageSystemRadio); });
  lblHomeLink = homeLabel(radio, "Waiting", 9, 21, 157, &lv_font_montserrat_22, 0xF4F8FF);
  lblHomeLinkDetail = homeLabel(radio, "Listening for Heltec", 10, 47, 154, &montserrat_11, 0xA1B5CE);
  homeLinkIcon = lv_obj_create(radio);
  lv_obj_set_pos(homeLinkIcon, 175, 25); lv_obj_set_size(homeLinkIcon, 28, 28);
  styleDarkObject(homeLinkIcon, 0x173B5D); lv_obj_set_style_radius(homeLinkIcon, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(homeLinkIcon, 0, 0); lv_obj_set_style_pad_all(homeLinkIcon, 0, 0);
  lv_obj_clear_flag(homeLinkIcon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  auto* icon = makeThemeIcon(homeLinkIcon,UiIcons::Id::Radio); lv_obj_center(icon);

  auto* battery = homeCard("BATTERY", UiIcons::Id::Battery, 6, 118, 105, 70, [](lv_event_t*) { showPage(pageBattery); });
  lblHomePower = homeLabel(battery, "--", 9, 21, 85, &lv_font_montserrat_22, 0xF4F8FF);
  lblHomePowerDetail = homeLabel(battery, "Checking battery", 9, 47, 87, &montserrat_11, 0xA1B5CE);
  homePowerBar = lv_bar_create(battery); lv_obj_set_pos(homePowerBar, 9, 65); lv_obj_set_size(homePowerBar, 84, 2);
  lv_bar_set_range(homePowerBar, 0, 100); lv_bar_set_value(homePowerBar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(homePowerBar, lv_color_hex(0x294766), LV_PART_MAIN);
  lv_obj_set_style_bg_color(homePowerBar, lv_color_hex(0x65B0FF), LV_PART_INDICATOR);
  lv_obj_clear_flag(homePowerBar, LV_OBJ_FLAG_CLICKABLE);

  auto* mesh = homeCard("NODES", UiIcons::Id::Nodes, 119, 118, 105, 70, [](lv_event_t*) { refreshNodeList(true); showPage(pageNodes); });
  lblHomeNodes = homeLabel(mesh, "0", 9, 21, 85, &lv_font_montserrat_22, 0xF4F8FF);
  lblHomeGps = homeLabel(mesh, "GPS waiting", 9, 49, 85, &montserrat_11, 0xA1B5CE);

  auto* messages = homeCard("Messages", UiIcons::Id::Messages, 6, 196, 218, 48, [](lv_event_t*) { showPage(pageOperate); }, &lblHomeMessageTitle);
  lv_obj_set_width(lblHomeMessageTitle, 90);
  lblHomeUnread = homeLabel(messages, "", 123, 6, 67, &montserrat_11, 0xA1B5CE);
  lv_obj_set_style_text_align(lblHomeUnread, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_bg_color(lblHomeUnread,lv_color_hex(0x1C3C60),0);
  lv_obj_set_style_bg_opa(lblHomeUnread,LV_OPA_COVER,0);
  lv_obj_set_style_radius(lblHomeUnread,5,0);
  lblHomeMessages = homeLabel(messages, "No messages yet", 9, 23, 195, &montserrat_15, 0xF4F8FF);
}
