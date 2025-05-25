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

unsigned char wiersz[8][960] = {
     {"POD TAKA SEKWENCJA\r\rPod tak� sentencj�\rK�ad� me nazwisko:\rNic nie kosztuje u�miech,\rA p�aci za wszystko !\r\r"},
     {"CZASEM GLUPOTY ...\r\rCzasem g�upoty cz�owiek sobie �yczy,\rBowiem w m�dro�ci tyle jest goryczy.\r\r"},
     {"PROSTE SLOWO\r\rS�owa kunsztowne, s�owa pi�kne bledn�\rwobec prostego, kt�re trafia w sedno.\r\r"},
     {"WIECZNA MLODOSC\r\rJedni s� wiecznie m�odzi,\rDrudzy wiecznie starzy-\rTo kwestia charakteru,\rA nie kalendarzy.\r\r"},
     {"ZYCIE\r\rZanim si� opami�tasz,\rKo�yska, o�tarz, cmentarz.\rI robisz smutne odkrycie,\r�e to ju� ca�e �ycie.\r\r"},
     {"W MILOSCI\r\rW mi�o�ci ceni�\rNiedo�wiadczenie.\rMi�o�� nie jest kunsztem,\rMi�o�� jest wzruszeniem.\r\r"},
     {"ZGODA BUDUJE\r\rGdy dwoje robi to samo,\rPotem trzecie wo�a: Mamo! \r\r"},
     {"CNOTY I GRZESZKI\r\rTak jak �upina pokrywa orzeszki,\rTak wielkie cnoty kryj� ma�e grzeszki.\r\r"},
     };
// =========================================================================

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


//===========================================================================
// informacje o polaczeniach  - Struktura połączeń
//===========================================================================

struct conninfo{
   int status;   
   int sock;
   unsigned char name[NAME_SIZE+1];
   unsigned int name_pos;
   int stepcnt;
   unsigned char send_buffer[1000];
   unsigned to_send;
   unsigned char *send_ptr;
};


//================================================================
// Ilość połączeń 
//===========================================================================
#define MAX_CONNECTION 10
struct conninfo conn[MAX_CONNECTION];    // Tablica klientów - 10 miejsc na klientów gdzie każde miejsce mówi pewne informacje
													  // jak naprzykład czy dane połączenie/klient czeka na dane czy chce je wysłać










// FUNKCJE I INNE TAKIE


//===========================================================================
// dodanie nowego polaczenia
//===========================================================================
void add_new_conn(int fd) // fd - deksryptor czyli gniazdo
{
    for (int i=0; i<MAX_CONNECTION; i++)
        if (conn[i].status == CONNSTATE_IDLE){
            bzero(&conn[i],sizeof(conn[i]));
            conn[i].sock = fd;
            conn[i].status = CONNSTATE_RECEIVING;
            conn[i].name_pos = 0;            
            break;
        }
}

//===========================================================================
// zamkniecie polaczenia
//===========================================================================
void close_conn(int i)
{
   close( conn[i].sock );
   bzero(&conn[i],sizeof(conn[i]));
   conn[i].status = CONNSTATE_IDLE;
}


//===========================================================================
// usuniecie polskich znakow
//===========================================================================
void korekta_znakow(char* s)
{
   int i, len = strlen(s);
   char *c = s;
   
   if (len>0)
      for (i=0; i<len; i++)
      {
         switch (*c)
         {
                    case -71: *c='a'; break;
                    case -22: *c='e'; break;
                    case -13: *c='o'; break;
                    case -65:
                    case -97: *c='z';
                        break;
                    case -26: *c='c'; break;
                    case -15: *c='n'; break;
                    case -77: *c='l'; break;
                    case -100:*c='s'; break;
                    case -81: *c='Z'; break;
                    case -116:*c='S'; break;
        }
        c++;
    }
}



//===========================================================================
// odebrano 1 znak  i - indeks polaczenia  c-odebrany znak
//===========================================================================
void recv_char(int i, unsigned char c)
{
   //printf("recv-name_pos: %d\n", conn[i].name_pos);
   if (conn[i].name_pos<NAME_SIZE){
      conn[i].name[conn[i].name_pos]=c;
      conn[i].name_pos++;
      conn[i].stepcnt=0;
   }
}


