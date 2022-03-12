#include "properties.h"

#include <string.h>
#include <malloc.h>

void properties_for_each(const void *properties, uint32_t length, void *ctx, properties_for_each_block block) {
    char *ptr = malloc(length + 1);

    memcpy(ptr, properties, length);
    ptr[length] = '\0';

    char *line_status = NULL;
    char *line = strtok_r(ptr, "\n", &line_status);
    while (line != NULL) {
        char *split = strchr(line, '=');
        if (split != NULL) {
            *split = '\0';
            block(ctx, line, split + 1);
        }
        line = strtok_r(NULL, "\n", &line_status);
    }

    free(ptr);
}
