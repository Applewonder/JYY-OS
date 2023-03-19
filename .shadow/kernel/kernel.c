#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>
#include "image_data.h"

#define SIDE 16

static int w, h;  // Screen size

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = { AM_KEYS(KEYNAME) };

static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
  }
}

void halt_if_escape() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    if (event.keycode == 0x1) {
      halt(0);
    } else {
      puts("Key pressed: ");
      puts(key_names[event.keycode]);
      // printf("%d", event.keycode);
      puts("\n");
    }
  }
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void splash() {
  w = 24;
  h = 24;
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;
  
  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      char low = my_array[(x*w + y)*3] ;
      char middle = my_array[(x*w + y)*3 + 1] << 8;
      char high = my_array[(x*w + y)*3 + 2] << 16 ;
      // if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x000000|high|middle|low); // white
      // }
    }
  }
}

// Operating system is a C program!
int main(const char *args) {// NOLINTNEXTLINE(clang-diagnostic-main-arg-wrong)
  ioe_init();

  puts("mainargs = \"");
  puts(args);  // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    halt_if_escape();
  }
  return 0;
}
