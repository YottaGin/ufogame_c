#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define setPlace(x, y); printf("\033[%d;%dH", (y), (x));
#define setBackColor(n); printf("\033[4%dm", (n));
#define setCharColor(n); printf("\033[3%dm", (n));
#define setType(n); printf("\033[%dm", (n));
#define setCharBold(); printf("\x1b[1m");
#define clearScreen(); printf("\033[2J");
#define cursolOn(); printf("\033[?25h");
#define cursolOff(); printf("\033[?25l");

// definition of COLOR used with setBackColor(n) or setCharColor(n)
#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7
#define DEFAULT 9

// definition of TYPE used with setType(n)
#define NORMAL 0
#define BLIGHT 1
#define DIM 2
#define UNDERBAR 4
#define BLINK 5
#define REVERSE 7
#define HIDE 8
#define STRIKE 9

#define XMAX 124
#define YMAX 31
#define UFO_HTS 5
#define UFO_WID 20

#define PLAY_NUM 8
#define CLEAR_NUM 9
#define WORD_HTS 6
#define WORD_WID 5

void printUFO(int x, int y, int color);
void clearUFO(int x, int y);
void printBeam(int x, int y);
void clearBeam(int x, int y);
void printHP(int hp);
void clearHP(int hp);

extern int errno;
struct termios otty, ntty;
int kbhit(void);
int getch(void);
static void onsignal(int sig);
int tinit(void);
void initialize(void);
void reset(void);

int playgame[PLAY_NUM][WORD_HTS][WORD_WID];
int gameclear[CLEAR_NUM][WORD_HTS][WORD_WID];
void printWord(int WORD_NUM, int[WORD_NUM][WORD_HTS][WORD_WID]);

typedef struct UFO {
    int color;
    int spd;
    float drptime;
} UFO;


