#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * COMPLETELY VIBECODED OS - 100MB RAM i386 "Liquid Glass" Edition
 * Features: 100MB PMM, Button Ease-Out Tweening, Interactive Task Manager
 * ============================================================================ */

#define RAM_LIMIT_BYTES  (100 * 1024 * 1024) /* 100MB ceiling (104,857,600 bytes) */
#define PAGE_SIZE        4096
#define TOTAL_PAGES      (RAM_LIMIT_BYTES / PAGE_SIZE) /* 25,600 pages */
#define BITMAP_SIZE      (TOTAL_PAGES / 8)             /* 3,200 bytes */

static uint8_t pmm_bitmap[BITMAP_SIZE];
static uint32_t used_pages = 1024;

void pmm_init(void) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = (i < (1024 / 8)) ? 0xFF : 0x00;
    }
    used_pages = 1024;
}

void* pmm_alloc_page(void) {
    for (uint32_t byte = 0; byte < BITMAP_SIZE; byte++) {
        if (pmm_bitmap[byte] != 0xFF) {
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (!(pmm_bitmap[byte] & (1 << bit))) {
                    pmm_bitmap[byte] |= (1 << bit);
                    used_pages++;
                    uintptr_t page_idx = (byte * 8) + bit;
                    return (void*)(page_idx * PAGE_SIZE);
                }
            }
        }
    }
    return NULL;
}

void pmm_free_page(void* addr) {
    uintptr_t page_idx = ((uintptr_t)addr) / PAGE_SIZE;
    if (page_idx < TOTAL_PAGES && (pmm_bitmap[page_idx / 8] & (1 << (page_idx % 8)))) {
        pmm_bitmap[page_idx / 8] &= ~(1 << (page_idx % 8));
        used_pages--;
    }
}

#define SCREEN_W 800
#define SCREEN_H 600

#if defined(__aarch64__)
static volatile uint32_t* framebuffer = (volatile uint32_t*)0x44000000; /* ARM64 QEMU ramfb */
#else
static volatile uint32_t* framebuffer = (volatile uint32_t*)0xE0000000; /* i386 VESA LFB */
#endif
static uint32_t backbuffer[SCREEN_W * SCREEN_H];

void swap_buffers(void) {
    for (uint32_t i = 0; i < SCREEN_W * SCREEN_H; i++) {
        framebuffer[i] = backbuffer[i];
    }
}

void kernel_main(void);

void _start(void) {
    kernel_main();
    while (1) {
#if defined(__aarch64__)
        __asm__ volatile("wfi");
#else
        __asm__ volatile("hlt");
#endif
    }
}

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        backbuffer[y * SCREEN_W + x] = color;
    }
}

void fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int row = y; row < y + h && row < SCREEN_H; row++) {
        for (int col = x; col < x + w && col < SCREEN_W; col++) {
            if (row >= 0 && col >= 0) {
                backbuffer[row * SCREEN_W + col] = color;
            }
        }
    }
}

uint32_t alpha_blend(uint32_t fg, uint32_t bg, uint8_t alpha) {
    uint32_t r_fg = (fg >> 16) & 0xFF, g_fg = (fg >> 8) & 0xFF, b_fg = fg & 0xFF;
    uint32_t r_bg = (bg >> 16) & 0xFF, g_bg = (bg >> 8) & 0xFF, b_bg = bg & 0xFF;
    uint32_t r = (r_fg * alpha + r_bg * (255 - alpha)) / 255;
    uint32_t g = (g_fg * alpha + g_bg * (255 - alpha)) / 255;
    uint32_t b = (b_fg * alpha + b_bg * (255 - alpha)) / 255;
    return (r << 16) | (g << 8) | b;
}

void draw_glass_rect(int x, int y, int w, int h, uint8_t alpha, uint32_t tint) {
    for (int row = 0; row < h && (y + row) < SCREEN_H; row++) {
        for (int col = 0; col < w && (x + col) < SCREEN_W; col++) {
            int px = x + col, py = y + row;
            if (px >= 0 && py >= 0) {
                uint32_t bg = backbuffer[py * SCREEN_W + px];
                backbuffer[py * SCREEN_W + px] = alpha_blend(tint, bg, alpha);
            }
        }
    }
}

