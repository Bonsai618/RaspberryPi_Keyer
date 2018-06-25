#define ram static ram
#define TRUE 1
#define FALSE 0
#define OK           1
#define NG           -1


#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "delays.h"
#include <pconfig.h>
#include "timers.h"
#include "p18cxxx.h"
#include "usart.h"

extern int ClockCtr;

void Pause (int MS);
void Pin15HIGH(void);
void Pin15LOW(void);
void DelayMS(unsigned long MS);


extern char volatile Msg1Ready;
extern char volatile Msg2Ready;
extern unsigned char volatile Buffer1[256]; 
extern unsigned char volatile Buffer2[256];
extern unsigned int volatile pMsg ;
extern unsigned int volatile QHead;
extern unsigned int volatile QTail;
extern unsigned int volatile QReset;
extern char volatile QBuff[];
extern char volatile QBuff2[];
extern unsigned int volatile nQueue;
extern unsigned int volatile nQueue2;
extern unsigned int volatile Semaphore;
ram unsigned long volatile TimerPopCount;

ram unsigned char volatile CUart1;
ram unsigned char volatile *uptr1;
ram unsigned char volatile CUart2;
ram unsigned char volatile *uptr2;
ram int volatile DataLen1;
ram int volatile DataLen2;
ram int volatile TimerTick;



ram int FSR0H_TEMP;
ram int FSR0L_TEMP;
ram int FSR1H_TEMP;
ram int FSR1L_TEMP;
ram int UARTErrorOR;
ram int UARTErrorFR;


void HighVector (void);
void MyHighISR (void);

void Pin4LOW(void);
void Pin4HIGH(void);


char *p1;
char *p2;
char *p2x;



#pragma code InterruptVectorHigh = 0x08 //interrupt pointer address (0x08 high priority)

void InterruptVectorHigh (void)
{
   _asm                     //assembly code starts
   movff FSR1L,FSR1L_TEMP
   movff FSR1H,FSR1H_TEMP
   movff FSR0L,FSR0L_TEMP
   movff FSR0H,FSR0H_TEMP
   _endasm


   // USART1 Interrupt
   if ( PIR1bits.RC1IF )  // If RCIF is set
   {


      USART1_Status.val &= 0xf0;      // Clear previous status flags



      CUart1= (unsigned char) RCREG1;

      if ( CUart1 == 'X' )
      {
         QReset = 1;
         memset((char *)QBuff,'\0',256);
         nQueue = 0;
         Reset();
         goto exit;
      }

      p1 = (QBuff + nQueue);
      *p1 = CUart1;
      nQueue ++;
      PIR1bits.RCIF = 0;

      goto exit;

   }


 //USART2 Interrupt
 if ( PIR3bits.RC2IF )  // If RCIF is set
   {


      USART2_Status.val &= 0xf0;      // Clear previous status flags



      CUart2= (unsigned char) RCREG2;


      p2x = (QBuff2 + nQueue2);
      *p2x = CUart2;
      nQueue2 ++;
      PIR3bits.RC2IF = 0;

      goto exit;

   }


// Check for Timer1 overflow
   if ( INTCONbits.TMR0IF )
   {


      TMR0H = 0x06;
      TMR0L = 0x30;
      INTCONbits.TMR0IF = 0;
      TimerPopCount += 1;
      
   }
   exit:  
   
   
  


   _asm


   movff FSR0H_TEMP,FSR0H
   movff FSR0L_TEMP,FSR0L
   movff FSR1L_TEMP,FSR1L
   movff FSR1H_TEMP,FSR1H

   retfie 1 
   _endasm



}


void StartTimer0(void)
{
   // One count per microsecond at 40 Mhz
   OpenTimer0(TIMER_INT_OFF & T0_16BIT & T0_SOURCE_INT &  T0_EDGE_RISE & T0_PS_1_64);        

}


