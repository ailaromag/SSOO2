#include "directorios.h"

#define DEBUG_BUSCAR_ENTRADA false
#define DEBUG_MI_WRITE false
#define DEBUG_MI_READ false

// tabla caché directorios
#if (USARCACHE == 2 || USARCACHE == 3)
#define CACHE_SIZE 3  // cantidad de entradas para la caché
static struct UltimaEntrada UltimasEntradasEscritura[CACHE_SIZE];
static struct UltimaEntrada UltimasEntradasLectura[CACHE_SIZE];
#endif

/**
 * Separa un camino en dos partes: inicial (primer componente) y final (resto del camino).
 * Determina si el primer componente es un directorio o fichero según si hay más componentes.
 * 
 * Parámetros de entrada:
 * - camino: cadena que comienza por '/' con la ruta completa
 * - inicial: buffer donde se almacena el primer componente del camino
 * - final: buffer donde se almacena el resto del camino
 * - tipo: puntero donde se almacena 'd' (directorio) o 'f' (fichero)
 * 
 * Return:
 * - EXITO (0) si se procesa correctamente
 * - FALLO (-1) si el camino es NULL o no comienza por '/'
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    // Verificar que el camino es válido y comienza con '/'
    if (camino == NULL || camino[0] != '/') {
        // fprintf(stderr, RED "Error: directorios.c -> extraer_camino() -> if (camino == NULL || camino[0] != '/')" RESET);
        return FALLO;
    }
    // Localizar el primer '/' después del inicial para separar componentes
    char *pos_final = strchr(camino + 1, '/');
    if (pos_final != NULL) {
        // Hay más componentes después, es un directorio
        *tipo = 'd';
        strcpy(final, pos_final);
        int inicial_lenght = pos_final - (camino + 1);
        strncpy(inicial, camino + 1, inicial_lenght);
        inicial[inicial_lenght] = '\0';
    } else {
        // No hay más componentes, es un fichero
        *tipo = 'f';
        strcpy(inicial, camino + 1);
        strcpy(final, "");
    }
    // printf("*camino: %s \n *inicial: %s \n *final: %s \n *tipo: %c \n", camino, inicial, final, *tipo);

    return EXITO;
}

/**
 * Busca una entrada en el sistema de archivos siguiendo un camino dado.
 * Puede reservar un nuevo inodo si la entrada no existe y se solicita.
 * 
 * Parámetros de entrada:
 * - camino_parcial: ruta a buscar en el sistema de archivos
 * - p_inodo_dir: puntero al inodo del directorio donde buscar
 * - p_inodo: puntero donde almacenar el inodo encontrado/creado
 * - p_entrada: puntero donde almacenar la posición de la entrada
 * - reservar: 0=solo consulta, 1=crear si no existe
 * - permisos: permisos para el nuevo inodo si se crea
 * 
 * Return:
 * - EXITO (0) si encuentra/crea la entrada correctamente
 * - ERROR_CAMINO_INCORRECTO (-2) si el camino es inválido
 * - ERROR_PERMISO_LECTURA (-3) si no hay permisos de lectura
 * - ERROR_NO_EXISTE_ENTRADA_CONSULTA (-4) si no existe y es consulta
 * - ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO (-5) si falta directorio intermedio
 * - ERROR_PERMISO_ESCRITURA (-6) si no hay permisos de escritura
 * - ERROR_ENTRADA_YA_EXISTENTE (-7) si ya existe y se intenta crear
 * - ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO (-8) si se intenta crear en fichero
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    mi_waitSem();

    struct entrada entrada = {"", 0};
    struct inodo inodo_dir;
    char inicial[TAMNOMBRE];
    char final[TAMNOMBRE];
    char tipo;
    int cant_entradas_inodo = 0;
    int num_entradas_inodo = 0;

    // Caso especial: directorio raíz
    if (camino_parcial[0] == '/' && strlen(camino_parcial) == 1) {
        struct superbloque sb;
        if (bread(posSB, &sb) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> bread(posSB, &sb) == FALLO" RESET);
        }
        *p_inodo = sb.posInodoRaiz;
        *p_entrada = 0;
        mi_signalSem();
        return EXITO;
    }

    // Extraer el primer componente del camino
    if (extraer_camino(camino_parcial, inicial, final, &tipo) == FALLO) {
        mi_signalSem();
        return ERROR_CAMINO_INCORRECTO;
    }

#if DEBUG_BUSCAR_ENTRADA
    printf(GRAY "[buscar_entrada() -> inicial: %s, final: %s, reservar: %d]\n" RESET, inicial, final, reservar);
#endif

    // Leer el inodo del directorio actual
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO" RESET);
    }

    // Verificar permisos de lectura del directorio
    if ((inodo_dir.permisos & 4) != 4) {
#if DEBUG_BUSCAR_ENTRADA
        printf(GRAY "[buscar_entrada() -> El inodo %d no tiene permisos de lectura]\n" RESET, *p_inodo_dir);
        fflush(stdout);  // Para imprimirlo en orden
#endif
        mi_signalSem();
        return ERROR_PERMISO_LECTURA;
    }

    // Buffer para leer entradas del directorio
    struct entrada buff_entradas[BLOCKSIZE / sizeof(struct entrada)];
    memset(buff_entradas, 0, sizeof(struct entrada));

    // Calcular número de entradas en el directorio
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Buscar la entrada inicial en el directorio si tiene entradas
    if (cant_entradas_inodo > 0) {
        int bytes_leidos = mi_read_f(*p_inodo_dir, buff_entradas, 0, BLOCKSIZE);
        if (bytes_leidos == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> mi_read_f(*p_inodo_dir, buff_entradas, 0, BLOCKSIZE) == FALLO\n" RESET);
            mi_signalSem();
            return FALLO;
        }
        num_entradas_inodo = 0;
        // Recorrer entradas buscando coincidencia con el nombre inicial
        while ((num_entradas_inodo < cant_entradas_inodo) && (strcmp(inicial, buff_entradas[num_entradas_inodo % (BLOCKSIZE / sizeof(struct entrada))].nombre) != 0)) {
            num_entradas_inodo++;
            // Si llegamos al final del bloque, leer el siguiente
            if (num_entradas_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
                memset(buff_entradas, 0, sizeof(struct entrada));
                bytes_leidos += mi_read_f(*p_inodo_dir, buff_entradas, bytes_leidos, BLOCKSIZE);
                if (bytes_leidos == FALLO) {
                    fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> while ((num_entradas_inodo < cant_entradas_inodo) && (strcmp(inicial, buff_entradas[num_entradas_inodo %% (BLOCKSIZE / sizeof(struct entrada))].nombre))) -> mi_read_f(*p_inodo_dir, buff_entradas, bytes_leidos, BLOCKSIZE) == FALLO\n" RESET);
                    mi_signalSem();
                    return FALLO;
                }
            }
        }
        // Copiar la entrada encontrada
        memcpy(&entrada, &buff_entradas[num_entradas_inodo % (BLOCKSIZE / sizeof(struct entrada))], sizeof(struct entrada));
    }
    // Si no se encuentra la entrada y se han recorrido todas
    if ((strcmp(inicial, entrada.nombre) != 0) && (num_entradas_inodo == cant_entradas_inodo)) {
        switch (reservar) {
        case 0:
            // Solo consulta, no crear nueva entrada
            mi_signalSem();    
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            break;
        case 1:
            // Crear nueva entrada
            if (inodo_dir.tipo == 'f') {
                mi_signalSem();
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }
            // Verificar permisos de escritura
            if ((inodo_dir.permisos & 2) != 2) {
                mi_signalSem();
                return ERROR_PERMISO_ESCRITURA;
            } else {
                // Crear nueva entrada con el nombre inicial
                strcpy(entrada.nombre, inicial);
                if (tipo == 'd') {
                    // Solo crear directorio si es el final del camino
                    if (strcmp(final, "/") == 0) {
                        entrada.ninodo = reservar_inodo(tipo, permisos);
                    } else {
                        mi_signalSem();
                        return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                    }
                } else {
                    // Crear fichero
                    entrada.ninodo = reservar_inodo(tipo, permisos);
                }
#if DEBUG_BUSCAR_ENTRADA
                printf(GRAY "[buscar_entrada() -> reservado inodo %d tipo %c con permisos %d para %s]\n" RESET, entrada.ninodo, tipo, permisos, inicial);
#endif
                // Escribir la nueva entrada al directorio
                if (mi_write_f(*p_inodo_dir, &entrada, num_entradas_inodo * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
                    // Si falla la escritura, liberar el inodo reservado
                    if (entrada.ninodo != -1) {
                        if (liberar_inodo(entrada.ninodo) == FALLO) {
                            fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> if (liberar_inodo(entrada.ninodo)) == FALLO\n" RESET);
                            mi_signalSem();
                            return FALLO;
                        }
                    }
                    fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> mi_write_f(*p_inodo_dir, &entrada, num_entradas_inodo * sizeof (struct entrada), sizeof (struct entrada)) == FALLO\n" RESET);
                    mi_signalSem();
                    return FALLO;
                }
#if DEBUG_BUSCAR_ENTRADA
                printf(GRAY "[buscar_entrada() -> creada entrada: %s, %d]\n" RESET, inicial, entrada.ninodo);
#endif
            }
            break;
        }
    }

    // Si hemos llegado al final del camino
    if ((strcmp(final, "/") == 0) || strcmp(final, "") == 0) {
        // Verificar si se intenta crear entrada que ya existe
        if ((num_entradas_inodo < cant_entradas_inodo) && (reservar == 1)) {
            mi_signalSem();
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entradas_inodo;
        mi_signalSem();
        return EXITO;
    } else {
        // Continuar recursivamente con el resto del camino
        *p_inodo_dir = entrada.ninodo;
        mi_signalSem();
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }

    mi_signalSem();
    return EXITO;
}

/**
 * Muestra un mensaje de error correspondiente al código de error de buscar_entrada.
 * Imprime mensajes descriptivos para cada tipo de error posible.
 * 
 * Parámetros de entrada:
 * - error: código de error devuelto por buscar_entrada
 * 
 * Return:
 * - void (no retorna valor)
 */
