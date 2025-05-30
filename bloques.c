#include "bloques.h"

#include "semaforo_mutex_posix.h"

// Semáforo mutex para sincronización entre procesos
static sem_t *mutex;
// Contador de procesos dentro de la sección crítica (para semáforos anidados)
static unsigned int inside_sc = 0;

// Descriptor de archivo para el dispositivo virtual montado
static int descriptor = 0;

/**
 * Monta el dispositivo virtual abriendo el archivo especificado y configurando el semáforo mutex.
 * Si ya existe un descriptor abierto, lo cierra antes de abrir el nuevo.
 * 
 * Parámetros de entrada:
 * - camino: ruta del archivo que actuará como dispositivo virtual
 * 
 * Return:
 * - descriptor del archivo si éxito
 * - FALLO (-1) si error en la apertura del archivo
 * - -1 si error en la inicialización del semáforo
 */
int bmount(const char *camino) {
    // Si ya hay un descriptor abierto, cerrarlo primero para evitar memory leaks
    if (descriptor > 0) {
        close(descriptor);
    }

    // El semáforo es único en el sistema y sólo se ha de inicializar 1 vez (padre)
    // Verificar si ya existe un semáforo inicializado antes de crear uno nuevo
    if (!mutex) {  
        mutex = initSem(); // Inicializar semáforo POSIX para sincronización entre procesos
        if (mutex == SEM_FAILED) {
            return -1; // Retornar error si falla la inicialización del semáforo
        }
    }

    // Abrir el archivo del dispositivo virtual con permisos de lectura/escritura
    // O_CREAT lo crea si no existe, 0666 son los permisos por defecto (rw-rw-rw-)
    if ((descriptor = open(camino, O_RDWR | O_CREAT, 0666)) == FALLO)  // Abrimos el fichero y controlamos que no se produzcan errores
    {
        perror(RED "Error: bloques.c -> bmount() -> open() == FALLO");
        printf(RESET);
        return FALLO;
    }
    // Asegurar que el archivo tiene los permisos correctos (lectura/escritura para todos)
    // Esto es necesario por si el archivo ya existía con permisos diferentes
    chmod(camino, 0666);

    return descriptor; // Retornar el descriptor válido del archivo abierto
}

/**
 * Desmonta el dispositivo virtual cerrando el descriptor de archivo y eliminando el semáforo.
 * Libera todos los recursos utilizados por el sistema de bloques.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - EXITO (0) si se desmonta correctamente
 * - FALLO (-1) si error al cerrar el descriptor
 */
int bumount() {
    // Eliminar el semáforo del sistema para liberar recursos de sincronización
    deleteSem();
    
    // Cerrar el descriptor de archivo y verificar que no hay errores
    descriptor = close(descriptor);
    if (descriptor == FALLO) {
        perror(RED "Error: bloques.c -> bumount -> close() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return EXITO; // Desmontaje completado exitosamente
}

/**
 * Escribe un bloque completo de datos en el dispositivo virtual en la posición especificada.
 * Posiciona el puntero del archivo y escribe exactamente BLOCKSIZE bytes.
 * 
 * Parámetros de entrada:
 * - nbloque: número del bloque donde escribir (offset = nbloque * BLOCKSIZE)
 * - buf: puntero al buffer que contiene los datos a escribir
 * 
 * Return:
 * - BLOCKSIZE (número de bytes escritos) si éxito
 * - FALLO (-1) si error en lseek() o write()
 */
int bwrite(unsigned int nbloque, const void *buf) {
    int bytesEscritos;
    
    // Posicionarse en el bloque específico: nbloque * BLOCKSIZE bytes desde el inicio
    // Esto nos lleva al offset exacto donde comienza el bloque deseado
    if ((lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET)) == FALLO) {
        perror(RED "Error: bloques.c -> bwrite() -> lseek() == FALLO");
        printf(RESET);
        return FALLO;
    }

    // Escribir exactamente BLOCKSIZE bytes desde el buffer al archivo
    // Esta operación es atómica a nivel de bloque para mantener consistencia
    if ((bytesEscritos = write(descriptor, buf, BLOCKSIZE)) == FALLO) {
        perror(RED "Error: bloques.c -> bwrite() -> write() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return bytesEscritos; // Retornar el número de bytes escritos (debería ser BLOCKSIZE)
}

/**
 * Lee un bloque completo de datos del dispositivo virtual desde la posición especificada.
 * Posiciona el puntero del archivo y lee exactamente BLOCKSIZE bytes.
 * 
 * Parámetros de entrada:
 * - nbloque: número del bloque a leer (offset = nbloque * BLOCKSIZE)
 * - buf: puntero al buffer donde almacenar los datos leídos
 * 
 * Return:
 * - BLOCKSIZE (número de bytes leídos) si éxito
 * - FALLO (-1) si error en lseek() o read()
 */
int bread(unsigned int nbloque, void *buf) {
    int bytesLeidos;
    
    // Posicionarse en el bloque específico que queremos leer
    // El offset se calcula multiplicando el número de bloque por el tamaño del bloque
    if ((lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET)) == FALLO) {
        perror(RED "Error: bloques.c -> bread() -> lseek() == FALLO");
        printf(RESET);
        return FALLO;
    }

    // Leer exactamente BLOCKSIZE bytes del archivo al buffer
    // Esta operación garantiza que se lee un bloque completo
    if ((bytesLeidos = read(descriptor, buf, BLOCKSIZE)) == FALLO) {
        perror(RED "Error: bloques.c -> bread -> read() == FALLO");
        printf(RESET);
        return FALLO;
    }
    return bytesLeidos; // Retornar el número de bytes leídos (debería ser BLOCKSIZE)
}

/**
 * Entra en sección crítica utilizando el semáforo mutex. Soporta llamadas anidadas
 * mediante un contador interno que solo hace wait en la primera llamada.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - void (no retorna valor)
 */
void mi_waitSem() {
   // Si no estamos ya dentro de la sección crítica, hacer wait
   // Esto evita deadlocks en llamadas anidadas del mismo proceso
   if (!inside_sc) { // inside_sc==0, no se ha hecho ya un wait
       waitSem(mutex); // Bloquear hasta obtener acceso exclusivo
   }
   // Incrementar contador de llamadas anidadas para rastrear la profundidad
   inside_sc++;
}

/**
 * Sale de sección crítica utilizando el semáforo mutex. Maneja llamadas anidadas
 * y solo hace signal cuando todas las llamadas anidadas han terminado.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - void (no retorna valor)
 */
void mi_signalSem() {
   // Decrementar contador de llamadas anidadas para balancear con mi_waitSem
   inside_sc--;
   
   // Solo hacer signal cuando ya no hay más llamadas anidadas
   // Esto garantiza que el semáforo se libere completamente
   if (!inside_sc) {
       signalSem(mutex); // Liberar el semáforo para otros procesos
   }
}
