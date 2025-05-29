#include "simulacion.h"

#define DEBUG_ESCRITURA false
#define DEBUG_PROCESO true

static int acabados = 0;

int main(int argc, char **argv) {
    // Asociar el la señal SIGCHLD al enterrador
    signal(SIGCHLD, reaper);

    // Comprobación de sintaxis
    if (argc != 2) {
        fprintf(stderr, RED "Sintaxis: ./simulacion <disco>\n" RESET);
        return FALLO;
    }

    // Obtenemos el nombre del dispositivo
    char *dispositivo = argv[1];

    // Montamos e dispositivo
    if (bmount(dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> bmount() == FALLO\n" RESET);
        return FALLO;
    }

    // Formateomos el nombre del directorio según la fecha actual
    char directorio_simulacion[TAMANO_DIRECTORIO_SIMULACION];
    time_t current_time = time(NULL);
    struct tm tm = *localtime(&current_time);
    if (strftime(directorio_simulacion, TAMANO_DIRECTORIO_SIMULACION, "/simul_%Y%m%d%H%M%S/", &tm) == 0) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> strftime() == 0\n" RESET);
        return FALLO;
    }

    // Creamos el directorio de simulación
    if (mi_creat(directorio_simulacion, 6) == FALLO) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_creat() == FALLO\n" RESET);
        return FALLO;
    }

    // Empezamos la simulación
    fprintf(stdout, RESET "*** SIMULACIÓN DE 3 PROCESOS REALIZANDO CADA UNO 10 ESCRITUAS ***\n");

    // Generamos los procesos
    for (int i = 0; i < NUM_PROCESOS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            fprintf(stderr, RED "Error: simualcion.c -> main() -> fork() == -1\n" RESET);
            return FALLO;
        }
        if (pid == 0) {  // Es el proceso hijo

            // Montamos un nuevo dispositivo para el proceos hijo
            if (bmount(dispositivo) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> if (pid == 0) -> bmount() == FALLO\n" RESET);
                return FALLO;
            }

            // Formateamos el directorio del proceso con su pid
            char directorio_proceso[TAMANO_DIRECTORIO_PROCESO];
            pid_t pid_hijo = getpid();
            if (snprintf(directorio_proceso, TAMANO_DIRECTORIO_PROCESO, "%sproceso_%d/", directorio_simulacion, pid_hijo) < 0) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> snprintf() < 0\n" RESET);
                return FALLO;
            }

            // Creamos el directorio del proceso
            if (mi_creat(directorio_proceso, 6) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_creat(&directorio_proceso, 6) == FALLO\n" RESET);
                return FALLO;
            }

            // Formateamos el fichero del proceso
            char fichero_proceso[TAMANO_FICHERO_PROCESO];
            strcpy(fichero_proceso, directorio_proceso);
            strcat(fichero_proceso, NOMBRE_FICHERO_PROCESO);

            // Creamos el fichero del proceso
            if (mi_creat(fichero_proceso, 6) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_creat(&fichero_proceso, 6) == FALLO\n" RESET);
                return FALLO;
            }

            // Reseteamos la semilla del random
            srand(time(NULL) + pid_hijo);

            // Simulamos la escritura de cada proceso
            struct REGISTRO registro;
            for (int j = 0; j < NUM_ESCRITURAS; j++) {
                // Inicializamos el registro
                registro.fecha = time(NULL);
                registro.pid = pid_hijo;
                registro.nEscritura = j;
                registro.nRegistro = rand() % REGMAX;  // [0, 499999]

                // Escribimos el registro en posición n, según nRegistro
                if (mi_write(fichero_proceso, &registro, registro.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO)) == FALLO) {
                    fprintf(stderr, RED "Error: simualcion.c -> main() -> mi_write() == FALLO\n" RESET);
                    return FALLO;
                }

#if DEBUG_ESCRITURA
                printf(GRAY "[simulacion.c -> Escritura %d en %s]\n" RESET, j, fichero_proceso);
#endif

                // Esperar 0.5s = 50 000 microsegundos
                usleep(50000);
            }

#if DEBUG_PROCESO
            printf(NEGRITA "[Proceso %d: Completadas %d escrituras en %s]\n" RESET, i, NUM_ESCRITURAS, fichero_proceso);
#endif

            // Acabamos, desmontamos el dispositivo
            if (bumount(dispositivo) == FALLO) {
                fprintf(stderr, RED "Error: simualcion.c -> main() -> bumount(dispositivo) == FALLO\n" RESET);
                return FALLO;
            }

            // Terminamos el proceso emitiendo la señal SIGCHLD
            exit(0);
        }

        // Esperar 0.15s = 150 000 microsegundos para lanzar el siguiente proceso
        usleep(150000);
    }

    // Esperamos que acabe todos los hijos
    while (acabados < NUM_PROCESOS) {
        pause();
    }

    // Desmontamos el dispositivo
    if (bmount(dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: simualcion.c -> main() -> bmount(dispositivo) == FALLO\n" RESET);
        return FALLO;
    }

    return EXITO;
}

void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}
