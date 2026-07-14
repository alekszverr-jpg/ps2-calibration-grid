#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define APP_VERSION "1.2.2"
#define PATTERN_COUNT 13
#define VIDEO_MODE_COUNT 4
#define LOGICAL_WIDTH 640
#define LOGICAL_HEIGHT 480
#define MODE_CONFIRM_SECONDS 10

typedef struct {
    const char *name;
    short mode;
    short interlace;
    short field;
    int width;
    int height;
    int refresh_hz;
} VideoMode;

static const VideoMode video_modes[VIDEO_MODE_COUNT] = {
    {"PAL 576I",  GS_MODE_PAL,  GS_INTERLACED,    GS_FIELD, 640, 512, 50},
    {"PAL 288P",  GS_MODE_PAL,  GS_NONINTERLACED, GS_FRAME, 640, 256, 50},
    {"NTSC 480I", GS_MODE_NTSC, GS_INTERLACED,    GS_FIELD, 640, 448, 60},
    {"NTSC 240P", GS_MODE_NTSC, GS_NONINTERLACED, GS_FRAME, 640, 224, 60}
};

static GSGLOBAL *gs;
static unsigned char pad_buffer[256] __attribute__((aligned(64)));
static unsigned int old_buttons;
static int pattern;
static int show_help = 1;
static int menu_open;
static int menu_selection;
static int video_mode_index;
static int previous_video_mode;
static int mode_confirm_frames;

static const char *pattern_names[PATTERN_COUNT] = {
    "GEOMETRY", "OVERSCAN", "PLUGE", "COLOR BARS", "CHECKER 16",
    "CONVERGENCE", "SHARPNESS", "COLOR BLEED", "FULL WHITE",
    "FULL GRAY", "FULL RED", "FULL GREEN", "FULL BLUE"
};

static const VideoMode *current_video_mode(void)
{
    return &video_modes[video_mode_index];
}

static float screen_y(float logical_y)
{
    return logical_y * (float)gs->Height / (float)LOGICAL_HEIGHT;
}

static u64 rgb(unsigned int r, unsigned int g, unsigned int b)
{
    return GS_SETREG_RGBAQ(r, g, b, 0x80, 0x00);
}

static void rect(float x1, float y1, float x2, float y2, u64 color)
{
    gsKit_prim_sprite(gs, x1, screen_y(y1), x2, screen_y(y2), 1, color);
}

/* Raw GS coordinates for patterns that must address exact output pixels. */
static void pixel_rect(float x1, float y1, float x2, float y2, u64 color)
{
    gsKit_prim_sprite(gs, x1, y1, x2, y2, 1, color);
}

static void line(float x1, float y1, float x2, float y2, u64 color)
{
    gsKit_prim_line(gs, x1, screen_y(y1), x2, screen_y(y2), 2, color);
}

static void outline(float x1, float y1, float x2, float y2, u64 color)
{
    line(x1, y1, x2, y1, color);
    line(x2, y1, x2, y2, color);
    line(x2, y2, x1, y2, color);
    line(x1, y2, x1, y1, color);
}

static void circle(float cx, float cy, float rx, float ry, u64 color)
{
    int i;
    float px = cx + rx;
    float py = cy;
    for (i = 1; i <= 96; ++i) {
        const float a = (float)i * 6.28318530718f / 96.0f;
        const float x = cx + cosf(a) * rx;
        const float y = cy + sinf(a) * ry;
        line(px, py, x, y, color);
        px = x;
        py = y;
    }
}

