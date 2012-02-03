#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>

/* Macro variables */
#define NCLIENTS 5

/* Global variable */
int listensock, b_psiesta, s_psensor;
int sent, got, tmp, i, status,pid,flag, flag2;
char recvbuf[1024];
char sendbuf[1024];
char query[1024];
char *serveraddr, *serverport, *localport, *time;
char serverip[20];
char archv[15];
struct in_addr inaddr;
struct hostent* host;
struct sockaddr_storage data ;
struct sockaddr_in server, address;
FILE *file, *point, *cache;


/******************************************/
/* Helper functions */

void make_query(char *host, char *page){

    char * message  = "\nGET /%s HTTP/1.0\r\nHost: %s\r\nConnection: keep-alive\r\n\r\n";
    sprintf(query,message, page, host);
    
}

void parser(int argc, char *argv[]){
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
                serveraddr = argv[2];
                if(inet_aton(serveraddr, &inaddr))
                    host = gethostbyaddr((char *)&inaddr, sizeof(inaddr), AF_INET);
                else
                    host = gethostbyname(serveraddr);

                if(!host){
                    herror("Error looking up host\n");
                    exit(1);
                }

                sprintf(serverip,"%s",inet_ntoa(*(struct in_addr *) host->h_addr));
                printf("Psensor Host: %s\n", host->h_name);
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

}

/******************************************/

