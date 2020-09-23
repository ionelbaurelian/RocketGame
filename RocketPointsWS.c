#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>     // uint32_t
#include <unistd.h>     // delay
#include <signal.h>     // signal handling
#include <sys/select.h>
#include <time.h>       // time()
#include <sys/types.h>  // getch
#include <termios.h>    // getch
 
#define MAX_CHAR_COLUMNS             16
#define MAX_CHAR_ROWS                2

#define COMANDA_SCHIMBARE_RAND       1
#define COMANDA_INAINTARE            2

#define SCREEN_UPDATE_DELAY          1000

#define SCENARIO_CONFIG_NUMBER       5
 
char rows[2][16];
int crt_x = 0, crt_y = 0;
int k=0;

volatile int r_x = 0, r_y = 0;
volatile int last_r_x = 0, last_r_y = 0;

char x;
         
int scenario_nb;

typedef struct {
    int x;
    int y;
} coordinates; 
/* Configuratiile posibile de obstacole in cadrul jocului */
coordinates obstacles[SCENARIO_CONFIG_NUMBER][4] = { { {1,1}, {5,0}, { 9,1}, {13,1} },
                                                     { {3,1}, {7,0}, {11,0}, {15,1} },
                                                     { {3,0}, {6,1}, { 9,0}, {12,1} },
                                                     { {4,1}, {7,0}, {10,1}, {13,0} },
                                                     { {2,1}, {5,0}, { 8,1}, {13,0} }
                                                   }; 
/* Configuratiile posibile de reward-uri in cadrul jocului */
coordinates rewards[SCENARIO_CONFIG_NUMBER] = { {14, 1}, {15, 0}, {13, 1}, {14, 0}, {12, 0} };


void Delay( uint32_t nTime );
char getch( void );
void lcd16x2_gotoxy(unsigned int x, unsigned int y);
void lcd16x2_puts(char *s);
void check_keyboard_pressed( void );
void generate_scenario( void );
uint8_t verificare_tasta_apasata( char c );
void handler_schimbare_rand( );
void handler_inaintare ( );

int main()
{
    int i, pid, c=0;
    time_t t;
    int max_score, score = 0;
    
    printf("Introdu un punctaj maxim: \r\n");
    scanf("%d", &max_score);
    printf( "\nYou entered: %d ", max_score);
    pid = fork();
    
    if( pid != 0 ) // Proces parinte - programul de baza care primeste apasarile corecte si emuleaza un LCD
    {   int flag=0;
        srand((unsigned) time(&t));
        memset ( rows, ' ', 32 );
 
        /* Definerea handler-urilor pentru apasarea butoanelor W si S */
        signal(SIGUSR1, handler_schimbare_rand);
        signal(SIGUSR2, handler_inaintare);
        
        lcd16x2_puts("=>");
        generate_scenario();
        
        Delay(SCREEN_UPDATE_DELAY);
        
        while( 1 ){
           
           /* Plasare obstacole pe ecran */
            for( i = 0; i < 4; i++ ) {
                lcd16x2_gotoxy(obstacles[scenario_nb][i].x, obstacles[scenario_nb][i].y);
                lcd16x2_puts("X");
            }
            /* Plasare punct pe ecran */
            lcd16x2_gotoxy(rewards[scenario_nb].x, rewards[scenario_nb].y);
            lcd16x2_puts("*");
            
            
        
        for (i = 0; i < 4; i++)
        {
            if ((r_x+1 == obstacles[scenario_nb][i].x || r_x==obstacles[scenario_nb][i].x) && r_y == obstacles[scenario_nb][i].y)
            {
                flag = 1;
                break;
            }
        }
        if (flag == 1){
            break;
        }
              
            

    if ((r_x==rewards[scenario_nb].x||r_x+1== rewards[scenario_nb].x) && r_y==rewards[scenario_nb].y) 
    {
        score++;
        if( score == max_score )
            break;
        last_r_x=r_x;
        last_r_y=r_y;
        lcd16x2_gotoxy(last_r_x, last_r_y);
        lcd16x2_puts(" ");
        lcd16x2_gotoxy(rewards[scenario_nb].x, rewards[scenario_nb].y);
        lcd16x2_puts(" ");
                
        for(i=0;i<4;i++)
        {
            lcd16x2_gotoxy(obstacles[scenario_nb][i].x, obstacles[scenario_nb][i].y);
            lcd16x2_puts(" ");
        }
        //Regeneram un nou set de obstacole , si initializam racheta in punctul (0,0)        
        generate_scenario();
        r_x=0;
        r_y=0;
        lcd16x2_gotoxy(r_x, r_y);
        lcd16x2_puts("=>");
        printf ("Ai adunat UN punct. Puncte stranse: %d  \r\n",score);
                 
        }
  
    Delay(SCREEN_UPDATE_DELAY);
            
    }
        
    if( flag==1 )
        printf("FINAL DE JOC, racheta s-a lovit de obstacol! \r\n");
    else
        printf("FINAL DE JOC, racheta a adunat %d puncte\r\n", max_score);
        
    }
    else { 
        check_keyboard_pressed( );
    }
   
    return (0);
}


