#include <stdio.h>   
#include <string.h>   
#include "bloques.h" //??
#include "directorios.h"


//Dada una cadena de caracteres *camino (que comience por '/'), separa su contenido en dos: *inicial y *final
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    if (camino == NULL || camino[0] != '/') {
        perror(RED "Error"); 
        printf(RESET);
        return -1;
    }
    // Obtenemos inicial
    strcpy(inicial, camino + 1);

    // Localiza el primer '/'
    char *pos_final = strchr(inicial, '/');
    if (pos_final != NULL) {    
        strcpy(final,pos_final);   
        int tamaño = pos_final - inicial;
        strncpy(inicial, inicial, tamaño);
        inicial[tamaño] = '\0';  
        *tipo = 'd' ;
    }else{
        *tipo = 'f';
    } 
    printf("*camino: %s \n *inicial: %s \n *final: %s \n *tipo: %c \n",camino, inicial, final,*tipo);

    return 0;
}

int main() { //main de pruebas
    char inicial[100], final[100], tipo;
    // const char *camino = "/home/usuario/documentos/archivo.txt";

    // int result = extraer_camino(camino, inicial, final, &tipo);
    const char *camino2 = "/";

    int result2 = extraer_camino(camino2, inicial, final, &tipo);

    // if (result == 0) {
    //     printf("Inicial: %s\n", inicial);
    //     printf("Final: %s\n", final);
    //     printf("Tipo: %c\n", tipo);
    // } else {
    //     printf("Error al procesar el camino.\n");
    // }

    return 0;
}


int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos){

char inicial[TAMNOMBRE], final[TAMNOMBRE], tipo;

// Primero hay que aplicar extraer_camino a camino_parcial
if(extraer_camino(camino_parcial,inicial,final,tipo) != FALLO){

}
//Obtenemos *inicial

//Buscar *inicial entre las entradas del inodo correspondiente al directorio padre


}