void StartTimer1(void)
{

   // Enazble Timer1 High Priority interrupts
   IPR1 = IPR1 | 0x01;

   // Timer1 Gate Enable is turned off (Parameter 2)
   OpenTimer1(TIMER_INT_ON & T1_16BIT_RW & T1_SOURCE_PINOSC & T1_OSC1EN_ON & T1_PS_1_1, 0x00);
   TMR1H = 0x7F;
   TMR1L = 0xFF;

}

int vgets1USART(char *buffer,  unsigned int TOT)
{

   unsigned long Elapsed,  MS;

   // Elapsed time in MS since last reset.
   union Timers timer;

   StartTimer0();
   MS = TOT * 1040;
   while ( 1 )
   {

      // Interrupt driven version
      if ( Msg1Ready == TRUE )
      {
         memcpy((void *)buffer,(void *)Buffer1+1,DataLen1);
         memset(buffer+DataLen1,'\0',1);
         Msg1Ready = FALSE;
         return DataLen1;
      }
      else
      {
         timer.bt[0] = TMR0L;     // Copy Timer0 low byte into union
         timer.bt[1] = TMR0H; 
         Elapsed = timer.lt;
         if ( Elapsed > MS )
         {
            return NG;
         }
      }
   }

}



unsigned long ResetTimer0(void)
{

   unsigned long Elapsed, Remainder,  MS;

   // Elapsed time in MS since last reset.
   union Timers timer;
   timer.bt[0] = TMR0L;   // Copy Timer0 low byte into union
   timer.bt[1] = TMR0H; 
   Elapsed = timer.lt;
   // The value ClockCtr is the pathlength of this routine.
   MS = (Elapsed / 1000) ;
   Remainder = Elapsed - (MS * 1000) + ClockCtr;
   WriteTimer0(Remainder);
   return(MS);
}

unsigned long ResetTimer1(void)
{

   unsigned long Elapsed, Remainder,  MS;

   // Elapsed time in MS since last reset.
   union Timers timer;
   timer.bt[0] = TMR1L;   // Copy Timer0 low byte into union
   timer.bt[1] = TMR1H; 
   Elapsed = timer.lt;
   MS = (Elapsed / 250) ;
   Remainder = Elapsed - (MS * 250) + 54 ;
   WriteTimer1(Remainder);
   return(MS);
}






void SetClockSpeed(unsigned int speed) {

   if ( speed == 8 )
   {
      OSCTUNE = 0x00;
      OSCCON =  0x62;      // Set internal 8Mhz clock
   }

   if ( speed == 1 )
   {
      OSCTUNE = 0x00;
      OSCCON =  0x32;      // Set internal 1Mhz clock
   }

   if ( speed == 40 )
   {
      OSCTUNE = 0x00;
      OSCCON =  0x00;      // Set External clock}
   }
}


void OpenUSART1pic18f25K80( unsigned int speed)
{
   TXSTA1 = 0;            // Reset USART registers to POR state

   RCSTA1 = 0;

   RCSTA1bits.CREN = 1;     // Continuous Receive

   TXSTA1bits.BRGH = 1;     // Baud Rate High

   PIR1bits.TXIF = 0;     

   PIE1bits.RCIE = 1;      // Receive interrupt

   PIE1bits.TXIE = 0;      //No Transmit Interrupts

   // For 8 MHZ Clock

   if ( speed == 38400 )
   {
      SPBRG1 = 52;
   }

   if ( speed == 57600 )
   {
      SPBRG1 = 34;
   }


   BAUDCON1 = 0x08;         // Set BRG16

   TXSTA1bits.BRGH = 1;     // High Baud Rate 
   TRISCbits.TRISC6 = 0;    // Output TX
   TRISCbits.TRISC7 = 1;    // Input RX

   RCONbits.IPEN   = 1;    // Enable Interrupt Priority
   IPR1bits.RCIP   = 1;    // Receive Interrupts are High Priority
   INTCON1bits.PEIE = 1;    // Enable Priorities
   INTCON1bits.GIEH = 1;    // Enable High Priority Interrupts


   TXSTA1bits.TXEN = 1;     // Enable transmitter

   RCSTA1bits.SPEN = 1;     // Enable receiver


}


