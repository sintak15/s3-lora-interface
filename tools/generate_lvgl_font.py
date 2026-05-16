#!/usr/bin/env python3
from pathlib import Path
import argparse

from PIL import Image, ImageDraw, ImageFont


ASCII_START = 0x20
ASCII_END = 0x7E
EXTRA_CODEPOINTS = [0x00B0, 0x2022]


def pack_4bpp(mask):
    pixels = list(mask.getdata())
    packed = []
    for i in range(0, len(pixels), 2):
        hi = pixels[i] >> 4
        lo = (pixels[i + 1] >> 4) if i + 1 < len(pixels) else 0
        packed.append((hi << 4) | lo)
    return packed


def render_glyph(font, codepoint):
    ch = chr(codepoint)
    bbox = font.getbbox(ch, anchor="ls")
    advance = font.getlength(ch)
    if bbox is None:
        return {
            "bitmap": [],
            "adv_w": int(round(advance * 16)),
            "box_w": 0,
            "box_h": 0,
            "ofs_x": 0,
            "ofs_y": 0,
        }

    x0, y0, x1, y1 = bbox
    box_w = max(0, x1 - x0)
    box_h = max(0, y1 - y0)
    if box_w == 0 or box_h == 0:
        bitmap = []
    else:
        mask = Image.new("L", (box_w, box_h), 0)
        draw = ImageDraw.Draw(mask)
        draw.text((-x0, -y0), ch, font=font, fill=255, anchor="ls")
        bitmap = pack_4bpp(mask)

    return {
        "bitmap": bitmap,
        "adv_w": int(round(advance * 16)),
        "box_w": box_w,
        "box_h": box_h,
        "ofs_x": x0,
        "ofs_y": -y1,
    }


def c_array(values, indent="    ", per_line=12):
    if not values:
        return ""
    lines = []
    for i in range(0, len(values), per_line):
        chunk = values[i : i + per_line]
        lines.append(indent + ", ".join(f"0x{v:02X}" for v in chunk) + ",")
    return "\n".join(lines)


def generate(font_path, size, symbol, output):
    font = ImageFont.truetype(str(font_path), size)
    ascent, descent = font.getmetrics()
    line_height = ascent + descent
    baseline = descent
    codepoints = list(range(ASCII_START, ASCII_END + 1)) + EXTRA_CODEPOINTS

    glyphs = []
    bitmap = []
    for codepoint in codepoints:
        glyph = render_glyph(font, codepoint)
        glyph["bitmap_index"] = len(bitmap)
        bitmap.extend(glyph["bitmap"])
        glyphs.append(glyph)

    sparse_offsets = [cp - EXTRA_CODEPOINTS[0] for cp in EXTRA_CODEPOINTS]
    guard = f"LV_FONT_{symbol.upper()}"

    glyph_lines = [
        "    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,"
    ]
    for glyph in glyphs:
        glyph_lines.append(
            "    "
            f"{{.bitmap_index = {glyph['bitmap_index']}, .adv_w = {glyph['adv_w']}, "
            f".box_w = {glyph['box_w']}, .box_h = {glyph['box_h']}, "
            f".ofs_x = {glyph['ofs_x']}, .ofs_y = {glyph['ofs_y']}}},"
        )

    output.write_text(
        f"""/*******************************************************************************
 * Comic Neue static LVGL font.
 * Size: {size} px
 * Bpp: 4
 * Range: ASCII 0x20-0x7E, degree, bullet
 ******************************************************************************/

#include <lvgl.h>

#ifndef {guard}
#define {guard} 1
#endif

#if {guard}

static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {{
{c_array(bitmap)}
}};

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {{
{chr(10).join(glyph_lines)}
}};

static const uint16_t unicode_list_1[] = {{
    {", ".join(f"0x{offset:X}" for offset in sparse_offsets)}
}};

static const lv_font_fmt_txt_cmap_t cmaps[] = {{
    {{
        .range_start = {ASCII_START}, .range_length = {ASCII_END - ASCII_START + 1}, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }},
    {{
        .range_start = {EXTRA_CODEPOINTS[0]}, .range_length = {EXTRA_CODEPOINTS[-1] - EXTRA_CODEPOINTS[0] + 1}, .glyph_id_start = {ASCII_END - ASCII_START + 2},
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = {len(EXTRA_CODEPOINTS)}, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }}
}};

#if LV_VERSION_CHECK(8, 0, 0)
static lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {{
#else
static lv_font_fmt_txt_dsc_t font_dsc = {{
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
}};

#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t {symbol} = {{
#else
lv_font_t {symbol} = {{
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = {line_height},
    .base_line = {baseline},
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc
}};

#endif
""",
        encoding="ascii",
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", required=True, type=Path)
    parser.add_argument("--size", required=True, type=int)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    generate(args.font, args.size, args.symbol, args.output)


if __name__ == "__main__":
    main()
