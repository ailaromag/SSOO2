#include "directorios.h"

// Dada una cadena de caracteres *camino (que comience por '/'), separa su contenido en dos: *inicial y *final
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    if (camino == NULL || camino[0] != '/') {
        fprintf(stderr, RED "Error: directorios.c -> extraer_camino() -> if (camino == NULL || camino[0] != '/')" RESET);
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
    printf("*camino: %s \n *inicial: %s \n *final: %s \n *tipo: %c \n", camino, inicial, final, *tipo);

    return EXITO;
}

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    struct inodo inodo_dir;
    char inicial[TAMNOMBRE];
    char final[TAMNOMBRE];
    char tipo;
    int cant_entradas_inodo;
    int num_entradas_inodo;

    if (camino_parcial[0] == '/') {
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

    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO) {
        fprintf(stderr, RED "Error: directorios.c -> buscar_entrada() -> leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO" RESET);
    }

    if ((inodo_dir.permisos & 4) != 4) {
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
    }
    if ((strcmp(inicial, buff_entradas[num_entradas_inodo % (BLOCKSIZE / sizeof(struct entrada))].nombre) != 0) && (num_entradas_inodo == cant_entradas_inodo)) {
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
    switch (error) {
    case -2:
        fprintf(stderr, "Error: Camino incorrecto.\n");
        break;
    case -3:
        fprintf(stderr, "Error: Permiso denegado de lectura.\n");
        break;
    case -4:
        fprintf(stderr, "Error: No existe el archivo o el directorio.\n");
        break;
    case -5:
        fprintf(stderr, "Error: No existe algún directorio intermedio.\n");
        break;
    case -6:
        fprintf(stderr, "Error: Permiso denegado de escritura.\n");
        break;
    case -7:
        fprintf(stderr, "Error: El archivo ya existe.\n");
        break;
    case -8:
        fprintf(stderr, "Error: No es un directorio.\n");
        break;
    }
}