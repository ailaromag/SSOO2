#include "directorios.h"

#define TAMBUFFER (BLOCKSIZE * 4)

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_cat <disco> </ruta_fichero>\n" RESET);
    }
    char *disco = argv[1];
    char *ruta_fichero = argv[2];
    if (bmount(disco) == FALLO){
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    struct STAT stat;
    if (mi_stat(ruta_fichero, &stat) == FALLO) {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    }
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
        return FALLO;
    }
    unsigned char buffer_texto[TAMBUFFER];
    int error = mi_read(ruta_fichero, buffer_texto, 0, TAMBUFFER);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> mi_read() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }

    printf("%s", buffer_texto);

    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}