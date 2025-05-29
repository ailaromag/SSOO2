#include "bloques.h"

#include "semaforo_mutex_posix.h"

static sem_t *mutex;
static unsigned int inside_sc = 0;

static int descriptor = 0;

// Montar el fichero
int bmount(const char *camino) {
    if (descriptor > 0) {
        close(descriptor);
    }

    if (!mutex) {  // el semáforo es único en el sistema y sólo se ha de inicializar 1 vez (padre)
        mutex = initSem();
        if (mutex == SEM_FAILED) {
            return -1;
        }
    }

    if ((descriptor = open(camino, O_RDWR | O_CREAT, 0666)) == FALLO)  // Abrimos el fichero y controlamos que no se produzcan errores
    {
        perror(RED "Error: bloques.c -> bmount() -> open() == FALLO");
        printf(RESET);
        return FALLO;
    }
    chmod(camino, 0666);

    return descriptor;
}

// Hace un close del archivo
int bumount() {
    deleteSem();
    descriptor = close(descriptor);
    if (descriptor == FALLO) {
        perror(RED "Error: bloques.c -> bumount -> close() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}

// Escribe 1 bloque en el dispositivo virtual, devuelve BLOCKSIZE si ha ido bien o -1 si no ha ido bien
int bwrite(unsigned int nbloque, const void *buf) {
    int bytesEscritos;
    // Nos posicionamos: (descriptor apuntará a la posición deseada)
    if ((lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET)) == FALLO) {
        perror(RED "Error: bloques.c -> bwrite() -> lseek() == FALLO");
        printf(RESET);
        return FALLO;
    }

    if ((bytesEscritos = write(descriptor, buf, BLOCKSIZE)) == FALLO) {
        perror(RED "Error: bloques.c -> bwrite() -> write() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return bytesEscritos;
}

// Lee un bloque (como bwrite pero leyendo)
int bread(unsigned int nbloque, void *buf) {
    int bytesLeidos;
    if ((lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET)) == FALLO) {
        perror(RED "Error: bloques.c -> bread() -> lseek() == FALLO");
        printf(RESET);
        return FALLO;
    }

    if ((bytesLeidos = read(descriptor, buf, BLOCKSIZE)) == FALLO) {
        perror(RED "Error: bloques.c -> bread -> read() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return bytesLeidos;
}

void mi_waitSem() {
   if (!inside_sc) { // inside_sc==0, no se ha hecho ya un wait
       waitSem(mutex);
   }
   inside_sc++;
}


void mi_signalSem() {
   inside_sc--;
   if (!inside_sc) {
       signalSem(mutex);
   }
}
