#include "ficheros_basico.h"

int mostrar_sf(char *nombre_dispositivo) {
    struct superbloque SB;

    // Montar el dispositivo
    if (bmount(nombre_dispositivo) == FALLO) {
        perror(RED "Error: leer_sf.c, mostrar_sf(), bmount() == FALLO\n");
        printf(RESET);
        return FALLO;
    }

    // Leer el superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c, mostrar_sf(), bread() == FALLO\n");
        printf(RESET);
        bumount();
        return FALLO;
    }

    // Mostrar información del superbloque
    printf("DATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
    printf("totBloques = %d\n", SB.totBloques);
    printf("totInodos = %d\n\n", SB.totInodos);

    printf("sizeof struct superbloque: %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo: %lu\n\n", sizeof(struct inodo));

    // Mostrar lista enlazada de inodos libres
    printf("RECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n");
    struct inodo inodos[BLOCKSIZE / INODOSIZE];  // Only need one block at a time

    // Read blocks one at a time
    int actual;
    int siguiente = SB.posPrimerInodoLibre;
    struct inodo inodo;
    int count = 0;
    bool esSecuencial = true;

    while ((siguiente != UINT_MAX) && (count < SB.totInodos)) {
        int nbloque = SB.posPrimerBloqueAI + (siguiente * INODOSIZE) / BLOCKSIZE;
        int posInodo = (siguiente * INODOSIZE) % BLOCKSIZE;
        // Read the block containing the inode
        if (bread(nbloque, inodos) == -1) {
            perror(RED "Error: leer_sf.c, mostrar_sf(), while(), bread() == FALLO\n");
            printf(RESET);
            break;
        }

        inodo = inodos[posInodo / INODOSIZE];
        actual = siguiente;
        siguiente = inodo.punterosDirectos[0];
        if (siguiente != UINT_MAX) {
            esSecuencial = esSecuencial && (siguiente - actual) == 1;
            // Mostrar una parte de la lista
            if (siguiente <= 10) {
                printf("%d ", siguiente);
            }
            if (siguiente == 10) {
                printf("\n...\n");
            }
            if (siguiente >= SB.totInodos - 10) {
                printf("%d ", siguiente);
            }
        }
        count++;
    }
    printf("\nTest hecho sobre la secuencialidad de la lista AI ha pasado: %s\n", esSecuencial ? "Sí" : "No");
    // Desmontar el dispositivo
    if (bumount() == -1) {
        perror(RED "Error: leer_sf.c, mostrar_sf(), bumount() == FALLO\n");
        printf(RESET);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        perror(RED "Error: leer_sf.c, main(), argc != 2\n");
        printf(RESET);
        return FALLO;
    }

    if (mostrar_sf(argv[1]) == FALLO) {
        perror(RED "Error: leer_sf.c, main(), mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}