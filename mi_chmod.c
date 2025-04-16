#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n" RESET);
        return FALLO;
    }

    // Convertimos los permisos de string a int
    int permisos = atoi(argv[2]);
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_chmod.c -> main() -> bmount() == FALLO" RESET);
        return FALLO;
    }

    const char *camino = argv[3];
    
    int error = mi_chmod(camino, permisos);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_chmod.c -> main() -> mi_chmod() == FALLO" RESET);
        return FALLO;
    } else {
        mostrar_error_buscar_entrada(error);
    }

    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_chmod.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
}