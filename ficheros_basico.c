#include "ficheros_basico.h"

struct superbloque SB;

/**
 * Calcula el tamaño en bloques necesario para el mapa de bits.
 */
int tamMB(unsigned int nbloques){
    int tamMB = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques / 8) % BLOCKSIZE != 0)
    {
        tamMB++;
    }
    return tamMB;
}

/**
 * Calcula el tamaño en bloques del array de inodos.
 */
int tamAI(unsigned int ninodos){
    int tamAI = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0)
    {
        return tamAI++;
    }
    return tamAI;
}

/**
 * Inicializa los datos del superbloque.
 */
int initSB(unsigned int nbloques, unsigned int ninodos){
    // inicialización de los atributos
    SB.posPrimerBloqueMB = posSB + tamSB;   // posSB = 0, tamSB = 1
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // escribirlo en el fichero
    if (bwrite(nbloques, &SB) == -1) {
        perror(RED "Error: initSB(), bwrite() == -1");
        printf(RESET);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Inicializa el mapa de bits poniendo a 1 los bits que representan los metadatos.
 */
int initMB()
{
    // nbloques de metadatos --> n bits de MB a 1
    struct superbloque SB; // tiene que ser global?

    // Leer el superbloque del dispositivo virtual:
    if (bread(posSB, &SB))
    {
        perror("Error al leer el superbloque");
        return -1;
    }

    int numBloquesMB = SB.posUltimoBloqueMB - SB.posPrimerBloqueMB + 1; // Cantidad de bloques que ocupa el MB
    int bitsAuno = SB.posPrimerBloqueDatos;                             // Número de bits a 1 (metadatos ocupados)
    int bytesAuno = bitsAuno / 8;                                       // Número de bytes completos a 1
    int bitsRestantes = bitsAuno % 8;                                   // Bits adicionales en el siguiente byte

    unsigned int bufferMB[BLOCKSIZE];

    for (int i = SB.posPrimerBloqueMB; i < SB.posUltimoBloqueMB; i++)
    {
        bufferMB[i] = 255;
    }

    // Mirar si cabe exactamente en nbloques o necesitamos un bloque de mas
    // resto =(nbloques metadatos / 8)%BLOCKSIZE
    //  int e = 7;
    // if((resto!= 0){
    //    bufferMB[bytesAuno+1] =0;
    // for(int j = 0; j < resto;j++ )
    //    bufferMB[bytesAuno+1]+= 2 elevado a 7;
    //    e--;
    // }
    //  Los restantes bytes de ese bloque (desde el 393 al 1023) se tendrían que poner a 0.
    // Y luego salvar bufferMB al dispositivo virtual, en la posición correspondiente.
}

/**
 * Inicializa la lista de inodos libres, al principio enlaza todos los inodos secuencialmente ya que esta todo libre.
 */
int initAI() {
    struct inodo inodos[BLOCKSIZE/INODOSIZE];

    // Leer el superbloque del dispositivo virtual:
    if (bread(posSB, &SB) == -1)
    {
        perror("Error: initAI(), bread() == -1");
        return -1;
    }
    SB.posPrimerInodoLibre = 0;

    int bytesRestantes = BLOCKSIZE % INODOSIZE;

    int contInodos = SB.posPrimerInodoLibre + 1; // si hemos inicializado SB.posPrimerInodoLibre = 0
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
    { // para cada bloque del AI
        // leer el bloque de inodos i  en el dispositivo virtual
        // bread
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++)
        {                         // para cada inodo del bloque
            inodos[j].tipo = 'l'; // libre
            if (contInodos < SB.totInodos)
            {                                               // si no hemos llegado al último inodo del AI
                inodos[j].punterosDirectos[0] = contInodos; // enlazamos con el siguiente
                contInodos++;
            }
            else
            { // hemos llegado al último inodo
                inodos[j].punterosDirectos[0] = UINT_MAX;
                // hay que salir del bucle, el último bloque no tiene por qué estar completo !!!
            }
            // escribir el bloque de inodos i  en el dispositivo virtual
            if (bwrite(i, &SB) == -1)
            {
                perror("Error al escribir el superbloque");
            }
        }

        return 0;
    }
}