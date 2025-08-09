#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "config_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool load_config(void);
const app_config_t* get_config(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_PARSER_H
