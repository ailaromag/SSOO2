/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "directorios.h"

/**
 * Programa principal que elimina un archivo del sistema de archivos.
 * Similar al comando rm de Unix, elimina la entrada de directorio y libera el inodo si es el último enlace.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es el dispositivo y argv[2] la ruta del fichero a eliminar
 * 
 * Return:
 * - EXITO (0) si elimina el archivo correctamente
 * - FALLO (-1) si error en argumentos, montaje, el archivo no existe/no es fichero,
 *              eliminación del enlace o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 2 argumentos
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_rm disco /ruta\n");
        return FALLO;
    }
    
    // Extraer argumentos de línea de comandos
    char *disco = argv[1];
    char *ruta = argv[2];
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Verificar que el archivo existe y obtener sus metadatos
    struct STAT stat;
    int error = mi_stat(ruta, &stat);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    
    // Verificar que la ruta corresponde a un fichero (no a un directorio)
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
        return FALLO;
    }
    
    // Eliminar el enlace al archivo (desvincularlo del directorio)
    error = mi_unlink(ruta);
    if (error == FALLO) {
        // fprintf(stderr, RED "Error: mi_rm.c -> main() -> mi_unlink() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    
    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_rm.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}