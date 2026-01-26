#pragma once

#define nameof(x) #x

#define countof(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

static const char *TAG = "cmake-template";
