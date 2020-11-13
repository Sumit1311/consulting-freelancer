#include "zc_io.h"
#include <malloc.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

// The zc_file struct is analogous to the FILE struct that you get from fopen.
struct zc_file {
  // Insert the fields you need here.
  int fd;//fd for opened file
  off_t offset;//last read/written offset 
  void *start_addr;//starting address of mmapped region
  size_t size;//file size
  pthread_rwlock_t rwlock;//Reader writer lock used for synchronization
  pthread_mutex_t mutex;//Mutex for restricting access to single thread to this struct
};


/**************
 * Exercise 1 *
 **************/

zc_file *zc_open(const char *path) {
  // To implement
  // Allocate the basic metadata structure
  zc_file* zfile = (zc_file *)calloc(1, sizeof(zc_file));
  void *start_addr;
  struct stat sb;

  if(zfile == NULL) {
    printf("Memmory allocation failed\n");
    return NULL;
  }

  //Open the file
  int fd = open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);

  if(fd == -1) {
    printf("Failed to open %s\n", path);
    return NULL;
  }


  //Get size of file for mmap
  if(fstat(fd, &sb) == -1) {
    printf("Failed to get stat\n");
    return NULL;
  }

  //mmap if file size is not zero
  if(sb.st_size != 0 && (start_addr= mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE ,MAP_SHARED, fd, 0)) == MAP_FAILED){
    printf("mmap failed\n");
    return NULL;
  }

  //Initialize the metadata
  zfile->fd = fd;
  zfile->offset = 0;
  zfile->start_addr = start_addr;
  zfile->size=sb.st_size;
  pthread_rwlock_init(&(zfile->rwlock), NULL);
  pthread_mutex_init(&(zfile->mutex), NULL);

  return zfile;
}

int zc_close(zc_file *file) {
  // To implement
  pthread_mutex_lock(&(file->mutex));
  munmap(file->start_addr, file->size);
  close(file->fd);
  pthread_rwlock_destroy(&(file->rwlock));
  pthread_mutex_unlock(&(file->mutex));
  free(file);
  pthread_mutex_destroy(&(file->mutex));
  return 0;
}

const char *zc_read_start(zc_file *file, size_t *size) {
  // To implement
  char *address_ret = NULL;
  //Get reader lock
  pthread_rwlock_rdlock(&(file->rwlock));
  //Get  lock for metadata access
  pthread_mutex_lock(&(file->mutex));

  if(file->offset == file->size) {
     //If file is completely read
     return address_ret;
  }
  if( file->size < (file->offset+(*size)) ) {
     //When size is greater than the remaining size to be read
     (*size) = (file->size - file->offset);
  }

  address_ret = ((char *)(file->start_addr) + file->offset);

  file->offset += (*size);
  pthread_mutex_unlock(&(file->mutex));

  return address_ret;
}

void zc_read_end(zc_file *file) {
  // To implement
  pthread_rwlock_unlock(&(file->rwlock));
}

/**************
 * Exercise 2 *
 **************/

char *zc_write_start(zc_file *file, size_t size) {
  // To implement
  void *new_addr, *ret_addr=NULL;
  //Get writer lock
  pthread_rwlock_wrlock(&(file->rwlock));
  pthread_mutex_lock(&(file->mutex));


  if(file->size != 0 && (size + file->offset) > file->size) {
     //If file does not have sufficient space
     if(( new_addr = mremap(file->start_addr, file->size, (file->offset + size), MREMAP_MAYMOVE) ) == (void *)-1 ) {
        printf("mremap failed : %s\n", strerror(errno));
	return ret_addr;
     }
     //Reset file size
     ftruncate(file->fd, size+file->offset);
     file->start_addr = new_addr;
     file->size = (file->offset + size);
  }

  if( file->size == 0) {
     //If this is new file created, mmap now
     if((new_addr= mmap(NULL, size, PROT_READ | PROT_WRITE ,MAP_SHARED, file->fd, 0)) == MAP_FAILED){
        printf("mmap failed\n");
        return NULL;
     }
     ftruncate(file->fd, size);
     file->start_addr = new_addr;
     file->size = (size);
  }

  ret_addr = ((char *)file->start_addr + file->offset);

  file->offset += size;
  pthread_mutex_unlock(&(file->mutex));

  return ret_addr;
}

void zc_write_end(zc_file *file) {
  // To implement
  // msync is for flushing of the changes to file
  if((msync(file->start_addr, file->size, MS_SYNC)) == -1) {
     printf("msync failed with %s\n",strerror(errno));
  }
  pthread_rwlock_unlock(&(file->rwlock));
}

/**************
 * Exercise 3 *
 **************/

off_t zc_lseek(zc_file *file, long offset, int whence) {
  // To implement
  off_t final_offset;
  pthread_rwlock_wrlock(&(file->rwlock));
  pthread_mutex_lock(&(file->mutex));
  if(whence == SEEK_SET) {
     final_offset = 0 + offset;
  } else if(whence == SEEK_CUR) {
     final_offset = (file->offset) + offset;
  } else if(whence == SEEK_END) {
     final_offset = (file->size) + offset;
  } else {
     return -1;
  }
  file->offset = final_offset;
  pthread_mutex_unlock(&(file->mutex));
  pthread_rwlock_unlock(&(file->rwlock));
  return final_offset;
}

/**************
 * Exercise 5 *
 **************/

int zc_copyfile(const char *source, const char *dest) {
  // To implement
  zc_file *sourceFile = zc_open(source);
  zc_file *destFile = zc_open(dest);
  off_t sourceOffset = 0, destOffset = 0;

  zc_read_start(sourceFile, &(sourceFile->size));
  zc_write_start(destFile, sourceFile->size);

  copy_file_range(sourceFile->fd, &(sourceOffset), destFile->fd, &(destOffset), sourceFile->size, 0);

  zc_read_end(sourceFile);
  zc_write_end(destFile);
  zc_close(sourceFile);
  zc_close(destFile);

  return 0;
}
