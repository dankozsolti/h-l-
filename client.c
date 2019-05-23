#include "global.h"

char lang[1];

int table[3][8];

int valid(char msg[], char * to){
    int i = 0;
    for(i = 0; i < strlen(to); i++)
        if(msg[i] != to[i])
            return 1;
    return 0;
}
//kiürítjük a konzolt
void clear(){
    printf("\e[1;1H\e[2J");
}
// \x1B[32m●\x1B[0m - zöld pötty
// \x1B[31m●\x1B[0m - piros pötty
char * format(int a) {
    return a == 0 ? "○" : ((a == 1 || a == 3) ? "\x1B[32m●\x1B[0m" : "\x1B[31m●\x1B[0m");
}

void printTable() {
    printf("%s ------------ %s ------------ %s\n", format(table[0][0]), format(table[0][1]), format(table[0][2]));
    printf("|              |              |\n");
    printf("|    %s ------- %s ------- %s    |\n", format(table[1][0]), format(table[1][1]), format(table[1][2]));
    printf("|    |         |         |    |\n");
    printf("|    |    %s -- %s -- %s    |    |\n", format(table[2][0]), format(table[2][1]), format(table[2][2]));
    printf("|    |    |         |    |    |\n");
    printf("%s -- %s -- %s         %s -- %s -- %s\n", format(table[0][7]), format(table[1][7]), format(table[2][7]), format(table[2][3]), format(table[1][3]), format(table[0][3]));
    printf("|    |    |         |    |    |\n");
    printf("|    |    %s -- %s -- %s    |    |\n", format(table[2][6]), format(table[2][5]), format(table[2][4]));
    printf("|    |         |         |    |\n");
    printf("|    %s ------- %s ------- %s    |\n", format(table[1][6]), format(table[1][5]), format(table[1][4]));
    printf("|              |              |\n");
    printf("%s ------------ %s ------------ %s\n", format(table[0][6]), format(table[0][5]), format(table[0][4]));
}

void update() {
    clear();
    printTable();
}

int main() {
    int on = 1;
    int ply;
    char buffer[32], pS[4];

    struct sockaddr_in server;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("Hiba a letrehozaskor!\n");
        exit(1);
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)on, sizeof on);
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)on, sizeof on);
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);
//a szervernek küldi. listen -> connect -> accept
    if (connect(fd, (struct sockaddr *)&server, sizeof server) < 0) {
        printf("Hiba a letrehozaskor!\n");
        exit(1);
    }
// a végtelen ciklus addig fut, amíg nem kap üres üzenetet, vagyis a buffer üres.
    while(1){
        recv(fd, buffer, 32, 0); // a csomag egy max. 32 karakterhosszúságú azonosító
        if (buffer[0] == '\0') break;
        
        if (valid(buffer, ":getpos") == 0 || valid(buffer, ":getrem") == 0 || valid(buffer, ":startmove") == 0) {
            recv(fd, pS, 4, 0);
            ply = atoi(pS);
            while(1) {
                if (valid(buffer, ":getpos") == 0)
                    printf("Még %d bábúd maradt!\n", ply);

                if (valid(buffer, ":startmove") == 0)
                    printf("Válassz ki egy bábút a mozgatáshoz.\n");
                    
                printf("Adj meg egy pozíciót (X Y): ");
            
                int x, rx, y, ry;
                scanf("%d %d", &x, &y);
                if (valid(buffer, ":getrem") == 0) {
                    if(table[x][y] == 0) {
                        printf("Itt nincs bábú.\n");
                        continue;
                    }
                    if (table[x][y] == ply || table[x][y] == ply + 2) {
                        printf("Ez a saját bábúd.\n");
                        continue;
                    }
                } else if (valid(buffer, ":startmove") == 0) {
                    if(table[x][y] == 0) {
                        printf("Itt nincs bábú.\n");
                        continue;
                    }
                    if (!(table[x][y] == ply || table[x][y] == ply + 2)) {
                        printf("Ez nem a saját bábúd.\n");
                        continue;
                    }
                } else {
                    if(table[x][y] > 0) {
                        printf("Ide már van lerakva bábú.\n");
                        continue;
                    }
                }

                if (valid(buffer, ":startmove") == 0) {
                    while (1) {
                        printf("Most add meg hova szeretnéd helyezni a bábút: ");
                        scanf("%d %d", &rx, &ry);
                        if(table[rx][ry] > 0) {
                            printf("Itt már van egy bábú.\n");
                            continue;
                        }
                        break;
                    }
                }

                send(fd, valid(buffer, ":getrem") == 0 ? ":remove" : (valid(buffer, ":startmove") == 0 ? ":move" : ":place"), 32, 0);
                snprintf(pS, sizeof(pS), "%d", x);
                send(fd, pS, 4, 0);
                snprintf(pS, sizeof(pS), "%d", y);
                send(fd, pS, 4, 0);

                if (valid(buffer, ":startmove") == 0) {
                    snprintf(pS, sizeof(pS), "%d", rx);
                    send(fd, pS, 4, 0);
                    snprintf(pS, sizeof(pS), "%d", ry);
                    send(fd, pS, 4, 0);
                }
                break;
            }
        } 
            //beállítja a bábut
            else if (valid(buffer, ":set") == 0) {
            recv(fd, pS, 4, 0);
            int x = atoi(pS);
            recv(fd, pS, 4, 0);
            int y = atoi(pS);
            recv(fd, pS, 4, 0);
            int c = atoi(pS);

            table[x][y] = c;

            update();
        } else if (valid(buffer, ":waitforother") == 0) {
            printf("Várakozás a másik félre...\n");
        } else if (valid(buffer, ":new") == 0) {
            printf("Az ellenfeled kirakott egy malmot.\nMost lehetősége van elvenni egy bábúdat.\n");
        } else if (valid(buffer, ":you") == 0) {
            printf("Kiraktál egy malmot!\nLehetőséged van elvenni ellenfeled egy bábúját.\n");
        } else if (valid(buffer, ":win") == 0) {
            printf("Megnyerted a játékot.\n");
        } else if (valid(buffer, ":lose") == 0) {
            printf("Elveszítetted sajnos a játékot.\n");
        } else if (valid(buffer, ":update") == 0) {
            update();
        } else {
            printf("Fogadva: %s\n", buffer);
        }

        bzero(buffer, sizeof(buffer));
    }
    printf("Kilep!\n");

    close(fd);

    return 0;
}