void mostrar_error_buscar_entrada(int error) {
    // fprintf(stderr, "Error: %d\n", error);
    fprintf(stderr, RED);
    switch (error) {
    case -2:  // ERROR_CAMINO_INCORRECTO
        fprintf(stderr, "Error: Camino incorrecto.\n");
        break;
    case -3:  // ERROR_PERMISO_LECTURA
        fprintf(stderr, "Error: Permiso denegado de lectura.\n");
        break;
    case -4:  // ERROR_NO_EXISTE_ENTRADA_CONSULTA
        fprintf(stderr, "Error: No existe el archivo o el directorio.\n");
        break;
    case -5:  // ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
        fprintf(stderr, "Error: No existe algún directorio intermedio.\n");
        break;
    case -6:  // ERROR_PERMISO_ESCRITURA
        fprintf(stderr, "Error: Permiso denegado de escritura.\n");
        break;
    case -7:  // ERROR_ENTRADA_YA_EXISTENTE
        fprintf(stderr, "Error: El archivo ya existe.\n");
        break;
    case -8:  // ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
        fprintf(stderr, "Error: No es un directorio.\n");
        break;
    }
    fprintf(stderr, RESET);
}

/**
 * Crea un nuevo archivo en el sistema de archivos con los permisos especificados.
 * Utiliza buscar_entrada para crear la entrada y reservar el inodo correspondiente.
 * 
 * Parámetros de entrada:
 * - camino: ruta completa del archivo a crear
 * - permisos: permisos del nuevo archivo (rwx en formato octal)
 * 
 * Return:
 * - EXITO (0) si crea el archivo correctamente
 * - códigos de error de buscar_entrada si hay problemas
 */
