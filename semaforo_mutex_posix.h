#include <semaphore.h>
#include <fcntl.h>       // For O_CREAT
#include <sys/stat.h>    // For S_IRWXU
#include <stddef.h>      // For NULL

#define SEM_NAME "/mymutex" // Nombre del semáforo en el espacio de nombres del sistema
#define SEM_INIT_VALUE 1 // Valor inicial del semáforo (1 para mutex binario)

// Declaraciones de funciones para manejo de semáforos POSIX
sem_t *initSem();
void deleteSem();
void signalSem(sem_t *sem);
void waitSem(sem_t *sem);