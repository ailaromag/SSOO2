#include "ficheros_basico.h"

#define DEBUG_TRADUCIR_BLOQUE_INODO false
#define DEBUG_LIBERAR_BLOQUES_INODO false

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
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initMB -> bread()== FALLO");
        printf(RESET);
        return FALLO;
    }

    int bloquesMetadatos = tamSB + tamMB(SB.totBloques) + tamAI(SB.totInodos);
    int bloquesMB = bloquesMetadatos / (BYTE_SIZE * BLOCKSIZE);

    unsigned char bufMB[BLOCKSIZE];  // Seran los bits que hay que poner a 1
    memset(bufMB, 255, BLOCKSIZE);
    for (int i = 0; i < bloquesMB; i++) {
        // Escribimos el bloque completo
        if (bwrite(posSB + tamSB + i, bufMB) == FALLO) {
            perror(RED "Error: ficheros_basico.c -> initMB() -> bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
    }

    // Ahora hay que escribir el último bloque
    // Calculamos los bits que quedan sueltos
    int bitsUltimoBloque = bloquesMetadatos % (8 * BLOCKSIZE);
    int bytesLlenos = bitsUltimoBloque / 8;
    int bitsRestantes = bitsUltimoBloque % 8;

    // Reseteamos la memoria del buffer
    memset(bufMB, 0, BLOCKSIZE);
    // Llenamos los bytes completos del último bloque con 1s
    for (int i = 0; i < bytesLlenos; i++) {
        bufMB[i] = 255;
    }

    // Si hay bits restantes, activarlos en el siguiente byte
    if (bitsRestantes > 0) {
        bufMB[bytesLlenos] = (0b11111111 << (8 - bitsRestantes));
    }
    // Escribir el último bloque
    if (bwrite(posSB + tamSB + bloquesMB, bufMB) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> initMB() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }

    SB.cantBloquesLibres -= bloquesMetadatos;
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
    int posbyte = 0;
    // Localizamos el 1er byte con algún 0
    while ((bufferMB[posbyte] == 255) && posbyte < BLOCKSIZE) {
        posbyte++;
    }
    // Localizamos el bit que vale 0 dentro del byte
    unsigned char mascara = 0b10000000;
    int posbit = 0;
    while (bufferMB[posbyte] & mascara) {
        bufferMB[posbyte] <<= 1;

        posbit++;
    }

    // Finalmente calculamos el bloque físico que podemos reservar:
    int nbloque = ((((nbloqueMB - SB.posPrimerBloqueMB) * BLOCKSIZE) + posbyte) * BYTE_SIZE) + posbit;
    // int nbloque = ((((nbloqueMB) * BLOCKSIZE) + posbyte) * BYTE_SIZE) + posbit;
    //////////////////////////////////////////

    if (escribir_bit(nbloque, 1) == FALLO) {
        return FALLO;
    }
    // reservamos la zona a 0
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
int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {
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
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {
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
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
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
    SB.posPrimerInodoLibre = inodoLibre.punterosDirectos[0];  // Apuntamos al siguiente inodo libre
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

/**
 * Función que devuelve  el rango de punteros en que se situa el bloque lógico.
 * Obtenemos la dirección almacenada en el puntero correspondiente del inodo,ptr
 */
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr) {
    if (nblogico < DIRECTOS) {  // <12
        *ptr = inodo->punterosDirectos[nblogico];
        return 0;
    } else if (nblogico < INDIRECTOS0) {  // <268
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    } else if (nblogico < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos[1];  // <65.804
        return 2;
    } else if (nblogico < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos[2];  // <16.843.020
        return 3;
    } else {
        *ptr = 0;
        perror(RED "Error: ficheros_basico.c -> obtener_rangoBL() == BLOQUE LÓGICO FUERA DE RANGO");
        printf(RESET);
        return FALLO;
    }
}

/**
 * Función que generaliza la obtención de los índices de los bloques de punteros
 */
int obtener_indice(unsigned int nblogico, int nivel_punteros) {
    if (nblogico < DIRECTOS) {
        return nblogico;
    } else if (nblogico < INDIRECTOS0) {
        return nblogico - DIRECTOS;
    } else if (nblogico < INDIRECTOS1) {
        if (nivel_punteros == 2) {
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    } else if (nblogico < INDIRECTOS2) {
        switch (nivel_punteros) {
        case 3:
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        case 2:
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        case 1:
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }
    perror(RED "Error: ficheros_basico.c -> obtener_indice() -> nblogico >= INDIRECTOS2");
    printf(RESET);
    return FALLO;
}

/**
 * Función que obtiene el nº de bloque físico correspondiente a un bloque lógico indicado.
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {
    unsigned int ptr, ptr_ant, salvar_inodo;
    int nRangoBL, nivel_punteros, indice;
    unsigned int buffer[NPUNTEROS];
    struct inodo inodo;
    ptr = 0;
    ptr_ant = 0;
    salvar_inodo = 0;
    indice = 0;

    leer_inodo(ninodo, &inodo);
    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);
    nivel_punteros = nRangoBL;
    while (nivel_punteros > 0) {
        if (ptr == 0) {  // No cuelgan bloques de punteros
            if (reservar == 0) {
                // perror(RED "Error: ficheros_basico.c -> traducir_bloque_inodo() -> while (nivel_punteros > 0) -> reservar == 0");
                // printf(RESET);
                return FALLO;
            }
            ptr = reservar_bloque();
            if (ptr == FALLO) {
                perror(RED "Error: ficheros_basico.c -> traducir_bloque_inodo() -> while (nivel_punteros > 0) -> reservar_bloque() == FALLO");
                printf(RESET);
                return FALLO;
            }
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            salvar_inodo = 1;
            if (nivel_punteros == nRangoBL) {
                inodo.punterosIndirectos[nRangoBL - 1] = ptr;
#if DEBUG_TRADUCIR_BLOQUE_INODO
                printf(GRAY "[traducir_bloque_inodo() -> inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n", nRangoBL - 1, ptr, ptr, nRangoBL);
                printf(RESET);
#endif
            } else {
                buffer[indice] = ptr;
                bwrite(ptr_ant, buffer);
#if DEBUG_TRADUCIR_BLOQUE_INODO
                printf(GRAY "[traducir_bloque_inodo() -> punteros_nivel%d [%d] = %d (reservado BF %d para punteros_nivel%d)]\n", nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
                printf(RESET);
#endif
            }
            memset(buffer, 0, BLOCKSIZE);
        } else {
            bread(ptr, buffer);
        }
        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;         // guardamos el puntero actual
        ptr = buffer[indice];  // y lo desplazamos al siguiente nivel
        nivel_punteros--;
    }
    // Ahora nos encontramos en el nivel de datos
    if (ptr == 0) {  // Si no existe bloque de datos
        if (reservar == 0) {
            // perror(RED "Error: ficheros_basico.c -> traducir_bloque_inodo() -> reservar == 0");
            // printf(RESET);
            return FALLO;
        }
        ptr = reservar_bloque();
        if (ptr == FALLO) {
            perror(RED "Error: ficheros_basico.c -> traducir_bloque_inodo() -> reservar_bloque() == FALLO");
            printf(RESET);
            return FALLO;
        }
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;
        if (nRangoBL == 0) {
            inodo.punterosDirectos[nblogico] = ptr;
            // Print de depuración para punteros directos
#if DEBUG_TRADUCIR_BLOQUE_INODO
            printf(GRAY "[traducir_bloque_inodo() -> inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n", nblogico, ptr, ptr, nblogico);
            printf(RESET);
#endif
        } else {
            buffer[indice] = ptr;
            bwrite(ptr_ant, buffer);
#if DEBUG_TRADUCIR_BLOQUE_INODO
            printf(GRAY "[traducir_bloque_inodo() -> punteros_nivel1 [%d] = %d (reservado BF %d para BL %d)]\n", indice, ptr, ptr, nblogico);
            printf(RESET);
#endif
        }
    }
    // Salvar el inodo si se han hecho cambios
    if (salvar_inodo == 1) {
        escribir_inodo(ninodo, &inodo);
    }
    return ptr;
}

int liberar_inodo(unsigned int ninodo) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_inodo() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    int numBloquesOcupados = inodo.numBloquesOcupados;
    // Liberamos todos los bloques ocupades de inodo
    int numBloquesLiberados = liberar_bloques_inodo(0, &inodo);
    if (numBloquesLiberados == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_inodo() -> numBloquesLiberados == FALLO");
        printf(RESET);
        return FALLO;
    }
    if (numBloquesLiberados != numBloquesOcupados) {
        printf("\nnumBloquesLiberados: %d, numBloquesOcupados: %d", numBloquesLiberados, numBloquesOcupados);
        perror(RED "Error: ficheros_basico.c -> liberar_inodo() -> numBloquesLiberados != numBloquesOcupados");
        printf(RESET);
        return FALLO;
    }
    // Actualizamos los atributos del inodo
    inodo.tipo = 'l';
    inodo.tamEnBytesLog = 0;
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_inodo() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    inodo.punterosDirectos[0] = sb.posPrimerInodoLibre;
    sb.posPrimerInodoLibre = ninodo;
    sb.cantInodosLibres++;
    if (bwrite(posSB, &sb) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_inodo() -> bwrite() == FALLO");
        printf(RESET);
        return FALLO;
    }
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> liberar_inodo() -> escribir_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return ninodo;
}
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
    // Si el fichero está vacío devolvemos 0 bloques liberados
    if (inodo->tamEnBytesLog == 0) return 0;

    unsigned int nivel_punteros = 0, indice = 0, ptr = 0, nBL, ultimoBL;
    int nRangoBL = 0;
    unsigned int bloques_punteros[3][NPUNTEROS];
    unsigned int bufAux_punteros[NPUNTEROS];
    int ptr_nivel[3];
    int indices[3];
    int liberados = 0;

    // Contador para contar escrituras y lecturas en bloques
    int total_breads = 0;
    int total_bwrites = 0;

    // Variables para rastrear saltos de mejora 1
    int nBLPrevio;

    memset(bufAux_punteros, 0, BLOCKSIZE);

    // Obtener el último bloque lógico ocupado
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
        ultimoBL = (inodo->tamEnBytesLog / BLOCKSIZE) - 1;
    } else {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    }

#if DEBUG_LIBERAR_BLOQUES_INODO
    // Debugging initial message
    printf(BLUE NEGRITA "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n" RESET, primerBL, ultimoBL);
#endif
    // Iteramos sobre los bloques lógicos
    for (nBL = primerBL; nBL <= ultimoBL; nBL++) {
        nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);  // 0: Directo, 1: I0, 2: I1, 3: I2
        if (nRangoBL < 0) return FALLO;
        nivel_punteros = nRangoBL;  // Nivel más alto de punteros

        // Descendemos por los niveles de punteros hasta llegar a los datos
        while (ptr > 0 && nivel_punteros > 0) {
            indice = obtener_indice(nBL, nivel_punteros);
            if (indice == 0 || nBL == primerBL) {
                if (bread(ptr, bloques_punteros[nivel_punteros - 1]) == FALLO) {
                    perror(RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> bread() == FALLO" RESET);
                    return FALLO;
                }
                total_breads++;
            }
            // Indicamos la lectura del bloque
            ptr_nivel[nivel_punteros - 1] = ptr;
            indices[nivel_punteros - 1] = indice;
            ptr = bloques_punteros[nivel_punteros - 1][indice];
            nivel_punteros--;
        }
        // Si existe un bloque de datos
        if (ptr > 0) {
            if (liberar_bloque(ptr) == FALLO) {
                fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> liberar_bloque() == FALLO" RESET);
                return FALLO;
            }
            liberados++;
#if DEBUG_LIBERAR_BLOQUES_INODO
            printf(GRAY "[liberar_bloques_inodo() -> liberado BF %d de datos para BL %d]\n" RESET, ptr, nBL);
#endif
            // Si estamos en el nivel 0 de inodo
            if (nRangoBL == 0) {
                inodo->punterosDirectos[nBL] = 0;
            } else {
                // Liberación en punteros indirectos
                nivel_punteros = 1;
                while (nivel_punteros <= nRangoBL) {
                    indice = indices[nivel_punteros - 1];
                    bloques_punteros[nivel_punteros - 1][indice] = 0;
                    ptr = ptr_nivel[nivel_punteros - 1];
                    // Si el bloque de punteros está vacío, lo liberamos
                    if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros, BLOCKSIZE) == 0) {
                        // Liberar el bloque de punteros
                        if (liberar_bloque(ptr) == FALLO) {
                            fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> liberar_bloque() == FALLO" RESET);
                            return FALLO;
                        }
#if DEBUG_LIBERAR_BLOQUES_INODO
                        printf(GRAY "[liberar_bloques_inodo() -> liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, ptr, nivel_punteros, nBL);
                        liberados++;
#endif
                        // Mejora 1: Saltar los BLs restantes que no se requiera explorar
                        nBLPrevio = nBL;
                        if (nivel_punteros == 1) {
                            // Para nivel 1, saltamos al final de este bloque de punteros
                            nBL += NPUNTEROS - (indice + 1);
                        } else if (nivel_punteros == 2) {
                            // Para nivel 2, saltamos NPUNTEROS bloques por cada entrada restante
                            nBL += (NPUNTEROS * (NPUNTEROS - (indice + 1)));
                        } else if (nivel_punteros == 3) {
                            // Para nivel 3, saltamos NPUNTEROS^2 bloques por cada entrada restante
                            nBL += (NPUNTEROS * NPUNTEROS * (NPUNTEROS - (indice + 1)));
                        }

                        if (nBL > ultimoBL) nBL = ultimoBL;  // No pasarse del último bloque
#if DEBUG_LIBERAR_BLOQUES_INODO
                        printf(MAGENTA "[liberar_bloques_inodo() -> Saltamos del BL %d saltamos hasta BL %d]\n" RESET, nBLPrevio, nBL);
#endif
                        // Borramos el puntero indirecto del inodo
                        if (nivel_punteros == nRangoBL) {
                            inodo->punterosIndirectos[nRangoBL - 1] = 0;
                        }
                        nivel_punteros++;
                    } else {
                        // Escribimos el bloque de punteros modificado
                        if (bwrite(ptr, bloques_punteros[nivel_punteros - 1]) == FALLO) {
                            perror(RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> bwrite() == FALLO" RESET);
                            return FALLO;
                        }
#if DEBUG_LIBERAR_BLOQUES_INODO
                        printf(ORANGE "[liberar_bloques_inodo() -> salvado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, ptr, nivel_punteros, nBL);
#endif
                        total_bwrites++;
                        nivel_punteros = nRangoBL + 1;
                    }
                }
            }
        } else {
            if (nivel_punteros > 0) {
                int indice_actual = obtener_indice(nBL, nivel_punteros);
                int bloquesRestantes = 0;

                if (nivel_punteros == 1) {
                    // Saltar hasta el final del bloque de punteros de nivel 1
                    bloquesRestantes = NPUNTEROS - (indice_actual + 1);
                    nBL += bloquesRestantes;
                } else if (nivel_punteros == 2) {
                    // Saltar NPUNTEROS bloques por cada entrada restante en nivel 2
                    bloquesRestantes = NPUNTEROS + NPUNTEROS * (NPUNTEROS - (indice_actual + 1));
                    nBL += bloquesRestantes;
                } else if (nivel_punteros == 3) {
                    // Saltar NPUNTEROS^2 bloques por cada entrada restante en nivel 3
                    bloquesRestantes = (NPUNTEROS * NPUNTEROS) + NPUNTEROS * NPUNTEROS * (NPUNTEROS - (indice_actual + 1));
                    nBL += bloquesRestantes;
                }
#if DEBUG_LIBERAR_BLOQUES_INODO
                printf(BLUE "[liberar_bloques_inodo() -> BL %d no existe, saltamos %d bloques hasta BL %d]\n" RESET, nBL - bloquesRestantes, bloquesRestantes, nBL);
#endif
            }
        }
    }
    // Actualizar el número de bloques ocupados en el inodo
    inodo->numBloquesOcupados -= liberados;
// Final summary
#if DEBUG_LIBERAR_BLOQUES_INODO
    printf(BLUE NEGRITA "[liberar_bloques_inodo()→ total bloques liberados: %d, total_breads: %d, total_bwrites: %d]\n" RESET, liberados, total_breads, total_bwrites);
#endif
    return liberados;
}

int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> mi_truncar_f() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if ((inodo.permisos & 0b00000010) != 0b00000010) {
        perror(RED "Error: ficheros_basico.c -> mi_truncar_f() -> inodo.permisos & 0b00000010 != 0b00000010");
        printf(RESET);
        return FALLO;
    }
    if (inodo.tamEnBytesLog < nbytes) {
        perror(RED "Error: ficheros_basico.c -> mi_truncar_f() -> inodo.tamEnBytesLog > nbytes");
        printf(RESET);
        return FALLO;
    }
    int primerBL = nbytes / BLOCKSIZE;
    if ((nbytes % BLOCKSIZE) != 0) {
        primerBL++;
    }
    int bloquesLiberados = liberar_bloques_inodo(primerBL, &inodo);
    if (bloquesLiberados == FALLO) {
        perror(RED "Error: ficheros_basico.c -> mi_truncar_f() -> liberar_bloques_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    time_t currTime = time(NULL);
    inodo.mtime = currTime;
    inodo.ctime = currTime;
    inodo.tamEnBytesLog = nbytes;
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros_basico.c -> mi_truncar_f() -> escribir_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return bloquesLiberados;
}
