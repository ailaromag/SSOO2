#include "directorios.h"

// Flags de depuración para controlar qué tests ejecutar
#define DEBUGTMP false
#define DEBUGN3 false
#define DEBUGN4 false
#define DEBUGN5 false
#define DEBUGN7 false

// Declaraciones de funciones para las diferentes pruebas y utilidades
int mostrar_sf();
int test_secuencialidad_AI();
int reservar_liberar_bloque();
int mostrar_bitmap_bordes_seccion();
int imprimir_info_leer_bit(int pos, int posPrimerBloqueMB);
int mostrar_directorio_raiz();
int mostrar_datos_inodo(int posInodoReservado);
void mostrar_buscar_entrada(char *camino, char reservar);

/**
 * Muestra información completa del superbloque del sistema de archivos.
 * Lee y presenta todos los campos del superbloque incluyendo posiciones y contadores.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si muestra la información correctamente
 * - FALLO (-1) si error al leer el superbloque
 */
int mostrar_sf() {
    struct superbloque SB;
    // Leer el superbloque desde el dispositivo virtual
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_sf() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Mostrar encabezado y todos los campos del superbloque
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

/**
 * Verifica la secuencialidad de la lista enlazada de inodos libres.
 * Recorre toda la lista y comprueba si los inodos están ordenados secuencialmente.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si completa el test correctamente
 * - FALLO (-1) si error al leer bloques o inodos
 */
int test_secuencialidad_AI() {
    struct superbloque SB;
    // Leer el superbloque para obtener información del sistema
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_sf() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    // Mostrar encabezado del test
    printf("RECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n");
    
    // Buffer para leer inodos de un bloque completo
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    
    // Variables para el recorrido de la lista enlazada
    int actual;
    int siguiente = SB.posPrimerInodoLibre;
    struct inodo inodo;
    int count = 0;
    bool esSecuencial = true;

    // Recorrer la lista enlazada de inodos libres
    while ((siguiente != UINT_MAX) && (count < SB.totInodos)) {
        // Calcular la posición física del inodo en el array de inodos
        int nbloque = SB.posPrimerBloqueAI + (siguiente * INODOSIZE) / BLOCKSIZE;
        int posInodo = (siguiente * INODOSIZE) % BLOCKSIZE;
        
        // Leer el bloque que contiene el inodo
        if (bread(nbloque, inodos) == -1) {
            perror(RED "Error: leer_sf.c -> mostrar_sf() -> while() -> bread() == FALLO\n");
            printf(RESET);
            break;
        }
        
        // Extraer el inodo específico del bloque leído
        inodo = inodos[posInodo / INODOSIZE];
        actual = siguiente;
        siguiente = inodo.punterosDirectos[0];  // El siguiente inodo libre está en el primer puntero directo
        
        // Verificar secuencialidad solo si hay un siguiente inodo
        if (siguiente != UINT_MAX) {
            esSecuencial = esSecuencial && (siguiente - actual) == 1;
            // Mostrar los primeros inodos de la lista
            if (siguiente <= 10) {
                printf("%d ", siguiente);
            }
            // Mostrar indicador de continuación
            if (siguiente == 10) {
                printf("\n...\n");
            }
            // Mostrar los últimos inodos de la lista
            if (siguiente >= SB.totInodos - 10) {
                printf("%d ", siguiente);
            }
        }
        count++;
    }
    // Mostrar resultado del test de secuencialidad
    printf("\nTest hecho sobre la secuencialidad de la lista AI ha pasado: %s\n\n", esSecuencial ? "Sí" : "No");
    return EXITO;
}

/**
 * Demuestra el funcionamiento de reservar y liberar bloques.
 * Reserva un bloque, muestra el estado antes y después de liberarlo.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si completa la operación correctamente
 * - FALLO (-1) si error en reservar_bloque, liberar_bloque o bread
 */
int reservar_liberar_bloque() {
    printf("RESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    
    // Reservar un bloque del sistema
    int nBloqueReservado = reservar_bloque();
    if (nBloqueReservado == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> reservar_bloque() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", nBloqueReservado);
    
    // Leer el superbloque para mostrar el estado después de reservar
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("SB.cantBloquesLibres: %d\n", SB.cantBloquesLibres);
    
    // Liberar el bloque previamente reservado
    if (liberar_bloque(nBloqueReservado) == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> liberar_bloque() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Leer el superbloque nuevamente para mostrar el estado después de liberar
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> reservar_liberar_bloque() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("Liberamos ese bloque y después SB.cantBloquesLibres = %d\n\n", SB.cantBloquesLibres);
    return EXITO;
}

/**
 * Muestra el estado del mapa de bits en los bordes de las secciones de metadatos.
 * Verifica que los bloques de metadatos estén marcados como ocupados en el bitmap.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si muestra la información correctamente
 * - FALLO (-1) si error al leer el superbloque
 */
int mostrar_bitmap_bordes_seccion() {
    struct superbloque SB;
    // Leer el superbloque para obtener las posiciones de las estructuras de metadatos
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_bitmap_bordes_seccion() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Mostrar encabezado de la sección
    printf("MAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
    
    // Mostrar información detallada del superbloque
    imprimir_info_leer_bit(posSB, SB.posPrimerBloqueMB);
    printf("posSB: %d -> leer_bit(%d) = %d\n", posSB, posSB, leer_bit(posSB));
    
    // Mostrar información del primer bloque del mapa de bits
    imprimir_info_leer_bit(SB.posPrimerBloqueMB, SB.posPrimerBloqueMB);
    printf("posPrimerBloqueMB: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
    
    // Mostrar información del último bloque del mapa de bits
    imprimir_info_leer_bit(SB.posUltimoBloqueMB, SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueMB, SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
    
    // Mostrar información del primer bloque del array de inodos
    imprimir_info_leer_bit(SB.posPrimerBloqueAI, SB.posPrimerBloqueMB);
    printf("posPrimerBloqueAI: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueAI, SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
    
    // Mostrar información del último bloque del array de inodos
    imprimir_info_leer_bit(SB.posUltimoBloqueAI, SB.posPrimerBloqueMB);
    printf("posUltimoBloqueAI: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
    
    // Mostrar información del primer bloque de datos
    imprimir_info_leer_bit(SB.posPrimerBloqueDatos, SB.posPrimerBloqueMB);
    printf("posPrimerBloqueDatos: %d -> leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
    
    // Mostrar información del último bloque de datos
    imprimir_info_leer_bit(SB.posUltimoBloqueDatos, SB.posPrimerBloqueMB);
    printf("posUltimoBloqueDatos: %d -> leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));
    
    // Añadir línea en blanco al final para separar de la siguiente salida
    printf("\n");
    return EXITO;
}

/**
 * Imprime información detallada del cálculo de posición de un bit en el mapa de bits.
 * Muestra los valores intermedios usados para localizar un bit específico.
 * 
 * Parámetros de entrada:
 * - pos: posición del bit a analizar
 * - posPrimerBloqueMB: posición del primer bloque del mapa de bits
 * 
 * Return:
 * - EXITO (0) siempre (función de utilidad)
 */
int imprimir_info_leer_bit(int pos, int posPrimerBloqueMB) {
    // Mostrar todos los cálculos intermedios para localizar un bit específico
    printf(GRAY "leer_bit(%d) -> postbyte: %d, posbyte (ajustado): %d, posbit: %d, nbloquesMB: %d, nbloqueabs: %d\n",
           pos,                                                     // Posición del bit solicitado
           pos / BYTE_SIZE,                                        // Byte que contiene el bit
           (pos / BYTE_SIZE) % BLOCKSIZE,                         // Posición del byte dentro del bloque
           pos % BYTE_SIZE,                                       // Posición del bit dentro del byte
           (pos / BYTE_SIZE) / BLOCKSIZE,                        // Número de bloque relativo en el MB
           ((pos / BYTE_SIZE) / BLOCKSIZE) + posPrimerBloqueMB); // Número de bloque absoluto
    printf(RESET);
    return EXITO;
}

/**
 * Muestra información completa del directorio raíz del sistema de archivos.
 * Reserva el inodo raíz y presenta todos sus metadatos formateados.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si muestra la información correctamente
 * - FALLO (-1) si error en reservar_inodo, bread o leer_inodo
 */
int mostrar_directorio_raiz() {
    // Reservar el primer inodo para el directorio raíz
    int posInodoReservado = reservar_inodo('d', 7);
    if (posInodoReservado == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> reservar_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Leer el superbloque para obtener la posición del inodo raíz
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Leer los metadatos del inodo del directorio raíz
    struct inodo inodo;
    if (leer_inodo(SB.posInodoRaiz, &inodo) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> leer_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Variables para formatear los timestamps
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    char btime[80];
    
    // Mostrar encabezado y metadatos básicos
    printf("DATOS DEL DIRECTORIO RAIZ\n");
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    
    // Formatear y mostrar tiempo de último acceso
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("atime: %s\n", atime);
    
    // Formatear y mostrar tiempo de última modificación
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("mtime: %s\n", mtime);
    
    // Formatear y mostrar tiempo de último cambio de metadatos
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("ctime: %s\n", ctime);
    
    // Formatear y mostrar tiempo de creación
    ts = localtime(&inodo.btime);
    strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("btime: %s\n", btime);
    
    // Mostrar información adicional del inodo
    printf("nlinks: %d\n", inodo.nlinks);
    printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n\n", inodo.numBloquesOcupados);
    return EXITO;
}

/**
 * Muestra información detallada de un inodo específico.
 * Presenta todos los metadatos del inodo incluyendo timestamps formateados.
 * 
 * Parámetros de entrada:
 * - posInodoReservado: número del inodo cuyos datos mostrar
 * 
 * Return:
 * - EXITO (0) si muestra la información correctamente
 * - FALLO (-1) si error al leer el superbloque o el inodo
 */
int mostrar_datos_inodo(int posInodoReservado) {
    // Leer el superbloque (aunque no se usa, mantiene consistencia)
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Leer los metadatos del inodo especificado
    struct inodo inodo;
    if (leer_inodo(posInodoReservado, &inodo) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_directorio_raiz() -> leer_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Variables para formatear los timestamps
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];
    char btime[80];
    
    // Mostrar encabezado y metadatos básicos del inodo
    printf("DATOS DEL INODO RESERVADO\n");
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    
    // Formatear y mostrar tiempo de último acceso
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("atime: %s\n", atime);
    
    // Formatear y mostrar tiempo de última modificación
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("mtime: %s\n", mtime);
    
    // Formatear y mostrar tiempo de último cambio de metadatos
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("ctime: %s\n", ctime);
    
    // Formatear y mostrar tiempo de creación
    ts = localtime(&inodo.btime);
    strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("btime: %s\n", btime);
    
    // Mostrar información adicional del inodo
    printf("nlinks: %d\n", inodo.nlinks);
    printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n\n", inodo.numBloquesOcupados);
    return EXITO;
}

/**
 * Prueba la función buscar_entrada con un camino específico y muestra el resultado.
 * Función de testing que llama a buscar_entrada y maneja los errores.
 * 
 * Parámetros de entrada:
 * - camino: ruta a buscar en el sistema de archivos
 * - reservar: 0=solo consulta, 1=crear si no existe
 * 
 * Return:
 * - void (no retorna valor)
 */
void mostrar_buscar_entrada(char *camino, char reservar) {
    // Variables para almacenar los resultados de buscar_entrada
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    
    #if DEBUGN7
    // Mostrar información de debug sobre la llamada
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    #endif
    
    // Llamar a buscar_entrada y manejar errores si los hay
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
    }
    
    // Separador visual entre pruebas
    printf("**********************************************************************\n");
    return;
}

/**
 * Función principal que ejecuta diferentes tests según las macros de debug activadas.
 * Monta el dispositivo, ejecuta las pruebas correspondientes y desmonta el sistema.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos
 * - argv: array de argumentos (argv[1] debe ser el archivo del dispositivo)
 * 
 * Return:
 * - EXITO (0) si todas las operaciones son exitosas
 * - FALLO (-1) si hay errores en cualquier operación
 */
int main(int argc, char **argv) {
    // Verificar que se proporciona exactamente un argumento (archivo del dispositivo)
    if (argc != 2) {
        perror(RED "Error: leer_sf.c -> main() -> argc != 2\n");
        printf(RESET);
        return FALLO;
    }
    
    // Montar el dispositivo virtual para poder trabajar con el sistema de archivos
    if (bmount(argv[1]) == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> bmount() == FALLO\n");
        printf(RESET);
        return FALLO;
    }

#if DEBUGTMP
    // Test temporal: reservar algunos bloques para verificar funcionamiento básico
    printf("%d\n", reservar_bloque());
    printf("%d\n", reservar_bloque());
    printf("%d\n", reservar_bloque());
    printf("%d\n", reservar_bloque());
#endif

#if DEBUGN3
    // Batería de tests nivel 3: funcionalidades básicas del sistema de archivos
    
    // Mostrar información del superbloque
    if (mostrar_sf() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Verificar secuencialidad de la lista de inodos libres
    if (test_secuencialidad_AI() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> test_secuencialidad_AI() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Probar reserva y liberación de bloques
    if (reservar_liberar_bloque() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> reservar_liberar_bloque() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Mostrar estado del mapa de bits en las fronteras de secciones
    if (mostrar_bitmap_bordes_seccion() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_bitmap_bordes_seccion() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Mostrar información del directorio raíz
    if (mostrar_directorio_raiz() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_directorio_raiz() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
#endif

#if DEBUGN4
    // Batería de tests nivel 4: pruebas de traducción de bloques lógicos a físicos
    
    // Mostrar información básica del sistema
    if (mostrar_sf() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Reservar un inodo para realizar las pruebas de traducción
    unsigned int posInodoReservado = reservar_inodo('f', 6);
    if (posInodoReservado == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> reservar_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Array de bloques lógicos para probar diferentes tipos de punteros
    int test_set[] = {8, 204, 30004, 400004, 468750};
    
    // Probar traducción para cada bloque lógico del conjunto de prueba
    for (int ntest = 0; ntest < (sizeof(test_set) / sizeof(test_set[0])); ntest++) {
        if (traducir_bloque_inodo(posInodoReservado, test_set[ntest], 'f') == FALLO) {
            perror(RED "Error: leer_sf.c -> main() -> for(ntest) -> traducir_bloque_inodo() == FALLO\n");
            printf(RESET);
            return FALLO;
        }
        printf("\n");
    }
    
    // Mostrar los datos del inodo después de las traducciones
    if (mostrar_datos_inodo(posInodoReservado) == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> for(ntest) -> mostrar_datos_inodo() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    // Mostrar estado del superbloque después de las operaciones
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        perror(RED "Error: leer_sf.c -> mostrar_sf() -> bread() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    printf("SB.posPrimerinodoLibre = %d\n", SB.posPrimerInodoLibre);
#endif

#if DEBUGN5
    // Test nivel 5: mostrar solo información básica del superbloque
    if (mostrar_sf() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
#endif

    // Mostrar información del superbloque (ejecutado siempre)
    if (mostrar_sf() == FALLO) {
        perror(RED "Error: leer_sf.c -> main() -> mostrar_sf() == FALLO\n");
        printf(RESET);
        return FALLO;
    }
    
    #if DEBUGN7
    // Batería de tests nivel 7: pruebas de creación de directorios y manejo de errores
    
    mostrar_buscar_entrada("pruebas/", 1);            // ERROR_CAMINO_INCORRECTO
    mostrar_buscar_entrada("/pruebas/", 0);           // ERROR_NO_EXISTE_ENTRADA_CONSULTA
    mostrar_buscar_entrada("/pruebas/docs/", 1);      // ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
    mostrar_buscar_entrada("/pruebas/", 1);           // Crear /pruebas/
    mostrar_buscar_entrada("/pruebas/docs/", 1);      // Crear /pruebas/docs/
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1);  // Crear /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1); // ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
    mostrar_buscar_entrada("/pruebas/", 1);           // ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/docs/doc1", 0);  // Consultar /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1);  // ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/casos/", 1);     // Crear /pruebas/casos/
    mostrar_buscar_entrada("/pruebas/docs/doc2", 1);  // Crear /pruebas/docs/doc2
#endif

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount() == -1) {
        perror(RED "Error: leer_sf.c -> main() -> bumount() == FALLO\n");
        printf(RESET);
    }
    return EXITO;
}