//===========================================================================
// nie odebrano �adnego znaku   - funkcja wykonywana gdy recv zwroicil 0
// i - indeks polaczenia
//===========================================================================
void recv_zero(int i)
{
    unsigned sum = 0;
    if (conn[i].name_pos>0){   // wczesniej cos juz odebrano
        //printf("r0-name_pos: %d  %s\n", conn[i].name_pos,(char*)conn[i].name);
        conn[i].stepcnt++;
        if (conn[i].stepcnt>4){
            if (conn[i].name_pos>0){
                conn[i].status = CONNSTATE_SENDING;
                for (int n=0; n<conn[i].name_pos; n++)
                   sum += conn[i].name[n];
                sum = sum & 7;
                sprintf(conn[i].send_buffer,"Fraszka dla %s  \r\r %s",conn[i].name, wiersz[sum]);
                korekta_znakow(conn[i].send_buffer);
                conn[i].to_send = strlen(conn[i].send_buffer);
                conn[i].send_ptr = (unsigned char*)&conn[i].send_buffer;
                conn[i].stepcnt=0;
            }
        }
    }
}



//===========================================================================
// wyslano  1 znak, - reakcja gdy send zwrocil 1
//===========================================================================
void send_char(int i)
{
  if (conn[i].to_send>0)
  {
      conn[i].to_send--;
      conn[i].send_ptr++;
  }
  else
  {
      conn[i].status = CONNSTATE_CLOSING;
  }
}







