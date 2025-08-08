#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "config_types.h"

void display_init(void);
void display_draw_test_pattern(void);
void display_get_grid_rect(int x, int y, int w, int h, int *px, int *py, int *pw, int *ph);
void display_render_widgets(void);
void display_update_widget_by_topic(const char *topic, const char *data);

#endif // DISPLAY_MANAGER_H
