#include "directorios.h"

#define DEBUGN5 true

int main(int argc, char **argv) {
#if DEBUGN5
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>");
        printf(RESET);
        return FALLO;
    }
    const char *texto = argv[3];
    int longitud = strlen(texto);
    printf("longitud texto: %d\n\n", longitud);  // Imprimir longitud del texto
    char *disco = argv[1];
    int offset = atoi(argv[4]);
    if (bmount(disco) == FALLO) {
        fprintf(stderr,RED "Error: escribir.c -> main() -> bmount() == FALLO"RESET);
        return FALLO;
    }
    char *ruta_fichero= argv[2]; 

    // Escritura 
    int bytes_escritos = mi_write(ruta_fichero, texto, offset, longitud);
    if (bytes_escritos < 0) {
        fprintf(stderr, RED "Error: escribir.c -> main() -> mi_write() == FALLO" RESET);
        bumount();
        return FALLO;
    }
    printf("Bytes escritos:  %d \n", bytes_escritos);

    if (bumount() == FALLO) {
        fprintf(stderr,RED "Error: escribir.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
#endif
    return EXITO;
}
