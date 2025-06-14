//=========================================================================
/*   SERVER TCP SELECT - SZABLON
*/
//===========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

//===========================================================================
// status połączenia
//===========================================================================
#define CONNSTATE_IDLE 0
#define CONNSTATE_RECEIVING 1
#define CONNSTATE_CLOSING 3

//===========================================================================
// dane wstępne serwera
//===========================================================================
#define NAME_SIZE 20
#define SERWER_IP "0.0.0.0"
int port_number,Nowy_port;
struct sockaddr_in serwer;
int id_client=0;

int free_conn = 0, recv_conn = 0, send_conn = 0; // Tworze zmienne globalne


//===========================================================================
// informacje o połączeniach
//===========================================================================
struct conninfo{
   int status;   
   int sock;
   unsigned char name[NAME_SIZE+1];
   unsigned int name_pos;
   int stepcnt;
   int port;
   char type;
	
	struct sockaddr_in gniazdo,klient;   
	socklen_t klient_len;    
   
	int id_client;
	int id_sub_client;
	int pom_sub_id;   
	uint32_t full_id;
   
   
   // Odbieranie
   unsigned char recv_buffer[1024];
   unsigned to_recv;
   unsigned char *recv_ptr;   
   
   // Wysylanie
   unsigned char send_buffer[1024];
   unsigned to_send;
   unsigned char *send_ptr;
};

#define MAX_CONNECTION 10
struct conninfo conn[MAX_CONNECTION];


//===========================================================================
//  i - indeks polaczenia  c-odebrany znak
//===========================================================================
void save_name(int i, unsigned char* c)		//zapisujemy nazwe
{
    int j = 0;
    while (conn[i].name_pos < NAME_SIZE) {
        conn[i].name[conn[i].name_pos] = c[j];
        conn[i].name_pos++;
        j++;
    }
}


//===========================================================================
// dodanie nowego połączenia
//===========================================================================
void add_new_conn(int fd,char* name,int main_id,int sub_id,char type)
{
    for (int i=0; i<MAX_CONNECTION; i++)	// Iteruje po wszystkich polaczeniach
    {
        if (conn[i].status == CONNSTATE_IDLE)  // Sprawdza czy stan polaczenia jest "NIEOBECNY"
        {
            bzero(&conn[i],sizeof(conn[i]));		// Czysci miejsce na polaczenie jesli cos zostalo
            
            conn[i].sock = fd;								// Przypisuje gniazdo
            conn[i].status = CONNSTATE_RECEIVING;		// Zmieniamy status polaczenia 
            conn[i].name_pos = 0;    						// Ustawiamy pozycje do nazwy na 0 by mógł zapisać od poczatku
            conn[i].id_client = main_id;
            conn[i].type = type;
            
          
            
            
				if ((name != NULL) && (strlen(name) > 0))
				{
				    save_name(i, name);
				}          
				if(sub_id != 0)
				{
					 conn[i].id_sub_client = sub_id;
				}            
				
				conn[i].full_id = (conn[i].id_client << 16) | conn[i].id_sub_client;
            
				printf ("Udało sie dodać połączenie\n");
            break;
        }
	 }

}

//===========================================================================
// zamknięcie połączenia
//===========================================================================
void close_conn(int i) {
   close(conn[i].sock);
   bzero(&conn[i], sizeof(conn[i]));
   conn[i].status = CONNSTATE_IDLE;
}



//===========================================================================
// wysłano jeden znak, kontynuuj lub zakończ
//===========================================================================