void draw_glass_panel(int x, int y, int w, int h, uint8_t alpha, uint32_t tint) {
    draw_glass_rect(x + 4, y + 6, w, h, 70, 0x00081020);
    draw_glass_rect(x, y, w, h, alpha, tint);
    for (int col = x; col < x + w; col++) {
        put_pixel(col, y, 0x00FFFFFF);
        put_pixel(col, y + h - 1, 0x0090A8C0);
    }
    for (int row = y; row < y + h; row++) {
        put_pixel(x, row, 0x00FFFFFF);
        put_pixel(x + w - 1, row, 0x0090A8C0);
    }
}

static const uint8_t font8x8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00},
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00},
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00},
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00},
    {0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0x00},
    {0x00,0x00,0x1E,0x33,0x3f,0x03,0x1E,0x00},
    {0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0x00},
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F},
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E},
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00},
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00},
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00},
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F},
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78},
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00},
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00},
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00},
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00},
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00},
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F},
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00},
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}
};

void draw_char(int x, int y, char c, uint32_t color) {
    if (c < 32 || c > 126) return;
    const uint8_t* glyph = font8x8[c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << col)) put_pixel(x + col, y + row, color);
        }
    }
}

void draw_text(int x, int y, const char* str, uint32_t color) {
    int cx = x;
    while (*str) {
        if (*str == '\n') { cx = x; y += 12; }
        else { draw_char(cx, y, *str, color); cx += 8; }
        str++;
    }
}

void draw_text_bold(int x, int y, const char* str, uint32_t color) {
    draw_text(x, y, str, color);
    draw_text(x + 1, y, str, color);
}

typedef struct {
    int x, y, w, h;
    const char* label;
    uint32_t base_color;
    uint32_t hover_color;
    int hover_t;
    int target_t;
    int press_offset;
} TweenButton;

static TweenButton tm_buttons[3] = {
    { 465, 442, 92, 24, "End Task",  0x00EF4444, 0x00DC2626, 0, 1000, 0 },
    { 567, 442, 82, 24, "Refresh",   0x003B82F6, 0x002563EB, 0, 0,    0 },
    { 659, 442, 96, 24, "New Task",  0x0010B981, 0x00059669, 0, 0,    0 }
};

int ease_out_cubic(int t) {
    int inv = 1000 - t;
    int inv3 = (inv * inv / 1000) * inv / 1000;
    return 1000 - inv3;
}

uint32_t lerp_color(uint32_t c1, uint32_t c2, int t) {
    uint32_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint32_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    uint32_t r = r1 + ((int)(r2 - r1) * t) / 1000;
    uint32_t g = g1 + ((int)(g2 - g1) * t) / 1000;
    uint32_t b = b1 + ((int)(b2 - b1) * t) / 1000;
    return (r << 16) | (g << 8) | b;
}

void draw_tween_button(TweenButton* btn) {
    int eased = ease_out_cubic(btn->hover_t);
    int scale_expand = (eased * 4) / 1000;
    int bx = btn->x - scale_expand / 2;
    int by = btn->y - scale_expand / 2 + btn->press_offset;
    int bw = btn->w + scale_expand;
    int bh = btn->h + scale_expand;

    uint32_t bg = lerp_color(btn->base_color, btn->hover_color, eased);
    draw_glass_panel(bx, by, bw, bh, 235, bg);
    draw_glass_rect(bx + 1, by + 1, bw - 2, bh / 2, 75, 0x00FFFFFF);

    int len = 0;
    while (btn->label[len]) len++;
    draw_text_bold(bx + (bw - len * 8) / 2, by + (bh - 8) / 2, btn->label, 0x00FFFFFF);
}

void draw_glass_window(int x, int y, int w, int h, const char* title) {
    draw_glass_panel(x, y, w, h, 220, 0x00F4F8FC);
    draw_glass_rect(x + 1, y + 1, w - 2, 28, 240, 0x00FFFFFF);
    for (int col = x + 1; col < x + w - 1; col++) {
        put_pixel(col, y + 29, 0x00CBD5E1);
    }
    fill_rect(x + 12, y + 10, 10, 10, 0x00FF5F56);
    fill_rect(x + 28, y + 10, 10, 10, 0x00FFBD2E);
    fill_rect(x + 44, y + 10, 10, 10, 0x0027C93F);

    int len = 0;
    while (title[len]) len++;
    draw_text_bold(x + (w - len * 8) / 2, y + 11, title, 0x000F172A);
}

