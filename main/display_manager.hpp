#ifndef DISPLAY_MANAGER_HPP
#define DISPLAY_MANAGER_HPP

#include "config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void display_init(void);
void display_render_widgets(void);
void display_default_view(void);
void display_update_widget_by_topic(const char *topic, const char *data);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_MANAGER_HPP
