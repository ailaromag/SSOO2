#include "directorios.h"

int main(int argc, char** argv) {
    // check de sintaxis
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_link disco /ruta_fichero_original /ruta_enlace\n" RESET);
        return FALLO;
    }
    // guardamos los parámetros
    char *disco = argv[1];
    char *ruta_fichero_original = argv[2];
    char *ruta_enlace = argv[3];
    // montamos el disco
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    // check si las rutas son ficheros
    struct STAT stat;
    int error = mi_stat(ruta_fichero_original, &stat);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
        return FALLO;
    }
    // linkeamos el fichero
    error = mi_link(ruta_fichero_original, ruta_enlace);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> mi_link() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    // desmontamos el disco
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}