#include "kdr-running-text.h"
#include <obs-module.h>

struct kdr_running_text_data {
    char *text;
    int width;
    int height;
    int speed;
    bool reverse;
};

static const char *kdr_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "KDR Running Text";
}

static void *kdr_create(obs_data_t *settings, obs_source_t *source)
{
    struct kdr_running_text_data *data = bzalloc(sizeof(*data));
    data->width = (int)obs_data_get_int(settings, "width");
    data->height = (int)obs_data_get_int(settings, "height");
    data->speed = (int)obs_data_get_int(settings, "speed");
    data->reverse = obs_data_get_bool(settings, "reverse");
    data->text = bstrdup(obs_data_get_string(settings, "text"));
    UNUSED_PARAMETER(source);
    return data;
}

static void kdr_destroy(void *obj)
{
    struct kdr_running_text_data *data = obj;
    if (!data) return;
    bfree(data->text);
    bfree(data);
}

static void kdr_update(void *obj, obs_data_t *settings)
{
    struct kdr_running_text_data *data = obj;
    bfree(data->text);
    data->text = bstrdup(obs_data_get_string(settings, "text"));
    data->width = (int)obs_data_get_int(settings, "width");
    data->height = (int)obs_data_get_int(settings, "height");
    data->speed = (int)obs_data_get_int(settings, "speed");
    data->reverse = obs_data_get_bool(settings, "reverse");
}

static uint32_t kdr_width(void *obj)
{
    return (uint32_t)((struct kdr_running_text_data *)obj)->width;
}

static uint32_t kdr_height(void *obj)
{
    return (uint32_t)((struct kdr_running_text_data *)obj)->height;
}

static void kdr_video_render(void *obj, gs_effect_t *effect)
{
    /* Rendering engine is intentionally kept minimal in V0.1.
       The next milestone adds GPU text rendering and smooth scrolling. */
    UNUSED_PARAMETER(obj);
    UNUSED_PARAMETER(effect);
}

static obs_properties_t *kdr_properties(void *obj)
{
    UNUSED_PARAMETER(obj);
    obs_properties_t *props = obs_properties_create();

    obs_properties_add_text(props, "text", "Text", OBS_TEXT_DEFAULT);
    obs_properties_add_int(props, "speed", "Speed", 1, 100, 1);
    obs_properties_add_bool(props, "reverse", "Reverse direction");
    obs_properties_add_int(props, "width", "Canvas width", 100, 4096, 10);
    obs_properties_add_int(props, "height", "Canvas height", 20, 1080, 10);

    obs_property_t *theme = obs_properties_add_list(props, "theme", "Theme",
        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(theme, "Modern", "modern");
    obs_property_list_add_string(theme, "News", "news");
    obs_property_list_add_string(theme, "Sports", "sports");
    obs_property_list_add_string(theme, "Minimal", "minimal");
    obs_property_list_add_string(theme, "KDR", "kdr");

    obs_properties_add_int(props, "font_size", "Font size", 8, 300, 1);
    obs_properties_add_text(props, "logo_path", "Logo path", OBS_TEXT_DEFAULT);

    return props;
}

static void kdr_defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, "text", "LIVE STREAMING KDR MULTIMEDIA");
    obs_data_set_default_int(settings, "speed", 30);
    obs_data_set_default_bool(settings, "reverse", false);
    obs_data_set_default_int(settings, "width", 1920);
    obs_data_set_default_int(settings, "height", 80);
    obs_data_set_default_string(settings, "theme", "modern");
    obs_data_set_default_int(settings, "font_size", 42);
    obs_data_set_default_string(settings, "logo_path", "");
}

bool kdr_running_text_register(void)
{
    static struct obs_source_info info = {
        .id = "kdr_running_text_source",
        .type = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_VIDEO,
        .get_name = kdr_get_name,
        .create = kdr_create,
        .destroy = kdr_destroy,
        .get_width = kdr_width,
        .get_height = kdr_height,
        .video_render = kdr_video_render,
        .get_properties = kdr_properties,
        .get_defaults = kdr_defaults,
        .update = kdr_update,
    };

    obs_register_source(&info);
    return true;
}
