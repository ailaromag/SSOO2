#include "ficheros.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        perror(RED"Sintaxis: permitir <nombre_dispositivo> <ninodo> <permisos>\n");
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned char permisos = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO) {
        perror(RED "Error al montar el dispositivo\n");
        return FALLO;
    }

    if (mi_chmod_f(ninodo, permisos) == FALLO) {
        perror(RED "Error al cambiar los permisos del inodo \n");
        bumount();
        return FALLO;
    }

    if (bumount() == FALLO) {
        perror(RED "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return 0;
}