char getch() 
{
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (buf);
}

void Delay( uint32_t nTime ) 
{
    usleep( nTime * 1000 );
}
 
void lcd16x2_gotoxy(unsigned int x, unsigned int y)
{
    ( x < MAX_CHAR_COLUMNS ) ? ( crt_x = x ) : ( crt_x = 15 );

    ( y < MAX_CHAR_ROWS ) ? ( crt_y = y ) : ( crt_y = 1 );
}
 
void lcd16x2_puts(char *s)
{
    int s_len = strlen(s);
    int len = s_len;
    int i, j;
   
    char screen_frame_full[39] = "# # # # # # # # # # # # # # # # # # \r\n";
    char screen_frame_start[2] = "#";
    char screen_frame_end[5] = " #\r\n";
   
   
    if( s_len > (MAX_CHAR_COLUMNS - crt_x) )
        len = (MAX_CHAR_COLUMNS - crt_x);
   
    for(i = 0; i < len; i++)
    {
        rows[crt_y][i + crt_x] = s[i];
    }
   
    system("clear");
   
    printf( "%s", screen_frame_full );
    printf( "%s", screen_frame_start );
    for( i = 0; i < MAX_CHAR_COLUMNS; i++ ) {
        printf(" %c", rows[0][i]);
    }
    printf( "%s", screen_frame_end );
    printf( "%s", screen_frame_start );
    for( i = 0; i < MAX_CHAR_COLUMNS; i++ ) {
        printf(" %c", rows[1][i]);
    }
    printf( "%s", screen_frame_end );
    printf( "%s", screen_frame_full );
}


void generate_scenario( ) 
{
    scenario_nb = rand() % SCENARIO_CONFIG_NUMBER;
}

void check_keyboard_pressed( )
{
    char c;
    uint8_t cmd; 
    while( 1 )
    {
        c = getch();
        cmd = verificare_tasta_apasata( c );
        if( cmd == COMANDA_SCHIMBARE_RAND )
            kill( getppid(), SIGUSR1);
        else if( cmd == COMANDA_INAINTARE )
            kill( getppid(), SIGUSR2);
    }
}

uint8_t verificare_tasta_apasata (char c)
{
  if (c=='W'||c=='w')
    {
      return COMANDA_SCHIMBARE_RAND;
    }
  else if(c=='S'||c=='s')
        {
            return COMANDA_INAINTARE;
        }
}

void handler_schimbare_rand() 
{
    if(r_y==0) 
    {
        r_y=1;
        last_r_y=0;
    }
    else if(r_y==1) 
            {
                r_y=0;
                last_r_y=1;
            }
        
    lcd16x2_gotoxy(r_x, last_r_y);
    lcd16x2_puts("  ");
    lcd16x2_gotoxy(r_x, r_y);
    lcd16x2_puts("=>");
    
    printf("S-a apasat tasta W. Schimba randul.\r\n");
    Delay(1000);
}

void handler_inaintare ( ) 
{
    last_r_x=r_x;
    r_x++;
    lcd16x2_gotoxy(last_r_x, r_y);
    lcd16x2_puts("  ");
    lcd16x2_gotoxy(r_x, r_y);
    lcd16x2_puts("=>");
    
    printf("S-a apasat tasta S. Inainteaza.\r\n");
    Delay(1000);
}
