#include "kdr-running-text.h"
#include <obs-module.h>
#include <graphics/graphics.h>
#include <util/platform.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstring>

using namespace Gdiplus;

struct kdr_running_text_data {
    obs_source_t *source = nullptr;
    char *text = nullptr;
    char *theme = nullptr;
    int width = 1920, height = 80, speed = 30, font_size = 42;
    bool reverse = false, shadow = true;
    uint32_t text_color = 0xFFFFFFFF, outline_color = 0x000000FF;
    int outline_size = 2, shadow_offset = 3;
    char *logo_path = nullptr;
    int logo_size = 56, logo_gap = 18;
    gs_texture_t *texture = nullptr;
    uint32_t texture_width = 2, texture_height = 2;
    float text_width = 2.0f;
    float offset = 0.0f;
};

static ULONG_PTR gdip_token = 0;

static const char *kdr_get_name(void *) { return "KDR Running Text"; }

static std::wstring utf8_to_wide(const char *s)
{
    if (!s || !*s) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (n <= 0) return L"";
    std::wstring out((size_t)n - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), n);
    return out;
}

static Color to_gdip(uint32_t c)
{
    return Color((BYTE)((c >> 24) & 0xff), (BYTE)((c >> 16) & 0xff), (BYTE)((c >> 8) & 0xff), (BYTE)(c & 0xff));
}

struct theme_style {
    Color text;
    Color outline;
    int font_style = FontStyleRegular;
    int outline_size = 2;
    bool shadow = true;
};

static theme_style get_theme_style(const kdr_running_text_data *d)
{
    theme_style s{to_gdip(d->text_color), to_gdip(d->outline_color), FontStyleRegular, d->outline_size, d->shadow};
    const char *theme = d->theme ? d->theme : "modern";

    if (strcmp(theme, "news") == 0) {
        s.text = Color(255, 255, 255, 255);
        s.outline = Color(255, 210, 30, 30);
        s.outline_size = (std::max)(3, d->outline_size);
        s.shadow = true;
    } else if (strcmp(theme, "sports") == 0) {
        s.text = Color(255, 255, 255, 255);
        s.outline = Color(255, 20, 80, 210);
        s.font_style = FontStyleBold;
        s.outline_size = (std::max)(2, d->outline_size);
        s.shadow = true;
    } else if (strcmp(theme, "minimal") == 0) {
        s.text = to_gdip(d->text_color);
        s.outline = Color(0, 0, 0, 0);
        s.outline_size = 0;
        s.shadow = false;
    } else if (strcmp(theme, "kdr") == 0) {
        s.text = Color(255, 255, 255, 255);
        s.outline = Color(255, 0, 170, 255);
        s.font_style = FontStyleBold;
        s.outline_size = (std::max)(2, d->outline_size);
        s.shadow = true;
    }
    return s;
}

static void destroy_texture(kdr_running_text_data *d)
{
    if (d->texture) {
        obs_enter_graphics();
        gs_texture_destroy(d->texture);
        d->texture = nullptr;
        obs_leave_graphics();
    }
}

static void render_text(kdr_running_text_data *d)
{
    std::wstring text = utf8_to_wide(d->text);
    if (text.empty()) text = L" ";

    FontFamily family(L"Arial");
    const theme_style style = get_theme_style(d);
    Font font(&family, (REAL)d->font_size, style.font_style, UnitPixel);
    Bitmap measure(8, 8, PixelFormat32bppARGB);
    Graphics mg(&measure);
    RectF box(0, 0, 16000, (REAL)d->height);
    RectF measured;
    StringFormat fmt;
    mg.MeasureString(text.c_str(), -1, &font, box, &measured);

    const int pad = (std::max)(8, d->outline_size + d->shadow_offset + 4);
    const bool has_logo = d->logo_path && *d->logo_path;
    std::wstring logo_file = has_logo ? utf8_to_wide(d->logo_path) : L"";
    Bitmap *logo = nullptr;
    UINT logo_w = 0, logo_h = 0;
    if (has_logo && !logo_file.empty()) {
        logo = new Bitmap(logo_file.c_str());
        if (logo->GetLastStatus() == Ok) {
            logo_w = logo->GetWidth(); logo_h = logo->GetHeight();
        } else {
            delete logo; logo = nullptr;
        }
    }
    const int logo_extra = logo ? d->logo_size + d->logo_gap : 0;
    const int w = (std::max)(2, (int)std::ceil(measured.Width) + pad * 2 + logo_extra);
    const int h = (std::max)(2, d->height);
    Bitmap bitmap(w, h, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.SetCompositingMode(CompositingModeSourceOver);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.Clear(Color(0, 0, 0, 0));

    RectF draw((REAL)(pad + logo_extra), 0, (REAL)(w - pad * 2 - logo_extra), (REAL)h);
    SolidBrush text_brush(style.text);
    SolidBrush outline_brush(style.outline);

    if (style.shadow) {
        SolidBrush shadow(Color(150, 0, 0, 0));
        RectF shadow_box = draw;
        shadow_box.X += (REAL)d->shadow_offset;
        shadow_box.Y += (REAL)d->shadow_offset;
        g.DrawString(text.c_str(), -1, &font, shadow_box, &fmt, &shadow);
    }

    if (style.outline_size > 0) {
        GraphicsPath path;
        path.AddString(text.c_str(), -1, &family, FontStyleRegular, (REAL)d->font_size, draw, &fmt);
        Pen pen(style.outline, (REAL)style.outline_size * 2.0f);
        pen.SetLineJoin(LineJoinRound);
        g.DrawPath(&pen, &path);
    }
    g.DrawString(text.c_str(), -1, &font, draw, &fmt, &text_brush);

    if (logo) {
        const REAL target_w = (REAL)d->logo_size;
        const REAL target_h = (REAL)d->logo_size;
        const REAL scale = (REAL)(logo_w ? logo_w : 1) / (REAL)(logo_h ? logo_h : 1);
        REAL draw_w = target_w, draw_h = target_h;
        if (scale > 1.0f) draw_h = target_w / scale;
        else draw_w = target_h * scale;
        RectF logo_rect((REAL)pad, ((REAL)h - draw_h) * 0.5f, draw_w, draw_h);
        g.DrawImage(logo, logo_rect);
        delete logo; logo = nullptr;
    }

    BitmapData bd;
    Rect rect(0, 0, w, h);
    if (bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bd) != Ok) return;

    std::vector<uint8_t> pixels((size_t)w * h * 4);
    for (int y = 0; y < h; ++y)
        memcpy(pixels.data() + (size_t)y * w * 4, (uint8_t *)bd.Scan0 + (size_t)y * bd.Stride, (size_t)w * 4);
    bitmap.UnlockBits(&bd);

    destroy_texture(d);
    obs_enter_graphics();
    const uint8_t *data = pixels.data();
    d->texture = gs_texture_create((uint32_t)w, (uint32_t)h, GS_BGRA, 1, &data, 0);
    obs_leave_graphics();
    d->texture_width = (uint32_t)w;
    d->texture_height = (uint32_t)h;
    d->text_width = (float)w;
    d->offset = 0.0f;
}