int main(void) {
    UFO UpUFO[3];

    UpUFO[0].color = GREEN;
    UpUFO[0].spd = 2.0;
    UpUFO[0].drptime = 0.15;

    UpUFO[1].color = RED;
    UpUFO[1].spd = 1.0;
    UpUFO[1].drptime = 0.05;

    UpUFO[2].color = BLUE;
    UpUFO[2].spd = 3.0;
    UpUFO[2].drptime = 0.25;

    initialize();
    int choice;

    for (;;) {
        printWord(PLAY_NUM, playgame);
        setPlace(50, 13+WORD_HTS+3);
        setType(NORMAL);
        printf("PRESS 1 or 2 or 3");
        fflush(stdout);
        if (kbhit()) {
            choice = getch();
            if ((choice == 49)||(choice == 50)||(choice == 51)) break;
        } 
    }
    choice -= 49;
    clearScreen();

    int r, upx, downx, downy, preupx, beamx, beamy, keycode, hp, direction, mvcnt, sign;
    struct timespec start_time, now_time, pre_time, now_time2, pre_time2;
    float duration, duration2, drptime, Down_ufo_time;
    
    srand((unsigned)time(NULL));
    r = rand() % (XMAX - UFO_WID-1) + 1;
    drptime = UpUFO[choice].drptime;
    Down_ufo_time = 0.1;

    clock_gettime(CLOCK_REALTIME, &start_time);
    pre_time = start_time;
    pre_time2 = start_time;
    
    upx = XMAX/2;
    downx = r;
    downy = YMAX - 1 - UFO_HTS;

    preupx = upx;
    beamx = upx + UFO_WID/2 -1;
    beamy = UFO_HTS;

    mvcnt = 0;
    hp = 5;
    direction = 1;
    sign = 0;

    printUFO(upx, 1, UpUFO[choice].color);
    printUFO(downx, downy, DEFAULT);
    printHP(hp);
    

    for (;;) {

        clock_gettime(CLOCK_REALTIME, &now_time);
        duration = now_time.tv_sec - pre_time.tv_sec + (now_time.tv_nsec - pre_time.tv_nsec)/1000000000.0;

        clock_gettime(CLOCK_REALTIME, &now_time2);
        duration2 = now_time2.tv_sec - pre_time2.tv_sec + (now_time2.tv_nsec - pre_time2.tv_nsec)/1000000000.0;

        preupx = upx;

        if (kbhit()) {
            keycode = getch();
            if (keycode == 0x1b) {
                keycode = getch();
                if (keycode == 0x5b) {
                    keycode = getch();
                    switch (keycode) {
                        case 0x41: //up
                            break;
                        case 0x42: //down
                            if (sign == 0) {
                                beamx = upx + UFO_WID/2 -1;
                                sign++;
                            }
                            break;
                        case 0x43: //right
                            if (upx < XMAX - UFO_WID -2)
                                upx += UpUFO[choice].spd;
                            break;
                        case 0x44: //left
                            if (3 <= upx)
                                upx -= UpUFO[choice].spd;
                            break;

                    }
                }
            } else {
                reset();
                exit(0);
            }
        }

        if (preupx != upx) {
            clearUFO(preupx, 1);
            printUFO(upx, 1, UpUFO[choice].color);

        }
        
        if (duration > drptime && sign != 0) {
            pre_time = now_time;
            if (beamy > UFO_HTS) {
                clearBeam(beamx, beamy);
            }
            beamy++;
            if (beamy <= YMAX - 1) {
                printBeam(beamx, beamy);
            } else { // when beamy = YMAX
                beamy = UFO_HTS;
                sign = 0;
            }
        }

        if (duration2 > Down_ufo_time) {
            pre_time2 = now_time2;
            mvcnt++;
            if (downx == 1) {
                direction = 1;
            }
            if (downx == XMAX - UFO_WID) {
                direction = -1;
            }
            if (mvcnt == 50) {
                clearUFO(downx, downy);
                r = rand() % (XMAX - UFO_WID -1) + 1;
                downx = r;
                mvcnt = 0;
                direction = -direction;
            }
            downx += direction;
            printUFO(downx, downy, DEFAULT);
        }

        if ((downx <= beamx) && (beamx <= downx + UFO_WID) && (beamy == YMAX - UFO_HTS -2)) {
            clearHP(hp);
            hp--;
            printHP(hp);
            if (hp == 0) {
                clearScreen();
                printWord(CLEAR_NUM, gameclear);
                reset();
                exit(0);
            }

            pre_time = now_time;
            clearBeam(beamx, YMAX - UFO_HTS -2);
            beamy = UFO_HTS;
            sign = 0;
        }
    }

    return 0;
}

void printUFO(int x, int y, int color) {
    setType(NORMAL);
    setCharColor(color);
    setCharBold();
    setPlace(x, y);
    printf("   ／￣￣￣￣＼   ");
    setPlace(x, y+1);
    printf("   |: ○  ○  ○ :|   ");
    setPlace(x, y+2);
    printf("  ／            ＼  ");
    setPlace(x, y+3);
    printf("／＿＿＿＿＿＿＿＿＼");
    setPlace(x, y+4);
    printf("￣ ＼＿＼＿／＿／￣");
}

void clearUFO(int x, int y) {
    for (int i=0; i<UFO_HTS; i++) {
        setPlace(x, y+i);
        setCharColor(BLACK);
        setType(NORMAL);
        for (int j=0; j<UFO_WID; j++) {
            printf("    ");
            fflush(stdout);
        }
    }

    setCharColor(DEFAULT);
}

void printBeam(int x, int y) {
    setType(REVERSE);
    setCharColor(DEFAULT);
    setPlace(x, y);
    printf("　　");
    fflush(stdout);
}

void clearBeam(int x, int y) {
    setType(NORMAL);
    setCharColor(BLACK);
    setPlace(x, y);
    printf("　　");
    fflush(stdout);
    setCharColor(DEFAULT);
}

void printHP(int hp) {
    setType(NORMAL);
    setPlace(3, YMAX);
    printf("HP ");
    setPlace(7, YMAX);
    setType(REVERSE);
    for (int i=0; i<hp; i++) {
        printf("       ");
        fflush(stdout);
    }
}