/* Compact 5x7 font. Each byte is one row, most significant five bits used. */
static const unsigned char *glyph(char c)
{
    static const unsigned char blank[7] = {0,0,0,0,0,0,0};
    static const unsigned char digits[10][7] = {
        {0x70,0x88,0x98,0xA8,0xC8,0x88,0x70}, {0x20,0x60,0x20,0x20,0x20,0x20,0x70},
        {0x70,0x88,0x08,0x10,0x20,0x40,0xF8}, {0xF0,0x08,0x08,0x70,0x08,0x08,0xF0},
        {0x10,0x30,0x50,0x90,0xF8,0x10,0x10}, {0xF8,0x80,0x80,0xF0,0x08,0x08,0xF0},
        {0x30,0x40,0x80,0xF0,0x88,0x88,0x70}, {0xF8,0x08,0x10,0x20,0x40,0x40,0x40},
        {0x70,0x88,0x88,0x70,0x88,0x88,0x70}, {0x70,0x88,0x88,0x78,0x08,0x10,0x60}
    };
    static const unsigned char letters[26][7] = {
        {0x70,0x88,0x88,0xF8,0x88,0x88,0x88}, {0xF0,0x88,0x88,0xF0,0x88,0x88,0xF0},
        {0x70,0x88,0x80,0x80,0x80,0x88,0x70}, {0xE0,0x90,0x88,0x88,0x88,0x90,0xE0},
        {0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8}, {0xF8,0x80,0x80,0xF0,0x80,0x80,0x80},
        {0x70,0x88,0x80,0xB8,0x88,0x88,0x78}, {0x88,0x88,0x88,0xF8,0x88,0x88,0x88},
        {0x70,0x20,0x20,0x20,0x20,0x20,0x70}, {0x38,0x10,0x10,0x10,0x90,0x90,0x60},
        {0x88,0x90,0xA0,0xC0,0xA0,0x90,0x88}, {0x80,0x80,0x80,0x80,0x80,0x80,0xF8},
        {0x88,0xD8,0xA8,0xA8,0x88,0x88,0x88}, {0x88,0xC8,0xA8,0x98,0x88,0x88,0x88},
        {0x70,0x88,0x88,0x88,0x88,0x88,0x70}, {0xF0,0x88,0x88,0xF0,0x80,0x80,0x80},
        {0x70,0x88,0x88,0x88,0xA8,0x90,0x68}, {0xF0,0x88,0x88,0xF0,0xA0,0x90,0x88},
        {0x78,0x80,0x80,0x70,0x08,0x08,0xF0}, {0xF8,0x20,0x20,0x20,0x20,0x20,0x20},
        {0x88,0x88,0x88,0x88,0x88,0x88,0x70}, {0x88,0x88,0x88,0x88,0x88,0x50,0x20},
        {0x88,0x88,0x88,0xA8,0xA8,0xD8,0x88}, {0x88,0x88,0x50,0x20,0x50,0x88,0x88},
        {0x88,0x88,0x50,0x20,0x20,0x20,0x20}, {0xF8,0x08,0x10,0x20,0x40,0x80,0xF8}
    };
    static const unsigned char dash[7] = {0,0,0,0x70,0,0,0};
    static const unsigned char slash[7] = {0x08,0x08,0x10,0x20,0x40,0x80,0x80};
    static const unsigned char plus[7] = {0,0x20,0x20,0xF8,0x20,0x20,0};
    static const unsigned char dot[7] = {0,0,0,0,0,0x20,0x20};
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c == '-') return dash;
    if (c == '/') return slash;
    if (c == '+') return plus;
    if (c == '.') return dot;
    return blank;
}

static void text(float x, float y, const char *s, int scale, u64 color)
{
    for (; *s; ++s, x += 6 * scale) {
        const unsigned char *g = glyph(*s);
        int yy, xx;
        for (yy = 0; yy < 7; ++yy)
            for (xx = 0; xx < 5; ++xx)
                if (g[yy] & (0x80 >> xx))
                    rect(x + xx * scale, y + yy * scale,
                         x + (xx + 1) * scale, y + (yy + 1) * scale, color);
    }
}

static void draw_grid(int w, int h)
{
    const u64 minor = rgb(55, 55, 55);
    const u64 major = rgb(180, 180, 180);
    const u64 center = rgb(255, 55, 55);
    int x, y;
    for (x = 0; x <= w; x += 32) line(x, 0, x, h - 1, (x % 128) ? minor : major);
    for (y = 0; y <= h; y += 32) line(0, y, w - 1, y, (y % 128) ? minor : major);
    outline(1, 1, w - 2, h - 2, rgb(255,255,255));
    line(w / 2, 0, w / 2, h - 1, center);
    line(0, h / 2, w - 1, h / 2, center);
    circle(w / 2, h / 2, h * 0.36f, h * 0.36f, rgb(255,255,255));
    circle(w / 2, h / 2, h * 0.18f, h * 0.18f, major);
}