int mi_creat(const char *camino, unsigned char permisos) {
    mi_waitSem();

    // Obtener posición del inodo raíz desde el superbloque
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_creat() -> bread(posSB, &sb) == FALLO" RESET);
        mi_signalSem();
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    // Buscar/crear la entrada con reservar=1 para crear si no existe
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    mi_signalSem();

    return error;
}

/**
 * Lista el contenido de un directorio o muestra información de un archivo.
 * Puede mostrar formato simple (ls) o detallado (ls -l) según el flag.
 * 
 * Parámetros de entrada:
 * - camino: ruta del directorio o archivo a listar
 * - buffer: buffer donde almacenar la salida formateada
 * - tipo: 'd' para directorio, 'f' para archivo
 * - flag: 0=formato simple, 1=formato detallado (-l)
 * 
 * Return:
 * - EXITO (0) si lista correctamente
 * - códigos de error de buscar_entrada si hay problemas
 * - FALLO (-1) si hay errores en la operación
 */
int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
    // Obtener posición del inodo raíz desde el superbloque
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    // Buscar la entrada sin reservar (solo consulta)
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 2);  // no reservar y permiso para lectura
    if (error == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 2) == FALLO" RESET);
        return FALLO;
    } else if (error < 0) {
        return error;
    }

    // Obtener información estadística del inodo
    struct STAT inodo_stat;
    if (mi_stat_f(p_inodo, &inodo_stat) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> mi_stat_f(p_inodo, &inode_stat) == FALLO" RESET);
        return FALLO;
    }
    // Verificar permisos de lectura
    if ((inodo_stat.permisos & 4) != 4) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> inodo_stat.permisos & 4 != 4" RESET);
        return FALLO;
    }

    // Array de entradas donde se va guardar todas las entradas para posteriormente imprimirlas
    unsigned int nentrada;
    struct entrada *entradas;

    if (tipo == 'f') {
        // Caso fichero: crear entrada virtual para mostrar información del fichero
        entradas = malloc(sizeof(struct entrada));
        struct entrada entrada_fichero;
        strcpy(entrada_fichero.nombre, camino);
        entrada_fichero.ninodo = p_inodo;
        memcpy(entradas, &entrada_fichero, sizeof(struct entrada));
        nentrada = 1;
    } else if (tipo == 'd') {
        // Caso directorio: leer todas las entradas del directorio
        nentrada = inodo_stat.tamEnBytesLog / sizeof(struct entrada);
        entradas = malloc(nentrada * sizeof(struct entrada));
        mi_read_f(p_inodo, entradas, 0, nentrada * sizeof(struct entrada));
    } else {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> if (tipo != 'd' && tipo != 'f'), tiene que ser un fichero o directorio");
        return FALLO;
    }

    if (flag == 0) {
        // Formato simple (ls): mostrar solo nombres
        strcat(buffer, "Total: ");
        sprintf(buffer + strlen(buffer), "%d", nentrada);
        strcat(buffer, "\n");
        struct entrada entrada_actual;
        for (int i = 0; i < nentrada; i++) {
            entrada_actual = entradas[i];
            strcat(buffer, entrada_actual.nombre);
            strcat(buffer, "\t");
        }
        strcat(buffer, "\0");
        free(entradas);
    } else if (flag == 1) {
        // Formato detallado (ls -l): mostrar información completa
        strcat(buffer, "Total: ");
        sprintf(buffer + strlen(buffer), "%d", nentrada);
        strcat(buffer, "\n");
        strcat(buffer, "Tipo\tModo\tmTime\t\t\tTamaño\tNombre");
        strcat(buffer, "\n");
        strcat(buffer, "------------------------------------------------------");
        strcat(buffer, "\n");
        struct entrada entrada_actual;
        struct STAT stat_actual;
        struct tm *tm;
        for (int i = 0; i < nentrada; i++) {
            entrada_actual = entradas[i];
            // Obtener estadísticas de cada entrada
            mi_stat_f(entrada_actual.ninodo, &stat_actual);
            tm = localtime(&stat_actual.mtime);
            // Formatear tipo de archivo
            sprintf(buffer + strlen(buffer), "%c", stat_actual.tipo);
            strcat(buffer, "\t");
            // Formatear permisos (rwx)
            if ((stat_actual.permisos & 4) == 4) {
                strcat(buffer, "r");
            } else {
                strcat(buffer, "-");
            }
            if ((stat_actual.permisos & 2) == 2) {
                strcat(buffer, "w");
            } else {
                strcat(buffer, "-");
            }
            if ((stat_actual.permisos & 1) == 1) {
                strcat(buffer, "x");
            } else {
                strcat(buffer, "-");
            }
            strcat(buffer, "\t");
            // Formatear fecha de modificación
            sprintf(buffer + strlen(buffer), "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
            strcat(buffer, "\t");
            // Formatear tamaño y nombre
            sprintf(buffer + strlen(buffer), "%d", stat_actual.tamEnBytesLog);
            strcat(buffer, "\t");
            strcat(buffer, entrada_actual.nombre);
            strcat(buffer, "\n");
        }
        strcat(buffer, "\0");
        free(entradas);
    } else {
        free(entradas);
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> if (tipo != 0 && tipo != 1), tiene que ser un 0 (ls) o un 1 (ls -l)");
        return FALLO;
    }
    return error;
}

