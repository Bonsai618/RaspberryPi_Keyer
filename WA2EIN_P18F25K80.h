#define RAM static ram
//#define BYTE unsigned char
//#define BIT unsigned char
#define TRUE 1
#define FALSE 0
#define WORD unsigned int
//#define NIBBLE unsigned char
#define BIT unsigned char

// Define Software UART Speed  (Trace)
#define BPS38400

// Define processor clock speed
#define CLOCK8MHZ


#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "delays.h"
#include <pconfig.h>
#include "timers.h"
#include "p18cxxx.h"
#include "Myusart.h"

 void Pause (int MS);

   extern char volatile MsgReady;
   extern unsigned char volatile Buffer[121]; 
   extern unsigned int volatile pMsg ;
   extern unsigned int volatile QHead;
   extern unsigned int volatile QTail;
   extern unsigned int volatile QReset;
   extern char volatile QBuff[];
   extern unsigned int volatile nQueue;
   extern unsigned int volatile Semaphore;

   ram unsigned char volatile CUart;
   ram unsigned char volatile *uptr;
   ram unsigned long volatile TimerPopCount;

   char *p;

   RAM int volatile DataLen;
   RAM int FSR0H_TEMP;
   RAM int FSR0L_TEMP;
   RAM int FSR1H_TEMP;
   RAM int FSR1L_TEMP;
   RAM int UARTErrorOR;
   RAM int UARTErrorFR;

void MyHighISR (void);

void Pin4LOW(void);
void Pin4HIGH(void);

#pragma code HighVector=0x08
void HighVector (void)
{
   _asm
   movff FSR1L,FSR1L_TEMP
   movff FSR1H,FSR1H_TEMP
   movff FSR0L,FSR0L_TEMP
   movff FSR0H,FSR0H_TEMP
   goto MyHighISR
   _endasm
 }

#pragma code

// Use Low Priority interrupt to prevent compiler from using retfile FAST.
// This is a workaround for a bug in the PIC hardware.  See errata sheet.

#pragma interruptlow MyHighISR

void MyHighISR (void)
{
 

 if(PIR1bits.RCIF)  // If RCIF is set
{
  
  

   USART_Status.val &= 0xf0;         // Clear previous status flags
     

 
     CUart= (unsigned char) RCREG;

     if (CUart == 'X')
     {
        QReset = 1;
		memset((char *)QBuff,'\0',512);
        nQueue = 0;
        Reset();
        goto exit;
      }

     
      p = (QBuff + nQueue);
      *p = CUart;
      nQueue ++;
      goto exit;
   
}

// Check for Timer1 overflow
    if (INTCONbits.TMR0IF)
    {
        
      
       TMR0H = 0x06;
       TMR0L = 0x30;
       INTCONbits.TMR0IF = 0;
       TimerPopCount += 1;
       goto exit2;
     }
    exit:  
     PIR1bits.RCIF = 0;
    exit2:

   
   _asm
  
  
   movff FSR0H_TEMP,FSR0H
   movff FSR0L_TEMP,FSR0L
   movff FSR1L_TEMP,FSR1L
   movff FSR1H_TEMP,FSR1H
     
    retfie 1 
  _endasm

  
}



//===================================================
// Fixes for Software
//===================================================


// Fix for strncpy
char* strncpy (char *s1, const char *s2, size_t n)
{
    char *s;
    for (s=s1; 0 < n && *s2!= '\0'; --n)
        *s++ = *s2++;
    for (;0<n; --n)
        *s++ = '\0';
    return(s1);
}



// Fix for strncmp

signed char strncmp (const char *s1, const char *s2, size_t n)
{

    for (; 0<n; ++s1, ++s2, --n)
        if (*s1 != *s2)
            return((*(unsigned char *) s1
                    < *(unsigned char *)s2) ? -1: +1);
        else if (*s1 == '\0')
            return(0);
    return(0);
}



// fix for memcpy
void *(memcpy) (void *s1, const void *s2, size_t n)
{
   char *su1;
   const char *su2;

   for(su1 = s1, su2 = s2; 0<n; ++su1, ++su2, --n)
   {
      *su1 = *su2;
      
   }
return(s1);
}








          void SetClockSpeed(unsigned int speed) {

          if (speed == 8){
          OSCTUNE = 0x00;
          OSCCON =  0x72;  // Set internal 8Mhz clock
          }

          if (speed == 1){
          OSCTUNE = 0x00;
          OSCCON =  0x42;  // Set internal 1Mhz clock
          }

          if (speed == 40){
          OSCTUNE = 0x00;
          OSCCON =  0x00;  // Set External clock}
          }
        }


          void DelayMS(unsigned long MS){

         // Callibrated for 8MHZ Clock
          unsigned long  i;

          for (i = 0 ; i<MS; i++){
          Delay10TCYx(196);
          ClrWdt()
          }
          }




