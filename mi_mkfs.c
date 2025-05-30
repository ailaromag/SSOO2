/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include <string.h>

#include "ficheros.h"

/**
 * Programa principal que formatea un dispositivo virtual creando un sistema de archivos nuevo.
 * Similar al comando mkfs de Unix, inicializa todas las estructuras de metadatos del sistema.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es la ruta del dispositivo virtual
 *         y argv[2] el número total de bloques del sistema
 * 
 * Return:
 * - EXITO (0) si formatea el sistema correctamente
 * - FALLO (-1) si error en argumentos, montaje, inicialización de estructuras,
 *              reserva del directorio raíz o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 2 argumentos
    if (argc != 3) {
        fprintf(stderr, RED "Error: mi_mkfs.c -> main() -> argc != 3\n" RESET);
        return FALLO;
    }
    
    // Extraer argumentos de línea de comandos
    char *camino = argv[1];
    int nbloques = atoi(argv[2]);
    
    // Montar el dispositivo virtual para poder formatearlo
    int descriptor;
    unsigned char buf[BLOCKSIZE];
    if ((descriptor = bmount(camino)) == FALLO) {
        fprintf(stderr, RED "Error: mi_mkfs.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Limpiar todos los bloques del dispositivo poniéndolos a cero
    memset(buf, 0, BLOCKSIZE);
    for (int i = 0; i < nbloques; i++) {
        bwrite(i, buf);
    }
    
    // Inicializar el superbloque con la información básica del sistema
    if (initSB(nbloques, nbloques / 4) == FALLO) {
        fprintf(stderr, RED "Error: mi_mkfs.c -> main() -> initSB() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Inicializar el mapa de bits marcando los bloques de metadatos como ocupados
    if (initMB() == FALLO) {
        fprintf(stderr, RED "Error: mi_mkfs.c -> main() -> initMB() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Inicializar el array de inodos creando la lista enlazada de inodos libres
    if (initAI() == FALLO) {
        fprintf(stderr, RED "Error: mi_mkfs.c -> main() -> initAI() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Reservar el primer inodo para el directorio raíz con permisos completos
    if (reservar_inodo('d', 7) == FALLO) {
        fprintf(stderr, RED "Error: mi_mkfs.c -> main() -> reservar_inodo() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Desmontar el dispositivo virtual una vez completado el formateo
    if (bumount() == FALLO) {
        fprintf(stderr, RED "Error: mi_mkfs.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}