#include <semaphore.h>
#include <fcntl.h>       // For O_CREAT
#include <sys/stat.h>    // For S_IRWXU
#include <stddef.h>      // For NULL

#define SEM_NAME "/mymutex"
#define SEM_INIT_VALUE 1

sem_t *initSem();
void deleteSem();
void signalSem(sem_t *sem);
void waitSem(sem_t *sem);