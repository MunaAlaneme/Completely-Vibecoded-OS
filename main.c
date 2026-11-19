#include <stdint.h>
#include <stddef.h>

#define SCREEN_W 640
#define SCREEN_H 480

#define WIN_TEAL       0x00008080
#define WIN_GRAY       0x00C0C0C0
#define WIN_LIGHT_GRAY 0x00DFDFDF
#define WIN_DARK_GRAY  0x00808080
#define WIN_WHITE      0x00FFFFFF
#define WIN_BLACK      0x00000000
#define WIN_NAVY       0x00000080
#define WIN_BLUE       0x000000FF

#if defined(__aarch64__)
static volatile uint32_t* framebuffer = (volatile uint32_t*)0x44000000;
#else
static volatile uint32_t* framebuffer = (volatile uint32_t*)0xE0000000;
#endif

/* Mouse and Window State */
static int mouse_x = 312;
static int mouse_y = 232;
static int start_menu_open = 0;
static int explorer_window_open = 1; /* My Computer / Files window */

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        framebuffer[y * SCREEN_W + x] = color;
    }
}

void fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            put_pixel(col, row, color);
        }
    }
}

void draw_hline(int x, int y, int len, uint32_t color) { fill_rect(x, y, len, 1, color); }
void draw_vline(int x, int y, int len, uint32_t color) { fill_rect(x, y, 1, len, color); }

/* Simple 8x8 font rendering stub */
void draw_char(int x, int y, char c, uint32_t color) {
    /* Minimal placeholder for character drawing */
    fill_rect(x, y, 6, 8, color);
}

void draw_text(int x, int y, const char* str, uint32_t color) {
    int cx = x;
    while (*str) {
        if (*str != ' ') draw_char(cx, y, *str, color);
        cx += 8;
        str++;
    }
}

void draw_bevel_box(int x, int y, int w, int h, int sunken, int fill) {
    uint32_t tl_outer = sunken ? WIN_DARK_GRAY : WIN_WHITE;
    uint32_t tl_inner = sunken ? WIN_BLACK     : WIN_LIGHT_GRAY;
    uint32_t br_outer = sunken ? WIN_WHITE     : WIN_BLACK;
    uint32_t br_inner = sunken ? WIN_LIGHT_GRAY: WIN_DARK_GRAY;

    if (fill) fill_rect(x, y, w, h, WIN_GRAY);
    draw_hline(x, y, w, tl_outer);
    draw_vline(x, y, h, tl_outer);
    draw_hline(x + 1, y + 1, w - 2, tl_inner);
    draw_vline(x + 1, y + 1, h - 2, tl_inner);
    draw_hline(x, y + h - 1, w, br_outer);
    draw_vline(x + w - 1, y, h, br_outer);
    draw_hline(x + 1, y + h - 2, w - 2, br_inner);
    draw_vline(x + w - 2, y + 1, h - 2, br_inner);
}

void draw_window(int x, int y, int w, int h, const char* title, int active) {
    draw_bevel_box(x, y, w, h, 0, 1);
    fill_rect(x + 3, y + 3, w - 6, 18, active ? WIN_NAVY : WIN_DARK_GRAY);
    draw_text(x + 7, y + 8, title, WIN_WHITE);

    /* Close Button [X] */
    int bx = x + w - 20;
    draw_bevel_box(bx, y + 5, 14, 14, 0, 1);
    draw_text(bx + 3, y + 7, "x", WIN_BLACK);

    /* Sunken Client Workspace */
    draw_bevel_box(x + 5, y + 24, w - 10, h - 30, 1, 0);
    fill_rect(x + 7, y + 26, w - 14, h - 34, WIN_WHITE);
}

void render_desktop(void) {
    /* 1. Teal Wallpaper */
    fill_rect(0, 0, SCREEN_W, SCREEN_H - 28, WIN_TEAL);

    /* 2. Desktop Icons */
    draw_bevel_box(16, 16, 48, 48, 0, 0);
    draw_text(18, 70, "My Computer", WIN_WHITE);

    draw_bevel_box(16, 96, 48, 48, 0, 0);
    draw_text(22, 150, "Files", WIN_WHITE);

    /* 3. Interactive File Manager Window (My Computer / C:\) */
    if (explorer_window_open) {
        draw_window(100, 50, 420, 260, "My Computer - C:\\", 1);
        
        /* Toolbar */
        draw_text(112, 82, "File  Edit  View  Help", WIN_BLACK);
        draw_hline(108, 96, 404, WIN_DARK_GRAY);

        /* Drive / File Grid inside window */
        draw_bevel_box(114, 106, 60, 60, 0, 1);
        draw_text(120, 170, "3.5 Floppy", WIN_BLACK);

        draw_bevel_box(190, 106, 60, 60, 0, 1);
        draw_text(202, 170, "Drive C", WIN_BLACK);

        draw_bevel_box(266, 106, 60, 60, 0, 1);
        draw_text(276, 170, "Control", WIN_BLACK);

        draw_text(114, 195, "3 object(s) selected", WIN_BLACK);
    }

    /* 4. Bottom Taskbar */
    draw_bevel_box(0, SCREEN_H - 28, SCREEN_W, 28, 0, 1);
    draw_bevel_box(3, SCREEN_H - 25, 56, 22, start_menu_open, 1);
    draw_text(18, SCREEN_H - 18, "Start", WIN_BLACK);

    /* 5. Start Menu Popup */
    if (start_menu_open) {
        draw_bevel_box(2, SCREEN_H - 28 - 150, 150, 150, 0, 1);
        fill_rect(5, SCREEN_H - 25 - 150, 20, 144, WIN_NAVY);
        draw_text(12, SCREEN_H - 45, "95", WIN_WHITE);
        
        draw_text(32, SCREEN_H - 165, "Programs", WIN_BLACK);
        draw_text(32, SCREEN_H - 145, "Documents", WIN_BLACK);
        draw_text(32, SCREEN_H - 125, "Settings", WIN_BLACK);
        draw_text(32, SCREEN_H - 105, "Shut Down...", WIN_BLACK);
    }

    /* 6. Mouse Cursor */
    fill_rect(mouse_x, mouse_y, 4, 10, WIN_BLACK);
    fill_rect(mouse_x + 1, mouse_y + 1, 2, 8, WIN_WHITE);
}

void kernel_main(void) {
    render_desktop();
    while (1) {
        /* Main event loop / HALT */
#if defined(__aarch64__)
        __asm__ volatile("wfi");
#else
        __asm__ volatile("hlt");
#endif
    }
}