/**
 * Cambia los permisos de un archivo o directorio existente.
 * Busca la entrada y modifica los permisos del inodo correspondiente.
 * 
 * Parámetros de entrada:
 * - camino: ruta del archivo/directorio a modificar
 * - permisos: nuevos permisos en formato octal (rwx)
 * 
 * Return:
 * - EXITO (0) si cambia los permisos correctamente
 * - códigos de error de buscar_entrada si hay problemas
 * - FALLO (-1) si error en mi_chmod_f
 */
int mi_chmod(const char *camino, unsigned char permisos) {
    // Obtener posición del inodo raíz desde el superbloque
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_chmod() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    // Buscar la entrada del archivo/directorio
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos);
    // Cambiar permisos del inodo encontrado
    if (mi_chmod_f(p_inodo, permisos) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_chmod() -> mi_chmod_f(p_inodo, permisos) == FALLO" RESET);
        return FALLO;
    }
    return error;
}

/**
 * Obtiene información estadística de un archivo o directorio.
 * Busca la entrada y lee los metadatos del inodo correspondiente.
 * 
 * Parámetros de entrada:
 * - camino: ruta del archivo/directorio a consultar
 * - p_stat: puntero a estructura donde almacenar la información
 * 
 * Return:
 * - número del inodo si obtiene la información correctamente
 * - códigos de error de buscar_entrada si hay problemas
 * - FALLO (-1) si error en mi_stat_f
 */
