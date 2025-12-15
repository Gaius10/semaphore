#include "config.h"

#include <stdlib.h>
#include <stdio.h>

config_status_t config_parse_file(const char* path, config_t* config) {
    FILE* file = fopen(path, "r");

    if (file == NULL) {
        return CONFIG_EACCESS;
    }

    // @todo
    config->mode = MODE_DEFAULT;

    fclose(file);
    return CONFIG_OK;
}
