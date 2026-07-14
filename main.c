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

#define APP_VERSION "1.0"
#define PATTERN_COUNT 5

static GSGLOBAL *gs;
static unsigned char pad_buffer[256] __attribute__((aligned(64)));
static unsigned int old_buttons;
static int pattern;
static int pal_mode = 1;
static int show_help = 1;

static u64 rgb(unsigned int r, unsigned int g, unsigned int b)
{
    return GS_SETREG_RGBAQ(r, g, b, 0x80, 0x00);
}

static void rect(float x1, float y1, float x2, float y2, u64 color)
{
    gsKit_prim_sprite(gs, x1, y1, x2, y2, 1, color);
}

static void line(float x1, float y1, float x2, float y2, u64 color)
{
    gsKit_prim_line(gs, x1, y1, x2, y2, 2, color);
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
    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c == '-') return dash;
    if (c == '/') return slash;
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
            rect(x, y, x + size, y + size,
                 ((x / size + y / size) & 1) ? rgb(255,255,255) : rgb(0,0,0));
}

static const char *pattern_name(void)
{
    static const char *names[PATTERN_COUNT] = {"GRID", "OVERSCAN", "PLUGE", "COLOR BARS", "CHECKER"};
    return names[pattern];
}

static void draw_frame(void)
{
    const int w = gs->Width;
    const int h = gs->Height;
    gsKit_clear(gs, rgb(0,0,0));
    switch (pattern) {
        case 0: draw_grid(w, h); break;
        case 1: draw_overscan(w, h); break;
        case 2: draw_pluge(w, h); break;
        case 3: draw_color_bars(w, h); break;
        default: draw_checker(w, h); break;
    }
    if (show_help) {
        rect(8, h - 42, w - 8, h - 8, rgb(0,0,0));
        outline(8, h - 42, w - 8, h - 8, rgb(90,90,90));
        text(16, h - 35, pattern_name(), 1, rgb(255,255,255));
        text(w - 92, h - 35, pal_mode ? "PAL 50" : "NTSC 60", 1, rgb(255,220,50));
        text(16, h - 20, "LEFT/RIGHT PATTERN  TRIANGLE MODE  SELECT HELP", 1, rgb(180,180,180));
    }
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
}

static void init_video(void)
{
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gs = gsKit_init_global();
    gs->Mode = pal_mode ? GS_MODE_PAL : GS_MODE_NTSC;
    gs->Width = 640;
    gs->Height = pal_mode ? 512 : 448;
    gs->Interlace = GS_INTERLACED;
    gs->Field = GS_FIELD;
    gs->PSM = GS_PSM_CT24;
    gs->PSMZ = GS_PSMZ_16S;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_OFF;
    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);
    gsKit_set_test(gs, GS_ZTEST_OFF);
}

static void switch_video(void)
{
    gsKit_deinit_global(gs);
    pal_mode = !pal_mode;
    init_video();
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

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    init_pad();
    init_video();
    for (;;) {
        unsigned int now = read_buttons();
        unsigned int pressed = now & ~old_buttons;
        if (pressed & (PAD_RIGHT | PAD_CROSS)) pattern = (pattern + 1) % PATTERN_COUNT;
        if (pressed & PAD_LEFT) pattern = (pattern + PATTERN_COUNT - 1) % PATTERN_COUNT;
        if (pressed & PAD_TRIANGLE) switch_video();
        if (pressed & PAD_SELECT) show_help = !show_help;
        old_buttons = now;
        draw_frame();
    }
    return 0;
}
