#include "directorios.h"

int main(int argc, char **argv) {
    // Mira si tiene 4 argumentos incluidos el nombre de programa
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta_directorio/>\n" RESET);
        return FALLO;
    }
    // Convertimos los permisos de string a int
    int permisos = atoi(argv[2]);
    if(permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: mi_mkdir.c -> main() -> permiso < 0 || permiso > 7" RESET);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_mkdir.c -> main() -> bmount() == FALLO" RESET);
        return FALLO;
    }

    const char *camino = argv[3];
    int error = mi_creat(camino, permisos);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_mkdir.c -> main() -> mi_creat() == FALLO" RESET);
        return FALLO;
    } else {
        mostrar_error_buscar_entrada(error);
    }

    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_mkdir.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
}