/**
 * Autores:
 *   - Xiaozhe Cheng
 *   - Aila Romanguera Mezquida
 *   - Alba Auilera Cabellos
 */

#include "simulacion.h"

// Flags de depuración para controlar la salida de debug
#define DEBUG_ESCRITURA false  // No mostrar cada escritura individual
#define DEBUG_PROCESO true     // Mostrar cuando cada proceso termine

// Variable global para contar procesos hijo que han terminado
static int acabados = 0;

/**
 * Programa principal que simula múltiples procesos realizando escrituras concurrentes.
 * Crea un directorio de simulación con timestamp, lanza múltiples procesos hijo que escriben
 * registros en archivos separados para probar concurrencia y sincronización del sistema.
 * 
 * Parámetros de entrada:
 * - argc: número de argumentos de línea de comandos (debe ser 2)
 * - argv: array de argumentos donde argv[1] es el dispositivo del sistema de archivos
 * 
 * Return:
 * - EXITO (0) si completa la simulación correctamente
 * - FALLO (-1) si error en argumentos, montaje, creación de estructuras, fork o desmontaje
 */
int main(int argc, char **argv) {
    // Asociar la señal SIGCHLD al manejador reaper() para capturar terminación de hijos
    signal(SIGCHLD, reaper);

    // Comprobación de sintaxis - debe recibir exactamente un argumento (dispositivo)
    if (argc != 2) {
        fprintf(stderr, RED "Sintaxis: ./simulacion <disco>\n" RESET);
        return FALLO;
    }

    // Obtenemos el nombre del dispositivo desde los argumentos de línea de comandos
    char *dispositivo = argv[1];

    // Montamos el dispositivo virtual para poder trabajar con el sistema de archivos
    if (bmount(dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }

    // Formatear el nombre del directorio según la fecha/hora actual
    // Formato: /simul_YYYYMMDDHHMMSS/
    char directorio_simulacion[TAMANO_DIRECTORIO_SIMULACION];
    time_t current_time = time(NULL);
    struct tm tm = *localtime(&current_time);
    if (strftime(directorio_simulacion, TAMANO_DIRECTORIO_SIMULACION, "/simul_%Y%m%d%H%M%S/", &tm) == 0) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> strftime() == 0\n" RESET);
        return FALLO;
    }

    // Crear el directorio raíz de la simulación con permisos de lectura/escritura
    if (mi_creat(directorio_simulacion, 6) == FALLO) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_creat() == FALLO\n" RESET);
        return FALLO;
    }

    // Mostrar información de inicio de la simulación
    fprintf(stdout, "*** SIMULACIÓN DE %d PROCESOS REALIZANDO CADA UNO %d ESCRITURAS ***\n", NUM_PROCESOS, NUM_ESCRITURAS);
    fprintf(stdout, "Directorio simulación: %s\n", directorio_simulacion);

    // Crear y lanzar NUM_PROCESOS procesos hijo
    for (int i = 0; i < NUM_PROCESOS; i++) {
        pid_t pid = fork(); // Crear proceso hijo
        if (pid == -1) {
            fprintf(stderr, RED "Error: simualcion.c -> main() -> fork() == -1\n" RESET);
            return FALLO;
        }
        
        if (pid == 0) {  // Código ejecutado por el proceso hijo

            // Cada proceso hijo debe montar su propia instancia del dispositivo
            if (bmount(dispositivo) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> if (pid == 0) -> bmount() == FALLO\n" RESET);
                return FALLO;
            }

            // Formatear el nombre del directorio específico para este proceso
            // Formato: /simul_YYYYMMDDHHMMSS/proceso_PID/
            char directorio_proceso[TAMANO_DIRECTORIO_PROCESO];
            pid_t pid_hijo = getpid(); // Obtener PID del proceso actual
            if (snprintf(directorio_proceso, TAMANO_DIRECTORIO_PROCESO, "%sproceso_%d/", directorio_simulacion, pid_hijo) < 0) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> snprintf() < 0\n" RESET);
                return FALLO;
            }

            // Crear el directorio específico del proceso
            if (mi_creat(directorio_proceso, 6) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_creat(&directorio_proceso, 6) == FALLO\n" RESET);
                return FALLO;
            }

            // Construir la ruta completa del archivo donde escribirá este proceso
            char fichero_proceso[TAMANO_FICHERO_PROCESO];
            strcpy(fichero_proceso, directorio_proceso);
            strcat(fichero_proceso, NOMBRE_FICHERO_PROCESO); // Añadir "prueba.dat"

            // Crear el archivo donde se escribirán los registros
            if (mi_creat(fichero_proceso, 6) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_creat(&fichero_proceso, 6) == FALLO\n" RESET);
                return FALLO;
            }

            // Inicializar semilla del generador de números aleatorios
            // Usar tiempo actual + PID para garantizar diferentes secuencias por proceso
            srand(time(NULL) + pid_hijo);

            // Simular NUM_ESCRITURAS escrituras por parte de este proceso
            struct REGISTRO registro;
            for (int j = 0; j < NUM_ESCRITURAS; j++) {
                // Llenar los campos del registro con información del proceso y escritura
                registro.fecha = time(NULL);           // Timestamp actual
                registro.pid = pid_hijo;               // PID del proceso que escribe
                registro.nEscritura = j + 1;           // Número de escritura secuencial (1-based)
                registro.nRegistro = rand() % REGMAX;  // Posición aleatoria en el archivo [0, 499999]

                // Escribir el registro en una posición específica del archivo
                // La posición se calcula como: nRegistro * tamaño_del_registro
                if (mi_write(fichero_proceso, &registro, registro.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO)) == FALLO) {
                    fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_write() == FALLO\n" RESET);
                    return FALLO;
                }

#if DEBUG_ESCRITURA
                // Debug: mostrar cada escritura individual (solo si está habilitado)
                printf(GRAY "[simulacion.c -> Escritura %d en %s]\n" RESET, j, fichero_proceso);
#endif

                // Pausa de 50ms entre escrituras para simular trabajo real
                usleep(50000);
            }