static void draw_overscan(int w, int h)
{
    const int pct[] = {100, 95, 90, 85, 80};
    const u64 colors[] = {0,0,0,0,0};
    int i;
    (void)colors;
    for (i = 0; i < 5; ++i) {
        const float dx = w * (100 - pct[i]) / 200.0f;
        const float dy = h * (100 - pct[i]) / 200.0f;
        u64 c = (i == 0) ? rgb(255,255,255) : (i == 1) ? rgb(255,70,70) :
                (i == 2) ? rgb(70,255,70) : (i == 3) ? rgb(70,120,255) : rgb(255,220,50);
        outline(dx + 1, dy + 1, w - dx - 2, h - dy - 2, c);
        if (i) {
            char label[4] = {(char)('0' + pct[i] / 10), (char)('0' + pct[i] % 10), 0, 0};
            text(dx + 6, dy + 6, label, 1, c);
        }
    }
    line(w / 2, 0, w / 2, h, rgb(100,100,100));
    line(0, h / 2, w, h / 2, rgb(100,100,100));
}

static void draw_pluge(int w, int h)
{
    const int levels[12] = {0, 8, 16, 24, 32, 48, 64, 96, 128, 160, 192, 255};
    int i;
    float bw = (float)w / 12.0f;
    for (i = 0; i < 12; ++i)
        rect(i * bw, 0, (i + 1) * bw + 1, h * 0.70f, rgb(levels[i],levels[i],levels[i]));
    rect(0, h * 0.70f, w, h, rgb(16,16,16));
    rect(w * 0.18f, h * 0.76f, w * 0.34f, h * 0.94f, rgb(0,0,0));
    rect(w * 0.42f, h * 0.76f, w * 0.58f, h * 0.94f, rgb(8,8,8));
    rect(w * 0.66f, h * 0.76f, w * 0.82f, h * 0.94f, rgb(24,24,24));
}

static void draw_color_bars(int w, int h)
{
    const int c[8][3] = {
        {191,191,191},{191,191,0},{0,191,191},{0,191,0},
        {191,0,191},{191,0,0},{0,0,191},{0,0,0}
    };
    int i;
    for (i = 0; i < 8; ++i)
        rect((float)i*w/8, 0, (float)(i+1)*w/8 + 1, h*0.72f, rgb(c[i][0],c[i][1],c[i][2]));
    rect(0, h*0.72f, w/4, h, rgb(0,0,191));
    rect(w/4, h*0.72f, w/2, h, rgb(16,16,16));
    rect(w/2, h*0.72f, 3*w/4, h, rgb(235,235,235));
    rect(3*w/4, h*0.72f, w, h, rgb(191,0,0));
}

static void draw_checker(int w, int h)
{
    int x, y;
    const int size = 16;
    for (y = 0; y < h; y += size)
        for (x = 0; x < w; x += size)
            pixel_rect(x, y, x + size, y + size,
                       ((x / size + y / size) & 1) ? rgb(255,255,255) : rgb(0,0,0));
}

static void draw_convergence(int w, int h)
{
    int x, y;
    const u64 white = rgb(255,255,255);
    const u64 gray = rgb(72,72,72);
    for (x = 40; x < w; x += 40)
        line(x, 0, x, h, (x % 160) ? gray : white);
    for (y = 40; y < h; y += 40)
        line(0, y, w, y, (y % 160) ? gray : white);
    for (y = 40; y < h; y += 40) {
        for (x = 40; x < w; x += 40) {
            rect(x - 4, y - 1, x + 5, y + 2, white);
            rect(x - 1, y - 4, x + 2, y + 5, white);
        }
    }
    outline(1, 1, w - 2, h - 2, white);
}

static void draw_sharpness(int w, int h)
{
    int x, y;
    const int half_w = w / 2;
    const int half_h = h / 2;
    pixel_rect(0, 0, w, h, rgb(0,0,0));

    /* One- and two-pixel vertical stripes. */
    for (x = 0; x < half_w; ++x)
        if (x & 1) pixel_rect(x, 0, x + 1, half_h, rgb(255,255,255));
    for (x = half_w; x < w; x += 4)
        pixel_rect(x, 0, x + 2, half_h, rgb(255,255,255));

    /* One- and two-pixel horizontal stripes. */
    for (y = half_h; y < h; ++y)
        if (y & 1) pixel_rect(0, y, half_w, y + 1, rgb(255,255,255));
    for (y = half_h; y < h; y += 4)
        pixel_rect(half_w, y, w, y + 2, rgb(255,255,255));

    pixel_rect(half_w - 1, 0, half_w + 1, h, rgb(128,128,128));
    pixel_rect(0, half_h - 1, w, half_h + 1, rgb(128,128,128));
}

