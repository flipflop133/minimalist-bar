#ifndef PARSER_H
#define PARSER_H
#include "defs.h"
#include "modules/battery.h"
#include "modules/bluetooth.h"
#include "modules/date.h"
#include "modules/media.h"
#include "modules/network.h"
#include "modules/volume.h"
void parse_config(void);
#define CONFIG_PATH "/.config/minimalist-bar/laptop_config.json"
#endif
