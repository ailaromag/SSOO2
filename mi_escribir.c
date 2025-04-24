#include "directorios.h"

#define DEBUGN9 true

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n" RESET);
        return FALLO;
    }
    char *disco = argv[1];
    char *ruta_fichero = argv[2];
    char *texto = argv[3];
    int offset = atoi(argv[4]);
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    struct STAT stat;
    if (mi_stat(ruta_fichero, &stat) == FALLO) {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    }
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
        return FALLO;
    }
#if DEBUGN9
    printf("longitud texto: %d\n", strlen(texto));
#endif
    int error = mi_write(ruta_fichero, texto, offset, strlen(texto));
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> mi_write() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
#if DEBUGN9
    printf("Bytes escritos: %d\n", error);
#endif
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}
// =======
// #define DEBUGN5 true

// int main(int argc, char **argv) {
// #if DEBUGN5
//     if (argc != 5) {
//         fprintf(stderr, RED "Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>");
//         printf(RESET);
//         return FALLO;
//     }
//     const char *texto = argv[3];
//     int longitud = strlen(texto);
//     printf("longitud texto: %d\n\n", longitud);  // Imprimir longitud del texto
//     char *disco = argv[1];
//     int offset = atoi(argv[4]);
//     if (bmount(disco) == FALLO) {
//         fprintf(stderr,RED "Error: escribir.c -> main() -> bmount() == FALLO"RESET);
//         return FALLO;
//     }
//     char *ruta_fichero= argv[2]; 

//     // Escritura 
//     int bytes_escritos = mi_write(ruta_fichero, texto, offset, longitud);
//     if (bytes_escritos < 0) {
//         fprintf(stderr, RED "Error: escribir.c -> main() -> mi_write() == FALLO" RESET);
//         bumount();
//         return FALLO;
//     }
//     printf("Bytes escritos:  %d \n", bytes_escritos);

//     if (bumount() == FALLO) {
//         fprintf(stderr,RED "Error: escribir.c -> main() -> bumount() == FALLO" RESET);
//         return FALLO;
//     }
// #endif
//     return EXITO;
// }
// >>>>>>> d9edd3f560facaef2121a39d30487480d2f3b593
