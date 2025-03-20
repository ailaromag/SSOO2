#include "ficheros.h"

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    int nBytesEscritos = 0;
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if ((inodo.permisos & 0b00000010) != 0b00000010) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> inodo.permisos & 0b00000010 != 0b00000010");
        printf(RESET);
        return FALLO;
    }
    int primerBL = offset / BLOCKSIZE;
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    int desp1 = offset % BLOCKSIZE;
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    // Obtenemos el nº de bloque físico del primer BL
    int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
    if (nbfisico == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> traducir_bloque_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Leemos el bloque antes de escribir para preservar el valor de los bytes no escritos
    unsigned char buf_bloque[BLOCKSIZE];
    if (bread(nbfisico, buf_bloque) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if (primerBL == ultimoBL) {
        // Caso de escritura en un solo bloque
        memcpy(buf_bloque + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> if (primerBL == ultimoBL) -> bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
        nBytesEscritos += nbytes;
    } else {
        // Caso de escritura en varios bloques
        int bytesEscritura = BLOCKSIZE - desp1;
        memcpy(buf_bloque + desp1, buf_original, bytesEscritura);  // Escribir en el primer bloque
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> if (primerBL == ultimoBL) else -> bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
        nBytesEscritos += bytesEscritura;
        bytesEscritura = BLOCKSIZE;
        for (int actualBL = primerBL + 1; actualBL < ultimoBL; actualBL++) {
            nbfisico = traducir_bloque_inodo(ninodo, actualBL, 1);
            if (nbfisico == FALLO) {
                perror(RED "Error: ficheros.c -> mi_write_f() -> for (int actualBL = primerBL + 1; actualBL < (ultimoBL - 1); actualBL++) -> traducir_bloque_inodo() == FALLO");
                printf(RESET);
                return FALLO;
            }
            memcpy(buf_bloque, buf_original + nBytesEscritos, BLOCKSIZE);
            if (bwrite(nbfisico, buf_bloque) == FALLO) {
                perror(RED "Error: ficheros.c -> mi_write_f() -> for (int actualBL = primerBL + 1; actualBL < (ultimoBL - 1); actualBL++) -> bwrite() == FALLO");
                printf(RESET);
                return FALLO;
            }
            nBytesEscritos += bytesEscritura;
        }
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (nbfisico == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> nbfisico = traducir_bloque_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }
        if (bread(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> bread() == FALLO");
            printf(RESET);
            return FALLO;
        }
        memcpy(buf_bloque, buf_original + nBytesEscritos, desp2 + 1);
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
        // nBytesEscritos += bytesEscritura;
        nBytesEscritos += desp2 + 1;
    }
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    time_t tiempoModificacion = time(NULL);
    inodo.ctime = tiempoModificacion;
    inodo.mtime = tiempoModificacion;

    if (offset + nBytesEscritos > inodo.tamEnBytesLog) {
        inodo.tamEnBytesLog = offset + nBytesEscritos;
    }
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> escribir_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return nBytesEscritos;
}

int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    int nBytesLeidos = 0;
    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    // Comprobamos permisos de lectura
    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, "No hay permisos de lectura\n");
        return 0;
    }

    // Comprobamos offset
    if (offset >= inodo.tamEnBytesLog) {
        return 0;
    }

    // Ajustamos nbytes si es necesario
    if ((offset + nbytes) >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset;
    }

    int primerBL = offset / BLOCKSIZE;
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    int desp1 = offset % BLOCKSIZE;
    unsigned char buf_bloque[BLOCKSIZE];
    memset(buf_original, 0, nbytes);  // Inicializamos el buffer con ceros

    // Caso: un solo bloque
    if (primerBL == ultimoBL) {
        int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) {
                return FALLO;
            }
            memcpy(buf_original, buf_bloque + desp1, nbytes);
        }
        nBytesLeidos = nbytes;
    } else {
        // Primer bloque
        int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        int bytesLeer = BLOCKSIZE - desp1;
        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) {
                return FALLO;
            }
            memcpy(buf_original, buf_bloque + desp1, bytesLeer);
        }
        nBytesLeidos = bytesLeer;

        // Bloques intermedios
        for (int i = primerBL + 1; i < ultimoBL; i++) {
            nbfisico = traducir_bloque_inodo(ninodo, i, 0);
            if (nbfisico != FALLO) {
                if (bread(nbfisico, buf_bloque) == FALLO) {
                    return FALLO;
                }
                memcpy(buf_original + nBytesLeidos, buf_bloque, BLOCKSIZE);
            }
            nBytesLeidos += BLOCKSIZE;
        }

        // Último bloque
        int desp2 = (offset + nbytes - 1) % BLOCKSIZE;
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);
        if (nbfisico != FALLO) {
            if (bread(nbfisico, buf_bloque) == FALLO) {
                return FALLO;
            }
            memcpy(buf_original + nBytesLeidos, buf_bloque, desp2 + 1);
        }
        nBytesLeidos += desp2 + 1;
    }

    // Actualizar atime
    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        return FALLO;
    }

    return nBytesLeidos;
}

int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_stat_f() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    p_stat->atime = inodo.atime;
    p_stat->btime = inodo.btime;
    p_stat->ctime = inodo.ctime;
    p_stat->mtime = inodo.mtime;
    p_stat->nlinks = inodo.nlinks;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;
    p_stat->permisos = inodo.permisos;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->tipo = inodo.tipo;
    return EXITO;
}
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_chmod_f() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    inodo.permisos = permisos;
    if (escribir_inodo(ninodo, &inodo)) {
        perror(RED "Error: ficheros.c -> mi_chmod_f() -> escribir_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO;
}