#if DEBUG_PROCESO
            // Debug: mostrar cuando un proceso complete todas sus escrituras
            printf("[Proceso %d: Completadas %d escrituras en %s]\n", i + 1, NUM_ESCRITURAS, fichero_proceso);
#endif

            // El proceso hijo desmonta su instancia del dispositivo antes de terminar
            if (bumount(dispositivo) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> bumount(dispositivo) == FALLO\n" RESET);
                return FALLO;
            }

            // Terminar el proceso hijo exitosamente
            // Esto enviará SIGCHLD al proceso padre
            exit(0);
        }

        // Código ejecutado por el proceso padre:
        // Esperar 150ms antes de lanzar el siguiente proceso hijo
        // Esto evita sobrecargar el sistema creando todos los procesos simultáneamente
        usleep(150000);
    }

    // El proceso padre espera que terminen todos los procesos hijo
    // pause() suspende la ejecución hasta recibir una señal (SIGCHLD)
    while (acabados < NUM_PROCESOS) {
        pause(); // Dormir hasta que el manejador reaper() incremente 'acabados'
    }

    // Todos los procesos hijo han terminado, desmontar el dispositivo
    if (bumount(dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> bmount(dispositivo) == FALLO\n" RESET);
        return FALLO;
    }

    return EXITO; // Simulación completada exitosamente
}

/**
 * Manejador de señales para capturar la terminación de procesos hijo.
 * Recoge procesos zombie y actualiza el contador de procesos terminados.
 * 
 * Parámetros de entrada:
 * - ninguno (manejador de señal SIGCHLD)
 * 
 * Return:
 * - void (no retorna valor)
 */
void reaper() {
    pid_t ended;
    // Reinstalar el manejador para capturar futuras señales SIGCHLD
    signal(SIGCHLD, reaper);
    
    // Recoger todos los procesos zombie disponibles sin bloquear
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++; // Incrementar contador de procesos terminados
    }
}
