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
    int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
    if (nbfisico == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> traducir_bloque_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    unsigned char buffer_dest[BLOCKSIZE];
    if (bread(nbfisico, *buffer_dest) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> bread() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if (primerBL == ultimoBL) {
        // Caso de escritura en un solo bloque
        memcpy(buffer_dest + desp1, buf_original, nbytes);
        if (bwrite(nbfisico, *buffer_dest)) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> if (primerBL == ultimoBL) -> bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
        nBytesEscritos += nbytes;
    } else {
        // Caso de escritura en varios bloques
        int bytesEscritura = BLOCKSIZE - desp1;
        memcpy(buffer_dest + desp1, buf_original, bytesEscritura);  // Escribir en el primer bloque
        if (bwrite(nbfisico, *buffer_dest)) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> if (primerBL == ultimoBL) else -> bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
        nBytesEscritos += bytesEscritura;
        bytesEscritura = BLOCKSIZE;
        int actualBL = primerBL + 1;
        for (int nBloqueActual = 1; nBloqueActual < (ultimoBL - primerBL) - 1; nBloqueActual++) {
            nbfisico = traducir_bloque_inodo(ninodo, actualBL, 1);
            if (nbfisico == FALLO) {
                perror(RED "Error: ficheros.c -> mi_write_f() -> for (int nBloqueActual = 1; nBloqueActual < (ultimoBL - primerBL); nBloqueActual++) -> traducir_bloque_inodo() == FALLO");
                printf(RESET);
                return FALLO;
            }
            if (bwrite(nbfisico, buf_original + nBytesEscritos) == FALLO) {
                perror(RED "Error: ficheros.c -> mi_write_f() -> for (int nBloqueActual = 1; nBloqueActual < (ultimoBL - primerBL); nBloqueActual++) -> bwrite() == FALLO");
                printf(RESET);
                return FALLO;
            }
            nBytesEscritos += bytesEscritura;
            actualBL++;
        }
        nbfisico = traducir_bloque_inodo(ninodo, actualBL, 1);
        if (nbfisico == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> nbfisico = traducir_bloque_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }
        if (bread(nbfisico, *buffer_dest) == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> bread() == FALLO");
            printf(RESET);
            return FALLO;
        }
        memcpy(buffer_dest, buf_original + nBytesEscritos, desp2 + 1);
        if (bwrite(nbfisico, *buffer_dest) == FALLO) {
            perror(RED "Error: ficheros.c -> mi_write_f() -> bwrite() == FALLO");
            printf(RESET);
            return FALLO;
        }
        nBytesEscritos += bytesEscritura;
    }
    time_t tiempoModificacion = time(NULL);
    inodo.ctime = tiempoModificacion;
    inodo.mtime = tiempoModificacion;
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        perror(RED "Error: ficheros.c -> mi_write_f() -> escribir_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return nBytesEscritos;
}
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes){
    int nBytesLeidos = 0;
    struct inodo inodo;
    if (leer_inodo(ninodo,&inodo) == FALLO){
        perror(RED "Error: ficheros.c -> mi_read_f() -> leer_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    if ((inodo.permisos & 0b00000100) != 0b00000100) {
        perror(RED "Error: ficheros.c -> mi_read_f() -> inodo.permisos & 0b00000100 != 0b00000100");
        printf(RESET);
        return FALLO;
    }
    if (offset >= inodo.tamEnBytesLog) {
        return nBytesLeidos;
    }
    

}
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);
