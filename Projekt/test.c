//=========================================================================
/*   SERVER TCP SELECT



*/
//===========================================================================
#include <stdio.h>
#include <stdlib.h> // exit()
#include <string.h> // memset()
#include <arpa/inet.h> // inet_pton()
#include <sys/socket.h>


// Server program
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>         
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
// ==========================================================================
//---WH--- co ma byc wyswietlane
int show_network=1;
int show_select = 1;
int show_flow = 1;





//===========================================================================
// status polaczenia
//===========================================================================

#define CONNSTATE_IDLE 0
#define CONNSTATE_RECEIVING 1
#define CONNSTATE_SENDING 2
#define CONNSTATE_CLOSING 3



//================================================================
// dane Wstępne servera 
//===========================================================================

#define NAME_SIZE 20			
#define SERWER_IP "0.0.0.0" 
int port_number;
struct sockaddr_in serwer;
fd_set readfds; //robie to jako globalne

//===========================================================================
// informacje o polaczeniach  - Struktura połączeń
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


//================================================================
// Ilość połączeń 
//===========================================================================
#define MAX_CONNECTION 15
struct conninfo conn[MAX_CONNECTION];    // Tablica klientów - 20 miejsc na klientów gdzie każde miejsce mówi pewne informacje
													  // jak naprzykład czy dane połączenie/klient czeka na dane czy chce je wysłać
int free_conn = 0, recv_conn = 0, send_conn = 0; // Deklaruje zmienne ilości danych typów połączeń by były gloalne
int id_client=0;
int Nowy_port;







// FUNKCJE I INNE TAKIE



//===========================================================================
// odebrano 1 znak  i - indeks polaczenia  c-odebrany znak
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
// dodanie nowego polaczenia		fd - deksryptor czyli gniazdo
//===========================================================================
int add_new_conn(int fd,char* name,int main_id,int sub_id,char type)
{
    if (show_flow)
        printf("add_new_conn type:%c  sock:%d\n",type,fd);
    
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
            
				if(type == 'U')
				{
				    
					    serwer.sin_family = AF_INET;		// Typ IP
					    inet_pton(AF_INET, "0.0.0.0", &serwer);		//  IP					    
   					 serwer.sin_port = htons(Nowy_port);		// Numer portu konewrtowany z INT na typ "SIECIOWY" (HOST to NETWORK short)	

//                    ---WH---     
                   conn[i].gniazdo.sin_family = AF_INET;		// Typ IP
					    inet_pton(AF_INET, "0.0.0.0",&conn[i].gniazdo.sin_addr) ;        //  IP                      
   					 conn[i].gniazdo.sin_port = htons(Nowy_port);		// Numer portu konewrtowany z INT na typ "SIECIOWY" (HOST to NETWORK short)	
   					 conn[i].port = Nowy_port;
						 Nowy_port++;   
				}            
            
            
				if ((name != NULL) && (strlen(name) > 0))
				{
				    save_name(i, name);
				}          
				if(sub_id != 0)
				{
					 conn[i].id_sub_client = sub_id;
				}            
				
				conn[i].full_id = (conn[i].id_client << 16) | conn[i].id_sub_client;
            
				printf ("Udało sie dodać połączenie  %d\n",i);
                return(i);
        
        }
	 }
    //---WH---
    return(-1);

}





//===========================================================================
// zamkniecie polaczenia 	i - indeks polaczenia 
//===========================================================================
void close_conn(int i)
{
   close( conn[i].sock );		// Zamykamy polaczenie
   bzero(&conn[i],sizeof(conn[i]));		// Usuwamy je
   conn[i].status = CONNSTATE_IDLE;		// Zmieniamy status na "NIEOBECNY"
}




