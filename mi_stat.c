#include "directorios.h"

/**
 * Programa principal que muestra información estadística detallada de un archivo o directorio.
 * Similar al comando stat de Unix, presenta todos los metadatos del inodo especificado.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es el dispositivo y argv[2] la ruta a consultar
 * 
 * Return:
 * - EXITO (0) si muestra la información correctamente
 * - FALLO (-1) si error en argumentos, montaje, la ruta no existe, consulta o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 2 argumentos
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_stat <disco> <ruta>\n");
        return FALLO;
    }

    // Extraer el nombre del dispositivo de los argumentos
    char *nombre_dispositivo = argv[1];
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_stat.c -> main() -> bmount() == FALLO" RESET);
        return FALLO;
    }

    // Extraer la ruta a consultar de los argumentos
    char *camino = argv[2];
    struct STAT stat;
    
    // Obtener las estadísticas del archivo/directorio especificado
    int n_inodo = mi_stat(camino, &stat);
    if (n_inodo == FALLO) {
        fprintf(stderr, RED "Error: mi_stat.c -> main() -> mi_stat() == FALLO" RESET);
        return FALLO;
    } else {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(n_inodo);
    }

    // Mostrar toda la información estadística del inodo
    printf(BLUE "Nº de inodo: %d\n" RESET, n_inodo);
    printf("tipo: %c\n", stat.tipo);
    printf("permisos: %d\n", stat.permisos);
    
    // Mostrar timestamps formateados (ctime ya incluye salto de línea)
    printf("atime: %s", ctime(&stat.atime));
    printf("mtime: %s", ctime(&stat.mtime));
    printf("ctime: %s", ctime(&stat.ctime));
    printf("btime: %s", ctime(&stat.btime));
    
    // Mostrar información adicional del inodo
    printf("nlinks: %d\n", stat.nlinks);
    printf("tamEnByteLog: %d\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_stat.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
}