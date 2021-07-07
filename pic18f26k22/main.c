#include <stdio.h>
#include <stdlib.h>
#include <pic18.h>

//#pragma config CONFIG1H = 0x22
__CONFIG(1, FOSC_HSHP & PLLCFG_OFF & PRICLKEN_ON & FCMEN_OFF & IESO_OFF);
//#pragma config CONFIG2L = 0x1F
__CONFIG(2, PWRTEN_OFF & BOREN_SBORDIS & BORV_190);
//#pragma config CONFIG2H = 0x3C
__CONFIG(3, WDTEN_OFF & WDTPS_32768);
//#pragma config CONFIG3H = 0xBF
__CONFIG(4, CCP2MX_PORTC1 & PBADEN_ON & CCP3MX_PORTB5 & HFOFST_ON & T3CMX_PORTC0 & P2BMX_PORTB5 & MCLRE_EXTMCLR);
//#pragma config CONFIG4L = 0x81
__CONFIG(5, STVREN_ON & LVP_OFF & XINST_OFF);
//#pragma config CONFIG5L = 0xF
__CONFIG(6, CP0_OFF & CP1_OFF & CP2_OFF & CP3_OFF);
//#pragma config CONFIG5H = 0xC0
__CONFIG(7, CPB_OFF & CPD_OFF);
//#pragma config CONFIG6L = 0xF
__CONFIG(8, WRT0_OFF & WRT1_OFF & WRT2_OFF & WRT3_OFF);
//#pragma config CONFIG6H = 0xE0
__CONFIG(9, WRTC_OFF & WRTB_OFF & WRTD_OFF);
//#pragma config CONFIG7L = 0xF
__CONFIG(10, EBTR0_OFF & EBTR1_OFF & EBTR2_OFF & EBTR3_OFF);
//#pragma config CONFIG7H = 0x40
__CONFIG(11, EBTRB_OFF);

//Gate Status
#define UNKNOWN        0  
#define OPEN           1
#define OPENING        2
#define OPENING_STOPED 3
#define CLOSED         4
#define CLOSING        5
#define CLOSING_STOPED 6
  
//Gate status condition
#define GATE_CLOSED (gatestate == 4)
#define GATE_CLOSING_STOPED (gatestate == 6)


union{  /* State of IO's */
    unsigned char byte1;
    struct { 
        //Main IO Vars
        unsigned FC_OPEN         : 1;
        unsigned FC_CLOSED       : 1;
        unsigned PHTCELL         : 1;
        unsigned RELAY_OPEN      : 1;
        unsigned RELAY_CLOSE     : 1;
        unsigned START_BUTTON    : 1;
        //Aux IO Vars
        unsigned LS_START_BUTTON : 1; //Last Start Button state
        unsigned START_BUTTON_EN : 1; //Start Button enable
    };
}IO;

union{ /* FSM Managment Flags */
    unsigned char byte2;
    struct {  
        unsigned START       : 1;
        unsigned ERROR       : 1;
        unsigned TIMEOUT     : 1;
    };
}SYS;

volatile unsigned char timeout_count   = 0;                
unsigned char state           = 0;    
unsigned char gatestate       = 0;   

void putch(unsigned char txData){           
    while (!TXSTA1bits.TRMT);
    TXREG1 = txData;  
}  
void Serial_send(){
    
    char buff_Char1 = 0;
    buff_Char1      |= (SYS.ERROR << 3);
    buff_Char1      |= (gatestate);
    
    char buff_Char2  = 0; 
    buff_Char2 |= (IO.START_BUTTON << 6);
    buff_Char2 |= (IO.FC_OPEN      << 5);
    buff_Char2 |= (IO.FC_CLOSED    << 4);
    buff_Char2 |= (IO.PHTCELL      << 3);
    buff_Char2 |= (state  &  0b00000011);
    
    putch(buff_Char1);
}