//===========================================================================
// odebrano 1 znak  i - indeks polaczenia  c-odebrany znak
//===========================================================================
void commands(int i, unsigned char* c)		//Zapisujemy nazwe uzytkownika w tablicy conn[i].name
{
//---WH---
//	printf("\033[2J");
//	printf("\033[20;0H");

//---WH---  tak Pan nie moze wypisywac bo gdy w buforze nie bedzie \0 to bedzie segmentation fault
//	printf("\033[19;0H wiadomosc: %s\n",c);	
	char *wynik;	

	if(strchr(c, ':') > 0) {  //---ten sposob postepowania jest zly,. nie sprawdza tagow a tylko 2 komendy maja parametry
		char *start = strchr(c, ':');	
		char *end = strchr(c, ';');	
		size_t length = end - start - 1;
																	    		//|	Wyznaczam wartość z komendy 
																	    		
		wynik = malloc(length + 1);  // +1 na '\0'		//| //---WH--- agdzie jest free do tego malloca
		strncpy(wynik, start + 1, end - start - 1);				//|	
		wynik[length] = '\0';  // Dodajemy null-terminator		//|
		printf("Wartość : %s \n",wynik);							   //|	
	
	}
	else {
		wynik = "";
		printf("Brak wartosci komendy: %s \n",wynik);
	}

	int len = 0;	
	int retval;

   int required_length;
   char *send_buffer;
	
	
	switch (c[1])//---WH--- w specyfikacji nie ma komency C rozumiem ze pan ja sobie sztucznie tworzy ale czy to nie zaciemnia kodu?
	{
		
	case 'C':
	printf ("Komenda C: ");
	
	add_new_conn(i,NULL,id_client,0,'C'); //---WH--- a pod jaki indeks to zosta�o dodane czy to nie ma znaczenia?
				        
	break;	

// ==========================================
	case 'N':
	printf ("Komenda N: ");

		
	
	save_name(i,wynik);

    //fragment kodu odpowiedzialny za wysyłanie wiadomości (pamiętać o free bo segfault)
    required_length = snprintf(NULL, 0, "#NOK@CONNECTION:%d;!",conn[i].full_id);
    send_buffer = malloc(required_length + 1);
    sprintf(send_buffer,"#NOK@CONNECTION:%d;!",conn[i].full_id);
    
    send(conn[i].sock, send_buffer, required_length, MSG_NOSIGNAL); 			        
	 free(send_buffer);	
	break;	

// ==========================================
	
	case 'T':
	printf ("Komenda na utworzenie polaczenia TCP\n");
	
	int new_TCP_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);			//Tworze gniazdo TCP NIEBLOKUJĄCY
	
	conn[i].pom_sub_id++;	
	
	add_new_conn(new_TCP_socket,conn[i].name,conn[i].id_client,conn[i].pom_sub_id,'T');




    //fragment kodu odpowiedzialny za wysyłanie wiadomości (pamiętać o free bo segfault)
    required_length = snprintf(NULL, 0, "#TOK@PORT:%d;CONNECTION:%d;!", port_number, conn[i].full_id);
    send_buffer = malloc(required_length + 1);
    sprintf(send_buffer,"#TOK@PORT:%d;CONNECTION:%d;!",port_number,conn[i].full_id);
    send(conn[i].sock, send_buffer, required_length, MSG_NOSIGNAL); 
	
	free(send_buffer);
	break;	

// ==========================================

	case 'U':   
	printf ("Komenda na utworzenie polaczenia UDP\n");
	 
	 int new_UDP_socket = socket(AF_INET, SOCK_DGRAM , 0);		//Tworze gniazdo UDP
	 
    conn[i].pom_sub_id++;
   //---najpierw powinien byc bind potem dodanie polaczenia bo jak bind sie nie uda trzeba szukac nowego portu
   //---WH---  tu byl blad najwiekszy - programowany byl adres w polu gniazdo ale na indeks
   //  nowego polaczenia a i to indeks polaczenia C
	int newI = add_new_conn(new_UDP_socket,conn[i].name,conn[i].id_client,conn[i].pom_sub_id,'U');
    printf("New index of Uconnection: %d\n",newI);

    retval = bind(new_UDP_socket,(struct sockaddr*)&conn[newI].gniazdo, sizeof(conn[newI].gniazdo));
    if(retval == 0)
    {
		printf("Bind ok.") ;
    
    }
    //---WH---
    // to jest jeden z bardziej kuriozalnych sposobow wysylania odpowiedzi
    // gdzie jest free() do tego malloca,. po ilu odpowiedziech zabraknie pamieci?
    // a bufora nadawczego w polaczeniu nie ma?
    // a co bedzie jak send wysle tylo czesc odpowiedzi?  brak reakcji na wyniks end
    //--- a port number to czemu jest 8000?
    // required_length = snprintf(NULL, 0, "#UOK@PORT:%d;CONNECTION:%d;!", port_number, conn[i].full_id);
    required_length = sprintf(send_buffer,"#UOK@PORT:%d;CONNECTION:%d;!",conn[newI].port,conn[newI].full_id); 
    send_buffer = malloc(required_length + 1);
   // sprintf(send_buffer,"#UOK@PORT:%d;CONNECTION:%d;!",port_number,conn[i].full_id);
    sprintf(send_buffer,"#UOK@PORT:%d;CONNECTION:%d;!",conn[newI].port,conn[newI].full_id);
    send(conn[i].sock, send_buffer, required_length, MSG_NOSIGNAL); 

    //---WH--- w metodzie 1 programowania select zestawy zmieniamy TYLKO przed selectem
//	 FD_SET(new_UDP_socket,&readfds); // wrzucam 

//	 printf ("Poszło \n"); //---WH--- niby co poszlo jak nie jes sprawdzony rezultat send???
	free(send_buffer);
	break;		

// ==========================================

	case 'A':	//aktywne polaczenia
	printf ("Komenda A");

   
	printf("\nSend: %s  \n", conn[i].send_buffer); 
	
    required_length = snprintf(NULL, 0, "#AOK@ALL:%d;!", MAX_CONNECTION-free_conn);
    send_buffer = malloc(required_length + 1);
    sprintf(send_buffer,"#AOK@ALL:%d;!", MAX_CONNECTION-free_conn);
    send(conn[i].sock, send_buffer, required_length, MSG_NOSIGNAL); 
	free(send_buffer);
	
	break;	

// ==========================================
	
	case 'F': //Wolne polaczenia
	printf ("Komenda F");
	
	required_length = snprintf(NULL, 0, "#FOK@FREE:%d;!", free_conn);
    send_buffer = malloc(required_length + 1);
    sprintf(send_buffer,"#FOK@FREE:%d;!", free_conn);
    send(conn[i].sock, send_buffer, required_length, MSG_NOSIGNAL); 
	free(send_buffer);

   
	break;
	
	// ==========================================
	
	case 'K': //Wolne polaczenia
	printf ("Komenda K");

    //---WH---
    // gdy nie ma 0 nie mozna tak wypisywac jezeli nie ma perwnosci czy na ko�cu jest \0
	//printf("\033[19;0H wiadomosc: %s",c);	
	
	
   for (int i = 0; i < MAX_CONNECTION; i++)
   {
   	if(*wynik == conn[i].full_id)
   		conn[i].status = CONNSTATE_CLOSING;
   }
   

	break;

	}

    
}






