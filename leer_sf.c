#include "ficheros_basico.h"

int mostrar_sf() {
    struct superbloque SB;
    // Leer el superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_sf() -> bread() == FALLO\n");
        printf(RESET);
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

    return EXITO;
}

int test_secuencialidad_AI() {
    struct superbloque SB;
    // Leer el superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_sf() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
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
            perror(RED "Error: leer_sf.c -> mostrar_sf() -> while() -> bread() == FALLO\n");
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
    printf("\nTest hecho sobre la secuencialidad de la lista AI ha pasado: %s\n\n", esSecuencial ? "Sí" : "No");
    return EXITO;
}
int reservar_liberar_bloque() {
    printf("RESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    int nBloqueReservado = reservar_bloque();
    if (nBloqueReservado == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> reservar_bloque() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", nBloqueReservado);
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("SB.cantBloquesLibres: %d\n", SB.cantBloquesLibres);
    if (liberar_bloque(nBloqueReservado) == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> liberar_bloque() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("Liberamos ese bloque y después SB.cantBloquesLibres = %d\n\n", SB.cantBloquesLibres);
    return EXITO;
}
int mostrar_directorio_raiz() {
    // Reservamos el primer inodo (directorio raiz)
    int posInodoReservado = reservar_inodo('d', 7);
    if (posInodoReservado == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> reservar_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Leer el superbloque
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Leer el inodo del directorio raiz
    struct inodo inodo;
    if (leer_inodo(SB.posInodoRaiz, &inodo) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> leer_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Leer el contenido del directorio raiz
    printf("DATOS DEL DIRECTORIO RAIZ\n");
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    printf("atime: %ld\n", inodo.atime);
    printf("mtime: %ld\n", inodo.mtime);
    printf("ctime: %ld\n", inodo.ctime);
    printf("btime: %ld\n", inodo.btime);
    printf("nlinks: %d\n", inodo.nlinks);
    printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n\n", inodo.numBloquesOcupados);
    return EXITO;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        perror(RED "Error: leer_sf.c -> main() -> argc != 2\n");
        printf(RESET);
        return FALLO;
    }
    // Montar el dispositivo
    if (bmount(argv[1]) == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> bmount() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Mostrar los atributos básicos
    if (mostrar_sf() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Recorrer el AI y comprobar la secuencialidad
    if (test_secuencialidad_AI() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> test_secuencialidad_AI() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Reservar y liberar un bloque
    if (reservar_liberar_bloque() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> reservar_liberar_bloque() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Mostrar datos del directorio raiz
    if (mostrar_directorio_raiz() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_directorio_raiz() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Desmontar el dispositivo
    if (bumount() == -1) {
        perror(RED "Error: leer_sf.c -> main() -> bumount() == FALLO\n");
        printf(RESET);
    }
    return EXITO;
}