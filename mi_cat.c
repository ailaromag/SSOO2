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
    unsigned int offset = 0;
    int leidos = TAMBUFFER;
    int total_leidos = 0;

    while (leidos > 0) {
        memset(buffer_texto, 0, TAMBUFFER);
        leidos = mi_read(ruta_fichero, buffer_texto, offset, TAMBUFFER);
        if (leidos == FALLO) {
            fprintf(stderr, RED "Error: mi_cat.c -> main() -> mi_read() == FALLO\n" RESET);
            return FALLO;
        } else if (leidos < 0) {
            mostrar_error_buscar_entrada(leidos);
            return FALLO;
        }

        if (leidos > 0) {
            fflush(stdout);
            write(1, buffer_texto, leidos);
            // printf("\n");   // it's used for lvl 9 test
            total_leidos += leidos;
            offset += TAMBUFFER;
        }
    }

    fprintf(stderr, "\nTotal_leidos: %d\n", total_leidos);  // it's uses fprintf because when we execute "./mi_cat disco /ruta > output.txt" it doesn't go into output.txt

    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}