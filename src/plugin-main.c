#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

bool obs_module_load(void)
{
    blog(LOG_INFO, "KDR Running Text for OBS loaded (version %s)", PLUGIN_VERSION);
    return kdr_running_text_register();
}

void obs_module_unload(void)
{
    blog(LOG_INFO, "KDR Running Text for OBS unloaded");
}
