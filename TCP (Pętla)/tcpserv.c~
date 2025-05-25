/*
In short jak działą server:

1.	Nasłuchuje na porcie,

2.	Przyjmuje połączenie od jednego klienta,

4.	Odbiera wiadomość,

4.	Odpowiada,

5.	Kończy.



*/
// ===============================
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


int main()
{
	 int PORT = 12345;
	 int retval;												// Zmienna która przechowywuje artości funkcji send,recv i connnect
    int server_sock, client_sock;						// Tworze zmienne sock dla KLIENTA i SERVERA 
    struct sockaddr_in server_addr, client_addr;	// Tworze structure dla adresu SERVERA i KLIENTA
    socklen_t client_len = sizeof(client_addr);		// Tworzenie zmiennej ze specjalnym typem na przechowywanie "wielkosci" Adresu KLIENTA 
    char buffer[1024];										// Tworzenie buforu który będzie przechowywał informacje od klienta


	// Tworzenie gniazda
    server_sock = socket(AF_INET, SOCK_STREAM, 0); // Tworzenie zmiennej dla gniazada SERVERA
																				// AF_INET = Wyznacza typ adresu IP (IPv4)
																				// SOCK_STREAM oraz 0 = Jakieś parametry które można o kant dupy roztłuc

	
    if (server_sock < 0) { //obsługa błędu  
        perror("socket() error");
        exit(1);
    }

	// Dodawanie adresu SERVERA
    server_addr.sin_family = AF_INET;				// Ustawiam typ adresu IP (IPv4)
    server_addr.sin_port = htons(PORT);  		// port, na którym nasłuchujemy
    server_addr.sin_addr.s_addr = INADDR_ANY; 	// akceptuj połączenia na każdym interfejsie || Idk tak mi czat wypluł

		
	 // Łącze gniazdo z adresem i portem servera
	 retval = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	 															// server_sock = gniazdo SERVERA
	 															// (struct sockaddr*) &server_addr = adres SERVERA
	 															// sizeof(server_addr) = "Wielkosc" adresu SERVERA
	
	
    if (retval < 0) {    	// Obsługa błędu
        perror("bind() error");
        close(server_sock);
        exit(2);
    }


		retval = listen(server_sock, 5);		// Zaczyna słuchać na gnieżdzie i dopuszcza 5 innych połączeń w kolejnce

    if (retval < 0) {
        perror("listen() error");
        close(server_sock);
        exit(3);
    }
    
	// Pętla while która sprawia że server sie nie wyłącza po jednym kliencie 
	while(1)
	{
    
    
	 client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len); // Akceptuje połączenie 
    
    
    
    if (client_sock < 0) {
        perror("accept() error");
        close(server_sock);
        exit(4);
    }    
    
	
	struct timeval timeout;
	timeout.tv_sec = 10;  // 10 sekund
	timeout.tv_usec = 0;	// 0 nanosekund czy pikosekund nie wiem

	setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)); // Wyrzuca klienta jeśli nic nie robi po 
																								// parunastu sekundach.
	// Btw teraz ogarnołem że to nic nie robi bo klient się łączy i od 	// Dodatkowo lepiej sprawdzić po wykładach jak jest 
	//razu wysya wiadomość więc nawet nie ma jak tego przetestować    	// zrobiony ten kod bo kinda sus









    
    memset(buffer, 0, sizeof(buffer)); // Czyszcze wiadomości czyl buffer
    
    
	 retval = recv(client_sock, buffer, sizeof(buffer) - 1, 0);   // Odbieram wiadomości
																// client_sock = gniazdo KLIENTA	 
	 															// buffer = miejsce na wiadomosc
	 
	  
    if (retval < 0) {
        perror("recv() error");
        close(client_sock);
        close(server_sock);
        exit(5);
    }    
    
	
	printf("Odebrano wiadomość: %s\n", buffer); // Pokazuje wiadomość
	
	
	 const char* response = "Wiadomość odebrana."; // Wysyłam wiadomość potwierdzającą odebranie
	 
	 
	 
    retval = send(client_sock, response, strlen(response), 0); // Wysyłam wiadomość
		    											// to samo co z revcv()
    
    
    if (retval < 0) {
        perror("send() error");
        close(client_sock);
        close(server_sock);
        exit(6);
    }
    
    close(client_sock); // 🆕 zamyka połączenie z klientem (W PĘTLI WHILE)
	 }



    close(server_sock);
    printf("Połączenie zakończone.\n");


}