void interrupt high_ISR(){ //High priority interrupt
    if (INTCONbits.INT0IF && INTCONbits.INT0IE){ //PHTCELL
       INTEDG0           ^= 1;
       IO.PHTCELL        ^= 1;
       INTCONbits.INT0IF  = 0;
    }
    if (RC1IF && RC1IE) {   //Serial Receive
        if (RCREG1 == 's') { 
           SYS.START = 1; 
        }
    }
}
void interrupt low_priority low_ISR(){ //Low priority Interrupt
    if (INTCONbits.TMR0IF && INTCONbits.TMR0IE){ //Gate Timeout timer
        T0CONbits.TMR0ON = 0;
        timeout_count++;
        if (timeout_count >= 3 ){ //10seg
            timeout_count = 0;
            T0CONbits.TMR0ON = 0;
            TMR0L            = 0;
            TMR0H            = 0;
            SYS.TIMEOUT     = 1;
            SYS.ERROR       = 1;
        }           
        INTCONbits.TMR0IF = 0;
    }
    if (PIR1bits.TMR1IF && PIE1bits.TMR1IE){ //Start button Hold timer
        PIR1bits.TMR1IF    = 0;
        T1CONbits.TMR1ON   = 0;
        TMR1L              = 0;
        TMR1H              = 0;
        IO.START_BUTTON_EN = 1;
    }
}
void FC_Check(){  
    if (!IO.FC_OPEN  && !IO.FC_CLOSED) // Gate Open
        gatestate = OPEN;
    else  if (IO.FC_OPEN  && IO.FC_CLOSED) //Gate Closed
        gatestate = CLOSED;
  
    if (!IO.FC_OPEN  && IO.FC_CLOSED) //Error Flag
        SYS.ERROR = 1;
}
void IO_Check(){ //Inputs and Outputs Polling
    
    /* Start button managment (Edge detection and Hold Timer) */
    IO.START_BUTTON = !PORTBbits.RB3;       
    if ((IO.START_BUTTON != IO.LS_START_BUTTON) && IO.START_BUTTON_EN){
        SYS.START = IO.START_BUTTON;
        if (SYS.START){
            T1CONbits.TMR1ON   = 1;
            IO.START_BUTTON_EN = 0;
        } 
    } 
    IO.LS_START_BUTTON = IO.START_BUTTON;
    
    
    IO.FC_OPEN   = !PORTBbits.RB1;
    IO.FC_CLOSED = !PORTBbits.RB2;
    
    LATA0        = IO.RELAY_OPEN;
    LATA1        = IO.RELAY_CLOSE;
    
    /* Status LED*/
    if (SYS.ERROR){ //Error status
      LATA2 = 0;
      LATA3 = 1;
      LATA4 = 0;
    } else if (gatestate == 2 || gatestate == 5 && !SYS.ERROR){ //Moving Status
      LATA2 = 1;
      LATA3 = 0;
      LATA4 = 0;  
    } else {       //Ok status
      LATA2 = 0;
      LATA3 = 0;
      LATA4 = 1;  
    }              
}
void FSM_Main(){
    switch (state){
        case 0:
            FC_Check();        //Gate position check
            if (SYS.START){ 
                SYS.START  = 0;
                state      = 1;
            }           
            break;

        case 1: // Check Gate state  
            if (GATE_CLOSED || GATE_CLOSING_STOPED ) 
                state = 2;
            else
                state = 3;
            break;

        case 2: //Open gate    
            IO.RELAY_OPEN     = 1;
            IO.RELAY_CLOSE    = 0;
            T0CONbits.TMR0ON  = 1;
            gatestate         = OPENING; 
            if (!IO.FC_OPEN) {
                SYS.ERROR  = 0;
                state      = 4; 
            } else if ((SYS.START) || SYS.TIMEOUT){
                SYS.START    = 0;
                SYS.TIMEOUT  = 0;
                gatestate    = OPENING_STOPED; 
                state        = 4;
            }
            break;
            
        case 3: //CLOSE gate   
            IO.RELAY_OPEN     = 0;
            IO.RELAY_CLOSE    = 1;
            gatestate         = CLOSING; 
            T0CONbits.TMR0ON  = 1;
           
            if (IO.FC_CLOSED) {
                SYS.ERROR = 0;
                state      = 4;
            } else if ((SYS.START) || SYS.TIMEOUT || !IO.PHTCELL){
                SYS.START   = 0;
                SYS.TIMEOUT = 0;
                gatestate    = CLOSING_STOPED; 
                state        = 4;
            }
            break;
            
        case 4: //STOP Gate
            IO.RELAY_OPEN     = 0;
            IO.RELAY_CLOSE    = 0;
            
            //Timeout timer reset
            T0CONbits.TMR0ON  = 0;
            TMR0L             = 0;
            TMR0H             = 0;
            timeout_count     = 0;
            state             = 0;
            break;
    } 
}

