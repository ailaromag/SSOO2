#include "ficheros.h"

/**
 * Programa principal que cambia los permisos de un inodo específico del sistema de archivos.
 * Utilidad de bajo nivel que opera directamente sobre números de inodo sin usar rutas.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 4)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] el número de inodo
 *         y argv[3] los nuevos permisos (0-7)
 * 
 * Return:
 * - EXITO (0) si cambia los permisos correctamente
 * - FALLO (-1) si error en argumentos, montaje, cambio de permisos o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 3 argumentos
    if (argc != 4) {
        perror(RED "Sintaxis: permitir <nombre_dispositivo> <ninodo> <permisos>\n");
        return FALLO;
    }

    // Extraer argumentos de línea de comandos
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned char permisos = atoi(argv[3]);

    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(nombre_dispositivo) == FALLO) {
        perror(RED "Error al montar el dispositivo\n");
        return FALLO;
    }

    // Cambiar los permisos del inodo especificado
    if (mi_chmod_f(ninodo, permisos) == FALLO) {
        perror(RED "Error al cambiar los permisos del inodo \n");
        bumount();
        return FALLO;
    }

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount() == FALLO) {
        perror(RED "Error al desmontar el dispositivo\n");
        return FALLO;
    }

    return 0;
}