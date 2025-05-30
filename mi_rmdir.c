#include "directorios.h"

/**
 * Programa principal que elimina un directorio vacío del sistema de archivos.
 * Similar al comando rmdir de Unix, elimina directorios que no contienen entradas.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es el dispositivo y argv[2] la ruta del directorio a eliminar
 * 
 * Return:
 * - EXITO (0) si elimina el directorio correctamente
 * - FALLO (-1) si error en argumentos, montaje, la ruta no existe/no es directorio,
 *              el directorio no está vacío, eliminación o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 2 argumentos
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_rmdir disco /ruta\n");
        return FALLO;
    }
    
    // Extraer argumentos de línea de comandos
    char *disco = argv[1];
    char *ruta = argv[2];
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_rmdir.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Verificar que la ruta existe y obtener sus metadatos
    struct STAT stat;
    int error = mi_stat(ruta, &stat);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_rmdir.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    
    // Verificar que la ruta corresponde a un directorio (no a un fichero)
    if (stat.tipo != 'd') {
        fprintf(stderr, RED "Error: mi_rmdir.c -> main() -> El inodo de la ruta no es un directorio\n" RESET);
        return FALLO;
    }
    
    // Eliminar el directorio (desvincularlo de su directorio padre)
    error = mi_unlink(ruta);
    if (error == FALLO) {
        // fprintf(stderr, RED "Error: mi_rmdir.c -> main() -> mi_unlink() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    
    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_rmdir.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}