void OpenUSART2pic18f25K80( void)
{

   Open2USART(
   USART_TX_INT_OFF  &
   USART_RX_INT_OFF  &
   USART_ASYNCH_MODE &
   USART_EIGHT_BIT   &
   USART_CONT_RX     &
   USART_BRGH_LOW,
   64 );
   // Enabble Transmit at 9600 BPS



}



void vputs1USART( char *data, int len)
{

// Write variable message prefixed with single byte length code
// (C) 2010 P. Cranwell
   char *p;
   char *q;
   char Reset = 255;
   int i;


   i = len;

   // Create pointers & length code
   q = (char *) &len;


   // write Reset character
   memset(WorkBuffer,'\0',sizeof(WorkBuffer));
   memcpy(WorkBuffer,(void *) &Reset,1);
   p = WorkBuffer;

   while ( Busy1USART() );
   putc1USART(*p);

   // wait for receiver to reset
   DelayMS(10);


   // write length code
   memcpy((void *)&WorkBuffer,(void *)&i,1);                         // add length code


   // write length
   while ( Busy1USART() );
   p = WorkBuffer;
   putc1USART(*p);

   // wait for receiver 

   DelayMS(2);
   p++;

   memcpy((void *)&WorkBuffer[1] , (void *)data,len);    
   // Copy Data

   len++;

   for ( i=0; i<len; i++ )                        // Write Variable Message
   {
      // Transmit a byte
      while ( Busy1USART() );
      putc1USART(*p);
      p++;
   }




}





char DataRdyUSART1(void)
{
   if ( PIR1bits.RCIF ) // If RCIF is set
      return 1;    // Data is available, return TRUE
   return 0;   // Data not available, return FALSE
}





char Wait2USART(void)
{
   if ( !TXSTA2bits.TRMT )   // Is the transmit shift register empty
      return(1);            // No, return FALSE
   return(0);               // Return TRUE
}



void DelayMS(unsigned long MS)
{

   // Callibrated for 8MHZ Clock
   unsigned long  i;

   for ( i = 0 ; i<MS; i++ )
   {
      Delay100TCYx(20);
   }
}

void Myputs2USART( char *data, int len)
{

// Write variable message prefixed with single byte length code
// (C) 2010 P. Cranwell
   ram char *p;
   ram int ii;
   ram int Len;



   p = (char *)data;
   Len = len;

   for ( ii=0; ii<Len; ii++ )                        // Write Variable Message
   {
      // Transmit a byte
      while ( Wait2USART() );
      putc2USART(*p);
      p++;
   }
}









// Configuration for PIC18F2K80

void Pin2HIGH(void)
{
   // Port A, Bit 1

   TRISA = TRISA & 0xFE;
   LATA = LATA | 0x01;
   PORTA = PORTA | 0x01;
}

void Pin2LOW(void)
{
   //PORT A, Bit 0
   TRISA = TRISA & 0xFE;
   LATA = LATA & 0xFE;
   PORTA = PORTA & 0xFE;
}

unsigned char GetPin2(void)
{
   // Change PORT A , Bit 0 to Input
   TRISA = TRISA | 0x01;
   PORTA & 0x01;
   PORTA & 0x01; 
   return(PORTA & 0x01);
}

void Pin3HIGH(void)
{
   // Port A, Bit 1

   TRISA = TRISA & 0xFD;
   LATA = LATA | 0x02;
   PORTA = PORTA | 0x02;
}

void Pin3LOW(void)
{
   //PORT A, Bit 1
   TRISA = TRISA & 0xFD;
   LATA = LATA & 0xFD;
   PORTA = PORTA & 0xFD;
}

