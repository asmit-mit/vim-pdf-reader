#pragma once

#include <cstdint>

namespace settings {

constexpr uint32_t bg_ = 0x1d2021;
constexpr uint32_t fg_ = 0xebdfc3;
constexpr uint32_t cmd_bg_ = 0x3c3836;
constexpr uint32_t status_bg_ = 0x282828;
constexpr uint32_t scrollwheel_color_ = 0x458588;

constexpr float delta_zoom_ = 0.1f;

constexpr char font_normal_[] =
    "/home/asmitpaul/.local/share/fonts/JetBrainsMonoNerdFont-Light.ttf";
constexpr char font_bold_[] = "/home/asmitpaul/.local/share/fonts/JetBrainsMonoNerdFont-Bold.ttf";
constexpr char font_italic_[] =
    "/home/asmitpaul/.local/share/fonts/JetBrainsMonoNerdFont-Italic.ttf";

} // namespace settings
