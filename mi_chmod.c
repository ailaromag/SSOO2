#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n" RESET);
        return FALLO;
    }
    
}