unsigned char GetPin3(void)
{
   // Change PORT A , Bit 1 to Input
   TRISA = TRISA | 0x02;
   PORTA & 0x02;
   PORTA & 0x02; 
   return(PORTA & 0x02);
}


void Pin4HIGH(void)
{
   // Port A, Bit 2
   TRISA = TRISA & 0xFB;
   LATA = LATA | 0x04;
   PORTA = PORTA | 0x04;
}

void Pin4LOW(void)
{
   //PORT A, Bit 2
   TRISA = TRISA & 0xFB;
   LATA = LATA & 0xFB;
   PORTA = PORTA & 0xFB;
}

unsigned char GetPin4(void)
{
   // Change PORT A , Bit 2 to Input
   TRISA = TRISA | 0x04;
   PORTA & 0x04;
   PORTA & 0x04;
   return(PORTA & 0x04);
}



void Pin5HIGH(void)
{
   // Port A, Bit 3
   TRISA = TRISA & 0xF7;
   LATA = LATA | 0x08;
   PORTA = PORTA | 0x08;
}

void Pin5LOW(void)
{
   //PORT A, Bit 3
   TRISA = TRISA & 0xF7;
   LATA = LATA & 0xF7;
   PORTA = PORTA & 0xF7;
}

unsigned char GetPin5(void)
{
   // Change PORT A , Bit 3 to Input
   TRISA = TRISA | 0x08;
   PORTA & 0x08;
   PORTA & 0x08; 
   return(PORTA & 0x08);
}

void Pin6HIGH(void)
{
   // Port A, Bit 4
   TRISA = TRISA & 0xEF;
   LATA = LATA | 0x10;
   PORTA = PORTA | 0x10;
}

void Pin6LOW(void)
{
   //PORT A, Bit 4
   TRISA = TRISA & 0xEF;
   LATA = LATA & 0xEF;
   PORTA = PORTA & 0xEF;
}

unsigned char GetPin6(void)
{
   // Change PORT A , Bit 4 to Input
   TRISA = TRISA | 0x10;
   PORTA & 0x10;
   PORTA & 0x10;
   return(PORTA & 0x10);
}

void Pin7HIGH(void)
{
   // Port A, Bit 5
   TRISA = TRISA & 0xDF;
   PORTA = PORTA | 0x20;
}

void Pin7LOW(void)
{
   //PORT A, Bit 5
   TRISA = TRISA & 0xDF;
   PORTA = PORTA & 0xDF;
}

unsigned char GetPin7(void)
{
   // Change PORT A , Bit 5 to Input
   TRISA = TRISA | 0x20;
   PORTA & 0x20;
   PORTA & 0x20; 
   return(PORTA & 0x20);
}





void Pin9HIGH(void)
{
   // Port A, Bit 7
   TRISA = TRISA & 0x7F;
   PORTA = PORTA | 0x80;
}

void Pin9LOW(void)
{
   //PORT A, Bit 7
   TRISA = TRISA & 0x7F;
   PORTA = PORTA & 0x7F;
}

unsigned char GetPin9(void)
{
   // Change PORT A , Bit 7 to Input
   TRISA = TRISA | 0x80;
   PORTA & 0x80;
   PORTA & 0x80;
   return(PORTA & 0x80);
}





void Pin10HIGH(void)
{
   // Port A, Bit 6
   TRISA = TRISA & 0xBF;
   PORTA = PORTA | 0x40;
}

void Pin10LOW(void)
{
   //PORT A, Bit 6
   TRISA = TRISA & 0xBF;
   PORTA = PORTA & 0xBF;
}

unsigned char GetPin10(void)
{
   // Change PORT A , Bit 6 to Input
   TRISA = TRISA | 0x40;
   PORTA & 0x40;
   PORTA & 0x40; 
   return(PORTA & 0x40);
}





