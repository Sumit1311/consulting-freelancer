/*************************************
* Lab 4
* Name:
* Student No:
* Lab Group:
*************************************/

#include <stddef.h>
#include <semaphore.h>

/*
You should modify these structs to suit your implementation,
but remember that all the functions declared here must have
a signature that is callable using the APIs specified in the
lab document.

You may define other helper structs or convert the existing
structs to typedefs, as long as the functions satisfy the
requirements in the lab document.  If you declare additional names (helper structs or helper functions), they should be prefixed with "shmheap_" to avoid potential name clashes.
*/

typedef struct
{
    int fd;
    char *heap_start;
    size_t size;
} shmheap_memory_handle;

typedef struct
{
    size_t offset;
} shmheap_object_handle;

typedef struct
{
    size_t max_size;
} shmheap_initial_bk;

typedef struct
{
    size_t block_size;
    size_t isOccupied;
} shmheap_bk;

#define OCCUPIED_BLOCK 1
#define FREE_BLOCK 0

//sem_t heap_mutex;
sem_t *heap_mutex;
/*
These functions form the public API of your shmheap library.
*/

shmheap_memory_handle shmheap_create(const char *name, size_t len);
shmheap_memory_handle shmheap_connect(const char *name);
void shmheap_disconnect(shmheap_memory_handle mem);
void shmheap_destroy(const char *name, shmheap_memory_handle mem);
void *shmheap_underlying(shmheap_memory_handle mem);
void *shmheap_alloc(shmheap_memory_handle mem, size_t sz);
void shmheap_free(shmheap_memory_handle mem, void *ptr);
shmheap_object_handle shmheap_ptr_to_handle(shmheap_memory_handle mem, void *ptr);
void *shmheap_handle_to_ptr(shmheap_memory_handle mem, shmheap_object_handle hdl);
