#define BLOQUES_H

// Inclusión de librerías del sistema para funcionalidades básicas
#include <errno.h>     // errno
#include <fcntl.h>     // O_WRONLY, O_CREAT, O_TRUNC
#include <stdio.h>     // printf(), fprintf(), stderr, stdout, stdin
#include <stdlib.h>    // exit(), EXIT_SUCCESS, EXIT_FAILURE, atoi()
#include <string.h>    // strerror()
#include <sys/stat.h>  // S_IRUSR, S_IWUSR
#include <unistd.h>    // SEEK_SET, read(), write(), open(), close(), lseek()
#include <ctype.h>

// Definición de constantes del sistema de bloques
#define BLOCKSIZE 1024  // bytes - Tamaño estándar de bloque para operaciones de E/S
#define EXITO 0         // para gestión errores - Código de retorno exitoso
#define FALLO -1        // para gestión errores - Código de retorno de error

// Definiciones de colores ANSI para output formateado en terminal
#define BLACK "\x1B[30m"
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define WHITE "\x1B[37m"
#define ORANGE "\x1B[38;2;255;128;0m"
#define ROSE "\x1B[38;2;255;151;203m"
#define LBLUE "\x1B[38;2;53;149;240m"
#define LGREEN "\x1B[38;2;17;245;120m"
#define GRAY "\x1B[38;2;176;174;174m"
#define RESET "\x1b[0m"    // Reset color a valores por defecto
#define NEGRITA "\x1b[1m"  // Activar texto en negrita

// Declaraciones de funciones para manejo del sistema de bloques
int bmount(const char *camino);                    // Montar dispositivo virtual
int bumount();                                     // Desmontar dispositivo virtual
int bwrite(unsigned int nbloque, const void *buf); // Escribir bloque al dispositivo
int bread(unsigned int nbloque, void *buf);        // Leer bloque del dispositivo
void mi_waitSem();                                 // Entrar en sección crítica
void mi_signalSem();                               // Salir de sección crítica