int mi_stat(const char *camino, struct STAT *p_stat) {
    // Obtener posición del inodo raíz desde el superbloque
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_stat() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    // Buscar la entrada del archivo/directorio
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 2);
    if (error < 0) {
        return error;
    }
    // Obtener estadísticas del inodo encontrado
    if (mi_stat_f(p_inodo, p_stat) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_stat() -> mi_stat_f(p_inodo, p_stat) == FALLO" RESET);
        return FALLO;
    }
    return p_inodo;
}

/**
 * Escribe datos en un archivo utilizando caché LRU para optimizar accesos.
 * Busca el inodo en caché o mediante buscar_entrada si no está cacheado.
 * 
 * Parámetros de entrada:
 * - camino: ruta del archivo donde escribir
 * - buf: buffer con los datos a escribir
 * - offset: posición en bytes donde comenzar a escribir
 * - nbytes: número de bytes a escribir
 * 
 * Return:
 * - número de bytes escritos si éxito
 * - códigos de error de buscar_entrada si hay problemas
 * - FALLO (-1) si error en mi_write_f
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo;
    bool found;
    // Intentar leer del caché de escritura
    int read_index = leer_cache_lru(UltimasEntradasEscritura, camino, &p_inodo, &found);
    if (read_index == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_write() -> leer_cache_lru(UltimasEntradasEscritura, camino, p_inodo, &found) == FALLO" RESET);
        return FALLO;
    }
    if (found == false) {
        // No está en caché, buscar entrada en el sistema de archivos
        struct superbloque sb;
        if (bread(posSB, &sb) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_write() -> bread(posSB, &sb) == FALLO" RESET);
            return FALLO;
        }
        unsigned int p_inodo_dir = sb.posInodoRaiz;
        unsigned int p_entrada;
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
        if (error < 0) {
            return error;
        }
        // Actualizar el caché con la nueva entrada
        int written_index = escribir_cache_lru(UltimasEntradasEscritura, camino, p_inodo);
        if (written_index == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_write() -> escribir_cache_lru(UltimasEntradasEscritura, camino, p_inodo) == FALLO" RESET);
            return FALLO;
        }
#if DEBUG_MI_WRITE
        printf(ORANGE "[mi_write() -> Reemplazamos cache[%d]: %s]\n" RESET, written_index, camino);
#endif
    } else {
#if DEBUG_MI_READ
        printf(BLUE "[mi_write() -> Utilizamos cache[%d]: %s]\n" RESET, read_index, camino);
#endif
    }
    // Escribir datos al archivo usando el inodo obtenido
    int bytes_written = mi_write_f(p_inodo, buf, offset, nbytes);
    if (bytes_written == FALLO) {
        // fprintf(stderr, RED "Error: directorios.c -> mi_write() -> mi_write_f(p_inodo, buf, offset, nbytes) == FALLO\n" RESET);
        return FALLO;
    }
    return bytes_written;
}

/**
 * Lee datos de un archivo utilizando caché LRU para optimizar accesos.
 * Busca el inodo en caché o mediante buscar_entrada si no está cacheado.
 * 
 * Parámetros de entrada:
 * - camino: ruta del archivo a leer
 * - buf: buffer donde almacenar los datos leídos
 * - offset: posición en bytes donde comenzar a leer
 * - nbytes: número de bytes a leer
 * 
 * Return:
 * - número de bytes leídos si éxito
 * - códigos de error de buscar_entrada si hay problemas
 * - FALLO (-1) si error en mi_read_f
 */
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo;
    bool found;
    // Intentar leer del caché de lectura
    if (leer_cache_lru(UltimasEntradasLectura, camino, &p_inodo, &found) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_read() -> leer_cache_lru(UltimasEntradasLectura, camino, p_inodo, &found) == FALLO" RESET);
        return FALLO;
    }
    if (found == false) {
        // No está en caché, buscar entrada en el sistema de archivos
        struct superbloque sb;
        if (bread(posSB, &sb) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_read() -> bread(posSB, &sb) == FALLO" RESET);
            return FALLO;
        }
        unsigned int p_inodo_dir = sb.posInodoRaiz;
        unsigned int p_entrada;
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
        if (error < 0) {
            return error;
        }
        // Actualizar el caché con la nueva entrada
        if (escribir_cache_lru(UltimasEntradasLectura, camino, p_inodo) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_read() -> escribir_cache_lru(UltimasEntradasLectura, camino, p_inodo) == FALLO" RESET);
            return FALLO;
        }
#if DEBUG_MI_READ
        printf(ORANGE "[mi_read() -> Actualizamos la caché de lectura]\n" RESET);
#endif
    } else {
#if DEBUG_MI_READ
        printf(BLUE "[mi_read() -> Utilizamos la caché de lectura en vez de llamar a buscar_entrada()]\n" RESET);
#endif
    }
    // Leer datos del archivo usando el inodo obtenido
    int bytes_read = mi_read_f(p_inodo, buf, offset, nbytes);
    if (bytes_read == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_read() -> mi_read_f(p_inodo, buf, offset, nbytes) == FALLO" RESET);
        return FALLO;
    }
    return bytes_read;
}

