/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "directorios.h"

/**
 * Programa principal que crea un enlace duro entre dos rutas de archivos.
 * Similar al comando ln de Unix, crea una nueva entrada de directorio que apunta al mismo inodo.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 4)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] la ruta del fichero original
 *         y argv[3] la ruta del nuevo enlace a crear
 * 
 * Return:
 * - EXITO (0) si crea el enlace correctamente
 * - FALLO (-1) si error en argumentos, montaje, el fichero original no existe/no es fichero,
 *              creación del enlace o desmontaje
 */
int main(int argc, char** argv) {
    // Verificar que se proporcionan exactamente 3 argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_link disco /ruta_fichero_original /ruta_enlace\n" RESET);
        return FALLO;
    }
    
    // Extraer argumentos de línea de comandos
    char *disco = argv[1];
    char *ruta_fichero_original = argv[2];
    char *ruta_enlace = argv[3];
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Verificar que el archivo original existe y obtener sus metadatos
    struct STAT stat;
    int error = mi_stat(ruta_fichero_original, &stat);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    
    // Verificar que la ruta original corresponde a un fichero (no a un directorio)
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
        return FALLO;
    }
    
    // Crear el enlace duro entre el archivo original y la nueva ruta
    error = mi_link(ruta_fichero_original, ruta_enlace);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> mi_link() == FALLO\n" RESET);
        return FALLO;
    } else if (error < 0) {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    
    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_link.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}