#include <obs-module.h>
#include "kdr-running-text.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("kdr-running-text-obs", "en-US")

bool obs_module_load(void)
{
    kdr_running_text_graphics_init();
    blog(LOG_INFO, "KDR Running Text for OBS loaded (version %s)", PLUGIN_VERSION);
    return kdr_running_text_register();
}

void obs_module_unload(void)
{
    kdr_running_text_graphics_shutdown();
    blog(LOG_INFO, "KDR Running Text for OBS unloaded");
}
