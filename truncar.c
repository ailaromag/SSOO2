#include "ficheros.h"

#define DEBUGN6 true

int main(int argc, char **argv) {
#if DEBUGN6
    // Validación del sintaxis
    if (argc != 4) {
        perror(RED "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n");
        printf(RESET);
        return FALLO;
    }
    char *camino = argv[1];
    int nindo = atoi(argv[2]);
    if (nindo < 0) {
        perror(RED "Error: escribir.c -> main() -> diferentes_inodos != 0 && diferentes_inodos != 1");
        printf(RESET);
        return FALLO;
    }
    int nbytes = atoi(argv[3]);
    if (nbytes < 0) {
        perror(RED "Error: escribir.c -> main() -> diferentes_inodos != 0 && diferentes_inodos != 1");
        printf(RESET);
        return FALLO;
    }
    // Montar dispositivo virtual
    if (bmount(camino) == FALLO) {
        perror(RED "Error: escribir.c -> main() -> bmount() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Si nbytes = 0 liberar_inodo() si_no mi_truncar_f() fsi
    if (nbytes == 0) {
        if (liberar_inodo(nindo)){
            perror(RED "Error: escribir.c -> main() -> liberar_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }
    } else {
        if (mi_truncar_f(nindo, nbytes)) {
            perror(RED "Error: escribir.c -> main() -> mi_truncar_f() == FALLO");
            printf(RESET);
            return FALLO;
        }
    }
    // Desmontar dispositivo virtual
    if (bumount(camino) == FALLO) {
        perror(RED "Error: escribir.c -> main() -> bumount() == FALLO");
        printf(RESET);
        return FALLO;
    }
#endif
    return EXITO;
}