void Pin11HIGH(void)
{
   // Port C, Bit0 
   TRISC = TRISC & 0xFE;
   PORTC = PORTC | 0x01;
}

void Pin11LOW(void)
{
   //PORT C, Bit 0
   TRISC = TRISC & 0xFE;
   PORTC = PORTC & 0xFE;
}

unsigned char GetPin11(void)
{
   // Change PORT C , Bit 0 to Input
   TRISC = TRISC | 0x01;
   PORTC & 0x01;
   PORTC & 0x01;
   return(PORTC & 0x01);
}





void Pin12HIGH(void)
{
   // PORT C, Bit 1

   TRISC = TRISC & 0xFD;
   PORTC = PORTC | 0x02;
}

void Pin12LOW(void)
{
   //PORT C, Bit 1
   TRISC = TRISC & 0xFD;
   PORTC = PORTC & 0xFD;
}

unsigned char GetPin12(void)
{
   // Change PORT C , Bit 1 to Input
   TRISC = TRISC | 0x02;
   PORTC & 0x02;
   PORTC & 0x02;
   return(PORTC & 0x02);
}




void Pin13HIGH(void)
{
   // PORT C, Bit 2
   TRISC = TRISC & 0xFB;
   PORTC = PORTC | 0x04;
}

void Pin13LOW(void)
{
   //PORT C, Bit 2
   TRISC = TRISC & 0xFB;
   PORTC = PORTC & 0xFB;
}

unsigned char GetPin13(void)
{
   // Change PORT C , Bit 2 to Input
   TRISC = TRISC | 0x04;
   PORTC & 0x04;
   PORTC & 0x04;
   return(PORTC & 0x04);
}





void Pin14HIGH(void)
{
   // PORT C, Bit 3
   TRISC = TRISC & 0xF7;
   PORTC = PORTC | 0x08;
}

void Pin14LOW(void)
{
   //PORT C, Bit 3
   TRISC = TRISC & 0xF7;
   PORTC = PORTC & 0xF7;
}

unsigned char GetPin14(void)
{
   // Change PORT C , Bit 3 to Input
   TRISC = TRISC | 0x08;
   PORTC & 0x08;
   PORTC & 0x08;
   return(PORTC & 0x08);
}




void Pin15HIGH(void)
{
   // PORT C, Bit 4
   TRISC = TRISC & 0xEF;
   PORTC = PORTC | 0x10;
}

void Pin15LOW(void)
{
   //PORT C, Bit 4
   TRISC = TRISC & 0xEF;
   PORTC = PORTC & 0xEF;
}

unsigned char GetPin15(void)
{
   // Change PORT C , Bit 4 to Input
   TRISC = TRISC | 0x10;
   PORTC & 0x10;
   PORTC & 0x10;
   return(PORTC & 0x10);
}









void Pin16HIGH(void)
{
   // PORT C, Bit 5
   TRISC = TRISC & 0xDF;
   PORTC = PORTC | 0x20;
}

void Pin16LOW(void)
{
   //PORT C, Bit 5
   TRISC = TRISC & 0xDF;
   PORTC = PORTC & 0xDF;
}

unsigned char GetPin16(void)
{
   // Change PORT C , Bit 5 to Input
   TRISC = TRISC | 0x20;
   PORTC & 0x20;
   PORTC & 0x20; 
   return(PORTC & 0x20);
}




void Pin21HIGH(void)
{
   // PORT B, Bit0 
   TRISB = TRISB & 0xFE;
   PORTB = PORTB | 0x01;
}

void Pin21LOW(void)
{
   //PORT B, Bit 0
   TRISB = TRISB & 0xFE;
   PORTB = PORTB & 0xFE;
}

unsigned char GetPin21(void)
{
   // Bhange PORT B , Bit 0 to Input
   TRISB = TRISB | 0x01;
   PORTB & 0x01;
   PORTB & 0x01;
   return(PORTB & 0x01);
}





