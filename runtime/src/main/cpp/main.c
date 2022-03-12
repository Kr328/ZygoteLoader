#include "main.h"

#include "logger.h"

#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct Resource *resource_map_fd(int fd) {
    struct stat s;
    fatal_assert(fstat(fd, &s) >= 0);

    void *base = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    fatal_assert(base != MAP_FAILED);

    struct Resource *resource = (struct Resource *) malloc(sizeof(struct Resource));
    resource->base = base;
    resource->length = s.st_size;
    return resource;
}

struct Resource *resource_map_file(const char *path) {
    int fd = open(path, O_RDONLY);
    fatal_assert(fd >= 0);

    struct Resource *resource = resource_map_fd(fd);

    close(fd);

    return resource;
}

void resource_release(struct Resource *resource) {
    munmap(resource->base, resource->length);

    free(resource);
}