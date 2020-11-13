/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "mmf.h"
#include <stdio.h>

void *mmf_create_or_open(const char *name, size_t sz)
{
    /* TODO */
    char *addr;

    mappingData.fd = open(name, O_RDWR | O_CREAT);

    if (mappingData.fd == -1)
    {
        printf("Error while opening %s\n", name);
    }

    ftruncate(mappingData.fd, sz);

    addr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, mappingData.fd, 0);

    if (addr == MAP_FAILED)
    {
        printf("Mapping Failed\n");
    }

    return (void *)addr;
}

void mmf_close(void *ptr, size_t sz)
{
    /* TODO */
    munmap(ptr, sz);
    close(mappingData.fd);
}