int main() {
    
    /* IO Setup */
    //Inputs
    INTCON2bits.RBPU = 0; //internal Pull-Up Enable
    ANSELB = 0;           //Disable PORTB ADC
    
    TRISB0 = 1; //PHTCEL
    WPUB0  = 1; 
    TRISB1 = 1; //FC_OPEN
    WPUB1  = 1;
    TRISB2 = 1; //FC_Closed
    WPUB2  = 1;
    TRISB3 = 1; //Start Button
    WPUB3  = 1;
    
    //Outputs
    ANSELA = 0; //Disable PORTA ADC
    TRISA0 = 0; //Relay Open Output
    TRISA1 = 0; //Relat Closed Output
    
    TRISA2 = 0; //Status LED1
    TRISA3 = 0; //Status LED2
    TRISA4 = 0; //Status LED2

    /* Timer's setup */
    //Timer 0 Setup   (Gate Timeout)
    T0CONbits.TMR0ON  = 0;
    T0CONbits.T08BIT  = 0; //16bits
    T0CONbits.T0CS    = 0;
    T0CONbits.PSA     = 0;
    T0CONbits.T0PS    = 0b111; //1:256 PRE
    
    //Timer 1 Setup (Botão Start Hold))
    T1CONbits.TMR1CS  = 0b00; 
    T1CONbits.T1CKPS  = 0b11; //1:8 pré (máx)
    T1CONbits.T1RD16  = 1;
    T1CONbits.TMR1ON  = 0;
    
    /* Interrupts Setup */
    RCONbits.IPEN   = 1;   //Enable interrupt priority
    INTCONbits.GIEH = 1;     
    INTCONbits.GIEL = 1;
    
    //External Interrupts
    INTCONbits.INT0IE  = 1; //PHTCELL Interrupt (INT0 is always high priority)
    INTEDG0            = 1; 
    
    //Timer's Interrupts 
    INTCONbits.TMR0IE  = 1;
    INTCON2bits.TMR0IP = 0;
    PIE1bits.TMR1IE = 1;
    IPR1bits.TMR1IP = 0;
    
    /*SERIAL TRANSMITER CONFIG */
    SPBRGH1 = 0;    //9600 Baud Rate
    SPBRG1  = 129;

    TRISC6 = 1; // OUTPUT TX
    ANSC6  = 0;
    TRISC7 = 1; // INPUT RX
    ANSC7  = 0;
    
    TXSTA1bits.SYNC    = 0;
    BAUDCON1bits.BRG16 = 0;
    TXSTA1bits.BRGH    = 1;
    RCSTA1bits.SPEN    = 1;
    TXSTA1bits.TXEN    = 1;
    RCSTA1bits.CREN    = 1;
    RC1IE              = 1;
    
    /* Var's initial value setup */ 
    IO.byte1            = 0;
    SYS.byte2           = 0;
    
    IO.START_BUTTON_EN = 1;  
    IO.PHTCELL         = PORTBbits.RB0;
    
    /*Main loop*/
    while (1){
        IO_Check(); 
        FSM_Main(); 
        Serial_send();
    }
    
}

