/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "ficheros_basico.h"

#define DEBUG_TRADUCIR_BLOQUE_INODO false
#define DEBUG_LIBERAR_BLOQUES_INODO true

/**
 * Calcula el tamaño en bloques necesario para el mapa de bits.
 * Determina cuántos bloques se necesitan para almacenar el bitmap del sistema.
 * 
 * Parámetros de entrada:
 * - nbloques: número total de bloques del sistema de archivos
 * 
 * Return:
 * - número de bloques necesarios para el mapa de bits
 */
int tamMB(unsigned int nbloques) {
    // Calcular bloques necesarios para el mapa de bits
    int tamMB = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques / 8) % BLOCKSIZE != 0) {
        tamMB++;
    }
    return tamMB;
}

/**
 * Calcula el tamaño en bloques del array de inodos.
 * Determina cuántos bloques se necesitan para almacenar todos los inodos.
 * 
 * Parámetros de entrada:
 * - ninodos: número total de inodos del sistema de archivos
 * 
 * Return:
 * - número de bloques necesarios para el array de inodos
 */
int tamAI(unsigned int ninodos) {
    // Calcular bloques necesarios para el array de inodos
    int tamAI = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0) {
        tamAI++;
    }
    return tamAI;
}

/**
 * Inicializa los datos del superbloque con la estructura del sistema de archivos.
 * Calcula y establece las posiciones de todas las estructuras de metadatos.
 * 
 * Parámetros de entrada:
 * - nbloques: número total de bloques del sistema de archivos
 * - ninodos: número total de inodos del sistema de archivos
 * 
 * Return:
 * - EXITO (0) si inicializa correctamente
 * - FALLO (-1) si error en bwrite
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    struct superbloque SB;
    // Inicialización de los atributos del superbloque
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

    // Escribir el superbloque en el dispositivo
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> initSB() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Inicializa el mapa de bits poniendo a 1 los bits que representan los metadatos.
 * Marca como ocupados los bloques utilizados por el superbloque, mapa de bits y array de inodos.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si inicializa correctamente
 * - FALLO (-1) si error en bread o bwrite
 */
