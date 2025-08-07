#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "config_types.h"
#include <stdbool.h>

bool load_config(void);
const app_config_t* get_config(void);

#endif // CONFIG_PARSER_H