//===========================================================================
// glowna petla
//===========================================================================
int main(int argc, char *argv[])
{
	fd_set readfds, writefds; // fd_set - to struktura || Zmienne read i write(fds) są tworzone na podstawie 
	int maxfd;					  // maxfd - mówi ile jest Klientów obecnie.




	 // Czyszcimy dane Servera (gdyby jakieś były) oraz czyścimy tablice klientów (gdyby były by zapisani klienci widmo)  
    bzero((char*) &serwer,sizeof(serwer));
    bzero((char*) &conn,sizeof(conn));


    
    
	 // Sprawdzamy argumenty które podano z lini komend   || Obsługa błędów 
    if (argc<2){
        perror(" nie podano portu\n");
        exit(19);
    }
    
    
    port_number = atoi(argv[1]);    // Numer portu konwertowany z STRING na INT  (ASCII to INT)
    serwer.sin_family = AF_INET,		// Typ IP
    serwer.sin_port = htons(port_number);		// Numer portu konewrtowany z INT na typ "SIECIOWY" (HOST to NETWORK short)
    
    
        
	 int retval = inet_pton( AF_INET, SERWER_IP, & serwer.sin_addr );    // Konwertuje IP na BINARNY
    
    if( retval <= 0 ) // Obsługa błędów 
    {
        perror( "inet_pton() ERROR" );
        exit( 1 );
    }


   
	 int listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // Tworze Socket 
	 																		// AF_INET - Typ IPv4
	 																		// SOCK_STREAM - Typ | Dla TCP (za bardzo techniczne)
	 																		// SOCK_NONBLOCK - Flaga | Nieblokujący 
	 																		// 0 - Protokół | Niech system sam se wybierze co pasuje
    if(( listen_sock ) < 0 ) 			// Obługa błędów
    {
        perror( "socket() ERROR" );
        exit( 2 );
    }
   
   
   
    socklen_t len = sizeof( serwer ); 	// Długość adresu SERVERA
    
	 retval = bind( listen_sock,(struct sockaddr *) &serwer, len ); // PRzypisuje gniazdo Servera do portu i adresu    
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

 
   
    while (1)
{
FD_ZERO(&readfds);		// Zerowanie zbioru dla gniazd do odczytu
FD_ZERO(&writefds);		// Zerowanie zbioru dla gniazd do zapisu
maxfd = listen_sock;		// Ustawienie maxfd na gniazdo nasłuchujące (Ta zmienna w skrócie mówi ile jest obecnie połączeń)


    // Dodaj gniazdo nasłuchujące do zbioru read
    FD_SET(listen_sock, &readfds);					// Dodaje gniazdo nasłuchujące do zbioru readfds bo select będzie sprawdzał
    															// właśnie w tym zbiorze a dokłądniej na tym listen_sock czy ktoś sie nie 
    															// chce dobić do nas
// ====================================================================
// Segregacja połączeń
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
        
        
		  //  Jeśli połączenie jest gotowe do wysłania danych        
        else if (conn[i].status == CONNSTATE_SENDING)   // CONNSTATE_SENDING - Stan wysyłający
        {
            FD_SET(conn[i].sock, &writefds);	// dodajemy połączenie/gniazdo do writefds 
            if (conn[i].sock > maxfd)
                maxfd = conn[i].sock;	// podobnie jak wyżej
        }
    }
// ====================================================================
// ====================================================================

	 // Ustawienie timeout'u by select nie czekał wieczność
    struct timeval timeout;
    timeout.tv_sec = 0;			// 0s
    timeout.tv_usec = 100000; // 100ms timeout


	 
    int activity = select(maxfd + 1, &readfds, &writefds, NULL, &timeout);  //Serce programu
																		// maxfd + 1 - Sprawdzamy do Najwiekszego deskryptorora + 1 bo to nie >=
																		// Zbiór gniazd do odczytywania daynych
																		// Zbiór gniazd do wysyłania danych

    if (activity < 0)    // Obsługa błędów
    {
        perror("select() ERROR");
        continue;
    }

    // Sprawdz czy jest nowe połączenie
    if (FD_ISSET(listen_sock, &readfds))
    {
    	  // Akceptuje połączenie jeśli jakieś jest
        int newsock = accept(listen_sock, NULL, NULL);
        if (newsock >= 0)
        {
        		// Dodaje nowe połączenie
            add_new_conn(newsock);
        }
    }




    // Lecimy po wszystkich połączeniach i sie nimi zajmujemy
    for (int i = 0; i < MAX_CONNECTION; i++)
    {
    	// Jeśli połączenie jest gotowe do wysłania danych
        if (conn[i].status == CONNSTATE_RECEIVING && FD_ISSET(conn[i].sock, &readfds))
        {
        	
            unsigned char c; // Zmienna do odbierania
            int retval = recv(conn[i].sock, &c, 1, 0); // Odbieranie od połączenia wiadomości 1 znaku do zmiennej 'c'|| 0-flaga
            
            if (retval > 0)
                recv_char(i, c); // Funkcja || Pobiera indeks połączenia i bit wiadomości ze zmiennej c
            else
                recv_zero(i);		// Funkcja || Pobiera indeks połączenia i bit wiadomości ze zmiennej c
        }
        
        
	    	// Jeśli połączenie jest gotowe do odbierania danych        
        else if (conn[i].status == CONNSTATE_SENDING && FD_ISSET(conn[i].sock, &writefds))
        {
            int retval = send(conn[i].sock, conn[i].send_ptr, 1, MSG_NOSIGNAL);  // Wysyłąnie danych
            
            
            if (retval > 0) // to co wcześniej
                send_char(i);
            else
                close_conn(i);
        }



        else if (conn[i].status == CONNSTATE_CLOSING)   // No jak połączenie chce sie zamknąć to niech sie zamknie
        {
            close_conn(i);
        }
    }


	
	 // Dekoracja	
	
    int free_conn = 0, recv_conn = 0, send_conn = 0; // Deklaruje zmienne któ©e będą pokazywać ile jest danych połączeń
    
    
    for (int i = 0; i < MAX_CONNECTION; i++) // Zaczynam iterować po połączeniach
    {
        switch (conn[i].status) // jako wartość SWITCHA to stan połączenia
        {
            case CONNSTATE_IDLE: free_conn++; break;       // Jeśli jest wolne to dodaje +1
            case CONNSTATE_RECEIVING: recv_conn++; break;  // Jeśli jest wolne to dodaje +1
            case CONNSTATE_SENDING: send_conn++; break;    // Jeśli jest wolne to dodaje +1
        }
    }

    static int loopnr = 0;  // Ile razy sie cały pogram wykonał
    
    // Dekoracja
    printf("connections: sending: %d  receiving: %d   free: %d     %d \r", send_conn, recv_conn, free_conn, loopnr++);
    usleep(5000); // Nie wykonuje sie z prędkością procesora tylko co 5 sekund chyba albo 0.5 albo 0.05
}

   
    shutdown( listen_sock, SHUT_RDWR );   // Kończymy impreze
}


//===========================================================================
/*
Dalej są funkcje i inne rzeczy które sie używa w tym kodzie od hehehna 
ale ja niedość że mi sie na chwi;le obecną nie chce myśleć o tym jak to opisać ani nic 
to ja to pierdziele. NIe odpowiadam za ból głowy który zostanie wywołany poniższymi środkami memetycznymi

Powodzenia

*/
//===========================================================================






