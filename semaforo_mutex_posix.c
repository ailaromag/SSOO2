#include "semaforo_mutex_posix.h"

/**
 * Inicializa un semáforo mutex POSIX con nombre compartido entre procesos.
 * Crea o abre un semáforo con valor inicial para sincronización.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - puntero al semáforo si éxito
 * - NULL si error en sem_open
 */
sem_t *initSem() {
   // Declarar puntero para el semáforo
   sem_t *sem;
   
   // Crear o abrir el semáforo con permisos de usuario y valor inicial
   sem = sem_open(SEM_NAME, O_CREAT, S_IRWXU, SEM_INIT_VALUE);
   
   // Verificar que la creación/apertura fue exitosa
   if (sem == SEM_FAILED) {
      return NULL;
   }
   return sem;
}

/**
 * Elimina el semáforo del sistema desvinculándolo del espacio de nombres.
 * Libera los recursos del semáforo cuando ya no es necesario.
 * 
 * Parámetros de entrada:
 * - ninguno
 * 
 * Return:
 * - void (no retorna valor)
 */
void deleteSem() {
   // Desvincular el semáforo del espacio de nombres del sistema
   sem_unlink(SEM_NAME);
}

/**
 * Realiza una operación signal (post) sobre el semáforo incrementando su valor.
 * Libera el semáforo para que otros procesos puedan acceder a la sección crítica.
 * 
 * Parámetros de entrada:
 * - sem: puntero al semáforo sobre el cual hacer signal
 * 
 * Return:
 * - void (no retorna valor)
 */
void signalSem(sem_t *sem) {
   // Incrementar el valor del semáforo (operación V o signal)
   sem_post(sem);
}

/**
 * Realiza una operación wait sobre el semáforo decrementando su valor.
 * Bloquea hasta poder entrar en la sección crítica si el semáforo vale 0.
 * 
 * Parámetros de entrada:
 * - sem: puntero al semáforo sobre el cual hacer wait
 * 
 * Return:
 * - void (no retorna valor)
 */
void waitSem(sem_t *sem) {
   // Decrementar el valor del semáforo (operación P o wait)
   sem_wait(sem);
}