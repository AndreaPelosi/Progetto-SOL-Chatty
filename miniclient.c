#include <stdio.h>
#include <connections.h>


int main(){
    int fd;

    fd = openConnection("/tmp/chatty_socket", 10, 3);

    printf("ecco il fd della connessione col server: %d, test superato miniclient\n", fd);

    return 0;
}
