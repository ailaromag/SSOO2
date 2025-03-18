#include "ficheros.h"

#define DEBUGN5 true

int main(int argc, char **argv) {
    #if DEBUGN5
    int nInodoReservado;
    if(argc != 4) {
        perror(RED "Sintaxis: escribir <nombre_dispositivo> <\"$(cat fichero)\">  <diferentes_inodos>\n Offsets: 9000, 209000, 30725000, 409605000, 480000000 \n Si diferentes_inodos=0 se reserva solo un inodo para todos los offsets\n");
        printf(RESET);
        return FALLO;
    }
    char *camino = argv[1];
    char *texto = argv[2];
    int longitud = strlen(texto);
    printf("Longitud = %d \n", longitud);
    int diferentes_inodos = argv[3];
    //Offsets para probar los punteros:
    int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};
    int num_offsets = sizeof(offsets)/sizeof(offsets[0]);

    if(diferentes_inodos == 0){
        nInodoReservado = reservar_inodo('f',6);
        if (nInodoReservado == FALLO) {
            perror(RED "Error: escribir.c -> main() -> reservar_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }
    }
     for(int i = 0; i < num_offsets; i++){
    if(diferentes_inodos == 1){
        nInodoReservado = reservar_inodo('f', 6);
        if (nInodoReservado == FALLO) {
            perror(RED "Error: escribir.c -> main() -> reservar_inodo() == FALLO");
            printf(RESET);
            return FALLO;
        }
    }  
    int bytes_escritos = mi_write_f(nInodoReservado, texto, offsets[i],longitud);
    if (bytes_escritos == FALLO) {
        perror(RED "Error: escribir.c -> main() -> mi_write_f() == FALLO");
        printf(RESET);
        return FALLO;
    }     
    } 
    #endif
}