#include <signal.h>
#include <sys/wait.h>

#include "directorios.h"

#define REGMAX 500000                          // 500 000
#define TAMANO_DIRECTORIO_SIMULACION (22 + 1)  // "/simul_aaaammddhhmmss/" -> 22 char + 1 '\0' = 23 char

/**
 * "simul_aaaammddhhmmss/proceso_<pid>/" -> TAMANO_DIRECTORIO_SIMULACION + 8 + 7 + 1
 *   - "proceso_" -> 8 char
 *   - "<pid>" -> 7 char, lo sabemos ejecutando "cat /proc/sys/kernel/pid_max" que da "4194304"
 *   - "/" -> 1 char
 *   - "\0" -> ya está incluido en TAMANO_DIRECTORIO_SIMULACION
 */
#define TAMANO_DIRECTORIO_PROCESO (TAMANO_DIRECTORIO_SIMULACION + 8 + 7 + 1)
#define TAMANO_FICHERO_PROCESO (TAMANO_DIRECTORIO_PROCESO + 10)  // "prueba.dat" -> 10 char
#define NOMBRE_FICHERO_PROCESO "prueba.dat"
#define NUM_PROCESOS 100
#define NUM_ESCRITURAS 50

struct REGISTRO {    // sizeof(struct REGISTRO): 24 bytes
    time_t fecha;    // Precisión segundos [opcionalmente microsegundos con struct timeval]
    pid_t pid;       // PID del proceso que lo ha creado
    int nEscritura;  // Entero con el nº de escritura, de 1 a 50 (orden por tiempo)
    int nRegistro;   // Entero con el nº del registro dentro del fichero: [0..REGMAX-1] (orden por posición)
};

void reaper();