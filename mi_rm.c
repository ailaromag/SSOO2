#include "directorios.h"

int main(int argc, char **argv) {
    // comprobamos sintaxis
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_rm disco /ruta\n");
        return FALLO;
    }
    // guardamos los parametros
    char *disco = argv[1];
    char *ruta = argv[2];
    // montamos el disco
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    // check si las rutas son ficheros
    struct STAT stat;
    int error = mi_stat(ruta, &stat);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
        return FALLO;
    }
    // deslinkeamos el fichero
    error = mi_unlink(ruta);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> mi_unlink() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    // desmontamos el disco
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}