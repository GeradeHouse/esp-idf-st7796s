// main/decode_gif.h

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "st7796s.h"

esp_err_t decode_gif(TFT_t *dev, const char *file, int screenWidth, int screenHeight);