// Configuration for PIC18F2525

          void Pin3HIGH(void){
          // Port A, Bit 1

          TRISA = TRISA & 0xFD;
          LATA = LATA | 0x02;
          PORTA = PORTA | 0x02;}

          void Pin3LOW(void){
          //PORT A, Bit 1
          TRISA = TRISA & 0xFD;
          LATA = LATA & 0xFD;
          PORTA = PORTA & 0xFD;}

          BIT GetPin3(void){
          // Change PORT A , Bit 1 to Input
          TRISA = TRISA | 0x02;
          PORTA & 0x02;
          PORTA & 0x02; 
          return(PORTA & 0x02);}


          void Pin4HIGH(void){
          // Port A, Bit 2
          TRISA = TRISA & 0xFB;
          LATA = LATA | 0x04;
          PORTA = PORTA | 0x04;}

          void Pin4LOW(void){
          //PORT A, Bit 2
          TRISA = TRISA & 0xFB;
          LATA = LATA & 0xFB;
          PORTA = PORTA & 0xFB;}

          BIT GetPin4(void){
          // Change PORT A , Bit 2 to Input
          TRISA = TRISA | 0x04;
          PORTA & 0x04;
          PORTA & 0x04;
          return(PORTA & 0x04);}



          void Pin5HIGH(void){
          // Port A, Bit 3
          TRISA = TRISA & 0xF7;
          LATA = LATA | 0x08;
          PORTA = PORTA | 0x08;}

          void Pin5LOW(void){
          //PORT A, Bit 3
          TRISA = TRISA & 0xF7;
          LATA = LATA & 0xF7;
          PORTA = PORTA & 0xF7;}

          BIT GetPin5(void){
          // Change PORT A , Bit 3 to Input
          TRISA = TRISA | 0x08;
          PORTA & 0x08;
          PORTA & 0x08; 
          return(PORTA & 0x08);}

          void Pin6HIGH(void){
          // Port A, Bit 4
          TRISA = TRISA & 0xEF;
          LATA = LATA | 0x10;
          PORTA = PORTA | 0x10;}

          void Pin6LOW(void){
          //PORT A, Bit 4
          TRISA = TRISA & 0xEF;
          LATA = LATA & 0xEF;
          PORTA = PORTA & 0xEF;}

          BIT GetPin6(void){
          // Change PORT A , Bit 4 to Input
          TRISA = TRISA | 0x10;
          PORTA & 0x10;
          PORTA & 0x10;
          return(PORTA & 0x10);}

          void Pin7HIGH(void){
          // Port A, Bit 5
          TRISA = TRISA & 0xDF;
          PORTA = PORTA | 0x20;}

          void Pin7LOW(void){
          //PORT A, Bit 5
          TRISA = TRISA & 0xDF;
          PORTA = PORTA & 0xDF;}

          BIT GetPin7(void){
          // Change PORT A , Bit 5 to Input
          TRISA = TRISA | 0x20;
          PORTA & 0x20;
          PORTA & 0x20; 
          return(PORTA & 0x20);}





          void Pin9HIGH(void){
          // Port A, Bit 7
          TRISA = TRISA & 0x7F;
          PORTA = PORTA | 0x80;}

          void Pin9LOW(void){
          //PORT A, Bit 7
          TRISA = TRISA & 0x7F;
          PORTA = PORTA & 0x7F;}

          BIT GetPin9(void){
          // Change PORT A , Bit 7 to Input
          TRISA = TRISA | 0x80;
          PORTA & 0x80;
          PORTA & 0x80;
          return(PORTA & 0x80);}





          void Pin10HIGH(void){
          // Port A, Bit 6
          TRISA = TRISA & 0xBF;
          PORTA = PORTA | 0x40;}

          void Pin10LOW(void){
          //PORT A, Bit 6
          TRISA = TRISA & 0xBF;
          PORTA = PORTA & 0xBF;}

          BIT GetPin10(void){
          // Change PORT A , Bit 6 to Input
          TRISA = TRISA | 0x40;
          PORTA & 0x40;
          PORTA & 0x40; 
          return(PORTA & 0x40);}





          void Pin11HIGH(void){
          // Port C, Bit0 
          TRISC = TRISC & 0xFE;
          PORTC = PORTC | 0x01;}

          void Pin11LOW(void){
          //PORT C, Bit 0
          TRISC = TRISC & 0xFE;
          PORTC = PORTC & 0xFE;}

          BIT GetPin11(void){
          // Change PORT C , Bit 0 to Input
          TRISC = TRISC | 0x01;
          PORTC & 0x01;
          PORTC & 0x01;
          return(PORTC & 0x01);}





          void Pin12HIGH(void){
          // PORT C, Bit 1

          TRISC = TRISC & 0xFD;
          PORTC = PORTC | 0x02;}

          void Pin12LOW(void){
          //PORT C, Bit 1
          TRISC = TRISC & 0xFD;
          PORTC = PORTC & 0xFD;}

          BIT GetPin12(void){
          // Change PORT C , Bit 1 to Input
          TRISC = TRISC | 0x02;
          PORTC & 0x02;
          PORTC & 0x02;
          return(PORTC & 0x02);}




          void Pin13HIGH(void){
          // PORT C, Bit 2
          TRISC = TRISC & 0xFB;
          PORTC = PORTC | 0x04;}

          void Pin13LOW(void){
          //PORT C, Bit 2
          TRISC = TRISC & 0xFB;
          PORTC = PORTC & 0xFB;}

          BIT GetPin13(void){
          // Change PORT C , Bit 2 to Input
          TRISC = TRISC | 0x04;
          PORTC & 0x04;
          PORTC & 0x04;
          return(PORTC & 0x04);}





          void Pin14HIGH(void){
          // PORT C, Bit 3
          TRISC = TRISC & 0xF7;
          PORTC = PORTC | 0x08;}

          void Pin14LOW(void){
          //PORT C, Bit 3
          TRISC = TRISC & 0xF7;
          PORTC = PORTC & 0xF7;}

          BIT GetPin14(void){
          // Change PORT C , Bit 3 to Input
          TRISC = TRISC | 0x08;
          PORTC & 0x08;
          PORTC & 0x08;
          return(PORTC & 0x08);}




          void Pin15HIGH(void){
          // PORT C, Bit 4
          TRISC = TRISC & 0xEF;
          PORTC = PORTC | 0x10;}

          void Pin15LOW(void){
          //PORT C, Bit 4
          TRISC = TRISC & 0xEF;
          PORTC = PORTC & 0xEF;}

          BIT GetPin15(void){
          // Change PORT C , Bit 4 to Input
          TRISC = TRISC | 0x10;
          PORTC & 0x10;
          PORTC & 0x10;
          return(PORTC & 0x10);}









          void Pin16HIGH(void){
          // PORT C, Bit 5
          TRISC = TRISC & 0xDF;
          PORTC = PORTC | 0x20;}

          void Pin16LOW(void){
          //PORT C, Bit 5
          TRISC = TRISC & 0xDF;
          PORTC = PORTC & 0xDF;}

          BIT GetPin16(void){
          // Change PORT C , Bit 5 to Input
          TRISC = TRISC | 0x20;
          PORTC & 0x20;
          PORTC & 0x20; 
          return(PORTC & 0x20);}




          void Pin21HIGH(void){
          // PORT B, Bit0 
          TRISB = TRISB & 0xFE;
          PORTB = PORTB | 0x01;}

          void Pin21LOW(void){
          //PORT B, Bit 0
          TRISB = TRISB & 0xFE;
          PORTB = PORTB & 0xFE;}

          BIT GetPin21(void){
          // Bhange PORT B , Bit 0 to Input
          TRISB = TRISB | 0x01;
          PORTB & 0x01;
          PORTB & 0x01;
          return(PORTB & 0x01);}





          void Pin22HIGH(void){
          // PORT B, Bit 1

          TRISB = TRISB & 0xFD;
          PORTB = PORTB | 0x02;}

          void Pin22LOW(void){
          //PORT B, Bit 1
          TRISB = TRISB & 0xFD;
          PORTB = PORTB & 0xFD;}

          BIT GetPin22(void){
          // Change PORT B , Bit 1 to Input
          TRISB = TRISB | 0x02;
          PORTB & 0x02;
          PORTB & 0x02;
          return(PORTB & 0x02);}



          void Pin23HIGH(void){
          // PORT B, Bit 2
          TRISB = TRISB & 0xFB;
          PORTB = PORTB | 0x04;}

          void Pin23LOW(void){
          //PORT B, Bit 2
          TRISB = TRISB & 0xFB;
          PORTB = PORTB & 0xFB;}

          BIT GetPin23(void){
          // Change PORT B , Bit 2 to Input
          TRISB = TRISB | 0x04;
          PORTB & 0x04;
          PORTB & 0x04; 
          return(PORTB & 0x04);}



          void Pin24HIGH(void){
          // PORT B, Bit 3
          TRISB = TRISB & 0xF7;
          PORTB = PORTB | 0x08;}

          void Pin24LOW(void){
          //PORT B, Bit 3
          TRISB = TRISB & 0xF7;
          PORTB = PORTB & 0xF7;}

          BIT GetPin24(void){
          // Change PORT B , Bit 3 to Input
          TRISB = TRISB | 0x08;
          PORTB & 0x08;
          PORTB & 0x08;
          return(PORTB & 0x08);}





          void Pin25HIGH(void){
          // PORT B, Bit 4
          TRISB = TRISB & 0xEF;
          PORTB = PORTB | 0x10;}

          void Pin25LOW(void){
          //PORT B, Bit 4
          TRISB = TRISB & 0xEF;
          PORTB = PORTB & 0xEF;}

          BIT GetPin25(void){
          // Change PORT B , Bit 4 to Input
          TRISB = TRISB | 0x10;
          PORTB & 0x10;
          PORTB & 0x10;
          return(PORTB & 0x10);}




          void Pin26HIGH(void){
          // PORT B, Bit 5
          TRISB = TRISB & 0xDF;
          PORTB = PORTB | 0x20;}

          void Pin26LOW(void){
          //PORT B, Bit 5
          TRISB = TRISB & 0xDF;
          PORTB = PORTB & 0xDF;}

          BIT GetPin26(void){
          // Change PORT B , Bit 5 to Input
          TRISB = TRISB | 0x20;
          PORTB & 0x20;
          PORTB & 0x20;
          return(PORTB & 0x20);}



          void Pin27HIGH(void){
          // PORT B, Bit 6
          TRISB = TRISB & 0xBF;
          PORTB = PORTB | 0x40;}

          void Pin27LOW(void){
          //PORT B, Bit 6
          TRISB = TRISB & 0xBF;
          PORTB = PORTB & 0xBF;}

          BIT GetPin27(void){
          // Change PORT B , Bit 6 to Input
          TRISB = TRISB | 0x40;
          PORTB & 0x40;
          PORTB & 0x40;
          return(PORTB & 0x40);}




          void Pin28HIGH(void){
          // PORT B, Bit 7
          TRISB = TRISB & 0x7F;
          PORTB = PORTB | 0x80;}

          void Pin28LOW(void){
          //PORT B, Bit 7
          TRISB = TRISB & 0x7F;
          PORTB = PORTB & 0x7F;}

          BIT GetPin28(void){
          // Change PORT B , Bit 7 to Input
          TRISB = TRISB | 0x80;
          PORTB & 0x80;
          PORTB & 0x80;
          return(PORTB & 0x80);}



          void PinHIGH(int Pin){
          switch (Pin){

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



          void PinLOW(int Pin){
          switch (Pin){case 3:
          Pin3LOW();
          break;case 4:
          Pin4LOW();
          break;case 5:
          Pin5LOW();
          break;case 6:
          Pin6LOW();
          break; case 7:
          Pin7LOW();
          break;case 9:
          Pin9LOW();
          break;case 10:
          Pin10LOW();
          break; case 11:
          Pin11LOW();
          break;case 12:
          Pin12LOW();
          break;case 13:
          Pin13LOW();
          break; case 14:
          Pin14LOW();
          break; case 15:
          Pin15LOW();
          break;case 16:
          Pin16LOW();
          break;case 21:
          Pin21LOW();
          break;case 22:
          Pin22LOW();
          break;case 23:
          Pin23LOW();
          break;case 24:
          Pin24LOW();
          break; case 25:
          Pin25LOW();
          break;case 26:
          Pin26LOW();
          break;case 27:
          Pin27LOW();
          break;case 28:
          Pin28LOW();
          break;}

}





          int PinTest(int Port){
          switch (Port){case 3:
          return GetPin3();
          break;case 4:
          return GetPin4();
          break;case 5:
          return GetPin5();
          break;case 6:
          return GetPin6();
          break; case 7:
          return GetPin7();
          break;case 9:
          return GetPin9();
          break;case 10:
          return GetPin10();
          break;case 11:
          return GetPin11();
          break;case 12:
          return GetPin12();
          break;case 13:
          return GetPin13();
          break; case 14:
          return GetPin14();
          break; case 15:
          return GetPin15();
          break;case 16:
          return GetPin16();
          break;case 21:
          return GetPin21();
          break;case 22:
          return GetPin22();
          break;case 23:
          return GetPin23();
          break;case 24:
          return GetPin24();
          break;case 25:
          return GetPin25();
          break;case 26:
          return GetPin26();
          break;case 27:
          return GetPin27();
          break;case 28:
          return GetPin28();
          break;

}}





          void Pause (int MS){
          // Assumes a 40 Mhz clck

          Delay10KTCYx(MS);

}

          int abs (int i){
          return((i<0) ? -i : i);}