void draw_app_icon(int x, int y, const char* name, uint32_t color, int hover_boost) {
    int lift = hover_boost ? -3 : 0;
    draw_glass_panel(x, y + lift, 50, 50, 235, color);
    draw_glass_rect(x + 2, y + lift + 2, 46, 20, 95, 0x00FFFFFF);
    fill_rect(x + 15, y + lift + 15, 20, 20, 0x00FFFFFF);
    draw_glass_rect(x + 18, y + lift + 18, 14, 14, 140, color);

    int len = 0;
    while (name[len]) len++;
    draw_text(x + 25 - (len * 8) / 2, y + 56, name, 0x001E293B);
}

void draw_task_manager(int x, int y, int w, int h, int cpu_tick) {
    draw_glass_window(x, y, w, h, "Task Manager - 100MB System Monitor");

    draw_glass_panel(x + 14, y + 40, 155, 64, 210, 0x00E0F2FE);
    draw_text_bold(x + 24, y + 48, "CPU LOAD", 0x000369A1);
    draw_text_bold(x + 24, y + 66, "14.2%  (i386)", 0x000F172A);
    for (int b = 0; b < 12; b++) {
        int bh = 8 + ((b * 7 + cpu_tick) % 16);
        fill_rect(x + 24 + b * 11, y + 98 - bh, 8, bh, 0x000284C7);
    }

    draw_glass_panel(x + 182, y + 40, 174, 64, 210, 0x00ECFDF5);
    draw_text_bold(x + 192, y + 48, "MEMORY (100 MB)", 0x00047857);
    draw_text_bold(x + 192, y + 66, "6.4 / 100.0 MB", 0x000F172A);
    draw_glass_panel(x + 192, y + 84, 154, 12, 230, 0x00FFFFFF);
    fill_rect(x + 194, y + 86, 14, 8, 0x0010B981);

    draw_glass_rect(x + 14, y + 114, w - 28, 22, 235, 0x00E2E8F0);
    draw_text_bold(x + 22,  y + 121, "Process Name", 0x001E293B);
    draw_text_bold(x + 185, y + 121, "PID",          0x001E293B);
    draw_text_bold(x + 230, y + 121, "CPU",          0x001E293B);
    draw_text_bold(x + 285, y + 121, "RAM",          0x001E293B);

    const char* procs[6][4] = {
        { "kernel.bin",       "01", "2.1%", "2.4 MB" },
        { "glass_compositor", "02", "8.4%", "1.8 MB" },
        { "pmm_100mb_mgr",    "03", "0.3%", "0.4 MB" },
        { "tween_animator",   "04", "1.9%", "0.5 MB" },
        { "task_manager",     "05", "1.2%", "0.9 MB" },
        { "finder_dock",      "06", "0.3%", "0.4 MB" }
    };

    for (int i = 0; i < 6; i++) {
        int ry = y + 140 + i * 24;
        if (i == 4) {
            draw_glass_rect(x + 14, ry - 2, w - 28, 22, 210, 0x003B82F6);
            draw_text_bold(x + 22,  ry + 4, procs[i][0], 0x00FFFFFF);
            draw_text(x + 185,      ry + 4, procs[i][1], 0x00FFFFFF);
            draw_text(x + 230,      ry + 4, procs[i][2], 0x00FFFFFF);
            draw_text(x + 285,      ry + 4, procs[i][3], 0x00FFFFFF);
        } else {
            draw_text(x + 22,  ry + 4, procs[i][0], 0x000F172A);
            draw_text(x + 185, ry + 4, procs[i][1], 0x00475569);
            draw_text(x + 230, ry + 4, procs[i][2], 0x00475569);
            draw_text(x + 285, ry + 4, procs[i][3], 0x00047857);
        }
    }

    for (int i = 0; i < 3; i++) {
        draw_tween_button(&tm_buttons[i]);
    }
}

void draw_cursor(int x, int y) {
    static const char* arrow[14] = {
        "X.............","XX............","XOX...........","XOOX..........",
        "XOOOX.........","XOOOOX........","XOOOOOX.......","XOOOOOOX......",
        "XOOOOOOOX.....","XOOOOXXXXX....","XOOXOOX.......","XOX.XOOX......",
        "XX...XOOX.....","......XX......"
    };
    for (int r = 0; r < 14; r++) {
        for (int c = 0; c < 14; c++) {
            if (arrow[r][c] == 'X') put_pixel(x + c, y + r, 0x00000000);
            else if (arrow[r][c] == 'O') put_pixel(x + c, y + r, 0x00FFFFFF);
        }
    }
}