//===========================================================================
// nie odebrano �adnego znaku
//===========================================================================
void recv_zero(int i)
{
	sprintf(conn[i].send_buffer,"Nic nie odebrano "); // zapisuje w bufforze ze nic nie odebrano
}







//===========================================================================
// wyslano  1 znak, - reakcja gdy send zwrocil 1
//===========================================================================
void send_char(int i) 
{
  if (conn[i].to_send>0) // Sprawdza czy zostalo cos jeszcze do wyslania
  {
      conn[i].to_send--;	// Pointer który pomaga wysylac bufor
      conn[i].send_ptr++;
  }
}





//===========================================================================
// 
//===========================================================================
int Uconn_handle_read(int idx)
{
    conn[idx].klient_len =  sizeof(conn[idx].klient);

    int retval = recvfrom(conn[idx].sock,conn[idx].recv_buffer,sizeof(conn[idx].recv_buffer),0,(struct sockaddr*)&conn[idx].klient,&conn[idx].klient_len);
    if (show_network)
        printf("#%d  recvfrom returns:%d\n",idx,retval);
   //---WH--- dalej sobie Pan sam napisze, ja tylko na g�upio odsy�am
   if (retval>0)
   {
        int retsend = sendto(conn[idx].sock,conn[idx].recv_buffer,retval,0,(struct sockaddr*)&conn[idx].klient,conn[idx].klient_len);
        if (show_network)
            printf("#%d  sendto returns:%d\n",idx,retsend);
        
    }
   else {

    }
}

//===========================================================================
// 
//===========================================================================
int Tconn_handle_read(int idx)
{
    //---WH zalozenie ze buyfor jest pusty dla tcp jezeli odbieramy po kawalku jest zleto trzeba poprawic    
    int retval = recv(conn[idx].sock,conn[idx].recv_buffer,sizeof(conn[idx].recv_buffer),0);
    if (show_network)
        printf("#%d  recv returns:%d\n",idx,retval);
    //---WH--- dalej sobie Pan sam napisze ja odsylam bez liczenia
    if (retval>0)
    {
         int retsend = send(conn[idx].sock,conn[idx].recv_buffer,retval,MSG_NOSIGNAL);
         if (show_network)
             printf("#%d  send returns:%d\n",idx,retsend);
         
     }
    else {
    
     }

}


