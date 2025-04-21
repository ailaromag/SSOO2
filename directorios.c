#include "directorios.h"

#define DEBUG_BUSCAR_ENTRADA false

// Dada una cadena de caracteres *camino (que comience por '/'), separa su contenido en dos: *inicial y *final
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    if (camino == NULL || camino[0] != '/') {
        // fprintf(stderr, RED "Error: directorios.c -> extraer_camino() -> if (camino == NULL || camino[0] != '/')" RESET);
        return FALLO;
    }
    // Obtenemos inicial
    // Localiza el primer '/' después del inicial
    char *pos_final = strchr(camino + 1, '/');
    if (pos_final != NULL) {
        *tipo = 'd';
        strcpy(final, pos_final);
        int inicial_lenght = pos_final - (camino + 1);
        strncpy(inicial, camino + 1, inicial_lenght);
        inicial[inicial_lenght] = '\0';
    } else {
        *tipo = 'f';
        strcpy(inicial, camino + 1);
        strcpy(final, "");
    }
    // printf("*camino: %s \n *inicial: %s \n *final: %s \n *tipo: %c \n", camino, inicial, final, *tipo);

    return EXITO;
}

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada = {"", 0};
    struct inodo inodo_dir;
    char inicial[TAMNOMBRE];
    char final[TAMNOMBRE];
    char tipo;
    int cant_entradas_inodo = 0;
    int num_entradas_inodo = 0;

    if (camino_parcial[0] == '/' && strlen(camino_parcial) == 1) {
        struct superbloque sb;
        if (bread(posSB, &sb) == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> bread(posSB, &sb) == FALLO" RESET);
        }
        *p_inodo = sb.posInodoRaiz;
        *p_entrada = 0;
        return EXITO;
    }

    if (extraer_camino(camino_parcial, inicial, final, &tipo) == FALLO) {
        return ERROR_CAMINO_INCORRECTO;
    }

#if DEBUG_BUSCAR_ENTRADA
    printf(GRAY "[buscar_entrada() -> inicial: %s, final: %s, reservar: %d]\n" RESET, inicial, final, reservar);
#endif

    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO" RESET);
    }

    if ((inodo_dir.permisos & 4) != 4) {
#if DEBUG_BUSCAR_ENTRADA
        printf(GRAY "[buscar_entrada() -> El inodo %d no tiene permisos de lectura]\n" RESET, *p_inodo_dir);
        fflush(stdout);  // Para imprimirlo en orden
#endif
        return ERROR_PERMISO_LECTURA;
    }

    struct entrada buff_entradas[BLOCKSIZE / sizeof(struct entrada)];
    memset(buff_entradas, 0, sizeof(struct entrada));

    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    if (cant_entradas_inodo > 0) {
        int bytes_leidos = mi_read_f(*p_inodo_dir, buff_entradas, 0, BLOCKSIZE);
        if (bytes_leidos == FALLO) {
            fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> mi_read_f(*p_inodo_dir, buff_entradas, 0, BLOCKSIZE) == FALLO\n" RESET);
            return FALLO;
        }
        num_entradas_inodo = 0;
        while ((num_entradas_inodo < cant_entradas_inodo) && (strcmp(inicial, buff_entradas[num_entradas_inodo % (BLOCKSIZE / sizeof(struct entrada))].nombre) != 0)) {
            num_entradas_inodo++;
            if (num_entradas_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
                memset(buff_entradas, 0, sizeof(struct entrada));
                bytes_leidos += mi_read_f(*p_inodo_dir, buff_entradas, bytes_leidos, BLOCKSIZE);
                if (bytes_leidos == FALLO) {
                    fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> while ((num_entradas_inodo < cant_entradas_inodo) && (strcmp(inicial, buff_entradas[num_entradas_inodo %% (BLOCKSIZE / sizeof(struct entrada))].nombre))) -> mi_read_f(*p_inodo_dir, buff_entradas, bytes_leidos, BLOCKSIZE) == FALLO\n" RESET);
                    return FALLO;
                }
            }
        }
        memcpy(&entrada, &buff_entradas[num_entradas_inodo % (BLOCKSIZE / sizeof(struct entrada))], sizeof(struct entrada));
    }
    if ((strcmp(inicial, entrada.nombre) != 0) && (num_entradas_inodo == cant_entradas_inodo)) {
        switch (reservar) {
        case 0:
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            break;
        case 1:
            if (inodo_dir.tipo == 'f') {
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }
            if ((inodo_dir.permisos & 2) != 2) {
                return ERROR_PERMISO_ESCRITURA;
            } else {
                strcpy(entrada.nombre, inicial);
                if (tipo == 'd') {
                    if (strcmp(final, "/") == 0) {
                        entrada.ninodo = reservar_inodo(tipo, permisos);
                    } else {
                        return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                    }
                } else {
                    entrada.ninodo = reservar_inodo(tipo, permisos);
                }
#if DEBUG_BUSCAR_ENTRADA
                printf(GRAY "[buscar_entrada() -> reservado inodo %d tipo %c con permisos %d para %s]\n" RESET, entrada.ninodo, tipo, permisos, inicial);
#endif
                if (mi_write_f(*p_inodo_dir, &entrada, num_entradas_inodo * sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
                    if (entrada.ninodo != -1) {
                        if (liberar_inodo(entrada.ninodo) == FALLO) {
                            fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> if (liberar_inodo(entrada.ninodo)) == FALLO\n" RESET);
                            return FALLO;
                        }
                    }
                    fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> mi_write_f(*p_inodo_dir, &entrada, num_entradas_inodo * sizeof (struct entrada), sizeof (struct entrada)) == FALLO\n" RESET);
                    return FALLO;
                }
#if DEBUG_BUSCAR_ENTRADA
                printf(GRAY "[buscar_entrada() -> creada entrada: %s, %d]\n" RESET, inicial, entrada.ninodo);
#endif
            }
            break;
        }
    }

    if ((strcmp(final, "/") == 0) || strcmp(final, "") == 0) {
        if ((num_entradas_inodo < cant_entradas_inodo) && (reservar == 1)) {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entradas_inodo;
        return EXITO;
    } else {
        *p_inodo_dir = entrada.ninodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
    return EXITO;
}

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

int mi_creat(const char *camino, unsigned char permisos) {
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_creat() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    return error;
}

int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 2);  // no reservar y permiso para lectura
    if (error == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 2) == FALLO" RESET);
        return FALLO;
    } else if (error < 0) {
        return error;
    }

    // Miramos el tamaño y los permisos
    struct STAT inodo_stat;
    if (mi_stat_f(p_inodo, &inodo_stat) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> mi_stat_f(p_inodo, &inode_stat) == FALLO" RESET);
        return FALLO;
    }
    if ((inodo_stat.permisos & 2) != 2) {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> inodo_stat.permisos & 2 != 2" RESET);
        return FALLO;
    }

    // Array de entradas donde se va guardar todas las entradas para posteriormente imprimirlas
    unsigned int nentrada;
    struct entrada *entradas;

    if (tipo == 'f') {
        // Caso fichero
        entradas = malloc(sizeof(struct entrada));
        struct entrada entrada_fichero;
        strcpy(entrada_fichero.nombre, camino);
        entrada_fichero.ninodo = p_inodo;
        memcpy(entradas, &entrada_fichero, sizeof(struct entrada));
        nentrada = 1;
    } else if (tipo == 'd') {
        // Caso directorio
        unsigned int offset = 0;
        nentrada = inodo_stat.tamEnBytesLog / sizeof(struct entrada);
        entradas = malloc(nentrada * sizeof(struct entrada));
        for (int i = 0; i < nentrada; i++) {
            mi_read_f(p_inodo, &entradas[i], offset, sizeof(struct entrada));
            offset += sizeof(struct entrada);
        }
    } else {
        fprintf(stderr, RED "Error: directorios.c -> mi_dir() -> if (tipo != 'd' && tipo != 'f'), tiene que ser un fichero o directorio");
        return FALLO;
    }

    if (flag == 0) {
        // Caso ls normal
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
        // Caso ls -l
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
            mi_stat_f(entrada_actual.ninodo, &stat_actual);
            tm = localtime(&stat_actual.mtime);
            sprintf(buffer + strlen(buffer), "%c", stat_actual.tipo);
            strcat(buffer, "\t");
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
            sprintf(buffer + strlen(buffer), "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
            strcat(buffer, "\t");
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

int mi_chmod(const char *camino, unsigned char permisos) {
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_chmod() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos);
    if (mi_chmod_f(p_inodo, permisos) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_chmod() -> mi_chmod_f(p_inodo, permisos) == FALLO" RESET);
        return FALLO;
    }
    return error;
}

int mi_stat(const char *camino, struct STAT *p_stat) {
    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_stat() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;
    unsigned int p_entrada;
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 2);
    if (error < 0) {
        return error;
    }
    if (mi_stat_f(p_inodo, p_stat) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_stat() -> mi_stat_f(p_inodo, p_stat) == FALLO" RESET);
        return FALLO;
    }
    return p_inodo;
}