int initMB() {
    // Leer el superbloque del sistema
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> initMB -> bread()== FALLO\n" RESET);
        return FALLO;
    }

    // Calcular número total de bloques de metadatos
    int bloquesMetadatos = tamSB + tamMB(SB.totBloques) + tamAI(SB.totInodos);
    int bloquesMB = bloquesMetadatos / (BYTE_SIZE * BLOCKSIZE);

    // Buffer para escribir bits a 1 (todos los bytes a 255)
    unsigned char bufMB[BLOCKSIZE];
    memset(bufMB, 255, BLOCKSIZE);
    for (int i = 0; i < bloquesMB; i++) {
        // Escribir bloques completos con todos los bits a 1
        if (bwrite(posSB + tamSB + i, bufMB) == FALLO) {
            fprintf(stderr, RED "Error: ficheros_basico.c -> initMB() -> bwrite() == FALLO\n" RESET);
            return FALLO;
        }
    }

    // Calcular y escribir el último bloque parcial
    int bitsUltimoBloque = bloquesMetadatos % (8 * BLOCKSIZE);
    int bytesLlenos = bitsUltimoBloque / 8;
    int bitsRestantes = bitsUltimoBloque % 8;

    // Resetear buffer y llenar bytes completos
    memset(bufMB, 0, BLOCKSIZE);
    for (int i = 0; i < bytesLlenos; i++) {
        bufMB[i] = 255;
    }

    // Activar bits restantes en el siguiente byte si los hay
    if (bitsRestantes > 0) {
        bufMB[bytesLlenos] = (0b11111111 << (8 - bitsRestantes));
    }
    // Escribir el último bloque
    if (bwrite(posSB + tamSB + bloquesMB, bufMB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> initMB() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }

    // Actualizar contador de bloques libres en el superbloque
    SB.cantBloquesLibres -= bloquesMetadatos;
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> initMB() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Inicializa la lista de inodos libres, enlazando todos los inodos secuencialmente.
 * Crea una lista enlazada de todos los inodos disponibles para asignación.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si inicializa correctamente
 * - FALLO (-1) si error en bread o bwrite
 */
int initAI() {
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    struct superbloque SB;

    // Leer el superbloque del dispositivo virtual
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> initAI() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    SB.posPrimerInodoLibre = 0;

    // Contador para el siguiente inodo libre
    int contInodos = SB.posPrimerInodoLibre + 1;
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {  // para cada bloque del AI
        // Leer el bloque de inodos i en el dispositivo virtual
        if (bread(i, inodos) == FALLO) {
            fprintf(stderr, RED "Error: ficheros_basico.c -> initAI() -> bread() == FALLO\n" RESET);
            return FALLO;
        }
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {    // para cada inodo del bloque
            inodos[j].tipo = 'l';                            // marcar como libre
            if (contInodos < SB.totInodos) {                 // si no hemos llegado al último inodo del AI
                inodos[j].punterosDirectos[0] = contInodos;  // enlazar con el siguiente
                contInodos++;
            } else {  // hemos llegado al último inodo
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }
        // Escribir el bloque de inodos i en el dispositivo virtual
        if (bwrite(i, inodos) == FALLO) {
            fprintf(stderr, RED "Error: ficheros_basico.c -> initAI() ->  bwrite() == FALLO\n" RESET);
            return FALLO;
        }
    }
    return EXITO;
}

/**
 * Escribe un bit específico en el mapa de bits con el valor indicado.
 * Modifica un bit individual del bitmap para marcar bloques como libres u ocupados.
 * 
 * Parámetros de entrada:
 * - nbloque: número del bloque cuyo bit se quiere modificar
 * - bit: valor a escribir (0=libre, 1=ocupado)
 * 
 * Return:
 * - EXITO (0) si escribe correctamente
 * - FALLO (-1) si error en bread, bwrite o valor de bit inválido
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Calcular posición del byte y bit dentro del mapa de bits
    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;
    int nbloqueMB = posbyte / BLOCKSIZE;
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];
    posbyte = posbyte % BLOCKSIZE;
    // Leer el bloque físico que contiene el bit
    if (bread(nbloqueabs, &bufferMB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Escribir el bit en el buffer usando máscara
    unsigned char mascara = 0b10000000;
    mascara >>= posbit;
    if (bit == 1) {
        bufferMB[posbyte] |= mascara;
    } else if (bit == 0) {
        bufferMB[posbyte] &= ~mascara;
    } else {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bit != 0 ni 1\n" RESET);
        return FALLO;
    }

    // Escribir el buffer modificado en el dispositivo virtual
    if (bwrite(nbloqueabs, bufferMB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> initAI -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Lee un determinado bit del mapa de bits y devuelve su valor.
 * Consulta el estado de un bloque específico en el bitmap.
 * 
 * Parámetros de entrada:
 * - nbloque: número del bloque cuyo bit se quiere leer
 * 
 * Return:
 * - 0 si el bloque está libre
 * - 1 si el bloque está ocupado
 * - FALLO (-1) si error en bread
 */
char leer_bit(unsigned int nbloque) {
    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Calcular posición del byte y bit dentro del mapa de bits
    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;
    int nbloqueMB = posbyte / BLOCKSIZE;
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];
    posbyte = posbyte % BLOCKSIZE;
    // Leer el bloque físico
    if (bread(nbloqueabs, &bufferMB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Leer el bit usando máscara
    unsigned char mascara = 0b10000000;
    mascara >>= posbit;
    mascara &= bufferMB[posbyte];
    mascara >>= (7 - posbit);

    return mascara;
}

/**
 * Encuentra el primer bloque libre, lo marca como ocupado y devuelve su posición.
 * Busca en el mapa de bits el primer bit a 0, lo pone a 1 y limpia el bloque.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - número del bloque reservado si éxito
 * - FALLO (-1) si no hay bloques libres o error en operaciones
 */
int reservar_bloque() {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    if (SB.cantBloquesLibres <= 0) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> SB.cantBloquesLibres <= 0\n" RESET);
        return FALLO;
    }
    int nbloqueMB = SB.posPrimerBloqueMB;
    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];
    memset(bufferAux, 255, BLOCKSIZE);
    bool bloqueLibreEncontrado = false;
    // Localizar el primer bloque del mapa de bits con algún bit libre
    while (!bloqueLibreEncontrado && nbloqueMB <= SB.posUltimoBloqueMB) {
        if (bread(nbloqueMB, bufferMB) == FALLO) {
            fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bread() == FALLO\n" RESET);
            return FALLO;
        }
        if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0) {
            bloqueLibreEncontrado = true;
        } else {
            nbloqueMB++;
        }
    }
    int posbyte = 0;
    // Localizar el primer byte con algún bit a 0
    while ((bufferMB[posbyte] == 255) && posbyte < BLOCKSIZE) {
        posbyte++;
    }
    // Localizar el bit que vale 0 dentro del byte
    unsigned char mascara = 0b10000000;
    int posbit = 0;
    while (bufferMB[posbyte] & mascara) {
        bufferMB[posbyte] <<= 1;
        posbit++;
    }

    // Calcular el número de bloque físico que podemos reservar
    int nbloque = ((((nbloqueMB - SB.posPrimerBloqueMB) * BLOCKSIZE) + posbyte) * BYTE_SIZE) + posbit;

    // Marcar el bit como ocupado en el mapa de bits
    if (escribir_bit(nbloque, 1) == FALLO) {
        return FALLO;
    }
    // Limpiar el bloque reservado
    unsigned char bufferCeros[BLOCKSIZE];
    memset(bufferCeros, 0, BLOCKSIZE);
    if (bwrite(nbloque, bufferCeros) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    // Actualizar contador de bloques libres
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_bit() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    return nbloque;
}

/**
 * Libera un bloque determinado marcándolo como disponible en el mapa de bits.
 * Pone a 0 el bit correspondiente al bloque en el bitmap.
 * 
 * Parámetros de entrada:
 * - nbloque: número del bloque a liberar
 * 
 * Return:
 * - número del bloque liberado si éxito
 * - FALLO (-1) si error en escribir_bit o bwrite
 */
int liberar_bloque(unsigned int nbloque) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloque -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Marcar el bloque como libre en el mapa de bits
    if (escribir_bit(nbloque, 0) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloque -> escribir_bit() == FALLO\n" RESET);
        return FALLO;
    }
    // Incrementar contador de bloques libres
    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloque -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    return nbloque;
}