/**
 * Busca una entrada en la caché LRU y actualiza su tiempo de última consulta.
 * Implementa política LRU (Least Recently Used) para gestión de caché.
 * 
 * Parámetros de entrada:
 * - UltimasEntradas: array de entradas de la caché
 * - camino: ruta a buscar en la caché
 * - p_inodo: puntero donde almacenar el inodo si se encuentra
 * - found: puntero a booleano que indica si se encontró
 * 
 * Return:
 * - índice de la entrada encontrada en la caché
 * - FALLO (-1) si error en gettimeofday
 */
int leer_cache_lru(struct UltimaEntrada *UltimasEntradas, const char *camino, unsigned int *p_inodo, bool *found) {
    *found = false;
    int found_index;
    // Buscar el camino en todas las entradas de la caché
    for (int i = 0; i < CACHE_SIZE && *found == false; i++) {
        if (strcmp(UltimasEntradas[i].camino, camino) == 0) {
            // Actualizar el tiempo de consulta para política LRU
            if (gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL) == FALLO) {
                fprintf(stderr, RED "Error: directorios.c -> leer_cache_lru() -> gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL) == FALLO" RESET);
                return FALLO;
            }
            *p_inodo = UltimasEntradas[i].p_inodo;
            *found = true;
            found_index = i;
        }
    }
    return found_index;
}

