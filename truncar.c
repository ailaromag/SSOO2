/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "ficheros.h"

#define DEBUGN6 true

/**
 * Programa principal que trunca un archivo a un tamaño específico o libera completamente un inodo.
 * Utilidad de bajo nivel que opera directamente sobre números de inodo para testing y debugging.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 4)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] el número de inodo
 *         y argv[3] el nuevo tamaño en bytes (0 para liberar completamente el inodo)
 * 
 * Return:
 * - EXITO (0) si trunca o libera el inodo correctamente
 * - FALLO (-1) si error en argumentos, valores negativos, montaje, truncado/liberación o desmontaje
 */
int main(int argc, char **argv) {
#if DEBUGN6
    // Verificar que se proporcionan exactamente 3 argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n" RESET);
        return FALLO;
    }
    
    // Extraer argumentos de línea de comandos
    char *camino = argv[1];
    int nindo = atoi(argv[2]);
    
    // Validar que el número de inodo no sea negativo
    if (nindo < 0) {
        fprintf(stderr, RED "Error: truncar.c -> main() -> nindo < 0\n" RESET);
        return FALLO;
    }
    
    int nbytes = atoi(argv[3]);
    
    // Validar que el número de bytes no sea negativo
    if (nbytes < 0) {
        fprintf(stderr, RED "Error: truncar.c -> main() -> nbytes < 0\n" RESET);
        return FALLO;
    }
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(camino) == FALLO) {
        fprintf(stderr, RED "Error: truncar.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Decidir entre liberar completamente el inodo o truncar a un tamaño específico
    if (nbytes == 0) {
        // Si nbytes = 0, liberar completamente el inodo y todos sus bloques
        if (liberar_inodo(nindo) == FALLO){
            fprintf(stderr, RED "Error: truncar.c -> main() -> liberar_inodo() == FALLO\n" RESET);
            return FALLO;
        }
    } else {
        // Si nbytes > 0, truncar el archivo al tamaño especificado
        if (mi_truncar_f(nindo, nbytes) == FALLO) {
            fprintf(stderr, RED "Error: truncar.c -> main() -> mi_truncar_f() == FALLO\n" RESET);
            return FALLO;
        }
    }
    
    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(camino) == FALLO) {
        fprintf(stderr, RED "Error: truncar.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
#endif
    return EXITO;
}