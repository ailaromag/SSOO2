#include "directorios.h"

#define DEBUGTMP false
#define DEBUGN3 false
#define DEBUGN4 false
#define DEBUGN5 false
#define DEBUGN7 true

int mostrar_sf();
int test_secuencialidad_AI();
int reservar_liberar_bloque();
int mostrar_bitmap_bordes_seccion();
int imprimir_info_leer_bit(int pos, int posPrimerBloqueMB);
int mostrar_directorio_raiz();
int mostrar_datos_inodo(int posInodoReservado);
void mostrar_buscar_entrada(char *camino, char reservar);

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
int mostrar_bitmap_bordes_seccion() {
    struct superbloque SB;
    // Leer el superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_bitmap_bordes_seccion() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Mostrar el valor del bitmap de los bordes de las secciones
    printf("MAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
    imprimir_info_leer_bit(posSB, SB.posPrimerBloqueMB);
    printf("posSB: %d -> leer_bit(%d) = %d\n", posSB, posSB, leer_bit(posSB));
    imprimir_info_leer_bit(SB.posPrimerBloqueMB, SB.posPrimerBloqueMB);
    printf("posPrimerBloqueMB: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
    imprimir_info_leer_bit(SB.posUltimoBloqueMB, SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueMB, SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
    imprimir_info_leer_bit(SB.posPrimerBloqueAI, SB.posPrimerBloqueMB);
    printf("posPrimerBloqueAI: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueAI, SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
    imprimir_info_leer_bit(SB.posUltimoBloqueAI, SB.posPrimerBloqueMB);
    printf("posUltimoBloqueAI: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
    imprimir_info_leer_bit(SB.posPrimerBloqueDatos, SB.posPrimerBloqueMB);
    printf("posPrimerBloqueDatos: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
    imprimir_info_leer_bit(SB.posUltimoBloqueDatos, SB.posPrimerBloqueMB);
    printf("posUltimoBloqueDatos: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));
    printf("\n");
    return EXITO;
}
int imprimir_info_leer_bit(int pos, int posPrimerBloqueMB) {
    printf(GRAY "leer_bit(%d) -> postbyte: %d, posbyte (ajustado): %d, posbit: %d, nbloquesMB: %d, nbloqueabs: %d\n",
           pos,
           pos / BYTE_SIZE,
           (pos / BYTE_SIZE) % BLOCKSIZE,
           pos % BYTE_SIZE,
           (pos / BYTE_SIZE) / BLOCKSIZE,
           ((pos / BYTE_SIZE) / BLOCKSIZE) + posPrimerBloqueMB);
    printf(RESET);
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
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    char btime[80];
    printf("DATOS DEL DIRECTORIO RAIZ\n");
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("atime: %s\n", atime);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("mtime: %s\n", mtime);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("ctime: %s\n", ctime);
    ts = localtime(&inodo.btime);
    strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("btime: %s\n", btime);
    printf("nlinks: %d\n", inodo.nlinks);
    printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n\n", inodo.numBloquesOcupados);
    return EXITO;
}

int mostrar_datos_inodo(int posInodoReservado) {
    // Leer el superbloque
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Leer el inodo del directorio raiz
    struct inodo inodo;
    if (leer_inodo(posInodoReservado, &inodo) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> leer_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Leer el contenido del directorio raiz
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    char btime[80];
    printf("DATOS DEL INODO RESERVADO\n");
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("atime: %s\n", atime);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("mtime: %s\n", mtime);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("ctime: %s\n", ctime);
    ts = localtime(&inodo.btime);
    strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("btime: %s\n", btime);
    printf("nlinks: %d\n", inodo.nlinks);
    printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n\n", inodo.numBloquesOcupados);
    return EXITO;
}

void mostrar_buscar_entrada(char *camino, char reservar) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
    }
    printf("**********************************************************************\n");
    return;
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
#if DEBUGTMP
    printf("%d\n", reservar_bloque());
    printf("%d\n", reservar_bloque());
    printf("%d\n", reservar_bloque());
    printf("%d\n", reservar_bloque());
#endif

#if DEBUGN3
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
    // Mostrar el valor bitmap de los bordes de las secciones
    if (mostrar_bitmap_bordes_seccion() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_bitmap_bordes_seccion() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Mostrar datos del directorio raiz
    if (mostrar_directorio_raiz() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_directorio_raiz() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
#endif
#if DEBUGN4
    // Mostrar los atributos básicos
    if (mostrar_sf() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    unsigned int posInodoReservado = reservar_inodo('f', 6);
    if (posInodoReservado == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> reservar_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    int test_set[] = {8, 204, 30004, 400004, 468750};
    for (int ntest = 0; ntest < (sizeof(test_set) / sizeof(test_set[0])); ntest++) {
        if (traducir_bloque_inodo(posInodoReservado, test_set[ntest], 'f') == FALLO) {
            perror(RED "Error: leer_sf.c -> main() -> for(ntest) -> traducir_bloque_inodo() == FALLO\n");
            printf(RESET);
            return FALLO;
        }
        printf("\n");
    }
    if (mostrar_datos_inodo(posInodoReservado) == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> for(ntest) -> mostrar_datos_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    struct superbloque SB;
    // Leer el superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_sf() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("SB.posPrimerinodoLibre = %d\n", SB.posPrimerInodoLibre);
#endif
#if DEBUGN5
    if (mostrar_sf() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
#endif
#if DEBUGN7
    // Mostrar creación directorios y errores
    mostrar_buscar_entrada("pruebas/", 1);            // ERROR_CAMINO_INCORRECTO
    mostrar_buscar_entrada("/pruebas/", 0);           // ERROR_NO_EXISTE_ENTRADA_CONSULTA
    mostrar_buscar_entrada("/pruebas/docs/", 1);      // ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
    mostrar_buscar_entrada("/pruebas/", 1);           // creamos /pruebas/
    mostrar_buscar_entrada("/pruebas/docs/", 1);      // creamos /pruebas/docs/
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1);  // creamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);
    // ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
    mostrar_buscar_entrada("/pruebas/", 1);           // ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/docs/doc1", 0);  // consultamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1);  // ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/casos/", 1);     // creamos /pruebas/casos/
    mostrar_buscar_entrada("/pruebas/docs/doc2", 1);  // creamos /pruebas/docs/doc2

#endif
                                                      //  Desmontar el dispositivo
    if (bumount() == -1) {
        perror(RED "Error: leer_sf.c -> main() -> bumount() == FALLO\n");
        printf(RESET);
    }
    return EXITO;
}