void clearHP(int hp) {
    setPlace(7, YMAX);
    setType(NORMAL);
    setCharColor(BLACK);
    for (int i=0; i<hp; i++) {
        printf("       ");
        fflush(stdout);
    }
}

void initialize(void) {
    tinit();
    cursolOff();
    clearScreen();
    setBackColor(BLACK);
    setCharColor(WHITE);
    setType(NORMAL);
}

void reset(void) {
    setBackColor(BLACK);
    setCharColor(DEFAULT);
    setType(NORMAL);
    cursolOn();
    tcsetattr(1, TCSADRAIN, &otty);
    write(1, " \n", 1);
    printf("\n");
}

void printWord(int WORD_NUM, int array[WORD_NUM][WORD_HTS][WORD_WID]) {
    int adjx, adjy;
    adjy = 13;
    if (WORD_NUM == 8) {
        adjx = 15;
    } else if (WORD_NUM == 9) {
        adjx = 10;
    }

    for (int i=0; i<WORD_NUM; i++) {
        for (int j=0; j<WORD_HTS; j++) {
            setPlace(WORD_HTS * i * 2 + adjx, j + adjy);
            for (int k=0; k<WORD_WID; k++) {
                if (array[i][j][k] == 1) {
                    setType(REVERSE);
                    printf("  ");
                    fflush(stdout);
                } else {
                    setType(NORMAL);
                    printf("  ");
                    fflush(stdout);
                }
            }
        }
    }
}

int playgame[PLAY_NUM][WORD_HTS][WORD_WID] = {
    //P
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,
    1,0,0,0,0,
    1,0,0,0,0,
    //L
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,
    //A
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    //Y
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    //G
    0,1,1,1,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,1,1,1,
    1,0,0,0,1,
    0,1,1,1,0,
    //A
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    //M
    1,0,0,0,1,
    1,1,0,1,1,
    1,0,1,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    //E
    1,1,1,1,1,
    1,0,0,0,0,
    1,1,1,1,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,
};

int gameclear[CLEAR_NUM][WORD_HTS][WORD_WID] = {
    //G
    0,1,1,1,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,1,1,1,
    1,0,0,0,1,
    0,1,1,1,0,
    //A
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    //M
    1,0,0,0,1,
    1,1,0,1,1,
    1,0,1,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    //E
    1,1,1,1,1,
    1,0,0,0,0,
    1,1,1,1,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,
    //C
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,1,
    0,1,1,1,0,
    //L
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,
    //E
    1,1,1,1,1,
    1,0,0,0,0,
    1,1,1,1,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,
    //A
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    //R
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,
    1,0,0,1,0,
    1,0,0,0,1,
};

int kbhit(void) {
    int ret;
    fd_set rfd;
    struct timeval timeout = {0, 0};
    FD_ZERO(&rfd);
    FD_SET(0, &rfd); // 0:stdin
    ret = select(1, &rfd, NULL, NULL, &timeout);
    
    if (ret == 1) return 1;
    else return 0;
}

int getch(void) {
    unsigned char c;
    int n;
    while ((n == read(0, &c, 1))<0 && errno == EINTR);
        if (n == 0) return -1;
        else return (int) c;
}

static void onsignal(int sig) {
    signal(sig, SIG_IGN);
    switch(sig) {
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
        case SIGHUP:
            exit(0);
            break;
    }
}

int tinit(void) {
    if (tcgetattr(1, &otty) < 0) return -1;

    ntty = otty;
    ntty.c_iflag &= ~(INLCR|ICRNL|IXON|IXOFF|ISTRIP);
    ntty.c_oflag &= ~OPOST;
    ntty.c_lflag &= ~(ICANON|ECHO);
    ntty.c_cc[VMIN] = 1;
    ntty.c_cc[VTIME] = 0;
    tcsetattr(1, TCSADRAIN, &ntty);
    signal(SIGINT, onsignal);
    signal(SIGQUIT, onsignal);
    signal(SIGTERM, onsignal);
    signal(SIGHUP, onsignal);
    return 0;
}