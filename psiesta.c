#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

char *serverip, *serverport, *localport, *time;

void main(int argc, char *argv[]){

    switch(argc){
        case 2:
            if(!strcmp(argv[1],"-help")){
                printf("\t./siesta -help | -h <host psensor> -p <puerto psensor> -l <puerto local> -t <tiempo de ciclo>\n\n");
                printf("\t\t-help\tMuestra esta ayuda.\n\n");
                printf("\t\t-h <host psensor>\tNombre de la direccion IP de un computador que corre el programa psensor.\n\n");
                printf("\t\t-p <puerto psensor>\tNumero de puerto que emplea el programa psensor para recibir solicitudes de informacion.\n\n");
                printf("\t\t-l <puerto local>\tNumero de puerto que emplea el programa psiesta para recibir solicitudes de los usuarios del sistema a traves de un navegador web.\n\n");
                printf("\t\t-t <tiempo de ciclo>\tEspecifica cada cuanto tiempo se debe pedir la informacion de las paginas que estan localmente en psiesta, pero que corresponde a informacion que realmente se genera en psensor.\n\n");
                printf("\t\tEjemplo: ./siesta -h 127.0.0.1 -p 25503 -l 25506 -t 10\n");
                exit(0);
            }else{
                printf("Error de Uso.\n\t./siesta -help | -h <host psensor> -p <puerto psensor> -l <puerto local> -t <tiempo de ciclo>\n");
            }
            break;
        case 9:
            if(!strcmp(argv[1],"-h") && !strcmp(argv[3],"-p")
                    && !strcmp(argv[5],"-l") && !strcmp(argv[7],"-t")){
                serverip = argv[2];
                serverport = argv[4];
                localport = argv[6];
                time = argv[8];
                break;
            }
        default:
            printf("Error de Uso.\n\t./siesta -help | -h <host psensor> -p <puerto psensor> -l <puerto local> -t <tiempo de ciclo>\n");
            exit(0);
            break;
    }

    printf("IP %s\nServPort %s\nLocalPort %s\nTime %s\n", serverip,
            serverport, localport, time);

}