void main(int argc, char *argv[]){

    parser(argc,argv);

    printf("IP %s\nServPort %s\nLocalPort %s\nTime %s\n", serverip,
            serverport, localport, time);

    memset(sendbuf,0,sizeof(sendbuf));
    memset(archv,0,sizeof(archv));
    memset(recvbuf,0,sizeof(recvbuf));
    memset(&server,0,sizeof(server));
    memset(&address,0,sizeof(address));

    /* This child updates the files  */
    pid = fork();
    if(pid<0){
        perror("Error forking: \n");
        close(listensock);
        exit(1);

    }else if(pid==0){
        close(listensock);
        char nextfile[15];
        if((point = fopen("Files", "w+")) == NULL)
            perror("Error opening the cache file: \n");

        //Fill the structure with host data
        address.sin_family = AF_INET; 
        address.sin_port = htons(atoi(serverport));
        inet_pton(AF_INET,serverip,&(address.sin_addr));

        while(1){
            rewind(point);
            sleep(atoi(time));

            while(fscanf(point,"%[^\n]\n",nextfile)!=EOF){

                //Create socket for this child
                if((s_psensor=socket(AF_INET, SOCK_STREAM, 0))<0)
                    perror("Error creating file manager child: \n");

                //Send query again 
                if(connect(s_psensor, (struct sockaddr *)&address, sizeof(struct sockaddr))<0){
                    //The server psensor is down so this child continues with
                    //next file
                    printf("No hubo conexion\n");
                    continue;
                }

                //Build the query 
                char ip[20];
                sprintf(ip,"%s%s%s",serverip,":",serverport);
                //printf("ip %s\n\n",ip);
                make_query(ip, nextfile);
                printf("\nQUERY: \n<BEGIN>%s<END>\n",query);

                //Send the query to server. If there's something that has not been
                //sent, the rest is sent.
                sent =0;
                while(sent < strlen(query)){
                    tmp = send(s_psensor, &(query[sent]), strlen(query) - sent , 0);
                    if(tmp<0)
                        perror("Error sending query");

                    sent += tmp;
                }

                printf("\nnextfile %sc\n",nextfile);

                memset(recvbuf, 0, 1024);
                if ((file = fopen(nextfile,"w")) == NULL)
                    perror("Error opening file: \n");

                // Receiving the data 
                while((tmp = recv(s_psensor, recvbuf, 1023, 0)) > 0 ){

                    printf("tmp %d\n",tmp);
                    fwrite(recvbuf, sizeof(char) ,tmp, stdout);

                    fwrite(recvbuf, sizeof(char) ,tmp, file);
                    memset(recvbuf, 0, tmp);
                }

                if(tmp < 0){
                    perror("Error receiving data: \n");
                }

                memset(query,0,sizeof(query));
                fclose(file);
                close(s_psensor);
            }
        }

        printf("Exit hijo file manager\n");
        exit(0);
    }

    /* Code Server */

    /* Creating socket between server and client */
    if ((listensock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error creating server socket: \n");
        exit(1);
    }

    /* Populate the struct */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(localport)); 

    //Binding the socket
    if(bind(listensock, (struct sockaddr * ) &server, sizeof(server)) < 0){
        perror("Binding socket not created");
        exit(1);
    }

    //Put the socket to listen
    if (listen(listensock, NCLIENTS) < 0){
        perror("Listen call failed");
        exit(1);
    }

    socklen_t size_data; 
    size_data = sizeof(size_data);

    //Loop for receiving request from clients
    while(1){

        b_psiesta = accept(listensock,(struct sockaddr *)&data, &size_data);
        if (b_psiesta < 0){
            perror("Error accepting connection from browser");
            exit(1);
        }

        //Childs to attend browsers requests
        pid = fork();
        if(pid<0){
            perror("Error forking");
            close(listensock);
            close(b_psiesta);
            exit(1);
        }
        if(pid==0){
            close(listensock);
            // Receiving the data from browser 
            tmp = recv(b_psiesta, &recvbuf[got], 1023, 0);
            printf("tmp recv browser %d\n",tmp);
            fwrite(&recvbuf[got], sizeof(char) ,tmp, stdout);

            if(tmp<0)
                perror("Error receiving data");
    
            memset(archv,0,15);

            //Parsing the file to retrieve from psensor
            for(i=5;recvbuf[i]!=' ';i++){
                archv[i-5]=recvbuf[i];
            }

            archv[i-5]='\0';

            i=1; //File archv exists
            
            if((file = fopen(archv,"r")) == NULL)
                i = 0; //If archv doesnt exists
            else
                fclose(file);
            

            printf("i %d\n\n", i);

            //Fill the structure with host data
            address.sin_family = AF_INET; 
            address.sin_port = htons(atoi(serverport));
            inet_pton(AF_INET,serverip,&(address.sin_addr));

            //Create socket for this child
            if((s_psensor=socket(AF_INET, SOCK_STREAM, 0))<0)
                perror("Error creating file manager child");

            //Connecting psensor 
            if(connect(s_psensor, (struct sockaddr *)&address, sizeof(struct sockaddr))<0){
                //If psensor is not active search cache memory
                //If we got the archive send it to browser
                //If we dont, send a message
                printf("No connection to server: \n");
                exit(1);
            }

            //Make query for psensor
            char ip[20];
            sprintf(ip,"%s%s%s",serverip,":",serverport);
            make_query(ip, archv);
            printf("\nQUERY: \n<BEGIN>%s<END>\n",query);

            //Send query to psensor
            tmp = send(s_psensor, query, strlen(query), 0);
            printf("tmp send psensor %d\n\n",tmp);
            if(tmp<0)
                perror("Error sending query");

            memset(recvbuf, 0, 1024);
            
            printf("archv %s c\n",archv);

            //Receiving the data and overwriting the file
            flag=1;
            flag2=1;
            while((tmp = recv(s_psensor, recvbuf, 1023, 0))>0){

                printf("tmp recv psensor %d\n\n",tmp);
                fwrite(recvbuf, sizeof(char) ,tmp, stdout);

                printf("flag %d - code %d\n", flag , atoi(&recvbuf[9]));

                if(flag && 404!=atoi(&recvbuf[9])){
                    //If first loop and no problems recving
                    //open file archv
                    if((file = fopen(archv,"w")) == NULL)
                        perror("Error opening file");

                    printf("i adentro %d\n", i);
                    if(!i){
                        //i=0 if file doesnt exists
                        //If file doesnt exists, write in the cache directorie
                        printf("i adentro ***  %d\n", i);
                        if((cache = fopen("Files","a"))==NULL)
                            perror("Error opening file");

                        if(fwrite(archv, sizeof(char), strlen(archv), cache)!=strlen(archv))
                            printf("Error writing in file");
                        if(fputc('\n',cache)<0)
                            printf("Error writing char in file");

                        fclose(cache);
                    }

                    fwrite(recvbuf, sizeof(char), tmp, file);
                    memset(recvbuf, 0, tmp);
                    flag2=0; //flag2=0 no problems in first chunk of data
                }

                if(!flag && !flag2){
                    //Store next chuncks knowing its not the first loop
                    //and there was no problem with the first
                    printf("PRUEBA \n");
                    fwrite(recvbuf, sizeof(char), tmp, file);
                    memset(recvbuf, 0, tmp);
                }

                flag=0;
                
            }

            if(tmp < 0){
                perror("Error receiving data from psensor");
            }


            printf("HOLAAAAAAA\n");
            if(!flag2)
                fclose(file);


            printf("aqui\n");
            //perror("");

            if(flag2==1){
                //Psensor is up but file its not found 
                //Send to browser
    
                char response[60] = "Psensor was active but requested file was not found\n";
                printf("hola %s\n",response);
                tmp = send(b_psiesta, response, strlen(response), 0);

                if(tmp<0)
                    perror("Error sending response to browser");

                printf("chao tmp %d\n",tmp);

            }else if(flag2==0){
                //Psensor is up and file was found
                if((file = fopen(archv,"r")) == NULL)
                    perror("Error opening file");
                
                char line[1024];
                printf("Entre en flag2\n");

                //Send from file to browser
                //while(fscanf(file,"%[^\n]\n",line)!=EOF){ 
                    fread(line,sizeof(char), sizeof(line), file);
                    tmp = send(b_psiesta,line,strlen(line),0);
                    printf("linewidth %d, tmp send to browser %d\n",(int)strlen(line),tmp);
                    if(tmp<0)
                        perror("Error sending response to browser");
                //}

                fclose(file);
            }

            close(b_psiesta);
            close(listensock);
            exit(0);
        }
        close(b_psiesta);
    }
    
    //signal(pid,SIGTERM); //Kill the childs 

    close(listensock);

    //if (b_psiesta < 0)
    //    perror("Error accepting connection from browser: \n");

    //wait(&status);
    
}
