#ifndef CONFIG_H
#define CONFIG_H

typedef struct config {
    enum {
        MODE_DEFAULT,
        MODE_HELP,
    } mode;
} config_t;

typedef enum config_status {
    CONFIG_OK,
    CONFIG_EACCESS,
} config_status_t;

config_status_t config_parse_file(const char* path, config_t* config);

#endif
