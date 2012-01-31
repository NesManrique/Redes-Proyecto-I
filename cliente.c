#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>
     

int main(int argc, char *argv[])
{
    struct sockaddr_in address;
    struct in_addr inaddr;
    struct hostent* host;
    char *name;
    int sock;


    if(inet_aton(argv[1], &inaddr))
        host = gethostbyaddr((char *)&inaddr, sizeof(inaddr), AF_INET);
    else
        host = gethostbyname(argv[1]); 

    
    printf("IP: %s\n", inet_ntoa(*(struct in_addr *) host->h_addr));


    if(!host){
        herror("Error looking up host\n");
        exit(1);
    }
        

    //Create socket
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    //die("socket");
      perror("socket not created\n");
    
    
    //Fill the structure with host data
    address.sin_family = AF_INET ; 
    address.sin_port = htons(80);
    memcpy(&address.sin_addr , host->h_addr_list[0],sizeof(address.sin_addr));

    if(connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
        //die("connect");
        printf("connection not established");

    return 0;
}
