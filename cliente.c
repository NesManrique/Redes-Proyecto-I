#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>

/* Macro variables */
#define NCLIENTS 5
#define PAGE "index.html"
#define USERAGENT "WebHTML"

/* Global variable */
char *query;


/******************************************/
/* Helper functions */

void error_handler(char * msg){
    fprintf(stderr, "web server: %s\n", msg);
    exit(EXIT_FAILURE);
}

char * make_query(char *host, char *page){

    char * message  = "\nGET /%s HTTP/1.0\r\nHost: %s\r\nUser-agent: %s\r\n\r\n";
    query = (char *)malloc(strlen(host) + strlen(page) + strlen(message) +
                           strlen(USERAGENT));
    sprintf(query,message, page, host, USERAGENT);
    
    return query;
}


/******************************************/

int main(int argc, char *argv[])
{
    struct sockaddr_in address;
    struct in_addr inaddr;
    struct hostent* host;
    char *page;
    int sock;
    int sent, tmp;
    char buffer[BUFSIZ+1];
    FILE *file;
    

    page = PAGE;
    sent = 0;
   
    
    if(inet_aton(argv[1], &inaddr))
        host = gethostbyaddr((char *)&inaddr, sizeof(inaddr), AF_INET);
    else
        host = gethostbyname(argv[1]); 

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
    address.sin_family = AF_INET ; 
    address.sin_port = htons(80);
   
    inet_ntop(AF_INET, &(address.sin_addr), buffer, sizeof(buffer) );
    printf("ip en formato legible %s\n", buffer);
    
    memcpy(&address.sin_addr , host->h_addr_list[0], sizeof(address.sin_addr));
    
    inet_ntop(AF_INET, &(address.sin_addr), buffer, sizeof(buffer));
    printf("----- %s\n", buffer);
 
    /* Connect with host */
    if(connect(sock, (struct sockaddr *)&address, sizeof(struct sockaddr)) < 0)
        error_handler("connection not established");

    /*Build the query */ 
    query = make_query(host->h_name, PAGE);
    printf("\nQUERY: \n<BEGIN>%s<END>\n",query);

    /*
      Send the query to server. If there's something that has not been
      sent, the rest is sent.
    */
    while (sent < strlen(query)){
        tmp = send(sock, query, strlen(query) - sent , 0);
        if(tmp == -1){
            error_handler("Error sending query..."); 
        }
        
        sent += tmp;
    }
    
    memset(buffer, 0, sizeof(buffer));
    if ((file = fopen("response","w")) == NULL)
        error_handler("Error opening file... ");

    /* Receiving the data */
    while((tmp = recv(sock, buffer, BUFSIZ, 0)) > 0 ){

        fwrite(buffer, sizeof(char) ,sizeof(buffer), file);
        
        memset(buffer, 0, tmp); 

    } 

    if(tmp < 0){
        error_handler("Error receiving data...");
    }
   
    free(query); 
    close(sock);
    /*
    //Assign addres to socket
    if(bind(sock, (struct sockaddr * ) address, sizeof(address)) < 0)
        error_handler("Binding socket not created");
       
    //Put the socket to listen
    if (listen(sock, NCLIENTS) < 0)
        error_handler("listen call failed");

    //Loop for receiving request from clients
    while (1){

    }
    */    
    
    return 0;
}