static void *kdr_create(obs_data_t *settings, obs_source_t *source)
{
    auto *d = new kdr_running_text_data;
    d->source = source;
    d->text = bstrdup(obs_data_get_string(settings, "text"));
    d->theme = bstrdup(obs_data_get_string(settings, "theme"));
    d->logo_path = bstrdup(obs_data_get_string(settings, "logo_path"));
    d->width = (int)obs_data_get_int(settings, "width");
    d->height = (int)obs_data_get_int(settings, "height");
    d->speed = (int)obs_data_get_int(settings, "speed");
    d->reverse = obs_data_get_bool(settings, "reverse");
    d->font_size = (int)obs_data_get_int(settings, "font_size");
    d->text_color = (uint32_t)obs_data_get_int(settings, "text_color");
    d->outline_color = (uint32_t)obs_data_get_int(settings, "outline_color");
    d->outline_size = (int)obs_data_get_int(settings, "outline_size");
    d->shadow = obs_data_get_bool(settings, "shadow");
    d->shadow_offset = (int)obs_data_get_int(settings, "shadow_offset");
    d->logo_size = (int)obs_data_get_int(settings, "logo_size");
    d->logo_gap = (int)obs_data_get_int(settings, "logo_gap");
    return d;
}

static void kdr_destroy(void *obj)
{
    auto *d = (kdr_running_text_data *)obj;
    if (!d) return;
    destroy_texture(d);
    bfree(d->text); bfree(d->theme); bfree(d->logo_path);
    delete d;
}

static void kdr_update(void *obj, obs_data_t *s)
{
    auto *d = (kdr_running_text_data *)obj;
    bfree(d->text); bfree(d->theme); bfree(d->logo_path);
    d->text = bstrdup(obs_data_get_string(s, "text"));
    d->theme = bstrdup(obs_data_get_string(s, "theme"));
    d->logo_path = bstrdup(obs_data_get_string(s, "logo_path"));
    d->width = (int)obs_data_get_int(s, "width"); d->height = (int)obs_data_get_int(s, "height");
    d->speed = (int)obs_data_get_int(s, "speed"); d->reverse = obs_data_get_bool(s, "reverse");
    d->font_size = (int)obs_data_get_int(s, "font_size"); d->text_color = (uint32_t)obs_data_get_int(s, "text_color");
    d->outline_color = (uint32_t)obs_data_get_int(s, "outline_color"); d->outline_size = (int)obs_data_get_int(s, "outline_size");
    d->shadow = obs_data_get_bool(s, "shadow"); d->shadow_offset = (int)obs_data_get_int(s, "shadow_offset");
    d->logo_size = (int)obs_data_get_int(s, "logo_size"); d->logo_gap = (int)obs_data_get_int(s, "logo_gap");
    render_text(d);
}

static uint32_t kdr_width(void *obj) { return (uint32_t)((kdr_running_text_data *)obj)->width; }
static uint32_t kdr_height(void *obj) { return (uint32_t)((kdr_running_text_data *)obj)->height; }

