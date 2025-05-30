#include "directorios.h"

bool ends_with_slash_ignore_spaces (const char *str);

/**
 * Verifica si una cadena termina en '/' ignorando espacios en blanco al final.
 * Función auxiliar para determinar si una ruta se refiere a un directorio.
 * 
 * Parámetros de entrada:
 * - str: cadena a verificar
 * 
 * Return:
 * - true si la cadena termina en '/' (ignorando espacios)
 * - false si no termina en '/' o es NULL/vacía
 */
bool ends_with_slash_ignore_spaces (const char *str) {
    // Verificar que la cadena no sea NULL ni esté vacía
    if (str == NULL || str[0] == '\0') {
        return false;
    }
    // Buscar el último carácter que no sea espacio en blanco
    int i = strlen(str) - 1;
    while(i >= 0 && isspace((unsigned char) str[i])) i--;
    // Verificar si ese carácter es una barra diagonal
    return (i >= 0 && str[i] == '/');
}

/**
 * Programa principal que crea un nuevo directorio en el sistema de archivos.
 * Similar al comando mkdir de Unix, crea directorios con permisos específicos.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 4)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] los permisos (0-7)
 *         y argv[3] la ruta del directorio a crear (debe terminar en '/')
 * 
 * Return:
 * - EXITO (0) si crea el directorio correctamente
 * - FALLO (-1) si error en argumentos, permisos inválidos, ruta no termina en '/',
 *              montaje, creación del directorio o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 3 argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta_directorio/>\n" RESET);
        return FALLO;
    }
    // Convertir los permisos de string a entero y validar rango
    int permisos = atoi(argv[2]);
    if(permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos);
        return FALLO;
    }

    // Extraer el nombre del dispositivo de los argumentos
    char *nombre_dispositivo = argv[1];
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_mkdir.c -> main() -> bmount() == FALLO" RESET);
        return FALLO;
    }

    // Extraer la ruta del directorio a crear
    const char *camino = argv[3];
    // Verificar que la ruta termina en '/' (requisito para directorios)
    if (ends_with_slash_ignore_spaces(camino) == false) {
        fprintf(stderr, RED "Error: No es un directorio, no acaba en '/'. Para crear fichero use mi_touch.\n" RESET);
        return FALLO;
    }

    // Intentar crear el directorio con los permisos especificados
    int error = mi_creat(camino, permisos);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_mkdir.c -> main() -> mi_creat() == FALLO" RESET);
        return FALLO;
    } else {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
    }

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_mkdir.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
}