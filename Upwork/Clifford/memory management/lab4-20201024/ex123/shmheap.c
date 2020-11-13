/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/

#include "shmheap.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

shmheap_memory_handle shmheap_create(const char *name, size_t len)
{
    /* TODO */
    int fd = shm_open(name, O_RDWR | O_CREAT, S_IRWXG | S_IRWXO | S_IRWXU);
    void *addr;
    //shmheap_memory_handle *handle = malloc(sizeof(shmheap_memory_handle));
    shmheap_memory_handle handle;
    shmheap_initial_bk initial_book;

    if (fd == -1)
    {
        printf("Failed to create %s\n", name);
        return;
    }

    ftruncate(fd, len);

    if ((addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        printf("Mapping Failed\n");
        return;
    }

    memset(addr, 0, len);

    handle.fd = fd;
    handle.heap_start = addr;
    handle.size = len;

    initial_book.max_size = len;

    /*int semrc = sem_init(&heap_mutex, 0, 1);

    if(semrc == -1) {
        printf("Sem init failed : %s\n", strerror(errno));
    }*/

    heap_mutex = sem_open("shmheapmutex", O_RDWR | O_CREAT);

    if (heap_mutex == SEM_FAILED)
    {
        printf("Sem open failed %s\n", strerror(errno));
    }

    memcpy((void *)addr, (const void *)&initial_book, sizeof(initial_book));
    return handle;
}

shmheap_memory_handle shmheap_connect(const char *name)
{
    /* TODO */
    int fd = shm_open(name, O_RDWR, S_IRWXG | S_IRWXO | S_IRWXU);
    void *addr;
    struct stat sb;
    shmheap_memory_handle handle;

    if (fd == -1)
    {
        printf("Failed to create %s\n", name);
        return;
    }

    if (fstat(fd, &sb) == -1)
    {
        printf("Failed fstat %s\n", name);
        return;
    }

    if ((addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        printf("Mapping Failed\n");
        return;
    }

    handle.fd = fd;
    handle.heap_start = addr;
    handle.size = sb.st_size;

    heap_mutex = sem_open("shmheapmutex", O_RDWR | O_CREAT);

    return handle;
}

void shmheap_disconnect(shmheap_memory_handle mem)
{
    /* TODO */
    munmap(mem.heap_start, mem.size);
    close(mem.fd);
    sem_close(heap_mutex);
}

void shmheap_destroy(const char *name, shmheap_memory_handle mem)
{
    /* TODO */
    munmap(mem.heap_start, mem.size);
    close(mem.fd);
    shm_unlink(name);
    //sem_destroy(&heap_mutex);
    int rc = sem_unlink("shmheapmutex");
    if(rc == -1) {
        printf("sem_unlink failed with %s\n", strerror(errno));
    }
}

void *shmheap_underlying(shmheap_memory_handle mem)
{
    /* TODO */
    return mem.heap_start;
}

void *shmheap_alloc(shmheap_memory_handle mem, size_t sz)
{
    /* TODO */
    //sem_wait(&heap_mutex);
    sem_wait(heap_mutex);
    void *addr = mem.heap_start;
    shmheap_bk *bk = NULL, *tmpbk;
    size_t rounded_size = sz + (sz % 8);

    shmheap_initial_bk *initial_bk;

    size_t visited_size;

    initial_bk = (shmheap_initial_bk *)addr;
    visited_size = sizeof(shmheap_initial_bk);

    //printf("Allocating new block of size %zu, %zu, Max Size : %zu\n", sz, visited_size, initial_bk->max_size);
    bk = (shmheap_bk *)(addr + visited_size);

    for (; (visited_size + sizeof(shmheap_bk)) < initial_bk->max_size;)
    {
        bk = (shmheap_bk *)(addr + visited_size);
        //printf("Occupied : %zu, Visited Size : %zu, Block Size : %zu,  Size : %zu\n", bk->isOccupied, visited_size, bk->block_size, sz);
        if ((bk->isOccupied == FREE_BLOCK) &&
            (bk->block_size == 0 || (bk->block_size) > (rounded_size + sizeof(shmheap_bk))))
        {
            break;
        }
        else
        {
            visited_size += (bk->block_size + sizeof(shmheap_bk));
        }
    }

    if (visited_size >= initial_bk->max_size)
    {
        //sem_post(&heap_mutex);
        sem_post(heap_mutex);
        return NULL;
    }
    else
    {
        addr += visited_size;

        if (bk->block_size > (rounded_size + sizeof(shmheap_bk)))
        {
            tmpbk = (shmheap_bk *)(addr + sizeof(shmheap_bk) + rounded_size);
            tmpbk->block_size = (bk->block_size - rounded_size - sizeof(shmheap_bk));
            tmpbk->isOccupied = FREE_BLOCK;
            //printf("New free block : Occupied : %zu, Visited Size : %zu, Block Size : %zu,  Size : %zu\n", bk->isOccupied, visited_size, bk->block_size, sz);
        }

        bk->block_size = rounded_size;
        bk->isOccupied = OCCUPIED_BLOCK;
        addr += sizeof(shmheap_bk);
        //sem_post(&heap_mutex);
        sem_post(heap_mutex);
        return addr;
    }
}

void shmheap_free(shmheap_memory_handle mem, void *ptr)
{
    /* TODO */
    //sem_wait(&heap_mutex);
    sem_wait(heap_mutex);
    shmheap_bk *current_bk, *bk;
    void *start_block = (ptr - sizeof(shmheap_bk));
    current_bk = (shmheap_bk *)start_block;

    current_bk->isOccupied = FREE_BLOCK;
    memset(ptr, 0, current_bk->block_size);
    start_block = ptr + (current_bk->block_size);

    for (; (start_block + sizeof(shmheap_bk)) <= (void *)(mem.heap_start + mem.size); start_block += (bk->block_size + sizeof(shmheap_bk)))
    {
        bk = (shmheap_bk *)start_block;
        if (bk->isOccupied == OCCUPIED_BLOCK)
        {
            break;
        }
        else
        {
            current_bk->block_size += (bk->block_size + sizeof(shmheap_bk));
            memset(start_block, 0, (bk->block_size + sizeof(shmheap_bk)));
        }
    }
    //sem_post(&heap_mutex);
    sem_post(heap_mutex);
}

shmheap_object_handle shmheap_ptr_to_handle(shmheap_memory_handle mem, void *ptr)
{
    /* TODO */
    shmheap_object_handle handle;
    size_t offset = ptr - (void *)mem.heap_start;
    handle.offset = offset;
    return handle;
}

void *shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl)
{
    /* TODO */
    void *address = mem.heap_start + hdl.offset;
    return address;
}
