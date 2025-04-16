#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_stat <disco> <ruta>\n");
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_stat.c -> main() -> bmount() == FALLO" RESET);
        return FALLO;
    }

    char *camino = argv[2];
    struct STAT stat;
    int n_inodo = mi_stat(camino, &stat);
    if (n_inodo == FALLO) {
        fprintf(stderr, RED "Error: mi_stat.c -> main() -> mi_stat() == FALLO" RESET);
        return FALLO;
    } else {
        mostrar_error_buscar_entrada(n_inodo);
    }

    printf(BLUE "Nº de inodo: %d\n" RESET, n_inodo);
    printf("tipo: %c\n", stat.tipo);
    printf("permisos: %d\n", stat.permisos);
    printf("atime: %s", ctime(&stat.atime));
    printf("mtime: %s", ctime(&stat.mtime));
    printf("ctime: %s", ctime(&stat.ctime));
    printf("btime: %s", ctime(&stat.btime));
    printf("nlinks: %d\n", stat.nlinks);
    printf("tamEnByteLog: %d\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    if (bumount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: mi_stat.c -> main() -> bumount() == FALLO" RESET);
        return FALLO;
    }
}