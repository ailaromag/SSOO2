#include "ficheros_basico.h"

void mostrar_sf(char *nombre_dispositivo)
{
    struct superbloque SB;

    // Montar el dispositivo
    if (bmount(nombre_dispositivo) == -1)
    {
        fprintf(stderr, "Error en bmount\n");
        return;
    }

    // Leer el superbloque
    if (bread(posSB, &SB) == -1)
    {
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
    struct inodo inodos[BLOCKSIZE / INODOSIZE];  // Only need one block at a time
    
    // Read blocks one at a time
    int siguiente = SB.posPrimerInodoLibre;
    struct inodo inodo;
    int count = 0;
    
    while ((siguiente != UINT_MAX) && (count < SB.totInodos)) {
        int nbloque = SB.posPrimerBloqueAI + (siguiente * INODOSIZE) / BLOCKSIZE;
        int posInodo = (siguiente * INODOSIZE) % BLOCKSIZE;
        // Read the block containing the inode
        if (bread(nbloque, inodos) == -1) {
            fprintf(stderr, "Error reading inode block\n");
            break;
        }
        
        inodo = inodos[posInodo / INODOSIZE];
        siguiente = inodo.punterosDirectos[0];
        printf("%d\n", siguiente);
        count++;
    }

    // Desmontar el dispositivo
    if (bumount() == -1)
    {
        fprintf(stderr, "Error en bumount\n");
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Sintaxis: %s <dispositivo>\n", argv[0]);
        return -1;
    }

    mostrar_sf(argv[1]);
    return 0;
}