static void kdr_tick(void *obj, float seconds)
{
    auto *d = (kdr_running_text_data *)obj;
    if (!d->texture) render_text(d);
    float delta = (float)d->speed * seconds;
    d->offset += d->reverse ? delta : -delta;
    float cycle = d->text_width + 80.0f;
    if (d->offset < -cycle) d->offset += cycle;
    if (d->offset > cycle) d->offset -= cycle;
}

static void draw_texture(gs_effect_t *effect, gs_texture_t *tex, float x, uint32_t w, uint32_t h)
{
    gs_matrix_push();
    gs_matrix_translate3f(x, 0.0f, 0.0f);
    gs_effect_set_texture_srgb(gs_effect_get_param_by_name(effect, "image"), tex);
    gs_draw_sprite(tex, 0, w, h);
    gs_matrix_pop();
}

static void kdr_video_render(void *obj, gs_effect_t *effect)
{
    auto *d = (kdr_running_text_data *)obj;
    if (!d->texture) return;
    const bool previous = gs_framebuffer_srgb_enabled();
    gs_enable_framebuffer_srgb(true);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
    while (gs_effect_loop(effect, "Draw")) {
        float x = d->offset;
        draw_texture(effect, d->texture, x, d->texture_width, d->texture_height);
        draw_texture(effect, d->texture, x + d->text_width + 80.0f, d->texture_width, d->texture_height);
        draw_texture(effect, d->texture, x - d->text_width - 80.0f, d->texture_width, d->texture_height);
    }
    gs_blend_state_pop();
    gs_enable_framebuffer_srgb(previous);
}

static obs_properties_t *kdr_properties(void *)
{
    obs_properties_t *p = obs_properties_create();
    obs_properties_add_text(p, "text", "Running text", OBS_TEXT_DEFAULT);
    obs_properties_add_int(p, "speed", "Speed", 1, 200, 1);
    obs_properties_add_bool(p, "reverse", "Reverse direction");
    obs_properties_add_int(p, "width", "Canvas width", 100, 4096, 10);
    obs_properties_add_int(p, "height", "Canvas height", 20, 1080, 10);
    obs_property_t *theme = obs_properties_add_list(p, "theme", "Theme", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(theme, "Modern", "modern"); obs_property_list_add_string(theme, "News", "news");
    obs_property_list_add_string(theme, "Sports", "sports"); obs_property_list_add_string(theme, "Minimal", "minimal"); obs_property_list_add_string(theme, "KDR", "kdr");
    obs_properties_add_int(p, "font_size", "Font size", 8, 300, 1);
    obs_properties_add_color(p, "text_color", "Text color"); obs_properties_add_color(p, "outline_color", "Outline color");
    obs_properties_add_int(p, "outline_size", "Outline size", 0, 20, 1);
    obs_properties_add_bool(p, "shadow", "Drop shadow"); obs_properties_add_int(p, "shadow_offset", "Shadow offset", 0, 20, 1);
    obs_properties_add_path(p, "logo_path", "Logo / icon", OBS_PATH_FILE, "Image files (*.png *.jpg *.jpeg *.webp)", nullptr);
    obs_properties_add_int(p, "logo_size", "Logo size", 16, 300, 1);
    obs_properties_add_int(p, "logo_gap", "Logo-text gap", 0, 100, 1);
    return p;
}

static void kdr_defaults(obs_data_t *s)
{
    obs_data_set_default_string(s, "text", "LIVE STREAMING KDR MULTIMEDIA"); obs_data_set_default_int(s, "speed", 30);
    obs_data_set_default_bool(s, "reverse", false); obs_data_set_default_int(s, "width", 1920); obs_data_set_default_int(s, "height", 80);
    obs_data_set_default_string(s, "theme", "modern"); obs_data_set_default_int(s, "font_size", 42);
    obs_data_set_default_int(s, "text_color", 0xFFFFFFFF); obs_data_set_default_int(s, "outline_color", 0x000000FF);
    obs_data_set_default_int(s, "outline_size", 2); obs_data_set_default_bool(s, "shadow", true); obs_data_set_default_int(s, "shadow_offset", 3);
    obs_data_set_default_string(s, "logo_path", "");
    obs_data_set_default_int(s, "logo_size", 56); obs_data_set_default_int(s, "logo_gap", 18);
}

bool kdr_running_text_register(void)
{
    static obs_source_info info = {};
    info.id = "kdr_running_text_source"; info.type = OBS_SOURCE_TYPE_INPUT; info.output_flags = OBS_SOURCE_VIDEO;
    info.get_name = kdr_get_name; info.create = kdr_create; info.destroy = kdr_destroy; info.get_width = kdr_width; info.get_height = kdr_height;
    info.video_render = kdr_video_render; info.video_tick = kdr_tick; info.get_properties = kdr_properties; info.get_defaults = kdr_defaults; info.update = kdr_update;
    obs_register_source(&info); return true;
}

void kdr_running_text_graphics_init(void)
{
    GdiplusStartupInput input;
    GdiplusStartup(&gdip_token, &input, nullptr);
}
void kdr_running_text_graphics_shutdown(void)
{
    if (gdip_token) { GdiplusShutdown(gdip_token); gdip_token = 0; }
}
