#include "bloques.h"

static int descriptor = 0;

// Montar el fichero
int bmount(const char *camino) {
    if ((descriptor = open(camino, O_RDWR | O_CREAT, 0666)) == FALLO)  // Abrimos el fichero y controlamos que no se produzcan errores
    {
        perror(RED "Error: bmount(), open() == FALLO");
        printf(RESET);
        return FALLO;
    }
    chmod(camino, 0666);

    return descriptor;
}

// Hace un close del archivo
int bumount() {
    if (close(descriptor) == FALLO) {
        perror(RED "Error: bumount, close() == FALLO");
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
        perror(RED "Error: bwrite(), lseek() == FALLO");
        printf(RESET);
        return FALLO;
    }

    if ((bytesEscritos = write(descriptor, buf, BLOCKSIZE)) == FALLO) {
        perror(RED "Error: write() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return bytesEscritos;
}

// Lee un bloque (como bwrite pero leyendo)
int bread(unsigned int nbloque, void *buf) {
    int bytesLeidos;
    if ((lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET)) == FALLO) {
        perror(RED "Error: bread, lseek() == FALLO");
        printf(RESET);
        return FALLO;
    }

    if ((bytesLeidos = read(descriptor, buf, BLOCKSIZE)) == FALLO) {
        perror(RED "Error: bread, read() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return bytesLeidos;
}