int mi_write(const char *camino, const char *buf, unsigned int offset, unsigned int nbytes){

    struct superbloque sb;
    if (bread(posSB, &sb) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> mi_write() -> bread(posSB, &sb) == FALLO" RESET);
        return FALLO;
    }
    unsigned int p_inodo_dir = sb.posInodoRaiz;
    unsigned int p_inodo;      
    unsigned int p_entrada;    

    // Buscar la entrada para obtener el inodo
    int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0,6);
    if (error < 0) {
        fprintf(stderr, RED "Error: directorios.c -> mi_write() -> buscar_entrada() == FALLO" RESET);
        return error;
    }

    int bytes_escritos = mi_write_f(p_inodo, buf, offset, nbytes);
    if (bytes_escritos < 0) {
        fprintf(stderr, RED "Error: directorios.c -> mi_write() -> mi_write_f()== FALLO" RESET);
        return bytes_escritos; // Devuelve el código de error de mi_write_f
    }

    return bytes_escritos; // Retorna la cantidad de bytes escritos
}

int mi_read(const char *camino, char *buf, unsigned int offset, unsigned int nbytes){
        
struct superbloque sb;
if (bread(posSB, &sb) == FALLO) {
    fprintf(stderr, RED "Error: directorios.c -> mi_read() -> bread(posSB, &sb) == FALLO" RESET);
    return FALLO;
}
// Variables para búsqueda
unsigned int p_inodo_dir = sb.posInodoRaiz;
unsigned int p_inodo, p_entrada;
    
// Buscar la entrada para obtener el inodo del fichero
if (buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6) < 0) {
    fprintf(stderr, RED "Error: directorios.c -> mi_read() -> buscar_entrada() == FALLO" RESET);
    return FALLO;
}
// Llamar a la función de la capa de ficheros para leer los datos
int bytes_leidos = mi_read_f(p_inodo, buf, offset, nbytes);
if (bytes_leidos < 0) {
    fprintf(stderr, RED "Error: directorios.c -> mi_read() -> mi_write_f()== FALLO" RESET);
    return FALLO;
}
    
return bytes_leidos; // Retornar cantidad de bytes leídos
}