//===========================================================================
// ---WH--- tu obie prosze przeniesc odbior danych z kanalu c
//===========================================================================
int Cconn_handle_read(int idx)
{

}





//=============================================================================================================================
// glowna petla
//===========================================================================
int main(int argc, char *argv[])
{
	fd_set readfds; // fd_set - to struktura || Zmienne read i write(fds) są tworzone na podstawie 
	int maxfd;					  // maxfd - mówi ile jest Klientów obecnie.


	


	 // Czyszcimy dane Servera (gdyby jakieś były) oraz czyścimy tablice klientów (gdyby byli by zapisani klienci widmo)  
    bzero((char*) &serwer,sizeof(serwer));
    bzero((char*) &conn,sizeof(conn));


    
    
	 // Sprawdzamy ile argumentow podano z lini komend   || Obsługa błędów 
    if (argc<2){
        perror(" nie podano portu\n");
        exit(19);
    }
    
    
    port_number = atoi(argv[1]);    // Numer portu konwertowany z STRING na INT  (ASCII to INT)
    serwer.sin_family = AF_INET,		// Typ IP
    serwer.sin_port = htons(port_number);		// Numer portu konewrtowany z INT na typ "SIECIOWY" (HOST to NETWORK short)
    
   Nowy_port = port_number + 2000;	// Tworze zmienna do różnych portów dla UDP
        
	int retval = inet_pton( AF_INET, SERWER_IP, & serwer.sin_addr );    // Konwertuje IP na BINARNY
    
    if( retval <= 0 ) // Obsługa błędów 
    {
        perror( "inet_pton() ERROR" );
        exit( 1 );
    }



   //---WH--- listen sock jest nieblokujacy a mai� by� blokujacy !!!
	int listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // Tworze Socket 
	 																		// AF_INET - Typ IPv4
	 																		// SOCK_STREAM - Typ protokołu | Dla TCP 
	 																		// SOCK_NONBLOCK - Flaga | Nieblokujący 
	 																		// 0 - Protokół | Niech system sam se wybierze co pasuje
    if(( listen_sock ) < 0 ) 			// Obługa błędów
    {
        perror( "socket() ERROR" );
        exit( 2 );
    }
   
   
   

//---WH---
    int opt =1;
    int optlen = sizeof(opt); 
    setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt,optlen);
    setsockopt(listen_sock,SOL_SOCKET,SO_REUSEPORT,&opt,optlen);   
   
   
   
   
    socklen_t len = sizeof( serwer ); 	// Długość adresu SERVERA
    
	 retval = bind( listen_sock,(struct sockaddr *) &serwer, len ); // Przypisuje gniazdo Servera do portu i adresu servera
    																		// listen_sock - gniazdo które chcemy przypisać
    																		// (struct sockaddr *) &serwer - adres servera
    																		// len - długość adresu servera 
    																		
    if( retval < 0 ) // Obsługa błędu		
    {
        perror( "bind() ERROR" );
        exit( 3 );
    }





	 retval = listen( listen_sock,20); // Ustawiam socket na nasłuchiwanie i mówie że maxymalnie może obejmować 20 połączeń

    if( retval < 0 ) // Obsługa błędu
    {
        perror( "listen() ERROR" );
        exit( 3 );
    }


// ====================================================================
// Wyrzucilem przed petle bo tak
// ====================================================================






   //---WH---
   //printf("\033[2J");	// czyszcze ekran przed petla
   
