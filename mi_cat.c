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

    printf("Total_leidos: %d", error);

    printf("%s", buffer_texto);

    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
// =======
// #define TAMBUFFER 1500  // Tamaño del buffer de lectura

// int main(int argc, char **argv) {
//     if (argc != 3) {
//         fprintf(stderr, "Sintaxis: ./mi_cat <disco> </ruta_fichero>\n");
//         return FALLO;
//     }

//     char *disco = argv[1];
//     char *ruta_fichero = argv[2];

//     // Montar el sistema de archivos
//     if (bmount(disco) == -1) {
//         fprintf(stderr,RED "Error: mi_cat.c -> main() -> bmount() == FALLO"RESET);
//         return FALLO;
//     }

//     // Variables para la lectura
//     unsigned char buffer[TAMBUFFER];
//     unsigned int offset = 0;
//     int leidos = TAMBUFFER, total_leidos = 0;

//     while (leidos > 0) {
//         memset(buffer, 0, TAMBUFFER);
//         leidos = mi_read(ruta_fichero, buffer, offset, TAMBUFFER);

//         if (leidos < 0) {
//             fprintf(stderr, RED "Error: mi_cat.c -> main() -> mi_read() == FALLO" RESET);
//             bumount();
//             return FALLO;
//         }

//         if (leidos > 0) {
//             write(1, buffer, leidos);  // Imprimir el contenido leído en la salida estándar
//             total_leidos += leidos;
//             offset += leidos;
//         }
//     }

//     // Mostrar cantidad total de bytes leídos
//     printf("Bytes leídos: %d\n", total_leidos);

//     // Desmontar el sistema de archivos
//     if (bumount() == -1) {
//         fprintf(stderr,RED "Error: mi_cat.c -> main() -> bumount() == FALLO" RESET);
//         return FALLO;
//     }

// >>>>>>> d9edd3f560facaef2121a39d30487480d2f3b593
    return EXITO;
}