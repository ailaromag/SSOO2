#include "simulacion.h"

// Definiciones de constantes para el programa
#define NOMBRE_FICHERO_INFORME "informe.txt"
#define TAMANO_NOMBRE_FICHERO_INFORME (TAMANO_DIRECTORIO_SIMULACION + 11)
#define TAMANO_BUFFER_REGISTROS 256
#define TAMANO_INFORME_INDIVIDUAL 384

// Estructura para almacenar información estadística de cada proceso
struct INFORMACION {
    int pid;                                 // ID del proceso
    unsigned int nEscrituras;               // Número total de escrituras validadas
    struct REGISTRO PrimeraEscritura;       // Primer registro cronológicamente
    struct REGISTRO UltimaEscritura;        // Último registro cronológicamente
    struct REGISTRO MenorPosicion;          // Registro con menor número de registro
    struct REGISTRO MayorPosicion;          // Registro con mayor número de registro
};

/**
 * Programa principal que verifica y analiza los resultados de una simulación concurrente.
 * Lee los archivos generados por simulacion.c, extrae estadísticas de cada proceso
 * y genera un informe detallado con información cronológica y posicional.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 3)
 * - argv: array de argumentos donde argv[1] es el dispositivo y argv[2] el directorio de simulación
 * 
 * Return:
 * - EXITO (0) si genera el informe correctamente
 * - FALLO (-1) si error en argumentos, montaje, lectura de datos, generación del informe o desmontaje
 */