// =====================================================================================================================
// GLOWNA PETLA
// ====================================================================
   
    while (1)
{
	

	FD_ZERO(&readfds);		// Zerowanie zbioru dla gniazd do odczytu
	maxfd = listen_sock;		// Ustawienie maxfd na gniazdo nasłuchujące (Ta zmienna w skrócie mówi ile jest obecnie połączeń)

    // Dodaj gniazdo nasłuchujące do zbioru read
    FD_SET(listen_sock, &readfds);					// Dodaje gniazdo nasłuchujące do zbioru readfds bo select będzie sprawdzał
    															// właśnie w tym zbiorze a dokłądniej na tym listen_sock czy ktoś sie nie 
    															// chce dobić do nas
 
 
 
	 // Ustawienie timeout'u by select nie czekał wieczność
    struct timeval timeout;
    timeout.tv_sec = 0;			// 0s
    timeout.tv_usec = 100000; // 100ms timeout   






// ====================================================================
// Segregacja połączeń   ---WH--- to jest przygotowanie zestawow bardziej 
// ====================================================================
    // Dodaj aktywne połączenia do zbiorów
    for (int i = 0; i < MAX_CONNECTION; i++)
    {
			
		  //  Jeśli połączenie jest gotowe do odbioru danych
        if (conn[i].status == CONNSTATE_RECEIVING)		// CONNSTATE_RECEIVING - Stan odbierający
        {
            FD_SET(conn[i].sock, &readfds); // dodaje połączenie/gniazdo do readfds
            if (conn[i].sock > maxfd)
                maxfd = conn[i].sock; // Jeśli to jest najnowsze połączenie to mówimy o tym maxfd by wiedział ile jest połączeń
         }

    }

// ====================================================================
      if (show_select){        
         printf("przed select:\n");
         //for (int i = 0; i < MAX_CONNECTION; i++)            
         for (int i = 0; i < 6; i++)
             printf("#%d   s:%d   r:%d \n",i,conn[i].sock,FD_ISSET(conn[i].sock,&readfds));      
       }

// ====================================================================

	
	
	 
    int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);  //Serce programu - SELECT
																		// maxfd + 1 - Sprawdzamy do Najwiekszego deskryptorora + 1 bo to nie >=
																		// Zbiór gniazd do odczytywania daynych
																		// Zbiór gniazd do wysyłania danych

    if (activity < 0)    // Obsługa błędu
    {
        perror("select() ERROR");
        continue;
    }
//---WH---
//    printf("\033[20;30H Select(): %d ",activity);
      printf("Select(): %d \n",activity);


      if (show_select){        
         printf("po select:\n");
		//for (int i = 0; i < MAX_CONNECTION; i++)            
		 for (int i = 0; i < 6; i++)
    			printf("#%d   s:%d   r:%d \n",i,conn[i].sock,FD_ISSET(conn[i].sock,&readfds));      
      }
    
    	if( FD_ISSET(listen_sock,&readfds) ) // Patrzymy czy mamy jakieś nowe połączenie do dodania
	     	{
				int newsock = accept(listen_sock, NULL, NULL);

				int Zawiera = 1; 
	     		for(int i = 0; i < MAX_CONNECTION; i++)
	     		{
					if(newsock == conn[i].sock)
					{
						Zawiera = 0;
					}
	     		}    	  		
	     		if(Zawiera==1) {
		     			id_client++;
						commands(newsock,"#C@:;!");
	     		}
	     	}    
    //---WH---
    //printf("\033[20;0H");

//---WH---   a tu sobie niszczymy zawartosc readfs  - tu jest straszny ba�agan w kodzie
/*    for (int i = 0; i < MAX_CONNECTION; i++)
    {

		  //  Jeśli połączenie jest gotowe do odbioru danych
        if (conn[i].status == CONNSTATE_RECEIVING)		// CONNSTATE_RECEIVING - Stan odbierający
        {
            FD_SET(conn[i].sock, &readfds); // dodaje połączenie/gniazdo do readfds
            if (conn[i].sock > maxfd)
                maxfd = conn[i].sock; // Jeśli to jest najnowsze połączenie to mówimy o tym maxfd by wiedział ile jest połączeń
        }
    }
*/
    
//---WH--- Tu jest odbior danych 
    for (int i = 0; i < MAX_CONNECTION; i++){	// Sprawdzamy czy UDP i TCP coś mają do przekazania 
			printf("Status połączenia [%d] = %d || %d \n",i,conn[i].status,FD_ISSET(conn[i].sock,&readfds));  	
	     	if(FD_ISSET(conn[i].sock,&readfds) )
	     	{
		     		switch(conn[i].type) {
	     			
	     			case 'U':
                       //---WH---    przenioslem do osobnej funkcji
                       Uconn_handle_read(i);
                       //---WH---     
                       // tak nie mozna wytpisywac zawartosci binarnej !!!!! bedzie segmentatniam fault
                       // napisalem kod w poradach 
			         	//printf("\033[20;40H Co ja dostałem niby: %s \n",conn[i].recv_buffer);
					  break;	     	
										     			
	     			case 'T':
                        Tconn_handle_read(i);
	     			break;
	     			
	     			
	     			case 'C':
	     			break;
	     			
	     			}    
	 }
	}







