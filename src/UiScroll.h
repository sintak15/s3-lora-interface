#pragma once
#include <lvgl.h>
#include <stdint.h>

namespace UiScroll {
inline void vertical(lv_obj_t* object) {
  lv_obj_set_scroll_dir(object, LV_DIR_VER);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
}

struct ListPosition {
  lv_coord_t y = 0;
  uint32_t node = 0;
  lv_coord_t offset = 0;
};

inline ListPosition capture(lv_obj_t* list) {
  lv_obj_update_layout(list);
  ListPosition position;
  position.y = lv_obj_get_scroll_y(list);
  if (position.y <= 0) return position;
  lv_area_t viewport;
  lv_obj_get_content_coords(list, &viewport);
  for (uint32_t i = 0; i < lv_obj_get_child_cnt(list); ++i) {
    auto* row = lv_obj_get_child(list, i);
    uint32_t node = (uint32_t)(uintptr_t)lv_obj_get_user_data(row);
    lv_area_t area;
    lv_obj_get_coords(row, &area);
    if (node && area.y2 >= viewport.y1) {
      position.node = node;
      position.offset = area.y1 - viewport.y1;
      break;
    }
  }
  return position;
}

inline void restore(lv_obj_t* list, const ListPosition& position) {
  lv_obj_update_layout(list);
  lv_coord_t y = position.y;
  if (position.node) {
    lv_area_t viewport;
    lv_obj_get_content_coords(list, &viewport);
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(list); ++i) {
      auto* row = lv_obj_get_child(list, i);
      if ((uint32_t)(uintptr_t)lv_obj_get_user_data(row) != position.node) continue;
      lv_area_t area;
      lv_obj_get_coords(row, &area);
      y = lv_obj_get_scroll_y(list) + area.y1 - viewport.y1 - position.offset;
      break;
    }
  }
  // LVGL clamps a shorter list to its new end; an empty list returns to zero.
  lv_obj_scroll_to_y(list, y, LV_ANIM_OFF);
}
} // namespace UiScroll
