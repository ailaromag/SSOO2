#include "ficheros.h"

#define DEBUGN5 true

int main(int argc, char **argv) {
#if DEBUGN5
    if (argc != 4) {
        perror(RED "Sintaxis: escribir <nombre_dispositivo> <\"$(cat fichero)\">  <diferentes_inodos>\n Offsets: 9000, 209000, 30725000, 409605000, 480000000 \n Si diferentes_inodos=0 se reserva solo un inodo para todos los offsets\n");
        printf(RESET);
        return FALLO;
    }
    char *camino = argv[1];
    char *texto = argv[2];
    int longitud = strlen(texto);
    if (bmount(camino) == FALLO) {
        perror("Error al montar el dispositivo");
        return FALLO;
    }
    int diferentes_inodos = atoi(argv[3]);
    // Offsets para probar los punteros:
    int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};
    int num_offsets = sizeof(offsets) / sizeof(offsets[0]);

    printf("longitud texto: %d\n\n", longitud);  // Imprimir longitud del texto

    int nInodoReservado = 0;
    int InodoReservado;

    if (diferentes_inodos == 0) {
        InodoReservado = reservar_inodo('f', 6);
        if (InodoReservado == FALLO) {
            perror(RED "Error: escribir.c -> main() -> reservar_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }
        nInodoReservado++;
        printf("Nº inodo reservado: %d\n", nInodoReservado);
    }

    for (int i = 0; i < num_offsets; i++) {
        if (diferentes_inodos == 1) {
            InodoReservado = reservar_inodo('f', 6);
            if (InodoReservado == FALLO) {
                perror(RED "Error: escribir.c -> main() -> reservar_inodo() == FALLO");
                printf(RESET);
                return FALLO;
            }
            nInodoReservado++;
            printf("Nº inodo reservado: %d\n", nInodoReservado);
        }
        printf("offset: %d\n", offsets[i]);  // Imprimir offset

        int primerBL = offsets[i] / BLOCKSIZE;
        int nbfisico = traducir_bloque_inodo(nInodoReservado, primerBL, 1);  // Llamada real
        if (nbfisico == FALLO) {
            perror(RED "Error: escribir.c -> main() -> traducir_bloque_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }

        int bytes_escritos = mi_write_f(nInodoReservado, texto, offsets[i], longitud);
        if (bytes_escritos == FALLO) {
            perror(RED "Error: escribir.c -> main() -> mi_write_f() == FALLO");
            printf(RESET);
            return FALLO;
        }
        printf("Bytes escritos: %d\n", bytes_escritos);

        struct STAT stat;
        mi_stat_f(nInodoReservado, &stat);  // Llamada real
        printf("stat.tamEnBytesLog=%d\n", stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%d\n\n", stat.numBloquesOcupados);
    }

    if (bumount() == FALLO) {
        perror(RED "Error al desmontar el dispositivo");
        return FALLO;
    }
#endif

    return EXITO;
}
