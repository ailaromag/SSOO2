#include "ficheros.h"

#define TAMBUFFER 1500

int main(int argc, char **argv) {
    if (argc != 3) {
        perror(RED "Sintaxis: ./leer <nombre_dispositivo> <ninodo>\n");
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);

    if (bmount(nombre_dispositivo) == FALLO) {
        perror(RED "Error al montar el dispositivo\n");
        return FALLO;
    }

    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error al leer el inodo \n");
        bumount();
        return FALLO;
    }

    unsigned char buffer_texto[TAMBUFFER];
    unsigned int offset = 0;
    int leidos = TAMBUFFER;
    int total_leidos = 0;

    while (leidos > 0) {
        memset(buffer_texto, 0, TAMBUFFER);
        leidos = mi_read_f(ninodo, buffer_texto, offset, TAMBUFFER);

        if (leidos < 0) {
            perror(RED "Error en mi_read_f\n");
            bumount();
            return FALLO;
        }

        if (leidos > 0) {
            write(1, buffer_texto, leidos);
            total_leidos += leidos;
            offset += TAMBUFFER;
        }
    }

    fprintf(stderr, "\nbytes leídos %d\n", total_leidos);
    fprintf(stderr, "tamEnBytesLog %d\n", inodo.tamEnBytesLog);

    if (bumount() == FALLO) {
        perror(RED "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return 0;
}