/**
 * Escribe un inodo en la posición correspondiente del array de inodos.
 * Almacena la estructura inodo en el dispositivo virtual.
 * 
 * Parámetros de entrada:
 * - ninodo: número del inodo a escribir
 * - inodo: puntero a la estructura inodo a escribir
 * 
 * Return:
 * - EXITO (0) si escribe correctamente
 * - FALLO (-1) si error en bread o bwrite
 */
int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_inodo() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Calcular posición del bloque e índice dentro del bloque
    int posBloqueInodo = SB.posPrimerBloqueAI + (ninodo * INODOSIZE) / BLOCKSIZE;
    int posInodo = (ninodo * INODOSIZE) % BLOCKSIZE;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    // Leer el bloque que contiene el inodo
    if (bread(posBloqueInodo, inodos) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_inodo() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Copiar el inodo en la posición correcta
    inodos[posInodo / INODOSIZE] = *inodo;
    // Escribir el bloque modificado de vuelta al dispositivo
    if (bwrite(posBloqueInodo, inodos) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> escribir_inodo() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    return EXITO;
}

/**
 * Lee un inodo de la posición correspondiente del array de inodos.
 * Recupera la estructura inodo desde el dispositivo virtual.
 * 
 * Parámetros de entrada:
 * - ninodo: número del inodo a leer
 * - inodo: puntero donde almacenar el inodo leído
 * 
 * Return:
 * - EXITO (0) si lee correctamente
 * - FALLO (-1) si error en bread
 */
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> leer_inodo() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Calcular posición del bloque e índice dentro del bloque
    int posBloqueInodo = SB.posPrimerBloqueAI + (ninodo * INODOSIZE) / BLOCKSIZE;
    int posInodo = (ninodo * INODOSIZE) % BLOCKSIZE;
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    // Leer el bloque que contiene el inodo
    if (bread(posBloqueInodo, inodos) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> leer_inodo() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Copiar el inodo desde la posición correcta
    *inodo = inodos[posInodo / INODOSIZE];
    return EXITO;
}

/**
 * Reserva un inodo libre, lo inicializa con los parámetros dados y lo marca como ocupado.
 * Asigna un nuevo inodo para crear archivos o directorios.
 * 
 * Parámetros de entrada:
 * - tipo: tipo del inodo ('f' para fichero, 'd' para directorio)
 * - permisos: permisos en formato octal (rwx)
 * 
 * Return:
 * - número del inodo reservado si éxito
 * - FALLO (-1) si no hay inodos libres o error en operaciones
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> reservar_inodo() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    if (SB.cantInodosLibres <= 0) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> reservar_inodo() -> SB.cantInodosLibres <= 0\n" RESET);
        return FALLO;
    }

    // Preparar el inodo a escribir
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
    // Inicializar todos los punteros a 0
    for (int i = 0; i < sizeof(inodoReservado.punterosDirectos) / sizeof(inodoReservado.punterosDirectos[0]); i++) {
        inodoReservado.punterosDirectos[i] = 0;
    }
    for (int i = 0; i < sizeof(inodoReservado.punterosIndirectos) / sizeof(inodoReservado.punterosIndirectos[0]); i++) {
        inodoReservado.punterosIndirectos[i] = 0;
    }

    // Antes de escribir, apuntar al siguiente inodo libre
    struct inodo inodoLibre;
    if (leer_inodo(SB.posPrimerInodoLibre, &inodoLibre) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> reservar_inodo() -> leer_inodo() == FALLO\n" RESET);
        return FALLO;
    }
    SB.posPrimerInodoLibre = inodoLibre.punterosDirectos[0];  // Apuntar al siguiente inodo libre
    SB.cantInodosLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> reservar_inodo() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    // Escribir el inodo inicializado
    if (escribir_inodo(posInodoReservado, &inodoReservado) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> reservar_inodo() -> escribir_inodo() == FALLO\n" RESET);
        return FALLO;
    }
    return posInodoReservado;
}

/**
 * Determina el rango de punteros donde se sitúa un bloque lógico dado.
 * Clasifica si el bloque está en punteros directos o indirectos (nivel 1, 2 o 3).
 * 
 * Parámetros de entrada:
 * - inodo: puntero al inodo propietario
 * - nblogico: número del bloque lógico
 * - ptr: puntero donde almacenar la dirección del puntero correspondiente
 * 
 * Return:
 * - 0 si está en punteros directos
 * - 1, 2 o 3 si está en punteros indirectos de nivel respectivo
 * - FALLO (-1) si el bloque lógico está fuera de rango
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
        fprintf(stderr, RED "Error: ficheros_basico.c -> obtener_rangoBL() == BLOQUE LÓGICO FUERA DE RANGO\n" RESET);
        return FALLO;
    }
}

/**
 * Calcula el índice dentro de un bloque de punteros para un bloque lógico dado.
 * Función auxiliar para navegación en estructuras de punteros indirectos.
 * 
 * Parámetros de entrada:
 * - nblogico: número del bloque lógico
 * - nivel_punteros: nivel de indirección (1, 2 o 3)
 * 
 * Return:
 * - índice dentro del bloque de punteros correspondiente
 * - FALLO (-1) si el bloque lógico está fuera de rango
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
    fprintf(stderr, RED "Error: ficheros_basico.c -> obtener_indice() -> nblogico >= INDIRECTOS2\n" RESET);
    return FALLO;
}

/**
 * Obtiene el número de bloque físico correspondiente a un bloque lógico indicado.
 * Traduce direcciones lógicas a físicas, creando estructuras de punteros si es necesario.
 * 
 * Parámetros de entrada:
 * - ninodo: número del inodo propietario
 * - nblogico: número del bloque lógico a traducir
 * - reservar: 0=solo consulta, 1=crear si no existe
 * 
 * Return:
 * - número del bloque físico si éxito
 * - FALLO (-1) si no existe y reservar=0, o error en operaciones
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

    // Comienzo sección crítica
    mi_waitSem();

    // Leer el inodo y determinar el rango del bloque lógico
    leer_inodo(ninodo, &inodo);
    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);
    nivel_punteros = nRangoBL;

    // Navegar por los niveles de punteros hasta llegar a los datos
    while (nivel_punteros > 0) {
        if (ptr == 0) {  // No cuelgan bloques de punteros
            if (reservar == 0) {
                mi_signalSem();
                return FALLO;
            }
            // Reservar un nuevo bloque para punteros
            ptr = reservar_bloque();
            if (ptr == FALLO) {
                fprintf(stderr, RED "Error: ficheros_basico.c -> traducir_bloque_inodo() -> while (nivel_punteros > 0) -> reservar_bloque() == FALLO\n" RESET);
                mi_signalSem();
                return FALLO;
            }
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            salvar_inodo = 1;
            // Actualizar el puntero correspondiente en el inodo o bloque padre
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
            // Leer el bloque de punteros existente
            bread(ptr, buffer);
        }
        // Calcular índice y avanzar al siguiente nivel
        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;         // guardar el puntero actual
        ptr = buffer[indice];  // y desplazarlo al siguiente nivel
        nivel_punteros--;
    }
    // Ahora nos encontramos en el nivel de datos
    if (ptr == 0) {  // Si no existe bloque de datos
        if (reservar == 0) {
            mi_signalSem();
            return FALLO;
        }
        // Reservar un nuevo bloque para datos
        ptr = reservar_bloque();
        if (ptr == FALLO) {
            fprintf(stderr, RED "Error: ficheros_basico.c -> traducir_bloque_inodo() -> reservar_bloque() == FALLO\n" RESET);
            mi_signalSem();
            return FALLO;
        }
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;
        // Actualizar el puntero de datos correspondiente
        if (nRangoBL == 0) {
            inodo.punterosDirectos[nblogico] = ptr;
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

    // Fin sección crítica
    mi_signalSem();

    return ptr;
}

/**
 * Libera un inodo y todos los bloques de datos y punteros asociados.
 * Devuelve el inodo a la lista de inodos libres del sistema.
 * 
 * Parámetros de entrada:
 * - ninodo: número del inodo a liberar
 * 
 * Return:
 * - número del inodo liberado si éxito
 * - FALLO (-1) si error en operaciones o inconsistencia en bloques liberados
 */
int liberar_inodo(unsigned int ninodo) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_inodo() -> leer_inodo() == FALLO\n" RESET);
        return FALLO;
    }
    int numBloquesOcupados = inodo.numBloquesOcupados;
    // Liberar todos los bloques ocupados del inodo
    int numBloquesLiberados = liberar_bloques_inodo(0, &inodo);
    if (numBloquesLiberados == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_inodo() -> numBloquesLiberados == FALLO\n" RESET);
        return FALLO;
    }
    // Verificar consistencia en la liberación de bloques
    if (numBloquesLiberados != numBloquesOcupados) {
        printf("\nnumBloquesLiberados: %d, numBloquesOcupados: %d", numBloquesLiberados, numBloquesOcupados);
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_inodo() -> numBloquesLiberados != numBloquesOcupados\n" RESET);
        return FALLO;
    }
    // Actualizar los atributos del inodo para marcarlo como libre
    inodo.tipo = 'l';
    inodo.tamEnBytesLog = 0;
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_inodo() -> bread() == FALLO\n" RESET);
        return FALLO;
    }
    // Enlazar el inodo liberado al inicio de la lista de inodos libres
    inodo.punterosDirectos[0] = sb.posPrimerInodoLibre;
    sb.posPrimerInodoLibre = ninodo;
    sb.cantInodosLibres++;
    if (bwrite(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_inodo() -> bwrite() == FALLO\n" RESET);
        return FALLO;
    }
    // Actualizar tiempo de cambio y escribir el inodo
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_inodo() -> escribir_inodo() == FALLO\n" RESET);
        return FALLO;
    }
    return ninodo;
}

