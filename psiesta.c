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
char line[2000];
char *serveraddr, *serverport, *localport, *time;
char serverip[20];
char archv[500];
char response[85] = "La informacion aun no esta disponible por favor reintente su solicitud mas tarde\n"; 
struct in_addr inaddr;
struct hostent* host;
struct sockaddr_storage data ;
struct sockaddr_in server, address;
FILE *file, *point, *cache;

/******************************************/
/* Helper functions */

void ext_handler(int sig){
    //printf("Wake up call ... !!! - Catched signal: %d ... !!\n", sig); 
    if(kill(0,SIGTERM)<0) //Kill the childs 
        perror("Error killing childs");
        
    //(void) signal(SIGINT,SIG_DFL); 
}

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
                printf("Wellcome to Psiesta\n\nPsensor Host: %s\n", host->h_name);
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

    (void) signal(SIGINT,ext_handler);

    parser(argc,argv);

    printf("IP %s\nServPort %s\nLocalPort %s\nTime %s\n\n", serverip,
            serverport, localport, time);

    memset(sendbuf,0,sizeof(sendbuf));
    memset(archv,0,sizeof(archv));
    memset(recvbuf,0,sizeof(recvbuf));
    memset(&server,0,sizeof(server));
    memset(&address,0,sizeof(address));

    /* This child updates the files  */
    pid = fork();
    if(pid<0){
        perror("Error forking");
        close(listensock);
        exit(1);

    }else if(pid==0){
        close(listensock);
        char nextfile[500];
        if((point = fopen("Files", "w+")) == NULL)
            perror("**Cache Manager Child**\nError opening the cache file");

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
                    perror("**Cache Manager Child**\nError creating file manager child");

                //Connecting psensor 
                if(connect(s_psensor, (struct sockaddr *)&address, sizeof(struct sockaddr))<0){
                    //The server psensor is down so this child continues with
                    //next file
                    printf("**Cache Manager Child**\nNo connection to server\n");
                    continue;
                }

                printf("Updating %s...\n\n",nextfile);

                //Build the query for psensor 
                char ip[20];
                sprintf(ip,"%s%s%s",serverip,":",serverport);
                make_query(ip, nextfile);
                //printf("\n**Cache Manager Child**\nQUERY: \n<BEGIN>%s<END>\n",query);

                //Send the query to server
                tmp = send(s_psensor, query, strlen(query), 0);
                //printf("\n**Cache Manager Child**\ntmp send to psensor %d\n",tmp);
                if(tmp<0)
                    perror("**Cache Manager Child**\nError sending query");


                memset(recvbuf, 0, 1024);
               
                // Receiving the data and overwriting the cache
                flag=1;
                flag2=1;
                while((tmp = recv(s_psensor, recvbuf, 1023, 0)) > 0 ){

                    //printf("\n**Cache Manager Child**\ntmp from psensor %d\n",tmp);
                    //fwrite(recvbuf, sizeof(char) ,tmp, stdout);

                    if(flag && 200==atoi(&recvbuf[9])){

                        //If first loop and no problems recving
                        //open file nextfile
                        if ((file = fopen(nextfile,"w")) == NULL)
                            perror("**Cache Manager Child**\nError opening file");

                        if(fwrite(recvbuf, sizeof(char) ,tmp, file)!=tmp)
                            printf("**Cache Manager Child**\nError writing in file %s\n",nextfile);
                        memset(recvbuf, 0, tmp);
                        flag2=0; //flag2=0 no problems in first chunk of data
                    }

                    if(!flag && !flag2){
                        //Store next chuncks knowing its not the first loop
                        //and there was no problem with the first
                        if(fwrite(recvbuf, sizeof(char) ,tmp, file)!=tmp)
                            printf("**Cache Manager Child**\nError writing in file %s\n",nextfile);
                        memset(recvbuf, 0, tmp);
                    }

                    flag=0;
                }

                if(tmp < 0){
                    perror("**Cache Manager Child**\nError receiving data");
                }

                memset(query,0,sizeof(query));
                if(!flag2)
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
        perror("Error creating server socket");
        kill(0,SIGTERM);
        exit(1);
    }

    /* Populate the struct */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(localport)); 

    //Binding the socket
    if(bind(listensock, (struct sockaddr * ) &server, sizeof(server)) < 0){
        perror("Binding socket not created");
        kill(0,SIGTERM);
        exit(1);
    }

    //Put the socket to listen
    if (listen(listensock, NCLIENTS) < 0){
        perror("Listen call failed");
        kill(0,SIGTERM);
        exit(1);
    }

    socklen_t size_data; 
    size_data = sizeof(size_data);

    //Loop for receiving request from clients
    while(1){

        b_psiesta = accept(listensock,(struct sockaddr *)&data, &size_data);
        if (b_psiesta < 0){
            perror("Error accepting connection from browser");
            kill(0,SIGTERM);
            exit(1);
        }

        //Childs to attend browsers requests
        pid = fork();
        if(pid<0){
            perror("Error forking");
            close(listensock);
            close(b_psiesta);
            kill(0,SIGTERM);
            exit(1);
        }
        if(pid==0){
            close(listensock);
            // Receiving the data from browser 
            tmp = recv(b_psiesta, &recvbuf[got], 1023, 0);
            //printf("\ntmp recv from browser %d\n",tmp);
            //fwrite(&recvbuf[got], sizeof(char) ,tmp, stdout);

            if(tmp<0)
                perror("Error receiving data");
    
            memset(archv,0,500);

            //Parsing the file to retrieve from psensor
            for(i=5;recvbuf[i]!=' ';i++){
                archv[i-5]=recvbuf[i];
            }

            archv[i-5]='\0';

            i=1; //File archv exists
            
            if((file = fopen(archv,"r")) == NULL)
                i = 0; //i=0 If archv doesnt exists
            else
                fclose(file);

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
                if(i){
                    //If we got the archive, send it to browser
                    if((file=fopen(archv, "r"))==NULL)
                        perror("Error opening file");

                    memset(line,0,2000);

                    fread(line,sizeof(char), sizeof(line), file);
                    tmp = send(b_psiesta,line,strlen(line),0);
                    //printf("BUSCANDO EN CACHE... linewidth %d, tmp send to browser%d\n",(int)strlen(line),tmp);
                    printf("Found %s in cache, sending it to browser...\n\n",archv);
                    if(tmp<0)
                        perror("Error sending response to browser");

                    fclose(file); 
                }else{
                    //If we dont, send a message
                    //printf("no connection to server\n%s\n",response);
                    tmp = send(b_psiesta, response, strlen(response), 0);
                    if(tmp<0)
                        perror("Error sending response to browser");
                    
                }
                exit(1);
            }

            //Make query for psensor
            char ip[20];
            sprintf(ip,"%s%s%s",serverip,":",serverport);
            make_query(ip, archv);
            //printf("\nQUERY: \n<BEGIN>%s<END>\n",query);

            //Send query to psensor
            tmp = send(s_psensor, query, strlen(query), 0);
            //printf("\ntmp send to psensor %d\n",tmp);
            if(tmp<0)
                perror("Error sending query");

            memset(recvbuf, 0, 1024);
            
            //printf("\narchv %s\n",archv);

            //Receiving the data and overwriting the file
            flag=1;
            flag2=1;
            while((tmp = recv(s_psensor, recvbuf, 1023, 0))>0){

                //printf("\ntmp recv from psensor %d\n",tmp);
                //fwrite(recvbuf, sizeof(char) ,tmp, stdout);

                //printf("\nflag %d - code %d\n", flag , atoi(&recvbuf[9]));

                if(flag && 200==atoi(&recvbuf[9])){
                    //printf("first loop if, i is %d\n",i);

                    //If first loop and no problems recving
                    //open file archv
                    if((file = fopen(archv,"w")) == NULL)
                        perror("Error opening file");

                    if(!i){
                        //i=0 if file doesnt exists
                        //If file doesnt exists, add it in the cache file
                        if((cache = fopen("Files","a"))==NULL)
                            perror("Error opening file");

                        if(fwrite(archv, sizeof(char), strlen(archv), cache)!=strlen(archv))
                            printf("Error writing in cache file\n");
                        if(fputc('\n',cache)<0)
                            printf("Error writing char in cache file\n");

                        fclose(cache);
                    }

                    if(fwrite(recvbuf, sizeof(char), tmp, file)!=tmp)
                        printf("Error writing in file %s\n",archv);
                    memset(recvbuf, 0, tmp);
                    flag2=0; //flag2=0 no problems in first chunk of data
                }

                if(!flag && !flag2){
                    //Store next chuncks knowing its not the first loop
                    //and there was no problem with the first
                    if(fwrite(recvbuf, sizeof(char), tmp, file)!=tmp)
                        printf("Error writing in file %s\n",archv);
                    memset(recvbuf, 0, tmp);
                }

                flag=0;
            }

            if(tmp < 0){
                perror("Error receiving data from psensor");
            }

            if(!flag2)
                fclose(file);

            if(flag2==1){
                //Psensor is up but couldnt retrieve information
                //Send to browser
                if(i){
                    //If we got the archive, send it to browser
                    if((file=fopen(archv, "r"))==NULL)
                        perror("Error opening file");

                    memset(line,0,2000);

                    fread(line,sizeof(char), sizeof(line), file);
                    tmp = send(b_psiesta,line,strlen(line),0);
                    //printf("BUSCANDO EN CACHE... linewidth %d, tmp send to browser%d\n",(int)strlen(line),tmp);
                    printf("Found %s in cache, sending it to browser...\n\n",archv);
                    if(tmp<0)
                        perror("Error sending response to browser");

                    fclose(file); 
                }else{
                    //If we dont, send a message
                    //printf("Problem with info\n%s\n",response);
                    tmp = send(b_psiesta, response, strlen(response), 0);
                    if(tmp<0)
                        perror("Error sending response to browser");
                }
            }else if(flag2==0){
                //Psensor is up and file was found
                if((file = fopen(archv,"r")) == NULL)
                    perror("Error opening file");

                memset(line,0,2000);

                printf("Sending file %s from psensor to browser...\n\n",archv);

                fread(line,sizeof(char), sizeof(line), file);
                tmp = send(b_psiesta,line,strlen(line),0);
                //printf("linewidth %d, tmp send to browser %d\n",(int)strlen(line),tmp);
                if(tmp<0)
                    perror("Error sending response to browser");

                fclose(file);
            }

            close(b_psiesta);
            close(listensock);
            exit(0);
        }
        close(b_psiesta);
    }
    
}
