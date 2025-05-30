#include "directorios.h"

#define DEBUGN9 true

/**
 * Programa principal que escribe un texto en un archivo a partir de una posición específica.
 * Similar al comando de escritura de Unix, permite escribir datos en cualquier offset del archivo.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 5)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] la ruta del fichero,
 *         argv[3] el texto a escribir y argv[4] el offset donde escribir
 * 
 * Return:
 * - EXITO (0) si escribe correctamente
 * - FALLO (-1) si error en argumentos, montaje, la ruta no es fichero, escritura o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 4 argumentos
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n" RESET);
        return FALLO;
    }
    
    // Extraer argumentos de línea de comandos
    char *disco = argv[1];
    char *ruta_fichero = argv[2];
    char *texto = argv[3];
    int offset = atoi(argv[4]);
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Obtener metadatos del archivo para verificar que existe y es un fichero
    struct STAT stat;
    if (mi_stat(ruta_fichero, &stat) == FALLO) {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Verificar que la ruta corresponde a un fichero (no a un directorio)
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
        return FALLO;
    }
    
#if DEBUGN9
    // Mostrar información de depuración sobre la longitud del texto
    printf("longitud texto: %ld\n", strlen(texto));
#endif
    
    // Intentar escribir el texto en el archivo en la posición especificada
    int error = mi_write(ruta_fichero, texto, offset, strlen(texto));
    if (error == FALLO) {
        // Si no tiene permiso de escritura, establecer bytes escritos = 0
        error = 0;
    } else if (error < 0) {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
        return FALLO;
    }
    
#if DEBUGN9
    // Mostrar información de depuración sobre los bytes escritos
    printf("Bytes escritos: %d\n", error);
#endif
    
    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_escribir.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}