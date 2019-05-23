#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include "server.h"

//hozzárendeli a pthread-hez a klienst és a socketadress-t
struct thread_args {
    int *client;
    struct sockaddr_in *addr;
    int *id;
};

int sockets[MAX];
int table[3][8];

int left[MAX];

int fd;

//strcmp-hez hasonló függvény, két stringet összehasonlít
int valid(char msg[], char * to){
    int i = 0;
    for(i = 0; i < strlen(to); i++)
        if(msg[i] != to[i])
            return 1;
    return 0;
}

//Ctrl+C kezelés
void closeHandler(int dummy) {
    int i, max = MAX;
    for(i = 0; i < max; i++)
        if(sockets[i] > -1)
            close(sockets[i]);
    if(fd && fd > -1)
        shutdown(fd, SHUT_RDWR); // bezárja a kapcsolatokat
    printf("A szerver bezárva!\n");
    exit(1);
}

int main(int argc , char *argv[]) {
	int fd_client, c, *new_sock, on = 1, max = MAX, i, id, *new_id;
	struct sockaddr_in server, client, *new_addr;
	
    for(i = 0; i < max; i++) {
        sockets[i] = -1;
        left[i] = DOLLS;
    }

    signal(SIGINT, closeHandler);

//fd - file descriptor, fájl leíró, visszatér annak értékével
	fd = socket(AF_INET , SOCK_STREAM , 0);
	if (fd == -1){
		printf("create fail");
        exit(1);
	}
	
    //opció beállító, újrahasználható a cím, és életben tartja
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)on, sizeof on);
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)on, sizeof on);
// AF_INET-et használ, ami IPv4; az összes címhez hozzárendeli a bindet
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT); //a számítógép könnyebben olvassa a nyelvet jobbról balra
// bind: a socketet rendeli hozzá egy hálózati címhez
	if( bind(fd,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("bind fail");
		return 1;
	}
//listen: kapcsolat elfogadási szándék, egyszerre 10 ember tud kapcsolódni, még nincs feldolgozva
	listen(fd , 10);
	
	printf("Kapcsolatok varasa...\n");
	c = sizeof(struct sockaddr_in);
/*accept: ez dolgozza fel és fogadja el a kapcsolatokat; a kliens connect függvénye után jön az accept
  a kliensbeli kapcsolatokat fogadja el*/
	while((fd_client = accept(fd, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{ //global.h-ban 2 kliens van megadva, az alábbi felel azért, hogy két csatlakozásnál többet nem fogad el
        id = 0;
        while(sockets[id] != -1 && id < max)
            id++;
        if (id >= max) {
            printf("Kapcsolat elutasitva: %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            close(fd_client);
            continue;
        }

        sockets[id] = fd_client;

		printf("Kapcsolat fogadva: %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		//96-102 sorig: leklónozza, megcsinálja a thread argumentumait, új memóriacímre helyezi őket
        struct thread_args args;
		pthread_t sniffer_thread;
		new_sock = (int*)malloc(sizeof(int));
		*new_sock = fd_client;
        new_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        *new_addr = client;
        new_id = (int*)malloc(sizeof(int));
        *new_id = id;

        //majd beállítja az értékeit.
        args.client = new_sock;
        args.addr = new_addr;
        args.id = new_id;

		if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*)&args) < 0)
		{
			perror("could not create thread");
			return 1;
		}
	}
	
	if (fd_client < 0)
	{
		perror("accept failed");
		return 1;
	}

    close(fd);
	
	return 0;
}
//mindig pont annyi csomagméretet fog elküldeni, amennyit megadtunk neki, ezzel megakadályozzuk
//a "túlcsordulást" (vagy szegmentációs hibát).
//csinálunk egy külön függvényt a send-re
void sendTo(int sock, char * msg, int len){
    char newt[len];
    int i = 0;
    do {
        newt[i] = msg[i];
    } while(msg[i++]);
    send(sock, newt, len, 0);
}
//malomcsekk
int check() {
    int i, j, c;
    for (c = 1; c < 3; c++){
        for (i = 0; i < 3; i++) {
            // Felső 3
            if ((table[i][0] == c && table[i][1] == c && table[i][2] == c)
                || (table[i][0] == c + 2 && table[i][1] == c && table[i][2] == c)
                || (table[i][0] == c && table[i][1] == c + 2 && table[i][2] == c)
                || (table[i][0] == c && table[i][1] == c && table[i][2] == c + 2)
                || (table[i][0] == c + 2 && table[i][1] == c + 2 && table[i][2] == c)
                || (table[i][0] == c && table[i][1] == c + 2 && table[i][2] == c + 2)
                || (table[i][0] == c + 2 && table[i][1] == c && table[i][2] == c + 2)) {
                table[i][0] = c + 2;
                table[i][1] = c + 2;
                table[i][2] = c + 2;
                return c;
            }

            // Alsó 3
            if ((table[i][4] == c && table[i][5] == c && table[i][6] == c)
                || (table[i][4] == c + 2 && table[i][5] == c && table[i][6] == c)
                || (table[i][4] == c && table[i][5] == c + 2 && table[i][6] == c)
                || (table[i][4] == c && table[i][5] == c && table[i][6] == c + 2)
                || (table[i][4] == c + 2 && table[i][5] == c + 2 && table[i][6] == c)
                || (table[i][4] == c && table[i][5] == c + 2&& table[i][6] == c + 2)
                || (table[i][4] == c + 2 && table[i][5] == c && table[i][6] == c + 2)) {
                table[i][4] = c + 2;
                table[i][5] = c + 2;
                table[i][6] = c + 2;
                return c;
            }

            // Bal oldal
            if ((table[i][0] == c && table[i][7] == c && table[i][6] == c)
                || (table[i][0] == c + 2 && table[i][7] == c && table[i][6] == c)
                || (table[i][0] == c && table[i][7] == c + 2 && table[i][6] == c)
                || (table[i][0] == c && table[i][7] == c && table[i][6] == c + 2)
                || (table[i][0] == c + 2 && table[i][7] == c + 2 && table[i][6] == c)
                || (table[i][0] == c && table[i][7] == c + 2 && table[i][6] == c + 2)
                || (table[i][0] == c + 2 && table[i][7] == c && table[i][6] == c + 2)) {
                table[i][0] = c + 2;
                table[i][7] = c + 2;
                table[i][6] = c + 2;
                return c;
            }

            // Jobb oldal
            if ((table[i][2] == c && table[i][3] == c && table[i][4] == c)
                || (table[i][2] == c + 2 && table[i][3] == c && table[i][4] == c)
                || (table[i][2] == c && table[i][3] == c + 2 && table[i][4] == c)
                || (table[i][2] == c && table[i][3] == c && table[i][4] == c + 2)
                || (table[i][2] == c + 2 && table[i][3] == c + 2 && table[i][4] == c)
                || (table[i][2] == c && table[i][3] == c + 2 && table[i][4] == c + 2)
                || (table[i][2] == c + 2 && table[i][3] == c && table[i][4] == c + 2)) {
                table[i][2] = c + 2;
                table[i][3] = c + 2;
                table[i][4] = c + 2;
                return c;
            }
        }

        //Felső közép
        if ((table[0][1] == c && table[1][1] == c && table[2][1] == c)
            || (table[0][1] == c + 2 && table[1][1] == c && table[2][1] == c)
            || (table[0][1] == c && table[1][1] == c + 2 && table[2][1] == c)
            || (table[0][1] == c && table[1][1] == c && table[2][1] == c + 2)
            || (table[0][1] == c + 2 && table[1][1] == c + 2 && table[2][1] == c)
            || (table[0][1] == c && table[1][1] == c + 2 && table[2][1] == c + 2)
            || (table[0][1] == c + 2 && table[1][1] == c && table[2][1] == c + 2)) {
            table[0][1] = c + 2;
            table[1][1] = c + 2;
            table[2][1] = c + 2;
            return c;
        }

        //Alsó közép
        if ((table[0][5] == c && table[1][5] == c && table[2][5] == c)
            || (table[0][5] == c + 2 && table[1][5] == c && table[2][5] == c)
            || (table[0][5] == c && table[1][5] == c + 2 && table[2][5] == c)
            || (table[0][5] == c && table[1][5] == c && table[2][5] == c + 2)
            || (table[0][5] == c + 2 && table[1][5] == c + 2 && table[2][5] == c)
            || (table[0][5] == c && table[1][5] == c + 2 && table[2][5] == c + 2)
            || (table[0][5] == c + 2 && table[1][5] == c && table[2][5] == c + 2)) {
            table[0][5] = c + 2;
            table[1][5] = c + 2;
            table[2][5] = c + 2;
            return c;
        }
    }

    return 0;
}
//ha háromnál kevesebb bábuja van az ellenfélnek akkor nyert a játékos.
void win(int id) {
    int i, j, f1 = 0, f2 = 0;
    char oneS[4];
    for (i = 0; i < 3; i++)
        for (j = 0; j < 8; j++)
            if (table[i][j] == 1 || table[i][j] == 3)
                f1++;
            else if (table[i][j] == 2 || table[i][j] == 4)
                f2++;

    if (f1 < 3) {
        sendTo(sockets[1], ":win", 32);
        sendTo(sockets[0], ":lose", 32);
//close bezárja a kapcsolatokat
        for (i = 0; i < MAX; i++)
            if (sockets[i] > -1)
                close(sockets[i]);
		close(fd);
		exit(1);
    } else if (f2 < 3) {
        sendTo(sockets[0], ":win", 32);
        sendTo(sockets[1], ":lose", 32);

        for (i = 0; i < MAX; i++)
            if (sockets[i] > -1)
                close(sockets[i]);
		close(fd);
		exit(1);
    } else {
        sendTo(sockets[id == 0 ? 1 : 0], ":startmove", 32);
        snprintf(oneS, sizeof(oneS), "%d", id);
        sendTo(sockets[id == 0 ? 1 : 0], oneS, 4);
    }
}
// annyiszor van létrehozva, ahány kliens csatlakozik.

void *connection_handler(void *arguments)
{
    int read_size, i, j;
    struct thread_args *args = (struct thread_args *) arguments;
	int sock = *(int*)args->client;
    int id = *(int*)args->id; // a kliens egyedileg definiált ID-je
    struct sockaddr_in addr = *(args->addr);
	char client_message[32], wordS[1024], oneS[4];
//sock: a jelenleg használt kliens fájlleírója
    if (id == 0) {
        sendTo(sock, ":waitforother", 32);
        printf("Egy játékos felcsatlakozott.\n"); // Ha fellépett az első, várjon a másikra
    } else {
        sendTo(sockets[0], ":update", 32);
        sendTo(sockets[1], ":update", 32);
        sendTo(sockets[0], ":getpos", 32);
        snprintf(oneS, sizeof(oneS), "%d", left[0]);
        sendTo(sockets[0], oneS, 4);
    }
//recv: annyit olvas be, amennyit beállítunk csomagméretnek
	while ((read_size = recv(sock, client_message, 32, 0)) > 0) {
        if (valid(client_message, ":sendwords") == 0){
            bzero(wordS, sizeof(wordS));
            recv(sock, wordS, 1024, 0);
        } else if (valid(client_message, ":place") == 0 || valid(client_message, ":move") == 0){
            recv(sock, oneS, 4, 0);
            int x = atoi(oneS);
            recv(sock, oneS, 4, 0);
            int y = atoi(oneS);

            if (valid(client_message, ":place") == 0) {
                printf("Place %d - %d\n", x, y);
                
                sendTo(sockets[0], ":set", 32);
                sendTo(sockets[1], ":set", 32);
            //elküldi mindkét kliensnek az x,
                snprintf(oneS, sizeof(oneS), "%d", x);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);
            //y koordinátákat,
                snprintf(oneS, sizeof(oneS), "%d", y);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);
            //és hogy kié a bábu.
                snprintf(oneS, sizeof(oneS), "%d", id + 1);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);
            //szerveroldalon beállítja a táblát.
                table[x][y] = id + 1;
            //elvesz egy bábuta globális 9-ből:
                left[id]--;
            } else {
                //rx, ry: mozgatja az x,y koordinátán lévő bábut az rx,ry koordinátákra, majd a régi helyüket lenullázza.
                recv(sock, oneS, 4, 0);
                int rx = atoi(oneS);
                recv(sock, oneS, 4, 0);
                int ry = atoi(oneS);

                table[rx][ry] = table[x][y];
                table[x][y] = 0;

                sendTo(sockets[0], ":set", 32);
                sendTo(sockets[1], ":set", 32);

                snprintf(oneS, sizeof(oneS), "%d", x);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);

                snprintf(oneS, sizeof(oneS), "%d", y);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);

                snprintf(oneS, sizeof(oneS), "%d", 0);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);

                sendTo(sockets[0], ":set", 32);
                sendTo(sockets[1], ":set", 32);

                snprintf(oneS, sizeof(oneS), "%d", rx);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);

                snprintf(oneS, sizeof(oneS), "%d", ry);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);

                snprintf(oneS, sizeof(oneS), "%d", table[rx][ry]);
                sendTo(sockets[0], oneS, 4);
                sendTo(sockets[1], oneS, 4);
            }
            //van-e malom?
            int c = check() - 1;
            int f1 = 0, f2 = 0;
            //ha nincs malom,
            if (c == -1) {
                //ha van a másik kliensnek lerakható bábuja, bekéri az új pozíciót.
                if (left[id == 0 ? 1 : 0] > 0){
                    sendTo(sockets[id == 0 ? 1 : 0], ":getpos", 32);
                    snprintf(oneS, sizeof(oneS), "%d", left[id == 0 ? 1 : 0]);
                    sendTo(sockets[id == 0 ? 1 : 0], oneS, 4);
                } 
                //megnézzük a sajátjainkat is:
                    else if (left[id] > 0) {
                    sendTo(sockets[id], ":getpos", 32);
                    snprintf(oneS, sizeof(oneS), "%d", left[id]);
                    sendTo(sockets[id], oneS, 4);
                } 
                //ha senkinek nincs bábuja:
                else {
                    win(id);
                }
            } else 
            //ha malom van:
            {
                sendTo(sockets[c == 0 ? 1 : 0], ":new", 32); // tudatjuk az ellenféllel, hogy malmunk van,
                sendTo(sockets[c], ":you", 32); // majd saját magunkkal is.

                sendTo(sockets[c], ":getrem", 32); // bekér egy koordinátát, melyik bábut szeretnénk levenni.
                snprintf(oneS, sizeof(oneS), "%d", id == 0 ? 1 : 0);
                sendTo(sockets[c], oneS, 4);
            }
        } 
        //az elvétel ugyanúgy működik mint a place, annyi különbséggel, hogy lenullázza.
        else if (valid(client_message, ":remove") == 0){
            recv(sock, oneS, 4, 0);
            int x = atoi(oneS);
            recv(sock, oneS, 4, 0);
            int y = atoi(oneS);

            printf("Remove %d - %d\n", x, y);
            
            sendTo(sockets[0], ":set", 32);
            sendTo(sockets[1], ":set", 32);

            snprintf(oneS, sizeof(oneS), "%d", x);
            sendTo(sockets[0], oneS, 4);
            sendTo(sockets[1], oneS, 4);

            snprintf(oneS, sizeof(oneS), "%d", y);
            sendTo(sockets[0], oneS, 4);
            sendTo(sockets[1], oneS, 4);

            snprintf(oneS, sizeof(oneS), "%d", 0);
            sendTo(sockets[0], oneS, 4);
            sendTo(sockets[1], oneS, 4);

            table[x][y] = 0;

            if (left[id == 0 ? 1 : 0] > 0){
                sendTo(sockets[id == 0 ? 1 : 0], ":getpos", 32);
                snprintf(oneS, sizeof(oneS), "%d", left[id == 0 ? 1 : 0]);
                sendTo(sockets[id == 0 ? 1 : 0], oneS, 4);
            } else if (left[id] > 0) {
                sendTo(sockets[id], ":getpos", 32);
                snprintf(oneS, sizeof(oneS), "%d", left[id]);
                sendTo(sockets[id], oneS, 4);
            } else {
                win(id);
            }
		} else if (valid(client_message, ":surrender") == 0){
			sendTo(sockets[id == 0 ? 1 : 0], ":surrwin", 32);
		    for (i = 0; i < MAX; i++)
		        if (sockets[i] > -1)
		            close(sockets[i]);
			close(fd);
			exit(1);
        } else {
		    write(sock, client_message, read_size); //Ismeretlen üzenet, kiírja a konzolba amit nem tudott feldolgozni pl. :apple
        }
        //a tömböt lenullázza(feltölti üres értékekkel)
		bzero(client_message, sizeof(client_message));
	}
	//inet_ntoa: char array-é alakítja az "addr.sin_addr"-t; ntohs: portot visszaalakítja emberi értelmezésre
	if (read_size == 0) {
        printf("Kapcsolat bezarult: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        sockets[id] = -1;
	} else if(read_size == -1) {
		perror("recv failed");
	}

	return 0;
}
