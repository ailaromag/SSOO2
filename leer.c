#include "ficheros.h"

#define TAMBUFFER 1500

/**
 * Programa principal que lee y muestra el contenido completo de un archivo del sistema.
 * Lee el archivo por bloques y lo vuelca a la salida estándar, mostrando estadísticas finales.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es el dispositivo y argv[2] el número de inodo
 * 
 * Return:
 * - 0 si lee el archivo correctamente
 * - FALLO (-1) si error en argumentos, montaje, lectura o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 2 argumentos
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./leer <nombre_dispositivo> <ninodo>\n" RESET);
        return FALLO;
    }

    // Extraer argumentos de línea de comandos
    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);

    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    // Leer los metadatos del inodo especificado para verificar que existe
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error al leer el inodo \n" RESET);
        bumount();
        return FALLO;
    }

    // Variables para el bucle de lectura
    unsigned char buffer_texto[TAMBUFFER];
    unsigned int offset = 0;
    int leidos = TAMBUFFER;
    int total_leidos = 0;

    // Leer el archivo completo en bloques de tamaño TAMBUFFER
    while (leidos > 0) {
        // Limpiar el buffer antes de cada lectura
        memset(buffer_texto, 0, TAMBUFFER);
        
        // Leer el siguiente bloque de datos desde el archivo
        leidos = mi_read_f(ninodo, buffer_texto, offset, TAMBUFFER);

        // Verificar que no hay errores en la lectura
        if (leidos < 0) {
            fprintf(stderr, RED "Error en mi_read_f\n" RESET);
            bumount();
            return FALLO;
        }

        // Si se leyeron datos, escribirlos a la salida estándar y actualizar contadores
        if (leidos > 0) {
            write(1, buffer_texto, leidos);  // Escribir a stdout (descriptor 1)
            total_leidos += leidos;          // Acumular bytes leídos
            offset += TAMBUFFER;             // Avanzar el offset para la siguiente lectura
        }
    }

    // Mostrar estadísticas finales de la operación de lectura
    fprintf(stderr, "\nbytes leídos %d\n", total_leidos);
    fprintf(stderr, "tamEnBytesLog %d\n", inodo.tamEnBytesLog);

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount() == FALLO) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return 0;
}