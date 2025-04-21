#include "directorios.h"

#define TAMBUFFER 1500  // Tamaño del buffer de lectura

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_cat <disco> </ruta_fichero>\n");
        return FALLO;
    }

    char *disco = argv[1];
    char *ruta_fichero = argv[2];

    // Montar el sistema de archivos
    if (bmount(disco) == -1) {
        fprintf(stderr,RED "Error: mi_cat.c -> main() -> bmount() == FALLO"RESET);
        return FALLO;
    }

    // Variables para la lectura
    unsigned char buffer[TAMBUFFER];
    unsigned int offset = 0;
    int leidos = TAMBUFFER, total_leidos = 0;

    while (leidos > 0) {
        memset(buffer, 0, TAMBUFFER);
        leidos = mi_read(ruta_fichero, buffer, offset, TAMBUFFER);

        if (leidos < 0) {
            fprintf(stderr, RED "Error: mi_cat.c -> main() -> mi_read() == FALLO" RESET);
            bumount();
            return FALLO;
        }

        if (leidos > 0) {
            write(1, buffer, leidos);  // Imprimir el contenido leído en la salida estándar
            total_leidos += leidos;
            offset += leidos;
        }
    }

    // Mostrar cantidad total de bytes leídos
    printf("Bytes leídos: %d\n", total_leidos);

    // Desmontar el sistema de archivos
    if (bumount() == -1) {
        fprintf(stderr,RED "Error: mi_cat.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }

    return EXITO;
}