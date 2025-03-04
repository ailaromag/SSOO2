
#include <string.h>

#include "ficheros_basico.h"

// argc- indica el número de elementos escritos por consola
// argv- vector de punteros a los parámetros
int main(int argc, char **argv) {
    // Obtención de las variables
    if (argc != 3) {
        perror(RED "Error: mi_mkfs.c -> main() -> argc != 3");
        printf(RESET);
        return FALLO;
    }
    char *camino = argv[1];
    int nbloques = atoi(argv[2]);
    // Montar dispositivo
    int descriptor;
    unsigned char buf[BLOCKSIZE];
    if ((descriptor = bmount(camino)) == FALLO) {
        perror(RED "Error: mi_mkfs.c -> main() -> bmount() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Inicializar todos los bloques a 0
    memset(buf, 0, BLOCKSIZE);
    for (int i = 0; i < nbloques; i++) {
        bwrite(i, buf);
    }
    // Inicializar el superbloque:
    if (initSB(nbloques, nbloques / 4) == FALLO) {
        perror(RED "Error: mi_mkfs.c -> main() -> initSB() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Inicializar el mapa de bits:
    if (initMB() == FALLO){
        perror(RED "Error: mi_mkfs.c -> main() -> initMB() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Inicializar el array de inodos:
    if (initAI() == FALLO) {
        perror(RED "Error: mi_mkfs.c -> main() -> initAI() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Reservar el inodo del directorio raíz
    if (reservar_inodo('d', 7) == FALLO) {
        perror(RED "Error: mi_mkfs.c -> main() -> reservar_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Desmontar dispositivo
    if (bumount() == FALLO) {
        perror(RED "Error: mi_mkfs.c -> main() -> bumount() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}