void commands(int i, unsigned char* c)		//Zapisujemy nazwe uzytkownika w tablicy conn[i].name
{
	printf("\033[2J");
	printf("\033[20;0H");

	printf("Otrzymana Komenda : %s \n",c);			
	char* buffer;
	char *wynik;
	size_t length;	
	
	if(strstr(c, ":") >= 0){
	char *start = strchr(c, ':');									//|
	char *end = strchr(c, ';');									//|
	length = end - start - 1;								//|
																			//|	Wyznaczam wartość z komendy 
	wynik = malloc(length + 1);  // +1 na '\0'		//|
	strncpy(wynik, start + 1, end - start - 1);				//|	
	wynik[length] = '\0';  // Dodajemy null-terminator		//|
	}	
	
	printf("   Wartość : %s /n",wynik);							//|	
	


	switch (c[1])
	{
		
	case 'C':
	printf("Komenda C:\n");

	add_new_conn(i,NULL,id_client,0,'C');  // Zamiast indexu tutaj jest wyjątek i jest deskryptor gniazda

	printf("Komenda C:Wykonana\n");
	break;	

// ==========================================
	case 'N':
	printf("Komenda N:\n");

	save_name(i,wynik); // Zamiast indexu tutaj jest wyjątek i jest deskryptor gniazda
	
	 length = snprintf(NULL, 0, "#NOK@CONNECTION:%d;!",conn[i].full_id);
    buffer = malloc(length + 1);
    sprintf(buffer,"#NOK@CONNECTION:%d;!",conn[i].full_id);
    
    send(conn[i].sock, buffer, length, MSG_NOSIGNAL); 	

	printf("Komenda N:Wykonana\n");
	break;	

// ==========================================
	
	case 'T':
	printf("Komenda T:\n");

	int new_TCP_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	
	conn[i].pom_sub_id++;
	add_new_conn(new_TCP_socket,conn[i].name,conn[i].id_client,conn[i].pom_sub_id,'T'); // Zamiast indexu tutaj jest wyjątek i jest deskryptor gniazda

	printf("Komenda T:Wykonana\n");
	break;	

// ==========================================

	case 'U':   
	printf("Komenda U:\n");

	int new_UDP_socket = socket(AF_INET, SOCK_DGRAM , 0);
	add_new_conn(new_UDP_socket,conn[i].name,conn[i].id_client,conn[i].pom_sub_id,'U');

   if(bind(new_UDP_socket,(struct sockaddr*)&conn[i].gniazdo, sizeof(conn[i].gniazdo)) != 0)
   	printf("UDP Bind(): Error");

    length = snprintf(NULL, 0, "#UOK@PORT:%d;CONNECTION:%d;!", Nowy_port, conn[i].full_id);
    buffer = malloc(length + 1);
    sprintf(buffer,"#UOK@PORT:%d;CONNECTION:%d;!",Nowy_port,conn[i].full_id);


    send(conn[i].sock, buffer, length, MSG_NOSIGNAL); // Wysyłam komunikat do klienta o utworzeniu połączenia UDP

	printf("Komenda U:Wykonana\n");
	break;		

// ==========================================

	case 'A':	//aktywne polaczenia
	printf ("Komenda A");

   
	printf("\nSend: %s  \n", conn[i].send_buffer); // DEBUG - DEBUG - DEBUG - DEBUG - DEBUG - DEBUG    
                                                    // MUSHROOM! MUSHROOM!

    length = snprintf(NULL, 0, "#AOK@ALL:%d;!", MAX_CONNECTION-free_conn);
    buffer = malloc(length + 1);
    sprintf(buffer,"#AOK@ALL:%d;!", MAX_CONNECTION-free_conn);
    send(conn[i].sock, buffer, length, MSG_NOSIGNAL); 
    free(buffer);
	
	break;	

// ==========================================
	
	case 'F': //Wolne polaczenia
	printf ("Komenda F");
	
	length = snprintf(NULL, 0, "#FOK@FREE:%d;!", free_conn);
    buffer = malloc(length + 1);
    sprintf(buffer,"#FOK@FREE:%d;!", free_conn);
    send(conn[i].sock, buffer, length, MSG_NOSIGNAL); 
    free(buffer);

   
	break;
	
	// ==========================================
	
	case 'K': 
	
	break; 
	}
}



//===========================================================================
// główna pętla serwera
//===========================================================================
int main(int argc, char *argv[]) {
    fd_set readfds, writefds;
    int maxfd;

    bzero(&serwer, sizeof(serwer));
    bzero(&conn, sizeof(conn));

    if (argc < 2) {
        perror("Nie podano portu");
        exit(1);
    }

    port_number = atoi(argv[1]);
    serwer.sin_family = AF_INET;
    serwer.sin_port = htons(port_number);
    Nowy_port = port_number + 2000;

    if (inet_pton(AF_INET, SERWER_IP, &serwer.sin_addr) <= 0) {
        perror("inet_pton() ERROR");
        exit(2);
    }

    int listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listen_sock < 0) {
        perror("socket() ERROR");
        exit(3);
    }

    if (bind(listen_sock, (struct sockaddr *)&serwer, sizeof(serwer)) < 0) {
        perror("bind() ERROR");
        exit(4);
    }

    if (listen(listen_sock, 20) < 0) {
        perror("listen() ERROR");
        exit(5);
    }


