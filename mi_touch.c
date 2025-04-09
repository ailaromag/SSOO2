#include "directorios.h"

bool ends_with_slash_ignore_spaces (const char *str);

bool ends_with_slash_ignore_spaces (const char *str) {
    if (str == NULL || str[0] == '\0') {
        return false;
    }
    int i = strlen(str) - 1;
    while(i >= 0 && isspace((unsigned char) str[i])) i--;
    return (i > 0 && str[i] == '/');
}

int main(int argc, char **argv) {
    // Mira si tiene 4 argumentos incluidos el nombre de programa
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_touch <nombre_dispositivo> <permisos> </ruta_fichero>\n" RESET);
        return FALLO;
    }
    // Convertimos los permisos de string a int
    int permisos = atoi(argv[2]);
    if(permisos < 0 || permisos > 7) {
        fprintf(stderr, RED "Error: mi_touch.c -> main() -> permiso < 0 || permiso > 7" RESET);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_touch.c -> main() -> bmount() == FALLO" RESET);
        return FALLO;
    }

    const char *camino = argv[3];
    if (ends_with_slash_ignore_spaces(camino) == true) {
        fprintf(stderr, RED "Error: No es un fichero, acaba en '/'. Para crear fichero use mi_mkdir.\n" RESET);
        return FALLO;
    }

    int error = mi_creat(camino, permisos);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_touch.c -> main() -> mi_creat() == FALLO" RESET);
        return FALLO;
    } else {
        mostrar_error_buscar_entrada(error);
    }

    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_touch.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
}