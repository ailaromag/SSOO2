#include "ficheros_basico.h"

/**
 * Calcula el tamaño en bloques necesario para el mapa de bits.
 */
int tamMB(unsigned int nbloques) {
    int tamMB = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques / 8) % BLOCKSIZE != 0) {
        tamMB++;
    }
    return tamMB;
}

/**
 * Calcula el tamaño en bloques del array de inodos.
 */
int tamAI(unsigned int ninodos) {
    int tamAI = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0) {
        return tamAI++;
    }
    return tamAI;
}

/**
 * Inicializa los datos del superbloque.
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    struct superbloque SB;
    // inicialización de los atributos
    SB.posPrimerBloqueMB = posSB + tamSB;  // posSB = 0, tamSB = 1
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // escribirlo en el fichero
    if (bwrite(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initSB() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Inicializa el mapa de bits poniendo a 1 los bits que representan los metadatos.
 */
int initMB() {
    // Hay que leer el superbloque del sistema:
    struct superbloque SB;
    int bytesLeidos = bread(posSB, &SB);
    if (bytesLeidos == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initMB -> bread()== FALLO");
        printf(RESET);
        return FALLO;
    }

    int tamMetadatos = tamSB + tamMB(SB.totBloques) + tamAI(SB.totInodos);

    // int nBloques = (tamMetadatos / 8) / BLOCKSIZE; // esto nos da el valor de el último bloque que se rellenara
    int bytesAuno = tamMetadatos / 8;
    int bytesUltimoBloque = tamMetadatos % 8;
    unsigned char bufMB[bytesAuno];

    for (int i = 0; i < bytesAuno; i++) {
        bufMB[i] = 255;
    }
    if (bytesUltimoBloque != 0) {
        unsigned char mask = ((1 << bytesUltimoBloque) - 1) << (8 - bytesUltimoBloque);  // si bytesUltimoBloque = 3, 00001000 => 00000111 => 11100000
        bufMB[bytesAuno] = mask;
    }
    // Hay que restar los bloques que ocupan los metadatos
    // de los bloques libres:
    SB.cantBloquesLibres -= tamMetadatos;

    // Escribir el mapa de bits en el dispositivo
    if (bwrite(SB.posPrimerBloqueMB, bufMB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initMB() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }

    // Escribir el superbloque actualizado en el dispositivo
    if (bwrite(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initMB() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Inicializa la lista de inodos libres, al principio enlaza todos los inodos secuencialmente ya que esta todo libre.
 */
int initAI() {
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    struct superbloque SB;

    // Leer el superbloque del dispositivo virtual:
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initAI() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    SB.posPrimerInodoLibre = 0;

    int contInodos = SB.posPrimerInodoLibre + 1;                          // si hemos inicializado SB.posPrimerInodoLibre = 0
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {  // para cada bloque del AI
        // leer el bloque de inodos i  en el dispositivo virtual
        if (bread(i, inodos) == FALLO) {
            perror(RED "Error: ficheros_basico.c -> initAI() -> bread() == FALLO");
            printf(RESET);
            return FALLO;
        }
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {    // para cada inodo del bloque
            inodos[j].tipo = 'l';                            // libre
            if (contInodos < SB.totInodos) {                 // si no hemos llegado al último inodo del AI
                inodos[j].punterosDirectos[0] = contInodos;  // enlazamos con el siguiente
                contInodos++;
            } else {  // hemos llegado al último inodo
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }
        // escribir el bloque de inodos i  en el dispositivo virtual
        if (bwrite(i, inodos) == FALLO) {
            perror(RED "Error: ficheros_basico.c -> initAI() ->  bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
    }
    return EXITO;
}

/**
 * Lee un determinado bit del MB y devuele el valor del bit leído
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;
    int nbloqueMB = posbyte / BLOCKSIZE;
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];
    posbyte = posbyte % BLOCKSIZE;
    // Leemos el bloque físico que contiene el bit:
    if (bread(nbloqueabs, &bufferMB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Escribimos el bit en el buffer
    unsigned char mascara = 0b10000000;
    mascara >>= posbit;
    if (bit == 1) {
        bufferMB[posbyte] |= mascara;
    } else if (bit == 0) {
        bufferMB[posbyte] &= ~mascara;
    } else {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bit != 0 ni 1");
        printf(RESET);
        return FALLO;
    }

    // Escribimos el buffer en el dispositivo virtual:
    if (bwrite(nbloqueabs, bufferMB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initAI -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Lee un determinado bit del MB y devuelve su valor.
 */
char leer_bit(unsigned int nbloque) {
    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;
    int nbloqueMB = posbyte / BLOCKSIZE;
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];
    posbyte = posbyte % BLOCKSIZE;
    // Leemos el bloque físico:
    if (bread(nbloqueabs, &bufferMB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Leemos el bit
    unsigned char mascara = 0b10000000;
    mascara >>= posbit;
    mascara &= bufferMB[posbyte];
    mascara >>= (7 - posbit);

    return mascara;
}

/**
 * Encuentra el primer bloque libre, consultando el MB (primer bit a 0),
 * lo ocupa (poniendo el correspondiente bit a 1 con la ayuda de la
 * función escribir_bit()) y devuelve su posición.
 */
int reservar_bloque() {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if (SB.cantBloquesLibres <= 0) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> SB.cantBloquesLibres <= 0");
        printf(RESET);
        return FALLO;
    }
    int nbloqueMB = SB.posPrimerBloqueMB;
    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];
    memset(bufferAux, 255, BLOCKSIZE);
    bool bloqueLibreEncontrado = false;
    // Localizamos el 1er bloque libre:
    while (!bloqueLibreEncontrado && nbloqueMB <= SB.posUltimoBloqueMB) {
        if (bread(nbloqueMB, bufferMB) == FALLO) {
            perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO");
            printf(RESET);
            return FALLO;
        }
        if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0) {
            bloqueLibreEncontrado = true;
        } else {
            nbloqueMB++;
        }
    }
    bool byteACeroEncontrado = false;
    int posbyte = 0;
    // Localizamos el 1er byte con algún 0
    while (!byteACeroEncontrado && posbyte < BLOCKSIZE) {
        if (bufferMB[SB.posPrimerBloqueMB + posbyte] != 255) {
            byteACeroEncontrado = true;
        } else {
            posbyte++;
        }
    }
    // Localizamos el bit que vale 0 dentro del byte
    unsigned char mascara = 0b10000000;
    int posbit = 0;
    while (bufferMB[posbyte] & mascara) {
        bufferMB[posbyte] <<= 1;
        posbit++;
    }
    // Finalmente calculamos el bloque físico que podemos reservar:
    int nbloque = (nbloqueMB * BLOCKSIZE * BYTE_SIZE) + (posbyte * BYTE_SIZE) + posbit;

    if (escribir_bit(nbloque, 1) == FALLO) {
        return FALLO;
    }
    // revervamos la zona a
    unsigned char bufferCeros[BLOCKSIZE];
    memset(bufferCeros, 0, BLOCKSIZE);
    if (bwrite(nbloque, bufferCeros) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_bit() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return nbloque;
}

/**
 * Libera un bloque determinado.
 */
int liberar_bloque(unsigned int nbloque) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_bloque -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if (escribir_bit(nbloque, 0) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_bloque -> escribir_bit() == FALLO");
        printf(RESET);
        return FALLO;
    }
    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_bloque -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return nbloque;
}
/**
 * Escribimos el inodo en la posición correspondiente del array de inodos.
 */
int escribir_inodo(unsigned int ninodo, struct inodo *inodo){
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_inodo() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    int posBloqueInodo = SB.posPrimerBloqueAI + (ninodo * INODOSIZE) / BLOCKSIZE;
    int posInodo = (ninodo * INODOSIZE) % BLOCKSIZE;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(posBloqueInodo, inodos) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_inodo() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    inodos[posInodo / INODOSIZE] = *inodo;
    if (bwrite(posBloqueInodo, inodos) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> escribir_inodo() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}
/**
 * Leemos el inodo en la posición correspondiente del array de inodos.
 */
int leer_inodo(unsigned int ninodo, struct inodo *inodo){
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> leer_inodo() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    int posBloqueInodo = SB.posPrimerBloqueAI + (ninodo * INODOSIZE) / BLOCKSIZE;
    int posInodo = (ninodo * INODOSIZE) % BLOCKSIZE;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(posBloqueInodo, inodos) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> leer_inodo() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    *inodo = inodos[posInodo / INODOSIZE];
    return EXITO;
}
int reservar_inodo(unsigned char tipo, unsigned char permisos){
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> reservar_inodo() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if (SB.cantInodosLibres <= 0) {
        perror(RED "Error: ficheros_basico.c -> reservar_inodo() -> SB.cantInodosLibres <= 0");
        printf(RESET);
        return FALLO;
    }

    // Preparamos el inodo a escribir
    int posInodoReservado = SB.posPrimerInodoLibre;
    struct inodo inodoReservado;
    inodoReservado.tipo = tipo;
    inodoReservado.permisos = permisos;
    inodoReservado.nlinks = 1;
    inodoReservado.tamEnBytesLog = 0;
    inodoReservado.atime = time(NULL);
    inodoReservado.mtime = time(NULL);
    inodoReservado.ctime = time(NULL);
    inodoReservado.btime = time(NULL);
    inodoReservado.numBloquesOcupados = 0;
    for (int i = 0; i < sizeof(inodoReservado.punterosDirectos) / sizeof(inodoReservado.punterosDirectos[0]); i++) {
        inodoReservado.punterosDirectos[i] = 0;
    }
    for (int i = 0; i < sizeof(inodoReservado.punterosIndirectos) / sizeof(inodoReservado.punterosIndirectos[0]); i++) {
        inodoReservado.punterosIndirectos[i] = 0;
    }

    // Antes de escribit apuntamos al siguiente inodo libre
    struct inodo inodoLibre;
    if (leer_inodo(SB.posPrimerInodoLibre, &inodoLibre) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> reservar_inodo() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    SB.posPrimerInodoLibre = inodoLibre.punterosDirectos[0];    // Apuntamos al siguiente inodo libre
    SB.cantInodosLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> reservar_inodo() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Escribimos el inodo
    if (escribir_inodo(posInodoReservado, &inodoReservado) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> reservar_inodo() -> escribir_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return posInodoReservado;
}