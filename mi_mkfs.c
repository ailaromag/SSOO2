
#include "bloques.h"
#include <string.h>

// argc- indica el número de elementos escritos por consola
// argv- vector de punteros a los parámetros
int main(int argc, char **argv){
    int descriptor;    
    if(argc!=3){
        perror(RED "Error: main(), argc != 3");
        printf(RESET);
        return FALLO;
    }
    char *camino = argv[1];
    int nbloques = atoi(argv[2]);

    //Montar dispositivo
    unsigned char buf[BLOCKSIZE];
    
    if((descriptor = bmount(camino)) !=-1){
        memset(buf, 0,BLOCKSIZE);
        for(int i = 0; i<nbloques;i++){
            bwrite(i,buf);
         }
    }
    bumount();
}