/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "directorios.h"

#define TAMFILA 100
#define TAMBUFFER (TAMFILA * 1000)

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
 * Programa principal que lista el contenido de un directorio o muestra información de un archivo.
 * Similar al comando ls de Unix, soporta formato simple y detallado (-l).
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3 o 4)
 * - argv: array de argumentos donde opcionalmente argv[1] puede ser "-l",
 *         argv[n] es el dispositivo y argv[n+1] la ruta a listar
 * 
 * Return:
 * - EXITO (0) si lista correctamente
 * - FALLO (-1) si error en argumentos, flag inválido, montaje, listado o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan 2 o 3 argumentos (con o sin -l)
    if (argc != 3 && argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_ls [-l] <nombre_dispositivo> </ruta>\n" RESET);
        return FALLO;
    }

    // Variables para manejar argumentos opcionales
    int argv_offset = 0;
    char flag = 0;
    
    // Verificar si se especifica el flag -l para formato detallado
    if (argc == 4) {
        if (strcmp(argv[1], "-l") != 0) {
            fprintf(stderr, RED "Sintaxis: el primer argumento opcional debe ser -l\n" RESET);
            return FALLO;
        }
        argv_offset = 1;  // Ajustar índices de argumentos
        flag = 1;         // Activar formato detallado
    }
    
    // Extraer el nombre del dispositivo considerando el offset
    char *nombre_dispositivo = argv[1 + argv_offset];
    
    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_ls.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }

    // Extraer la ruta a listar considerando el offset
    const char *camino = argv[2 + argv_offset];

    // Determinar si la ruta se refiere a un fichero o directorio
    char tipo = 'f';  // Por defecto asumir que es un fichero
    if (ends_with_slash_ignore_spaces(camino)) {
        tipo = 'd';   // Si termina en '/', es un directorio
    }

    // Buffer para almacenar la salida del listado
    char output[TAMBUFFER];
    
    // Obtener el listado del directorio o información del archivo
    int error = mi_dir(camino, output, tipo, flag);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_ls.c -> main() -> mi_dir() == FALLO\n" RESET);
        return FALLO;
    } else {
        // Si hay un error específico de buscar_entrada, mostrarlo
        mostrar_error_buscar_entrada(error);
    }

    // Mostrar el resultado del listado
    printf("%s", output);

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_ls.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}