/**
 * Escribe una nueva entrada en la caché LRU reemplazando la menos recientemente usada.
 * Implementa política de reemplazo LRU para mantener las entradas más accedidas.
 * 
 * Parámetros de entrada:
 * - UltimasEntradas: array de entradas de la caché
 * - camino: ruta a almacenar en la caché
 * - p_inodo: número de inodo a asociar con el camino
 * 
 * Return:
 * - índice donde se almacenó la nueva entrada
 * - FALLO (-1) si error en gettimeofday
 */
int escribir_cache_lru(struct UltimaEntrada *UltimasEntradas, const char *camino, unsigned int p_inodo) {
    int oldest_index = 0;
    struct timeval curr_timeval = UltimasEntradas[0].ultima_consulta;
    // Encontrar la entrada menos recientemente usada
    for (int i = 1; i < CACHE_SIZE; i++) {
        if (UltimasEntradas[i].ultima_consulta.tv_sec < curr_timeval.tv_sec || (UltimasEntradas[i].ultima_consulta.tv_sec == curr_timeval.tv_sec && UltimasEntradas[i].ultima_consulta.tv_usec < curr_timeval.tv_usec)) {
            oldest_index = i;
            curr_timeval = UltimasEntradas[i].ultima_consulta;
        }
    }
    // Reemplazar la entrada más antigua con los nuevos datos
    strcpy(UltimasEntradas[oldest_index].camino, camino);
    UltimasEntradas[oldest_index].p_inodo = p_inodo;
    if (gettimeofday(&UltimasEntradas[oldest_index].ultima_consulta, NULL) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> escribir_cache_lru() -> gettimeofday(&UltimasEntradas[oldest_index].ultima_consulta, NULL) == FALLO\n" RESET);
        return FALLO;
    }
    return oldest_index;
}

/**
 * Crea un enlace duro entre dos rutas, haciendo que ambas apunten al mismo inodo.
 * Incrementa el contador de enlaces del inodo y crea nueva entrada de directorio.
 * 
 * Parámetros de entrada:
 * - camino1: ruta del archivo existente (origen del enlace)
 * - camino2: ruta del nuevo enlace a crear (destino)
 * 
 * Return:
 * - EXITO (0) si crea el enlace correctamente
 * - códigos de error de buscar_entrada si hay problemas
 * - FALLO (-1) si hay errores en las operaciones
 */