/**
 * Libera todos los bloques de un inodo a partir de un bloque lógico dado.
 * Implementa optimizaciones para saltar bloques no asignados eficientemente.
 * 
 * Parámetros de entrada:
 * - primerBL: primer bloque lógico a liberar
 * - inodo: puntero al inodo propietario de los bloques
 * 
 * Return:
 * - número de bloques liberados si éxito
 * - FALLO (-1) si error en operaciones
 */
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
    // Comienzo sección crítica
    mi_waitSem();

    // Si el fichero está vacío devolver 0 bloques liberados
    if (inodo->tamEnBytesLog == 0) {
        mi_signalSem();
        return 0;
    }

    unsigned int nivel_punteros = 0, indice = 0, ptr = 0, nBL, ultimoBL;
    int nRangoBL = 0;
    unsigned int bloques_punteros[3][NPUNTEROS];
    unsigned int bufAux_punteros[NPUNTEROS];
    int ptr_nivel[3];
    int indices[3];
    int liberados = 0;

    // Contadores para estadísticas de operaciones
    int total_breads = 0;
    int total_bwrites = 0;

#if DEBUG_LIBERAR_BLOQUES_INODO
    int nBLPrevio;
#endif

    memset(bufAux_punteros, 0, BLOCKSIZE);

    // Calcular el último bloque lógico ocupado
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
        ultimoBL = (inodo->tamEnBytesLog / BLOCKSIZE) - 1;
    } else {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    }

