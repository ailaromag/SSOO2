#include "ficheros_basico.h"

void mostrar_sf(char *nombre_dispositivo) {
    struct superbloque SB;
    
    // Montar el dispositivo
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, "Error en bmount\n");
        return;
    }

    // Leer el superbloque
    if (bread(posSB, &SB) == -1) {
        fprintf(stderr, "Error en bread\n");
        bumount();
        return;
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
    int siguiente = SB.posInodoRaiz + SB.posPrimerBloqueAI;
    struct inodo inodo;
    int count = 0;
    
    while (siguiente != -1 && count < SB.totInodos) {
        if (count > 0 && count % 28 == 0) {
            printf("…\n… ");
        }
        printf("%d ", siguiente);
        if (bread(siguiente, &inodo) == -1) {
            fprintf(stderr, "Error en leer_inodo\n");
            break;
        }
        siguiente = inodo.punterosDirectos[0];
        count++;
    }

    // Desmontar el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, "Error en bumount\n");
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Sintaxis: %s <dispositivo>\n", argv[0]);
        return -1;
    }

    mostrar_sf(argv[1]);
    return 0;
}