int mi_link(const char *camino1, const char *camino2) {
    // Obtener el superbloque para la posición del inodo raíz
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> bread(posSB, &sb) == FALLO\n" RESET);
        return FALLO;
    }
    // Buscar la entrada del archivo origen (camino1)
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_entrada;
    unsigned int p_inodo;

    // Comienzo de sección crítica
    mi_waitSem();

    int error = buscar_entrada(camino1, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> buscar_entrada(camino1, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    } else if (error < 0) {
        mi_signalSem();
        return error;
    }
    // Verificar que camino1 es un fichero y tiene permisos de lectura
    struct STAT stat;
    if (mi_stat_f(p_inodo, &stat) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> mi_stat_f(p_inodo, &stat) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    if (stat.tipo != 'f') {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> if (stat.tipo != 'f') == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    if ((stat.permisos & 4) != 4) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> if ((stat.permisos & 4) != 4)\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Crear la nueva entrada para camino2
    unsigned int p_inodo_previo = p_inodo;
    p_inodo_dir = sb.posInodoRaiz;
    error = buscar_entrada(camino2, &p_inodo_dir, &p_inodo, &p_entrada, 1, 6);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> buscar_entrada(camino2, &p_inodo_dir, &p_inodo, &p_entrada, 1, 6) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    } else if (error < 0) {
        mi_signalSem();
        return error;
    }
    // Liberar el inodo creado para camino2 (vamos a usar el de camino1)
    if (liberar_inodo(p_inodo) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> liberar_inodo(p_inodo) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Modificar la entrada de camino2 para que apunte al inodo de camino1
    struct entrada entrada;
    if (mi_read_f(p_inodo_dir, &entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> mi_read_f(p_inodo_dir, &entrada, p_entrada * sizeof (struct entrada), sizeof(struct entrada)) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    entrada.ninodo = p_inodo_previo;
    if (mi_write_f(p_inodo_dir, &entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> mi_write_f(p_inodo_dir, &entrada, p_entrada * sizeof (struct entrada), sizeof(struct entrada)) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Incrementar contador de enlaces y actualizar ctime del inodo
    struct inodo inodo;
    if (leer_inodo(p_inodo_previo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> leer_inodo(p_inodo_previo, &inodo) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    inodo.nlinks++;
    inodo.ctime = time(NULL);
    if (escribir_inodo(p_inodo_previo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_link() -> escribir_inodo(p_inodo_previo, &inodo) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Fin de sección crítica
    mi_signalSem();

    return EXITO;
}

/**
 * Elimina un enlace (entrada de directorio) y decrementa el contador de enlaces del inodo.
 * Si es el último enlace, libera el inodo. No puede eliminar directorios no vacíos.
 * 
 * Parámetros de entrada:
 * - camino: ruta del archivo/directorio a eliminar
 * 
 * Return:
 * - EXITO (0) si elimina el enlace correctamente
 * - códigos de error de buscar_entrada si hay problemas
 * - FALLO (-1) si hay errores en las operaciones o directorio no vacío
 */
int mi_unlink(const char *camino) {
    // No puede borrar directorio raíz
    if (strcmp(camino, "/") == 0) {
        fprintf(stderr, "Error: no se puede eliminar el inodo raiz '/'");
        return FALLO;
    }
    // Obtener el superbloque para la posición del inodo raíz
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> bread(posSB, &sb) == FALLO\n" RESET);
        return FALLO;
    }
    // Buscar la entrada del archivo/directorio a eliminar
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_entrada;
    unsigned int p_inodo;

    // Comienzo sección crítica
    mi_waitSem();

    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
    if (error == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> buscar_entrada(camino1, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    } else if (error < 0) {
        mi_signalSem();
        return error;
    }
    // Verificar que si es directorio esté vacío
    struct STAT stat;
    if (mi_stat_f(p_inodo, &stat) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> mi_stat_f(p_inodo, &stat) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    if (stat.tipo == 'd' && stat.tamEnBytesLog > 0) {
        fprintf(stderr, RED "Error: El directorio /dir2/dir21/ no está vacío\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Obtener información del directorio contenedor
    if (mi_stat_f(p_inodo_dir, &stat) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> mi_stat_f(p_inodo_dir, &stat) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Calcular posición de la última entrada del directorio
    int p_ultima_entrada = (stat.tamEnBytesLog / sizeof(struct entrada)) - 1;
    if (p_ultima_entrada > p_entrada) {
        // Si no es la última entrada, mover la última entrada a la posición eliminada
        struct entrada ultima_entrada;
        if (mi_read_f(p_inodo_dir, &ultima_entrada, p_ultima_entrada * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> mi_read_f(p_inodo_dir, &ultima_entrada, p_entrada * sizeof (struct entrada), sizeof (struct entrada)) == FALLO\n" RESET);
            mi_signalSem();
            return FALLO;
        }
        if (mi_write_f(p_inodo_dir, &ultima_entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> mi_write_f(p_inodo_dir, &ultima_entrada, p_entrada * sizeof (struct entrada), sizeof (struct entrada)) == FALLO\n" RESET);
            mi_signalSem();
            return FALLO;
        }
    }
    // Truncar el directorio para eliminar la última entrada
    if (mi_truncar_f(p_inodo_dir, p_ultima_entrada * sizeof(struct entrada)) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> mi_truncar_f(p_inodo_dir, 0) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    // Decrementar contador de enlaces del inodo
    struct inodo inodo;
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> leer_inodo(p_inodo, &inodo) == FALLO\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    inodo.nlinks--;
    // Si no quedan enlaces, liberar el inodo
    if (inodo.nlinks <= 0) {
        if (liberar_inodo(p_inodo) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> liberar_inodo(p_inodo) == FALLO\n" RESET);
            mi_signalSem();
            return FALLO;
        }
    } else {
        // Actualizar ctime y escribir el inodo modificado
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> mi_unlink() -> escribir_inodo(p_inodo, &inodo) == FALLO\n" RESET);
            mi_signalSem();
            return FALLO;
        }
    }

    // Fin sección crítica
    mi_signalSem();

    return EXITO;
}