#if DEBUG_LIBERAR_BLOQUES_INODO
    printf(BLUE NEGRITA "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n" RESET, primerBL, ultimoBL);
#endif
    // Iterar sobre los bloques lógicos a liberar
    for (nBL = primerBL; nBL <= ultimoBL; nBL++) {
        nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);  // 0: Directo, 1: I0, 2: I1, 3: I2
        if (nRangoBL < 0) {
            mi_signalSem();
            return FALLO;
        }
        nivel_punteros = nRangoBL;  // Nivel más alto de punteros

        // Descender por los niveles de punteros hasta llegar a los datos
        while (ptr > 0 && nivel_punteros > 0) {
            indice = obtener_indice(nBL, nivel_punteros);
            // Leer bloque de punteros solo si es necesario
            if (indice == 0 || nBL == primerBL) {
                if (bread(ptr, bloques_punteros[nivel_punteros - 1]) == FALLO) {
                    fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> bread() == FALLO\n" RESET);
                    mi_signalSem();
                    return FALLO;
                }
                total_breads++;
            }
            // Guardar información del nivel actual
            ptr_nivel[nivel_punteros - 1] = ptr;
            indices[nivel_punteros - 1] = indice;
            ptr = bloques_punteros[nivel_punteros - 1][indice];
            nivel_punteros--;
        }
        // Si existe un bloque de datos, liberarlo
        if (ptr > 0) {
            if (liberar_bloque(ptr) == FALLO) {
                fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> liberar_bloque() == FALLO\n" RESET);
                mi_signalSem();
                return FALLO;
            }
            liberados++;
#if DEBUG_LIBERAR_BLOQUES_INODO
            printf(GRAY "[liberar_bloques_inodo() -> liberado BF %d de datos para BL %d]\n" RESET, ptr, nBL);
#endif
            // Actualizar punteros según el nivel
            if (nRangoBL == 0) {
                inodo->punterosDirectos[nBL] = 0;
            } else {
                // Procesamiento de punteros indirectos
                nivel_punteros = 1;
                while (nivel_punteros <= nRangoBL) {
                    indice = indices[nivel_punteros - 1];
                    bloques_punteros[nivel_punteros - 1][indice] = 0;
                    ptr = ptr_nivel[nivel_punteros - 1];
                    // Si el bloque de punteros está vacío, liberarlo
                    if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros, BLOCKSIZE) == 0) {
                        if (liberar_bloque(ptr) == FALLO) {
                            fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> liberar_bloque() == FALLO\n" RESET);
                            mi_signalSem();
                            return FALLO;
                        }
#if DEBUG_LIBERAR_BLOQUES_INODO
                        printf(GRAY "[liberar_bloques_inodo() -> liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, ptr, nivel_punteros, nBL);
#endif
                        liberados++;
                        // Optimización: Saltar los BLs restantes que no requieren exploración
#if DEBUG_LIBERAR_BLOQUES_INODO
                        nBLPrevio = nBL;
#endif
                        if (nivel_punteros == 1) {
                            nBL += NPUNTEROS - (indice + 1);
                        } else if (nivel_punteros == 2) {
                            nBL += (NPUNTEROS * (NPUNTEROS - (indice + 1)));
                        } else if (nivel_punteros == 3) {
                            nBL += (NPUNTEROS * NPUNTEROS * (NPUNTEROS - (indice + 1)));
                        }

                        if (nBL > ultimoBL) nBL = ultimoBL;  // No pasarse del último bloque
#if DEBUG_LIBERAR_BLOQUES_INODO
                        printf(MAGENTA "[liberar_bloques_inodo() -> Saltamos del BL %d saltamos hasta BL %d]\n" RESET, nBLPrevio, nBL);
#endif
                        // Borrar el puntero indirecto del inodo si es del nivel más alto
                        if (nivel_punteros == nRangoBL) {
                            inodo->punterosIndirectos[nRangoBL - 1] = 0;
                        }
                        nivel_punteros++;
                    } else {
                        // Escribir el bloque de punteros modificado
                        if (bwrite(ptr, bloques_punteros[nivel_punteros - 1]) == FALLO) {
                            fprintf(stderr, RED "Error: ficheros_basico.c -> liberar_bloques_inodo() -> bwrite() == FALLO\n" RESET);
                            mi_signalSem();
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
            // Optimización: Si no existe el bloque, saltar eficientemente
            if (nivel_punteros > 0) {
                int indice_actual = obtener_indice(nBL, nivel_punteros);
                int bloquesRestantes = 0;

                if (nivel_punteros == 1) {
                    bloquesRestantes = NPUNTEROS - (indice_actual + 1);
                    nBL += bloquesRestantes;
                } else if (nivel_punteros == 2) {
                    bloquesRestantes = NPUNTEROS + NPUNTEROS * (NPUNTEROS - (indice_actual + 1));
                    nBL += bloquesRestantes;
                } else if (nivel_punteros == 3) {
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
#if DEBUG_LIBERAR_BLOQUES_INODO
    printf(BLUE NEGRITA "[liberar_bloques_inodo()→ total bloques liberados: %d, total_breads: %d, total_bwrites: %d]\n" RESET, liberados, total_breads, total_bwrites);
#endif

    // Fin sección crítica
    mi_signalSem();

    return liberados;
}

/**
 * Trunca un archivo a un tamaño específico, liberando los bloques sobrantes.
 * Reduce el tamaño lógico del archivo y libera los bloques no necesarios.
 * 
 * Parámetros de entrada:
 * - ninodo: número del inodo del archivo a truncar
 * - nbytes: nuevo tamaño en bytes del archivo
 * 
 * Return:
 * - número de bloques liberados si éxito
 * - FALLO (-1) si error en permisos, tamaño inválido o operaciones
 */
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    struct inodo inodo;

    // Comienzo sección crítica
    mi_waitSem();

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> mi_truncar_f() -> leer_inodo() == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Verificar permisos de escritura
    if ((inodo.permisos & 0b00000010) != 0b00000010) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> mi_truncar_f() -> inodo.permisos & 0b00000010 != 0b00000010\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // No se puede truncar a un tamaño mayor al actual
    if (inodo.tamEnBytesLog < nbytes) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> mi_truncar_f() -> inodo.tamEnBytesLog > nbytes\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Calcular el primer bloque lógico a liberar
    int primerBL = nbytes / BLOCKSIZE;
    if ((nbytes % BLOCKSIZE) != 0) {
        primerBL++;
    }
    // Liberar los bloques sobrantes
    int bloquesLiberados = liberar_bloques_inodo(primerBL, &inodo);
    if (bloquesLiberados == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> mi_truncar_f() -> liberar_bloques_inodo() == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Actualizar metadatos del inodo
    time_t currTime = time(NULL);
    inodo.mtime = currTime;
    inodo.ctime = currTime;
    inodo.tamEnBytesLog = nbytes;
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error: ficheros_basico.c -> mi_truncar_f() -> escribir_inodo() == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Fin sección crítica
    mi_signalSem();

    return bloquesLiberados;
}
