#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool kdr_running_text_register(void);
void kdr_running_text_graphics_init(void);
void kdr_running_text_graphics_shutdown(void);

#ifdef __cplusplus
}
#endif