void render_desktop(int tick) {
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint32_t r = 16 + (y * 28) / SCREEN_H + (x * 22) / SCREEN_W;
            uint32_t g = 45 + (y * 72) / SCREEN_H;
            uint32_t b = 110 + (y * 115) / SCREEN_H - (x * 18) / SCREEN_W;
            if (y > 390) {
                r = 10 + ((y - 390) * 14) / 210;
                g = 30 + ((y - 390) * 32) / 210;
                b = 72 + ((y - 390) * 44) / 210;
            }
            backbuffer[y * SCREEN_W + x] = (r << 16) | (g << 8) | b;
        }
    }

    draw_glass_rect(0, 0, SCREEN_W, 26, 205, 0x00FFFFFF);
    draw_text_bold(14, 9, "OS", 0x000F172A);
    draw_text_bold(42, 9, "Finder", 0x000F172A);
    draw_text(104, 9, "File   Edit   View   Go   Window   Help", 0x001E293B);
    draw_text_bold(SCREEN_W - 158, 9, "100MB RAM | 100%", 0x000F172A);

    draw_glass_window(18, 40, 365, 440, "Applications - Liquid Glass");
    draw_app_icon(40,  84, "App Store", 0x002563EB, 0);
    draw_app_icon(112, 84, "Finder",    0x000284C7, 0);
    draw_app_icon(184, 84, "Safari",    0x003B82F6, 0);
    draw_app_icon(256, 84, "Messages",  0x0022C55E, 0);
    draw_app_icon(322, 84, "Mail",      0x000EA5E9, 0);

    draw_app_icon(40,  174, "Music",    0x00E11D48, 0);
    draw_app_icon(112, 174, "Photos",   0x00EC4899, 0);
    draw_app_icon(184, 174, "Notes",    0x00EAB308, 0);
    draw_app_icon(256, 174, "Calendar", 0x00DC2626, 0);
    draw_app_icon(322, 174, "Maps",     0x000D9488, 0);

    draw_app_icon(40,  264, "Terminal", 0x000F172A, 0);
    draw_app_icon(112, 264, "Activity", 0x00059669, 1);
    draw_app_icon(184, 264, "Settings", 0x00475569, 0);
    draw_app_icon(256, 264, "Books",    0x00EA580C, 0);
    draw_app_icon(322, 264, "Calc",     0x00334155, 0);

    draw_text(82, 372, "100.0 MB RAM | 93.6 MB Free", 0x00475569);

    draw_task_manager(412, 145, 370, 335, tick);

    draw_glass_panel(412, 40, 370, 92, 185, 0x00E0F2FE);
    draw_text_bold(426, 52, "CONTROL CENTER & PMM (100 MB)", 0x000F172A);
    draw_text(426, 74, "Total Pages: 25,600 (4KB/page)", 0x001E293B);
    draw_text(426, 92, "Wi-Fi: Active  |  Tween Engine: 60Hz", 0x00047857);
    draw_text(426, 110,"GPU/LFB: Double-Buffered 800x600", 0x000369A1);

    draw_glass_panel(160, SCREEN_H - 64, 480, 54, 190, 0x00FFFFFF);
    uint32_t dock_colors[8] = {
        0x000284C7, 0x003B82F6, 0x0022C55E, 0x000EA5E9,
        0x00E11D48, 0x00EAB308, 0x00059669, 0x00475569
    };
    for (int i = 0; i < 8; i++) {
        int dx = 178 + (i * 57);
        int dy = SCREEN_H - 56 - ((i == 6) ? 4 : 0);
        draw_glass_panel(dx, dy, 38, 38, 235, dock_colors[i]);
        draw_glass_rect(dx + 2, dy + 2, 34, 15, 95, 0x00FFFFFF);
        fill_rect(dx + 12, dy + 12, 14, 14, 0x00FFFFFF);
    }

    draw_cursor(505, 452);
    swap_buffers();
}

void kernel_main(void) {
    pmm_init();
    void* test_page = pmm_alloc_page();
    if (test_page != NULL) {
        pmm_free_page(test_page);
    }

    for (int frame = 0; frame <= 12; frame++) {
        render_desktop(frame);
    }

    while (1) {
        #if defined(__aarch64__)
        __asm__ volatile("wfi");
#else
        __asm__ volatile("hlt");
#endif
    }
}
