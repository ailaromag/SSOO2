#include "ficheros.h"

#define DEBUGN5 true

int main(int argc, char **argv) {
    #if DEBUGN5
    int nInodoReservado = reservar_inodo('f', 6);
    if (nInodoReservado == FALLO) {
        perror(RED "Error: permitir.c -> main() -> reservar_inodo() == FALLO");
        printf(RESET);
        return FALLO;
    }
    
    #endif
}