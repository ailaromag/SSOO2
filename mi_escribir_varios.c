/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "directorios.h"

/**
 * Programa principal que escribe un texto múltiples veces en un archivo con desplazamientos.
 * Escribe el mismo texto 10 veces consecutivas, cada una desplazada un bloque respecto a la anterior.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 5)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] la ruta del fichero,
 *         argv[3] el texto a escribir y argv[4] el offset inicial
 * 
 * Return:
 * - 0 si escribe correctamente
 * - -1 si error en argumentos, montaje, la ruta es directorio o escritura
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 4 argumentos
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: mi_escribir <nombre_dispositivo> </ruta_fichero> <texto> <offset>\n" RESET);
        exit(-1);
    }

    // struct STAT stat;

    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(argv[1]) < 0) return -1;
    
    // Extraer el texto a escribir y calcular su longitud
    char *buffer_texto = argv[3];
    int longitud = strlen(buffer_texto);

    // Verificar que la ruta no corresponde a un directorio (no termina en '/')
    if (argv[2][strlen(argv[2]) - 1] == '/') {
        fprintf(stderr, RED "Error: la ruta se corresponde a un directorio.\n" RESET);
        exit(-1);
    }
    char *camino = argv[2];
    
    // Extraer el offset inicial desde donde comenzar a escribir
    unsigned int offset = atoi(argv[4]);
    
    // Variables para controlar las escrituras múltiples
    // fprintf(stderr, MAGENTA "camino: %s\n" RESET, camino);
    // fprintf(stderr, MAGENTA "buffer_texto: %s\n" RESET, buffer_texto);
    // fprintf(stderr, MAGENTA "offset: %d\n" RESET, offset);
    int escritos = 0;
    int varios = 10;
    fprintf(stderr, ROSE "longitud texto: %d\n" RESET, longitud);
    
    // Escribir el mismo texto 10 veces, cada vez desplazado un bloque completo
    for (int i = 0; i < varios; i++) {
        // Escribir el texto en posiciones separadas por BLOCKSIZE bytes
        escritos += mi_write(camino, buffer_texto, offset + BLOCKSIZE * i, longitud);
    }
    
    // Mostrar estadísticas de la operación de escritura
    fprintf(stderr, ROSE "Bytes escritos: %d\n" RESET, escritos);
    
    /* Visualización del stat
    mi_stat_f(ninodo, &stat);
    printf("stat.tamEnBytesLog=%d\n",stat.tamEnBytesLog);
    printf("stat.numBloquesOcupados=%d\n",stat.numBloquesOcupados);
    */

    // Desmontar el dispositivo virtual antes de terminar
    bumount();
}
