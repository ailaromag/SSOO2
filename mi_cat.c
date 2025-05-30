#include "directorios.h"

#define TAMBUFFER (BLOCKSIZE * 4)

/**
 * Programa principal que lee y muestra el contenido de un archivo por su ruta.
 * Similar al comando cat de Unix, lee un archivo del sistema de archivos y lo vuelca a stdout.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es el dispositivo y argv[2] la ruta del fichero
 * 
 * Return:
 * - EXITO (0) si lee el archivo correctamente
 * - FALLO (-1) si error en argumentos, montaje, el archivo no existe/no es fichero, lectura o desmontaje
 */
int main(int argc, char** argv) {
    // Verificar que se proporcionan exactamente 2 argumentos
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_cat <disco> </ruta_fichero>\n" RESET);
    }
    
    // Extraer argumentos de línea de comandos
    char *disco = argv[1];
    char *ruta_fichero = argv[2];
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(disco) == FALLO){
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Obtener metadatos del archivo para verificar que existe y es un fichero
    struct STAT stat;
    if (mi_stat(ruta_fichero, &stat) == FALLO) {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Verificar que la ruta corresponde a un fichero (no a un directorio)
    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> El inodo de la ruta no es un fichero\n" RESET);
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
        leidos = mi_read(ruta_fichero, buffer_texto, offset, TAMBUFFER);
        
        // Verificar que no hay errores en la lectura
        if (leidos == FALLO) {
            fprintf(stderr, RED "Error: mi_cat.c -> main() -> mi_read() == FALLO\n" RESET);
            return FALLO;
        } else if (leidos < 0) {
            // Mostrar error específico si es un código de error de buscar_entrada
            mostrar_error_buscar_entrada(leidos);
            return FALLO;
        }

        // Si se leyeron datos, escribirlos a la salida estándar y actualizar contadores
        if (leidos > 0) {
            fflush(stdout);                      // Asegurar que se escriba inmediatamente
            write(1, buffer_texto, leidos);      // Escribir a stdout (descriptor 1)
            // printf("\n");   // it's used for lvl 9 test
            total_leidos += leidos;              // Acumular bytes leídos
            offset += TAMBUFFER;                 // Avanzar el offset para la siguiente lectura
        }
    }

    // Mostrar estadísticas finales usando stderr para que no interfiera con redirecciones
    fprintf(stderr, "\nTotal_leidos: %d\n", total_leidos);  // it's uses fprintf because when we execute "./mi_cat disco /ruta > output.txt" it doesn't go into output.txt

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(disco) == FALLO) {
        fprintf(stderr, RED "Error: mi_cat.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}