// ============================================================================









   printf("\033[2J");	// czyszcze ekran przed petla

    while (1) {
        FD_ZERO(&readfds);

        maxfd = listen_sock;

        FD_SET(listen_sock, &readfds);




        for (int i = 0; i < MAX_CONNECTION; i++) {			// Przypisuje gniazdo do zbioru
            if (conn[i].status == CONNSTATE_RECEIVING) {
                FD_SET(conn[i].sock, &readfds);
                if (conn[i].sock > maxfd) maxfd = conn[i].sock;
        			 }
		  }

        struct timeval timeout = {.tv_sec = 0, .tv_usec = 100000}; 						// Czas do timeout'u

        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);		// SELECT - serce programu
        if (activity < 0) {
            perror("select() ERROR");
            continue;
        }






			// Sprawdzamy czy liste_sock ma nowe połączenie 
        if (FD_ISSET(listen_sock, &readfds)) {
            int newsock = accept(listen_sock, NULL, NULL);
            if (newsock >= 0) {
            	 id_client++;
                commands(newsock,"#C@:;!");		// jesli jest to dodajemy je
            }
        }




		  printf("\033[20;0H \033[J \033[20;0H");
		  
			// Lecimy po wszystkich połączeniach i sie nimi zajmujemy
        for (int i = 0; i < MAX_CONNECTION; i++) {
        	
        	
				// Patrzymy czy w zbiorze readfds są gniazda które chcą nam coś przekazać        		
        		if (FD_ISSET(conn[i].sock, &readfds)) {    
        			printf("Połączenie [%d] jest typu %d.",i,conn[i].type);

        		
					 if(conn[i].type == 'C')	// jeśli to połączenie C
					 {
					 	printf("Połączenie typu C wysyła.");
	                int ret = recv(conn[i].sock, conn[i].recv_buffer, sizeof(conn[i].recv_buffer), 0);
						 commands(i,conn[i].recv_buffer);									 
					 }                        		            

					 
					 
					 
					 if(conn[i].type == 'T')	// jeśli to połączenie TCP
					 {
					 	printf("Połączenie typu T wysyła.");				 	
	                int ret = recv(conn[i].sock, conn[i].recv_buffer, sizeof(conn[i].recv_buffer), 0);
						 commands(i,conn[i].recv_buffer);									 
					 }                
				

					

					 if(conn[i].type == 'U')	// jeśli to połączenie UDP
					 {
						 recvfrom(conn[i].sock,conn[i].recv_buffer,sizeof(conn[i].recv_buffer),0,(struct sockaddr*)&conn[i].gniazdo,&conn[i].klient_len);
						 printf("Wiadomosc od U %s :",conn[i].recv_buffer);									 
					 }                
              
				}
                          
        		// jeśli klient chce się rozłączyć to go rozłączamy |||	SERVER NIE MOZE SAM GO ROZŁĄCZYĆ BEZ POWODU	|||                    
            else if (conn[i].status == CONNSTATE_CLOSING) {
                close_conn(i);
            }
		   }     
        



		  // ==================================================================================================
		  //							DEKORACJA
		  // ==================================================================================================
		  free_conn = 0, recv_conn = 0;
        for (int i = 0; i < MAX_CONNECTION; i++) {
            switch (conn[i].status) {
                case CONNSTATE_IDLE: 			free_conn++; break;
                case CONNSTATE_RECEIVING: 	recv_conn++; break;
            }
        printf("\033[%d;15H		\nNazwa uzytkownika: %s \033[35Gid: %d %d \033[45G Full_id: %d   \033[65GTyp: %c\n",i+1,conn[i].name,conn[i].id_client,conn[i].id_sub_client,conn[i].full_id,conn[i].type);         
        }
        static int loopnr = 0;
        printf("connections: sending: %d  receiving: %d   free: %d     %d \r", send_conn, recv_conn, free_conn, loopnr++);
        usleep(500000);
    }

}
