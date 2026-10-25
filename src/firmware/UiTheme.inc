// Shared visual treatment. Icon children are decorative and never catch touch.
#include "../UiIcons.h"

static lv_obj_t* makeThemeIcon(lv_obj_t* parent, UiIcons::Id id, unsigned size=20, uint32_t color=COLOR_ACCENT) {
  auto* icon = lv_img_create(parent);
  lv_img_set_src(icon, UiIcons::image(id, size));
  lv_obj_set_style_img_recolor(icon, lv_color_hex(color), 0);
  lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  return icon;
}

static UiIcons::Id navigationIcon(const char* title) {
  struct NamedIcon { const char* title; UiIcons::Id id; };
  static const NamedIcon icons[] = {
    {"Menu",UiIcons::Id::Menu}, {"Messages",UiIcons::Id::Messages}, {"Direct",UiIcons::Id::Messages},
    {"Nodes",UiIcons::Id::Nodes}, {"Mesh",UiIcons::Id::Nodes}, {"Map",UiIcons::Id::Map},
    {"TX Log",UiIcons::Id::Delivery}, {"Packets",UiIcons::Id::Delivery},
    {"Health",UiIcons::Id::Health}, {"System",UiIcons::Id::Settings}, {"Interface",UiIcons::Id::Display},
    {"Serial",UiIcons::Id::Serial}, {"Radio",UiIcons::Id::Radio}, {"GPS",UiIcons::Id::Gps},
    {"WiFi",UiIcons::Id::Wifi}, {"Display",UiIcons::Id::Display}, {"Sound",UiIcons::Id::Sound},
    {"Battery",UiIcons::Id::Battery}, {"Power",UiIcons::Id::Battery}, {"Msg Test",UiIcons::Id::Health}
  };
  for (const auto& icon : icons) if (strcmp(icon.title,title)==0) return icon.id;
  return UiIcons::Id::Channels;
}

static UiIcons::Id pageThemeIcon(lv_obj_t* page) {
  if (page==pageLauncher) return UiIcons::Id::Home;
  if (page==pageFunctions) return UiIcons::Id::Menu;
  if (page==pageOperate || page==pagePublicChat || page==pagePrivateChat || page==pageDirectChat) return UiIcons::Id::Messages;
  if (page==pageNodes || page==pageNodeDetail || page==pageMeshHealth) return UiIcons::Id::Nodes;
  if (page==pageGps || page==pageMapCoverage) return UiIcons::Id::Map;
  if (page==pageTxHistory || page==pagePacketInspector) return UiIcons::Id::Delivery;
  if (page==pageDiagnose || page==pageMessageDiag) return UiIcons::Id::Health;
  if (page==pageSystemGps || page==pageGpsConfig || page==pageGpsAdvanced) return UiIcons::Id::Gps;
  if (page==pageWifi || page==pageWifiStats || page==pageWifiLocal || page==pageWifiScan) return UiIcons::Id::Wifi;
  if (page==pageBattery) return UiIcons::Id::Battery;
  if (page==pagePowerLogs) return UiIcons::Id::Logs;
  if (page==pageSound) return UiIcons::Id::Sound;
  if (page==pageBacklight || page==pageSystemInterface) return UiIcons::Id::Display;
  if (page==pageSystemSerial) return UiIcons::Id::Serial;
  if (page==pageSystemRadio) return UiIcons::Id::Radio;
  return UiIcons::Id::Settings;
}

static void styleThemeControl(lv_obj_t* object) {
  if (!lv_obj_check_type(object,&lv_btn_class)) return;
  static const lv_style_prop_t properties[] = {LV_STYLE_BG_COLOR,LV_STYLE_BORDER_COLOR,LV_STYLE_PROP_INV};
  static lv_style_transition_dsc_t transition;
  static bool ready = false;
  if (!ready) {
    lv_style_transition_dsc_init(&transition,properties,lv_anim_path_ease_out,100,0,nullptr);
    ready = true;
  }
  lv_obj_set_style_radius(object,10,0);
  lv_obj_set_style_bg_color(object,lv_color_hex(0x20476E),LV_STATE_PRESSED);
  lv_obj_set_style_border_color(object,lv_color_hex(0x6596C9),LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(object,0,LV_STATE_PRESSED);
  lv_obj_set_style_transition(object,&transition,LV_STATE_DEFAULT);
  lv_obj_set_style_transition(object,&transition,LV_STATE_PRESSED);
}