int main(int argc, char **argv) {
    // Verificar que se proporcionan exactamente 2 argumentos de línea de comandos
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./verificacion <dispositivo> <directorio_simulacion>\n" RESET);
        return FALLO;
    }

    // Extraer argumentos de línea de comandos
    char *dispositivo = argv[1];            // Archivo del dispositivo virtual
    char *directorio_simulacion = argv[2];  // Directorio donde están los datos de simulación

    // Montar el dispositivo virtual para poder acceder al sistema de archivos
    if (bmount(dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }

    // Obtener información estadística del directorio de simulación
    struct STAT stat_directorio_simulacion;
    if (mi_stat(directorio_simulacion, &stat_directorio_simulacion) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_stat() == FALLO\n" RESET);
        return FALLO;
    }
    
    // Calcular el número de entradas (procesos) en el directorio
    int cantidad_entradas = stat_directorio_simulacion.tamEnBytesLog / sizeof(struct entrada);
    
    // Verificar que el número de entradas coincide con el número esperado de procesos
    if (cantidad_entradas != NUM_PROCESOS) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> cantidad_entradas (%d) != NUM_PROCESOS (%d)\n" RESET, cantidad_entradas, stat_directorio_simulacion.tamEnBytesLog);
        return FALLO;
    }

    // Construir la ruta completa para el archivo de informe
    char fichero_informe[TAMANO_NOMBRE_FICHERO_INFORME];
    strcpy(fichero_informe, directorio_simulacion);
    strcat(fichero_informe, NOMBRE_FICHERO_INFORME);
    
    // Crear el archivo de informe con permisos de lectura y escritura
    if (mi_creat(fichero_informe, 6) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_creat() == FALLO\n" RESET);
        return FALLO;
    }

    // Leer todas las entradas del directorio de simulación en un buffer
    struct entrada entradas_procesos[cantidad_entradas];
    if (mi_read(directorio_simulacion, entradas_procesos, 0, cantidad_entradas * sizeof(struct entrada)) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_read() == FALLO\n" RESET);
        return FALLO;
    }

    // Inicializar buffers para procesar la información
    struct INFORMACION buffer_informacion[cantidad_entradas];  // Información estadística por proceso
    struct REGISTRO buffer_escrituras[TAMANO_BUFFER_REGISTROS]; // Buffer para leer registros
    
    // Procesar cada directorio de proceso
    for (int i = 0; i < cantidad_entradas; i++) {
        // Extraer el PID del nombre del directorio (formato: proceso_PID)
        char input[strlen(entradas_procesos[i].nombre) + 1];
        strcpy(input, entradas_procesos[i].nombre);
        strtok(input, "_");                    // Saltar "proceso"
        int pid = atoi(strtok(NULL, "_"));     // Extraer PID
        
        // Inicializar la información del proceso
        buffer_informacion[i].pid = pid;
        buffer_informacion[i].nEscrituras = 0;

        // Construir la ruta completa al archivo prueba.dat del proceso
        char ruta_prueba_dat[strlen(directorio_simulacion) + strlen(entradas_procesos[i].nombre) + 12];
        strcpy(ruta_prueba_dat, directorio_simulacion);
        strcat(ruta_prueba_dat, entradas_procesos[i].nombre);
        strcat(ruta_prueba_dat, "/prueba.dat");

        // Verificar que el archivo prueba.dat existe
        struct STAT stat_prueba;
        if (mi_stat(ruta_prueba_dat, &stat_prueba) == FALLO) {
            continue; // Saltar este proceso si el archivo no existe
        }

        // Inicializar variables para el procesamiento de registros
        memset(buffer_escrituras, 0, sizeof(buffer_escrituras));
        bool primera_escritura = true;        // Flag para identificar el primer registro válido
        int offset_buffer_escritura = 0;      // Offset para leer el archivo por chunks
        
        // Leer el archivo prueba.dat en chunks usando el buffer
        int bytes_leidos = mi_read(ruta_prueba_dat, buffer_escrituras, offset_buffer_escritura, sizeof(buffer_escrituras));
        
        // Procesar todos los chunks del archivo
        while (bytes_leidos > 0) {
            // Calcular cuántos registros completos se leyeron
            int num_registros_leidos = bytes_leidos / sizeof(struct REGISTRO);
            
            // Procesar cada registro en el chunk actual
            for (int j = 0; j < num_registros_leidos; j++) {
                // Validar que el registro es válido y pertenece al proceso correcto
                if (buffer_escrituras[j].pid != pid || 
                    buffer_escrituras[j].nEscritura < 0 || 
                    buffer_escrituras[j].nRegistro < 0 ||
                    buffer_escrituras[j].fecha < 0) {
                    continue; // Saltar registros inválidos
                }

                // Si es el primer registro válido, inicializar todas las estadísticas
                if (primera_escritura) {
                    buffer_informacion[i].MayorPosicion = buffer_escrituras[j];
                    buffer_informacion[i].MenorPosicion = buffer_escrituras[j];
                    buffer_informacion[i].PrimeraEscritura = buffer_escrituras[j];
                    buffer_informacion[i].UltimaEscritura = buffer_escrituras[j];
                    buffer_informacion[i].nEscrituras = 1;
                    primera_escritura = false;
                } else {
                    // Actualizar estadísticas comparando con el registro actual
                    
                    // Actualizar mayor posición (número de registro más alto)
                    if (buffer_informacion[i].MayorPosicion.nRegistro < buffer_escrituras[j].nRegistro) {
                        buffer_informacion[i].MayorPosicion = buffer_escrituras[j];
                    }
                    
                    // Actualizar menor posición (número de registro más bajo)
                    if (buffer_informacion[i].MenorPosicion.nRegistro > buffer_escrituras[j].nRegistro) {
                        buffer_informacion[i].MenorPosicion = buffer_escrituras[j];
                    }
                    
                    // Actualizar primera escritura (número de escritura más bajo)
                    if (buffer_informacion[i].PrimeraEscritura.nEscritura > buffer_escrituras[j].nEscritura) {
                        buffer_informacion[i].PrimeraEscritura = buffer_escrituras[j];
                    }
                    
                    // Actualizar última escritura (número de escritura más alto)
                    if (buffer_informacion[i].UltimaEscritura.nEscritura < buffer_escrituras[j].nEscritura) {
                        buffer_informacion[i].UltimaEscritura = buffer_escrituras[j];
                    }
                    
                    // Incrementar contador de escrituras válidas
                    buffer_informacion[i].nEscrituras++;
                }
            }
            
            // Preparar para leer el siguiente chunk
            offset_buffer_escritura += bytes_leidos;
            memset(buffer_escrituras, 0, sizeof(buffer_escrituras)); // Limpiar buffer
            bytes_leidos = mi_read(ruta_prueba_dat, buffer_escrituras, offset_buffer_escritura, sizeof(buffer_escrituras));
        }
    }
    
    // Generar el contenido del informe formateado
    char *buffer_output = malloc(TAMANO_INFORME_INDIVIDUAL * cantidad_entradas);  // Usar heap por el tamaño
    char tmp[TAMANO_INFORME_INDIVIDUAL];  // Buffer temporal para formatear cada proceso
    buffer_output[0] = '\0';              // Inicializar string vacío para concatenación
    
    // Formatear información de cada proceso
    for (int i = 0; i < cantidad_entradas; i++) {
        snprintf(tmp, TAMANO_INFORME_INDIVIDUAL,
                 "PID: %d\n"
                 "Numero de escrituras: %d\n"
                 "Primera Escritura\t%d\t%d\t%s"      // nEscritura, nRegistro, fecha
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
        
        // Concatenar al buffer de salida y mostrar en pantalla
        strcat(buffer_output, tmp);
        printf("%s", tmp);
    }

    // Escribir el informe completo al archivo
    if(mi_write(fichero_informe, buffer_output, 0, strlen(buffer_output)) == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> mi_write() == FALLO\n" RESET);
        free(buffer_output);  // Liberar memoria antes de salir
        return FALLO;
    }

    // Liberar memoria dinámica asignada
    free(buffer_output);

    // Desmontar el dispositivo virtual antes de terminar
    if (bumount() == FALLO) {
        fprintf(stderr, RED "Error: verificacion.c -> main() -> bumount() == FALLO\n" RESET);
        return FALLO;
    }

    return EXITO;  // Finalización exitosa
}