static void draw_color_bleed(int w, int h)
{
    u64 test_colors[6];
    int i;
    const float top = h * 0.10f;
    const float bottom = h * 0.90f;
    const float band = (float)w / 6.0f;
    test_colors[0] = rgb(255,0,0);
    test_colors[1] = rgb(0,255,0);
    test_colors[2] = rgb(0,0,255);
    test_colors[3] = rgb(0,255,255);
    test_colors[4] = rgb(255,0,255);
    test_colors[5] = rgb(255,255,0);
    rect(0, 0, w, h, rgb(32,32,32));
    for (i = 0; i < 6; ++i) {
        const float x1 = i * band;
        const float x2 = (i + 1) * band;
        rect(x1, top, x2 + 1, bottom, test_colors[i]);
        rect(x1 + band * 0.46f, top, x1 + band * 0.54f, bottom, rgb(255,255,255));
        rect(x1 + band * 0.485f, top, x1 + band * 0.515f, bottom, rgb(0,0,0));
    }
    line(0, h / 2, w, h / 2, rgb(255,255,255));
}

static void draw_full_field(int w, int h, u64 color)
{
    rect(0, 0, w, h, color);
}

static const char *pattern_name(void)
{
    return pattern_names[pattern];
}

static void draw_pattern_menu(int ui_scale)
{
    int i;
    const int x1 = 104;
    const int x2 = 536;
    const int first_y = 82;
    const int row_h = 24;
    rect(x1, 28, x2, 452, rgb(0,0,0));
    outline(x1, 28, x2, 452, rgb(255,220,50));
    text(174, 42, "PS2 CRT SUITE V" APP_VERSION, ui_scale, rgb(255,220,50));
    for (i = 0; i < PATTERN_COUNT; ++i) {
        const int y = first_y + i * row_h;
        if (i == menu_selection) {
            rect(x1 + 12, y - 5, x2 - 12, y + 17, rgb(55,45,0));
            outline(x1 + 12, y - 5, x2 - 12, y + 17, rgb(150,125,20));
        }
        text(x1 + 28, y, pattern_names[i], ui_scale,
             (i == menu_selection) ? rgb(255,255,255) : rgb(150,150,150));
    }
    text(152, 416, "UP/DOWN SELECT  CROSS OPEN", ui_scale, rgb(190,190,190));
    text(194, 434, "CIRCLE BACK", ui_scale, rgb(150,150,150));
}

static void draw_frame(void)
{
    const int w = LOGICAL_WIDTH;
    const int h = LOGICAL_HEIGHT;
    const int ui_scale = (current_video_mode()->interlace == GS_NONINTERLACED) ? 2 : 1;
    gsKit_clear(gs, rgb(0,0,0));
    switch (pattern) {
        case 0: draw_grid(w, h); break;
        case 1: draw_overscan(w, h); break;
        case 2: draw_pluge(w, h); break;
        case 3: draw_color_bars(w, h); break;
        case 4: draw_checker(gs->Width, gs->Height); break;
        case 5: draw_convergence(w, h); break;
        case 6: draw_sharpness(gs->Width, gs->Height); break;
        case 7: draw_color_bleed(w, h); break;
        case 8: draw_full_field(w, h, rgb(255,255,255)); break;
        case 9: draw_full_field(w, h, rgb(128,128,128)); break;
        case 10: draw_full_field(w, h, rgb(255,0,0)); break;
        case 11: draw_full_field(w, h, rgb(0,255,0)); break;
        default: draw_full_field(w, h, rgb(0,0,255)); break;
    }
    if (show_help) {
        rect(8, h - 60, w - 8, h - 8, rgb(0,0,0));
        outline(8, h - 60, w - 8, h - 8, rgb(90,90,90));
        text(16, h - 53, pattern_name(), ui_scale, rgb(255,255,255));
        text(w - 126, h - 53, current_video_mode()->name, ui_scale, rgb(255,220,50));
        text(16, h - 35, "LEFT/RIGHT PATTERN  SQUARE MENU", ui_scale, rgb(180,180,180));
        text(16, h - 19, "TRIANGLE MODE  SELECT HELP  START+SELECT EXIT", ui_scale, rgb(180,180,180));
    }
    if (menu_open)
        draw_pattern_menu(ui_scale);
    if (mode_confirm_frames > 0) {
        int seconds = (mode_confirm_frames + current_video_mode()->refresh_hz - 1) /
                      current_video_mode()->refresh_hz;
        char count[3];
        if (seconds >= 10) {
            count[0] = '1'; count[1] = '0'; count[2] = 0;
        } else {
            count[0] = (char)('0' + seconds); count[1] = 0; count[2] = 0;
        }
        rect(128, 176, 512, 304, rgb(0,0,0));
        outline(128, 176, 512, 304, rgb(255,220,50));
        text(190, 194, current_video_mode()->name, 2, rgb(255,220,50));
        text(170, 224, "CROSS KEEP  CIRCLE BACK", 2, rgb(255,255,255));
        text(244, 258, "AUTO BACK", 2, rgb(180,180,180));
        text(364, 258, count, 2, rgb(255,80,80));
    }
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
}

