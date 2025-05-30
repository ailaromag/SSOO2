#include "ficheros.h"

#define DEBUGN5 true

/**
 * Programa principal que prueba la escritura en diferentes offsets para testing de punteros indirectos.
 * Utilidad de testing que escribe el mismo texto en posiciones específicas diseñadas para probar
 * punteros directos, indirectos simples, dobles y triples del sistema de archivos.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 4)
 * - argv: array de argumentos donde argv[1] es el dispositivo, argv[2] el texto a escribir
 *         y argv[3] indica si usar diferentes inodos (0=mismo inodo, 1=inodos diferentes)
 * 
 * Return:
 * - EXITO (0) si completa todas las escrituras correctamente
 * - FALLO (-1) si error en argumentos, parámetro inválido, montaje, reserva de inodos,
 *              escrituras o desmontaje
 */
int main(int argc, char **argv) {
#if DEBUGN5
    // Array con offsets diseñados para probar diferentes tipos de punteros
    int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};
    int num_offsets = sizeof(offsets) / sizeof(offsets[0]);
    
    // Verificar que se proporcionan exactamente 3 argumentos
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: escribir <nombre_dispositivo> <\"$(cat fichero)\">  <diferentes_inodos>\nOffsets:");
        for (int i = 0; i < num_offsets; i++) {
            fprintf(stderr, " %d", offsets[i]);
        }
        fprintf(stderr, "\nSi diferentes_inodos=0 se reserva solo un inodo para todos los offsets\n");
        printf(RESET);
        return FALLO;
    }
    
    // Validar el parámetro diferentes_inodos (debe ser 0 o 1)
    int diferentes_inodos = atoi(argv[3]);
    if (diferentes_inodos != 0 && diferentes_inodos != 1) {
        perror(RED "Error: escribir.c -> main() -> diferentes_inodos != 0 && diferentes_inodos != 1");
        printf(RESET);
        return FALLO;
    }
    
    // Montar el dispositivo virtual para poder trabajar con el sistema de archivos
    char *camino = argv[1];
    if (bmount(camino) == FALLO) {
        perror(RED "Error: escribir.c -> main() -> bmount() == FALLO");
        printf(RESET);
        return FALLO;
    }
    
    // Obtener el texto a escribir y calcular su longitud
    char *texto = argv[2];
    int longitud = strlen(texto);
    printf("longitud texto: %d\n\n", longitud);  // Imprimir longitud del texto
    
    struct superbloque sb;
    int nInodoReservado;
    int inodoReservado;
    
    // Si se usa un solo inodo para todos los offsets, reservarlo una vez
    if (diferentes_inodos == 0) {
        inodoReservado = reservar_inodo('f', 6);
        if (inodoReservado == FALLO) {
            perror(RED "Error: escribir.c -> main() -> reservar_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }
    }

    // Iterar sobre todos los offsets para realizar las escrituras de prueba
    for (int i = 0; i < num_offsets; i++) {
        // Si se usan diferentes inodos, reservar uno nuevo para cada offset
        if (diferentes_inodos == 1) {
            inodoReservado = reservar_inodo('f', 6);
            if (inodoReservado == FALLO) {
                perror(RED "Error: escribir.c -> main() -> reservar_inodo() == FALLO");
                printf(RESET);
                return FALLO;
            }
        }
        
        // Leer el superbloque para obtener información del sistema
        if (bread(posSB, &sb) == FALLO) {
            perror(RED "Error: escribir.c -> main() -> bread() == FALLO");
            printf(RESET);
            return FALLO;
        }
        
        // Calcular el número del inodo reservado excluyendo el inodo raíz
        nInodoReservado = sb.totInodos - sb.cantInodosLibres - 1;  // descontamos el inodo raiz
        printf("Nº inodo reservado: %d\n", nInodoReservado);
        printf("offset: %d\n", offsets[i]);  // Imprimir offset

        // Escribir el texto en el offset especificado
        int bytes_escritos = mi_write_f(inodoReservado, texto, offsets[i], longitud);
        if (bytes_escritos == FALLO) {
            perror(RED "Error: escribir.c -> main() -> mi_write_f() == FALLO");
            printf(RESET);
            return FALLO;
        }
        printf("Bytes escritos: %d\n", bytes_escritos);

        // Obtener estadísticas del inodo después de la escritura
        struct STAT stat;
        mi_stat_f(inodoReservado, &stat);  // Llamada real
        printf("stat.tamEnBytesLog=%d\n", stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%d\n\n", stat.numBloquesOcupados);
    }

    // Desmontar el dispositivo al finalizar todas las operaciones
    if (bumount() == FALLO) {
        perror(RED "Error: escribir.c -> main() -> bumount() == FALLO");
        return FALLO;
    }
#endif
    return EXITO;
}
