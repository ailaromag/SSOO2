/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "directorios.h"

/**
 * Programa principal que cambia los permisos de un archivo o directorio.
 * Similar al comando chmod de Unix, modifica los permisos de acceso de una ruta específica.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 4)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] los permisos (0-7) y argv[3] la ruta
 * 
 * Return:
 * - EXITO (0) si cambia los permisos correctamente
 * - FALLO (-1) si error en argumentos, permisos inválidos, montaje, cambio de permisos o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 3 argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n" RESET);
        return FALLO;
    }

    // Convertir los permisos de string a entero y validar rango
    int permisos = atoi(argv[2]);
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos);
        return FALLO;
    }

    // Extraer el nombre del dispositivo de los argumentos
    char *nombre_dispositivo = argv[1];
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_chmod.c -> main() -> bmount() == FALLO" RESET);
        return FALLO;
    }

    // Extraer la ruta del archivo/directorio cuyos permisos cambiar
    const char *camino = argv[3];
    
    // Intentar cambiar los permisos del archivo/directorio especificado
    int error = mi_chmod(camino, permisos);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_chmod.c -> main() -> mi_chmod() == FALLO" RESET);
        return FALLO;
    } else {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
    }

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_chmod.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
}