static void init_video(void)
{
    const VideoMode *mode = current_video_mode();
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gs = gsKit_init_global();
    gs->Mode = mode->mode;
    gs->Width = mode->width;
    gs->Height = mode->height;
    gs->Interlace = mode->interlace;
    gs->Field = mode->field;
    gs->PSM = GS_PSM_CT24;
    gs->PSMZ = GS_PSMZ_16S;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_OFF;
    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);
    gsKit_set_test(gs, GS_ZTEST_OFF);
}

static void apply_video_mode(int index)
{
    gsKit_deinit_global(gs);
    video_mode_index = index;
    init_video();
}

static void preview_next_video_mode(void)
{
    if (mode_confirm_frames <= 0)
        previous_video_mode = video_mode_index;
    apply_video_mode((video_mode_index + 1) % VIDEO_MODE_COUNT);
    mode_confirm_frames = current_video_mode()->refresh_hz * MODE_CONFIRM_SECONDS;
}

static void cancel_video_preview(void)
{
    mode_confirm_frames = 0;
    apply_video_mode(previous_video_mode);
}

static unsigned int read_buttons(void)
{
    struct padButtonStatus status;
    int state = padGetState(0, 0);
    if ((state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) && padRead(0, 0, &status))
        return 0xffffu ^ status.btns;
    return 0;
}

static void init_pad(void)
{
    SifInitRpc(0);
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
    padInit(0);
    padPortOpen(0, 0, pad_buffer);
}

static void exit_app(void)
{
    padPortClose(0, 0);
    padEnd();
    gsKit_deinit_global(gs);
    SifExitRpc();
    Exit(0);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    video_mode_index = (gsKit_detect_signal() == GS_MODE_PAL) ? 0 : 2;
    previous_video_mode = video_mode_index;
    init_pad();
    init_video();
    for (;;) {
        unsigned int now = read_buttons();
        unsigned int pressed = now & ~old_buttons;
        if ((now & (PAD_START | PAD_SELECT)) == (PAD_START | PAD_SELECT))
            exit_app();
        if (mode_confirm_frames > 0) {
            if (pressed & PAD_CROSS)
                mode_confirm_frames = 0;
            else if (pressed & PAD_CIRCLE)
                cancel_video_preview();
            else if (pressed & PAD_TRIANGLE)
                preview_next_video_mode();
        } else if (menu_open) {
            if (pressed & PAD_UP)
                menu_selection = (menu_selection + PATTERN_COUNT - 1) % PATTERN_COUNT;
            if (pressed & PAD_DOWN)
                menu_selection = (menu_selection + 1) % PATTERN_COUNT;
            if (pressed & PAD_CROSS) {
                pattern = menu_selection;
                menu_open = 0;
            } else if (pressed & (PAD_CIRCLE | PAD_SQUARE)) {
                menu_open = 0;
            }
        } else {
            if (pressed & (PAD_RIGHT | PAD_CROSS)) pattern = (pattern + 1) % PATTERN_COUNT;
            if (pressed & PAD_LEFT) pattern = (pattern + PATTERN_COUNT - 1) % PATTERN_COUNT;
            if (pressed & PAD_TRIANGLE) preview_next_video_mode();
            if (pressed & PAD_SQUARE) {
                menu_selection = pattern;
                menu_open = 1;
            }
            if (pressed & PAD_SELECT) show_help = !show_help;
        }
        old_buttons = now;
        draw_frame();
        if (mode_confirm_frames > 0 && --mode_confirm_frames == 0)
            apply_video_mode(previous_video_mode);
    }
    return 0;
}
