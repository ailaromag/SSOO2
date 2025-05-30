#include "directorios.h"

/**
 * Programa principal que prueba el funcionamiento de la caché LRU con múltiples archivos.
 * Crea varios ficheros y realiza accesos en un orden específico para demostrar el algoritmo LRU.
 *
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es el dispositivo y argv[2] el texto a escribir
 *
 * Return:
 * - void (función main sin return explícito)
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 2 argumentos
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./prueba_cache_tabla <nombre_dispositivo> <texto>\n" RESET);
        exit(FALLO);
    }
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    bmount(argv[1]);

    // Extraer el texto a escribir y calcular su longitud
    char *buffer_texto = argv[2];
    int nbytes = strlen(buffer_texto);
    int offset = 0;
    
    // Definir arrays con las rutas de los ficheros y el orden de acceso para la prueba
    char rutas[5][6] = {"/fic1", "/fic2", "/fic3", "/fic4", "/fic5"};
    char orden_acceso[12][6] = {"/fic2", "/fic3", "/fic2", "/fic1", "/fic5", "/fic2", "/fic4", "/fic5", "/fic3", "/fic2", "/fic5", "/fic2"};

    // Crear los 5 ficheros de prueba con permisos de lectura/escritura
    for (int i = 0; i < 5; i++) {
        mi_creat(rutas[i], 6);
        fprintf(stderr, MAGENTA "[prueba cache ()→ creado %s]\n" RESET, rutas[i]);
    }
    
    // Realizar 12 operaciones de escritura siguiendo el orden específico para probar la caché LRU
    for (int j = 0; j < 12; j++) {
        fprintf(stderr, CYAN "\n[prueba cache ()→ acceso a %s]\n" RESET, orden_acceso[j]);
        mi_write(orden_acceso[j], buffer_texto, offset, nbytes);
    }

    // Desmontar el dispositivo virtual antes de terminar
    bumount();
}