void Pin22HIGH(void)
{
   // PORT B, Bit 1

   TRISB = TRISB & 0xFD;
   PORTB = PORTB | 0x02;
}

void Pin22LOW(void)
{
   //PORT B, Bit 1
   TRISB = TRISB & 0xFD;
   PORTB = PORTB & 0xFD;
}

unsigned char GetPin22(void)
{
   // Change PORT B , Bit 1 to Input
   TRISB = TRISB | 0x02;
   PORTB & 0x02;
   PORTB & 0x02;
   return(PORTB & 0x02);
}



void Pin23HIGH(void)
{
   // PORT B, Bit 2
   TRISB = TRISB & 0xFB;
   PORTB = PORTB | 0x04;
}

void Pin23LOW(void)
{
   //PORT B, Bit 2
   TRISB = TRISB & 0xFB;
   PORTB = PORTB & 0xFB;
}

unsigned char GetPin23(void)
{
   // Change PORT B , Bit 2 to Input
   TRISB = TRISB | 0x04;
   PORTB & 0x04;
   PORTB & 0x04; 
   return(PORTB & 0x04);
}



void Pin24HIGH(void)
{
   // PORT B, Bit 3
   TRISB = TRISB & 0xF7;
   PORTB = PORTB | 0x08;
}

void Pin24LOW(void)
{
   //PORT B, Bit 3
   TRISB = TRISB & 0xF7;
   PORTB = PORTB & 0xF7;
}

unsigned char GetPin24(void)
{
   // Change PORT B , Bit 3 to Input
   TRISB = TRISB | 0x08;
   PORTB & 0x08;
   PORTB & 0x08;
   return(PORTB & 0x08);
}





void Pin25HIGH(void)
{
   // PORT B, Bit 4
   TRISB = TRISB & 0xEF;
   PORTB = PORTB | 0x10;
}

void Pin25LOW(void)
{
   //PORT B, Bit 4
   TRISB = TRISB & 0xEF;
   PORTB = PORTB & 0xEF;
}

unsigned char GetPin25(void)
{
   // Change PORT B , Bit 4 to Input
   TRISB = TRISB | 0x10;
   PORTB & 0x10;
   PORTB & 0x10;
   return(PORTB & 0x10);
}




void Pin26HIGH(void)
{
   // PORT B, Bit 5
   TRISB = TRISB & 0xDF;
   PORTB = PORTB | 0x20;
}

void Pin26LOW(void)
{
   //PORT B, Bit 5
   TRISB = TRISB & 0xDF;
   PORTB = PORTB & 0xDF;
}

unsigned char GetPin26(void)
{
   // Change PORT B , Bit 5 to Input
   TRISB = TRISB | 0x20;
   PORTB & 0x20;
   PORTB & 0x20;
   return(PORTB & 0x20);
}



void Pin27HIGH(void)
{
   // PORT B, Bit 6
   TRISB = TRISB & 0xBF;
   PORTB = PORTB | 0x40;
}

void Pin27LOW(void)
{
   //PORT B, Bit 6
   TRISB = TRISB & 0xBF;
   PORTB = PORTB & 0xBF;
}

unsigned char GetPin27(void)
{
   // Change PORT B , Bit 6 to Input
   TRISB = TRISB | 0x40;
   PORTB & 0x40;
   PORTB & 0x40;
   return(PORTB & 0x40);
}




void Pin28HIGH(void)
{
   // PORT B, Bit 7
   TRISB = TRISB & 0x7F;
   PORTB = PORTB | 0x80;
}

void Pin28LOW(void)
{
   //PORT B, Bit 7
   TRISB = TRISB & 0x7F;
   PORTB = PORTB & 0x7F;
}

unsigned char GetPin28(void)
{
   // Change PORT B , Bit 7 to Input
   TRISB = TRISB | 0x80;
   PORTB & 0x80;
   PORTB & 0x80;
   return(PORTB & 0x80);
}