//---WH---
//	printf("\033[1;0H");
//---WH---
// a tu jest drugie odbieranie danych - to sie robi bardzo nieczytelne kodu odbierajacego dane !!!!!!!!    
    // Lecimy po wszystkich połączeniach i sie nimi zajmujemy
    for (int i = 0; i < MAX_CONNECTION; i++)
    {
        if (conn[i].status == CONNSTATE_RECEIVING && FD_ISSET(conn[i].sock, &readfds)) // KLIENT "WYSYLA" 
        {
        	
            unsigned char c; // Zmienna do odbierania  //---WH--- to powodzenia zycze przy takim odbieraniu poloczen U
            int retval = recv(conn[i].sock, conn[i].recv_buffer, sizeof(conn[i].recv_buffer), 0); // Odbieranie od połączenia wiadomości 1 znaku do zmiennej 'c'|| 0-flaga            
            
            if ( retval > 0)
            {
                 //---WH--- a gdzie jest 0 na koncu odebranych danych nie mozna takl wypisywac
                 //---WH--- dodatem
                 if (retval<sizeof(conn[i].recv_buffer)){
                     retval,conn[i].recv_buffer[retval]='\0';
                 	 printf("Recv: %d  Wiadomosc: %s \n", retval,conn[i].recv_buffer); // DEBUG - DEBUG - DEBUG - DEBUG - DEBUG - DEBUG   
                 }
            	 
                printf("Wartosć :");
                commands(i, conn[i].recv_buffer); // Funkcja || Pobiera indeks połączenia i bit wiadomości ze zmiennej c
                memset(&conn[i].recv_buffer, 0, sizeof(conn[i].recv_buffer));
            }
            else
                recv_zero(i);		// Funkcja || Pobiera indeks połączenia i bit wiadomości ze zmiennej c
        }
  
        
        
  

        else if (conn[i].status == CONNSTATE_CLOSING)   // No jak połączenie chce sie zamknąć to niech sie zamknie    	
        {																																						
        		printf("\nZamknieto polaczenie: %d", conn[i].full_id); // DEBUG - DEBUG - DEBUG - DEBUG - DEBUG - DEBUG  						
            close_conn(i);																																
        }																																						

    }


	
	
	
	
	
// ====================================================================
// DEKORACJA
// ====================================================================
	
    free_conn = 0, recv_conn = 0, send_conn = 0; // Zeruje wartości bo bez tego będą szły w nieskończoność
    
    
    for (int i = 0; i < MAX_CONNECTION; i++) // Zaczynam iterować po połączeniach
    {
        switch (conn[i].status) // jako wartość SWITCHA to stan połączenia
        {
            case CONNSTATE_IDLE: 		free_conn++; 	break;       // Jeśli jest wolne to dodaje +1
            case CONNSTATE_RECEIVING:	recv_conn++; 	break; 		 // Jeśli jest odbierajace to dodaje +1
            case CONNSTATE_SENDING: 	send_conn++; 	break;   	 // Jeśli jest wysyłajace to dodaje +1
        }
//---WH---
       // printf("\033[%d;15H		\nNazwa uzytkownika: %s \033[35Gid: %d %d \033[45G Full_id: %d   \033[65GTyp: %c\n",i+1,conn[i].name,conn[i].id_client,conn[i].id_sub_client,conn[i].full_id,conn[i].type); 
        printf("%2d Nazwa: %s id: %d %d  Full_id:%d  Typ:%c\n",i+1,conn[i].name,conn[i].id_client,conn[i].id_sub_client,conn[i].full_id,conn[i].type); 

    }

    static int loopnr = 0;  // Ile razy sie cały pogram wykonał
    
    // Dekoracja

    //---WH---
    ///printf("\033[0;0H");
     printf("connections: sending: %d  receiving: %d   free: %d     %d \r", send_conn, recv_conn, free_conn, loopnr++);

	     
    //---WH--- no po to w�asnie jest select czy poll zeby nie robic takich rzeczy ale na razi moze zostac
    usleep(1000000); // Nie wykonuje sie z prędkością procesora tylko co 5 sekund chyba albo 0.5 albo 0.05													// usleep(1000000) = 1 sekunda
    
}
}


