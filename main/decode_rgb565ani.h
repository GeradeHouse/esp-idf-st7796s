// main/decode_rgb565ani.h

#pragma once

#include "esp_err.h"
#include "st7796s.h"

esp_err_t play_rgb565ani(TFT_t *dev, const char *file, int screenWidth, int screenHeight);
