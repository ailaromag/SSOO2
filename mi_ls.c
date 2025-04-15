#include "directorios.h"

#define TAMFILA 100
#define TAMBUFFER (TAMFILA * 1000)

bool ends_with_slash_ignore_spaces (const char *str);

bool ends_with_slash_ignore_spaces (const char *str) {
    if (str == NULL || str[0] == '\0') {
        return false;
    }
    int i = strlen(str) - 1;
    while(i >= 0 && isspace((unsigned char) str[i])) i--;
    return (i >= 0 && str[i] == '/');
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_ls [-l] <nombre_dispositivo> </ruta>\n" RESET);
        return FALLO;
    }

    int argv_offset = 0;
    char flag = 0;
    if (argc == 4) {
        if (strcmp(argv[1], "-l") != 0) {
            fprintf(stderr, RED "Sintaxis: el primer argumento opcional debe ser -l\n" RESET);
            return FALLO;
        }
        argv_offset = 1;
        flag = 1;
    }
    
    char *nombre_dispositivo = argv[1 + argv_offset];
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_ls.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }

    const char *camino = argv[2 + argv_offset];

    // Miramos si es un fichero
    char tipo = 'f';
    if (ends_with_slash_ignore_spaces(camino)) {
        tipo = 'd';
    }

    char output[TAMBUFFER];
    int error = mi_dir(camino, output, tipo, flag);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: mi_ls.c -> main() -> mi_dir() == FALLO\n" RESET);
        return FALLO;
    } else {
        mostrar_error_buscar_entrada(error);
    }

    printf("%s", output);

    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_ls.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }
}