void PinHIGH(int Pin)
{
   switch ( Pin )
   {
      
      case 2:
         Pin2HIGH();
         break;
      case 3:
         Pin3HIGH();
         break;
      case 4:
         Pin4HIGH();
         break;
      case 5:
         Pin5HIGH();
         break;
      case 6:
         Pin6HIGH();
         break;
      case 7:
         Pin7HIGH();
         break;
      case 9:
         Pin9HIGH();
         break;
      case 10:
         Pin10HIGH();
         break;
      case 11:
         Pin11HIGH();
         break;
      case 12:
         Pin12HIGH();
         break;
      case 13:
         Pin13HIGH();
         break;
      case 14:
         Pin14HIGH();
         break;
      case 15:
         Pin15HIGH();
         break;
      case 16:
         Pin16HIGH();
         break;
      case 21:
         Pin21HIGH();
         break;
      case 22:
         Pin22HIGH();
         break;
      case 23:
         Pin23HIGH();
         break;
      case 24:
         Pin24HIGH();
         break;
      case 25:
         Pin25HIGH();
         break;
      case 26:
         Pin26HIGH();
         break;
      case 27:
         Pin27HIGH();
         break;
      case 28:
         Pin28HIGH();
         break;
   }



}



void PinLOW(int Pin)
{
   switch ( Pin )
   {
      case 2:
         Pin2LOW();
         break;
      case 3:
         Pin3LOW();
         break;
      case 4:
         Pin4LOW();
         break;
      case 5:
         Pin5LOW();
         break;
      case 6:
         Pin6LOW();
         break;
      case 7:
         Pin7LOW();
         break;
      case 9:
         Pin9LOW();
         break;
      case 10:
         Pin10LOW();
         break;
      case 11:
         Pin11LOW();
         break;
      case 12:
         Pin12LOW();
         break;
      case 13:
         Pin13LOW();
         break;
      case 14:
         Pin14LOW();
         break;
      case 15:
         Pin15LOW();
         break;
      case 16:
         Pin16LOW();
         break;
      case 21:
         Pin21LOW();
         break;
      case 22:
         Pin22LOW();
         break;
      case 23:
         Pin23LOW();
         break;
      case 24:
         Pin24LOW();
         break;
      case 25:
         Pin25LOW();
         break;
      case 26:
         Pin26LOW();
         break;
      case 27:
         Pin27LOW();
         break;
      case 28:
         Pin28LOW();
         break;
   }

}





int PinTest(int Port)
{
   switch ( Port )
   {
      case 2:
         return GetPin2();
         break;
      case 3:
         return GetPin3();
         break;
      case 4:
         return GetPin4();
         break;
      case 5:
         return GetPin5();
         break;
      case 6:
         return GetPin6();
         break;
      case 7:
         return GetPin7();
         break;
      case 9:
         return GetPin9();
         break;
      case 10:
         return GetPin10();
         break;
      case 11:
         return GetPin11();
         break;
      case 12:
         return GetPin12();
         break;
      case 13:
         return GetPin13();
         break;
      case 14:
         return GetPin14();
         break;
      case 15:
         return GetPin15();
         break;
      case 16:
         return GetPin16();
         break;
      case 21:
         return GetPin21();
         break;
      case 22:
         return GetPin22();
         break;
      case 23:
         return GetPin23();
         break;
      case 24:
         return GetPin24();
         break;
      case 25:
         return GetPin25();
         break;
      case 26:
         return GetPin26();
         break;
      case 27:
         return GetPin27();
         break;
      case 28:
         return GetPin28();
         break;

   }
}



/*

void Pause (int MS)
{

    ram int i;


    for (i=0; i<MS; i++)
    {

        Delay10KTCYx(1);
    }

}

*/

int abs (int i)
{
   return((i<0) ? -i : i);
}




