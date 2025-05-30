#include "simulacion.h"

#define NOMBRE_FICHERO_INFORME "informe.txt"
#define TAMANO_NOMBRE_FICHERO_INFORME (TAMANO_DIRECTORIO_SIMULACION + 11)
#define TAMANO_BUFFER_REGISTROS 256
#define TAMANO_INFORME_INDIVIDUAL 384

struct INFORMACION {
    int pid;
    unsigned int nEscrituras;  // validadas
    struct REGISTRO PrimeraEscritura;
    struct REGISTRO UltimaEscritura;
    struct REGISTRO MenorPosicion;
    struct REGISTRO MayorPosicion;
};

int main(int argc, char **argv) {
    // Comprobar el sintaxis
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./verificacion <dispositivo> <directorio_simulacion>\n" RESET);
        return FALLO;
    }

    // Obtenemos los parámetros
    char *dispositivo = argv[1];
    char *directorio_simulacion = argv[2];

    // Montar dispositivo virtual
    if (bmount(dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }

    // Calcular el número de entradas del directorio de simulación.
    struct STAT stat_directorio_simulacion;
    if (mi_stat(directorio_simulacion, &stat_directorio_simulacion) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    }
    int cantidad_entradas = stat_directorio_simulacion.tamEnBytesLog / sizeof(struct entrada);
    if (cantidad_entradas != NUM_PROCESOS) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> cantidad_entradas (%d) != NUM_PROCESOS (%d)\n" RESET, cantidad_entradas, stat_directorio_simulacion.tamEnBytesLog);
        return FALLO;
    }

    // Crear el fichero "informe.txt"
    char fichero_informe[TAMANO_NOMBRE_FICHERO_INFORME];
    strcpy(fichero_informe, directorio_simulacion);
    strcat(fichero_informe, NOMBRE_FICHERO_INFORME);
    if (mi_creat(fichero_informe, 6) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_creat() == FALLO\n" RESET);
        return FALLO;
    }

    // Leemos los directorios correspondientes a los procesos con un buffer
    struct entrada entradas_procesos[cantidad_entradas];
    if (mi_read(directorio_simulacion, entradas_procesos, 0, cantidad_entradas * sizeof(struct entrada)) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_read() == FALLO\n" RESET);
        return FALLO;
    }

    // Iteramos por todos los directorios procesos y guardamos información de "prueba.dat" a "informe.txt"
    struct INFORMACION buffer_informacion[cantidad_entradas];
    struct REGISTRO buffer_escrituras[TAMANO_BUFFER_REGISTROS];
    for (int i = 0; i < cantidad_entradas; i++) {
        // Extraemos el pid de la entrada
        char input[strlen(entradas_procesos[i].nombre) + 1];
        strcpy(input, entradas_procesos[i].nombre);
        strtok(input, "_");
        int pid = atoi(strtok(NULL, "_"));
        buffer_informacion[i].pid = pid;
        buffer_informacion[i].nEscrituras = 0; // Inicializar a 0

        // Construir la ruta completa al archivo prueba.dat
        char ruta_prueba_dat[strlen(directorio_simulacion) + strlen(entradas_procesos[i].nombre) + 12];
        strcpy(ruta_prueba_dat, directorio_simulacion);
        strcat(ruta_prueba_dat, entradas_procesos[i].nombre);
        strcat(ruta_prueba_dat, "/prueba.dat");

        // Verificar que el archivo existe
        struct STAT stat_prueba;
        if (mi_stat(ruta_prueba_dat, &stat_prueba) == FALLO) {
            continue; // Saltar si el archivo no existe
        }

        // Recorremos el fichero y guardamos los registros usando un buffer
        memset(buffer_escrituras, 0, sizeof(buffer_escrituras));
        bool primera_escritura = true;
        int offset_buffer_escritura = 0;
        int bytes_leidos = mi_read(ruta_prueba_dat, buffer_escrituras, offset_buffer_escritura, sizeof(buffer_escrituras));
        
        while (bytes_leidos > 0) {
            int num_registros_leidos = bytes_leidos / sizeof(struct REGISTRO);
            
            for (int j = 0; j < num_registros_leidos; j++) {
                // Verificación más estricta de registros válidos
                if (buffer_escrituras[j].pid != pid || 
                    buffer_escrituras[j].nEscritura <= 0 || 
                    buffer_escrituras[j].nRegistro < 0 ||
                    buffer_escrituras[j].fecha <= 0) {
                    continue;
                }

                if (primera_escritura) {
                    buffer_informacion[i].MayorPosicion = buffer_escrituras[j];
                    buffer_informacion[i].MenorPosicion = buffer_escrituras[j];
                    buffer_informacion[i].PrimeraEscritura = buffer_escrituras[j];
                    buffer_informacion[i].UltimaEscritura = buffer_escrituras[j];
                    buffer_informacion[i].nEscrituras = 1;
                    primera_escritura = false;
                } else {
                    if (buffer_informacion[i].MayorPosicion.nRegistro < buffer_escrituras[j].nRegistro) {
                        buffer_informacion[i].MayorPosicion = buffer_escrituras[j];
                    }
                    if (buffer_informacion[i].MenorPosicion.nRegistro > buffer_escrituras[j].nRegistro) {
                        buffer_informacion[i].MenorPosicion = buffer_escrituras[j];
                    }
                    if (buffer_informacion[i].PrimeraEscritura.nEscritura > buffer_escrituras[j].nEscritura) {
                        buffer_informacion[i].PrimeraEscritura = buffer_escrituras[j];
                    }
                    if (buffer_informacion[i].UltimaEscritura.nEscritura < buffer_escrituras[j].nEscritura) {
                        buffer_informacion[i].UltimaEscritura = buffer_escrituras[j];
                    }
                    buffer_informacion[i].nEscrituras++;
                }
            }
            offset_buffer_escritura += bytes_leidos;
            memset(buffer_escrituras, 0, sizeof(buffer_escrituras));
            bytes_leidos = mi_read(ruta_prueba_dat, buffer_escrituras, offset_buffer_escritura, sizeof(buffer_escrituras));
        }
    }
    // Formateamos el string para introducir a "informe.txt"
    char *buffer_output = malloc(TAMANO_INFORME_INDIVIDUAL * cantidad_entradas);  // Usamos heap porque es bastante grande el string
    char tmp[TAMANO_INFORME_INDIVIDUAL];
    buffer_output[0] = '\0';  // Inicializamos el string para luego poder concatenar
    for (int i = 0; i < cantidad_entradas; i++) {
        snprintf(tmp, TAMANO_INFORME_INDIVIDUAL,
                 "PID: %d\n"
                 "Numero de escrituras: %d\n"
                 "Primera Escritura\t%d\t%d\t%s"
                 "Ultima Escritura\t%d\t%d\t%s"
                 "Menor Posición\t\t%d\t%d\t%s"
                 "Mayor Posición\t\t%d\t%d\t%s\n",
                 buffer_informacion[i].pid,
                 buffer_informacion[i].nEscrituras,
                 buffer_informacion[i].PrimeraEscritura.nEscritura,
                 buffer_informacion[i].PrimeraEscritura.nRegistro,
                 asctime(localtime(&buffer_informacion[i].PrimeraEscritura.fecha)),
                 buffer_informacion[i].UltimaEscritura.nEscritura,
                 buffer_informacion[i].UltimaEscritura.nRegistro,
                 asctime(localtime(&buffer_informacion[i].UltimaEscritura.fecha)),
                 buffer_informacion[i].MenorPosicion.nEscritura,
                 buffer_informacion[i].MenorPosicion.nRegistro,
                 asctime(localtime(&buffer_informacion[i].MenorPosicion.fecha)),
                 buffer_informacion[i].MayorPosicion.nEscritura,
                 buffer_informacion[i].MayorPosicion.nRegistro,
                 asctime(localtime(&buffer_informacion[i].MayorPosicion.fecha)));
        strcat(buffer_output, tmp);
        printf("%s", tmp);
    }

    // Escribimos el contenido al fichero
    if(mi_write(fichero_informe, buffer_output, 0, strlen(buffer_output)) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_write() == FALLO\n" RESET);
        free(buffer_output);
        return FALLO;
    }

    free(buffer_output);

    return 0;
}
