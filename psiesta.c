#include <stdio.h>
#include <stdlib.h>
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
#define PAGE "/datos.html"
#define USERAGENT "WebHTML"

/* Global variable */
char *query;
char *serverip, *serverport, *localport, *time;
char archv[15];
FILE *point;

/******************************************/
/* Helper functions */

void error_handler(char * msg){
    fprintf(stderr, "web server: %s\n", msg);
    exit(EXIT_FAILURE);
}

char * make_query(char *host, char *page){

    char * message  = "\nGET %s HTTP/1.0\r\nHost: %s\r\nUser-agent: %s\r\n\r\n";
    query = (char *)malloc(strlen(host) + strlen(page) + strlen(message) +
                           strlen(USERAGENT));
    sprintf(query,message, page, host, USERAGENT);
    
    return query;
}

/******************************************/

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

        
    /* Code Server */
    int listensock, b_psiesta;
    int sent, tmp, i;
    char recvbuf[BUFSIZ+1];
    char sendbuf[BUFSIZ+1];
    char * endstr;
    char *aux;
    struct sockaddr_storage data ;
    struct sockaddr_in server, address;
    struct in_addr inaddr;
    struct hostent* host;
    FILE *file;
    int sock;
    
    memset(sendbuf,0,sizeof(sendbuf));
    memset(archv,0,sizeof(archv));
    memset(recvbuf,0,sizeof(recvbuf));
    memset(&server,0,sizeof(server));

    /* Creating socket between server and client */
    if ((listensock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error_handler("Error creating server socket...\n");

    /* Populate the struct */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY ;
    server.sin_port = htons(atoi(localport));  /* temporally */

    //Binding the socket
    if(bind(listensock, (struct sockaddr * ) &server, sizeof(server)) < 0)
        error_handler("Binding socket not created...\n");
       
    //Put the socket to listen
    if (listen(listensock, NCLIENTS) < 0)
        error_handler("listen call failed\n");

    //Loop for receiving request from clients
    socklen_t size_data; 
    size_data = sizeof(size_data);


    /* this child updates the files  */
    if(!fork()){
        char c[15];
        sleep(atoi(time));
        while(1){
            if((point = fopen(aux, "r")) == NULL)
                perror("fopen: ");

            while(fscanf(point,"%s",c)){ 
                //Send query again 
                if(connect(sock, (struct sockaddr *)&address, sizeof(struct sockaddr)) < 0){
                    printf("connection not established");
                    //El servidor esta caido, busco en la cache y si no tengo
                    //el archivo devuelvo un msj 
                }

                //El servidor esta arriba sobreescribo la informacion
                //y la mando al browser

                //Build the query 
                char ip[20];
                inet_ntop(AF_INET,&(address.sin_addr),ip,20);
                sprintf(ip,"%s%s",ip,serverport);
                printf("ip %s\n\n",ip);
                query = make_query(ip, archv);
                printf("\nQUERY: \n<BEGIN>%s<END>\n",query);
                fprintf(point, "%s", archv); 

                //Send the query to server. If there's something that has not been
                //sent, the rest is sent.

                while (sent < strlen(query)){
                    tmp = send(sock, query, strlen(query) - sent , 0);
                    if(tmp == -1){
                        error_handler("Error sending query..."); 
                    }

                    sent += tmp;
                }

                memset(recvbuf, 0, sizeof(recvbuf));
                if ((file = fopen(archv,"w+")) == NULL)
                    error_handler("Error opening file... ");

                // Receiving the data 
                while((tmp = recv(sock, recvbuf, BUFSIZ, 0)) > 0 ){
                    fwrite(recvbuf, sizeof(char) ,sizeof(recvbuf), file);
                    perror("");
                    memset(recvbuf, 0, tmp); 

                } 

                if(tmp < 0){
                    error_handler("Error receiving data...");
                }

                free(query); 
                close(sock);

            }
        }
    }


    while (1){

        b_psiesta = accept(listensock,(struct sockaddr *)&data ,&size_data); 
        if (b_psiesta == -1)
            error_handler("Error accepting connection ...\n");

        inet_ntop(data.ss_family,
                (void *)(&(((struct sockaddr_in *)((struct sockaddr *)&data))->sin_addr)),
                recvbuf, sizeof(recvbuf));
        printf("server: got connection from %s\n", recvbuf);

        memset(recvbuf,0,sizeof(recv));

        if(!fork()){ // This is the child process
            close(listensock);

            while((tmp=recv(b_psiesta,recvbuf,BUFSIZ,0))<0){
                perror("");
            }

            endstr = strstr(&recvbuf[4]," ");            

            for(i=4;recvbuf[i]!=endstr[0];i++){
                archv[i-4]=recvbuf[i];
            }

            printf("archv %s\n", archv);

            sent = 0;

            if(inet_aton(serverip, &inaddr))
                host = gethostbyaddr((char *)&inaddr, sizeof(inaddr), AF_INET);
            else
                host = gethostbyname(serverip); 

            printf("IP: %s\n", inet_ntoa(*(struct in_addr *) host->h_addr));
            printf("Host: %s\n", host->h_name);

            if(!host){
                herror("Error looking up host\n");
                exit(1);
            }

            //Create socket
            if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                error_handler("listening socket not created");

            //Fill the structure with host data
            address.sin_family = AF_INET; 
            address.sin_port = htons(atoi(serverport));
            memcpy(&address.sin_addr , host->h_addr_list[0], sizeof(address.sin_addr));

            //Connect with host 
            if(connect(sock, (struct sockaddr *)&address, sizeof(struct sockaddr)) < 0){
                printf("connection not established");
                //El servidor esta caido, busco en la cache y si no tengo
                //el archivo devuelvo un msj 
            }

            //El servidor esta arriba sobreescribo la informacion
            //y la mando al browser

            //Build the query 
            char ip[20];
            inet_ntop(AF_INET,&(address.sin_addr),ip,20);
            sprintf(ip,"%s%s",ip,serverport);
            printf("ip %s\n\n",ip);
            query = make_query(ip, archv);
            printf("\nQUERY: \n<BEGIN>%s<END>\n",query);
            fprintf(point, "%s", archv); 

            //Send the query to server. If there's something that has not been
            //sent, the rest is sent.

            while (sent < strlen(query)){
                tmp = send(sock, query, strlen(query) - sent , 0);
                if(tmp == -1){
                    error_handler("Error sending query..."); 
                }

                sent += tmp;
            }

            memset(recvbuf, 0, sizeof(recvbuf));
            if ((file = fopen(archv,"w+")) == NULL)
                error_handler("Error opening file... ");

            // Receiving the data 
            while((tmp = recv(sock, recvbuf, BUFSIZ, 0)) > 0 ){
                fwrite(recvbuf, sizeof(char) ,sizeof(recvbuf), file);
                perror("");
                memset(recvbuf, 0, tmp); 

            } 

            if(tmp < 0){
                error_handler("Error receiving data...");
            }

            free(query); 
            close(sock);


            if(send(b_psiesta,"Hello, World!" ,13,0) == -1)
                error_handler("Error sending...\n");


            close(b_psiesta);
            exit(0);           

        }

        close(b_psiesta);
    }
    
}

