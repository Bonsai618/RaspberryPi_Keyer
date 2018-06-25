                       
#pragma config  FOSC=INTIO2, FCMEN=OFF, IESO=OFF, PWRTEN = OFF, BOREN=OFF,  WDTEN=OFF, MCLRE=ON,   XINST = OFF, PLLCFG = OFF

ram int                   MS;
ram unsigned long         ShowTimeMS; 
ram unsigned long         Timer1MS;
ram char                  WorkBuffer[256];

#define LED_ON  Pin15HIGH();  
#define LED_OFF Pin15LOW(); 
#define KEY_ON  Pin3HIGH();
#define KEY_OFF Pin3LOW();

// For testing ports
#define ALL_ON  Pin2HIGH(); Pin3HIGH(); Pin4HIGH(); Pin5HIGH(); Pin9HIGH(); Pin10HIGH(); Pin11HIGH(); Pin12HIGH(); Pin13HIGH(); Pin14HIGH(); Pin15HIGH();
#define ALL_OFF Pin2LOW();  Pin3LOW();  Pin4LOW();  Pin5LOW();  Pin9LOW();  Pin10LOW();  Pin11LOW();  Pin12LOW();  Pin13LOW();  Pin14LOW();  Pin15LOW(); 

#include "string.h"

#include "cranwellP18F25K80.h"     /* NOTE: Includes fixes for string and mem functions and all uart functions */
#include "usart.h"
#include "spi.h"
#include "timers.h"
#include "EEP.h"
#define In 2
#define Out 1
#define None 0
extern unsigned int volatile pMsg ;
extern unsigned char volatile Buffer1[256];
extern char volatile MsgReady;
extern unsigned int volatile nQueue;
extern unsigned int volatile QHead;
extern unsigned int volatile QTail;
extern char volatile QBuff[];
extern unsigned int volatile QReset;
extern unsigned int volatile Semaphore;

int ClockCtr;


void SetTiming(int speed);
void SendCode(char *DitDahs);
int Receive(char *Mesg);
int Rec(char *Mesg);
int Farnsworth = 1;

char CodeBuffer[256];


int TRC = 0;
int RecResult;
unsigned int i, nChar;
unsigned int zero = 0;

char *p, *p2 ;

int Speed = 20;		// Default Speed in WPM
float  Ta , Tc, Tw;
int DitTime, DahTime, CharTime, WordTime;
char MyBuff[256];
int MyCt;



void main(void)
{


    

    // Set Digital ports 
   
    TRISA = 0;
    ANCON0 = 0x00;
    PORTA = 0;
    
    

    // Set Internal 8 MHZ Clock
    SetClockSpeed(8);
    //CloseSPI();
    
    KEY_OFF   // Relay
    LED_OFF   // LED

    


  
    OpenUSART1pic18f25K80(38400);      // Hardware USART 1

	// Calculate timing 

	SetTiming(Speed);
    nQueue = 0;
    QHead = 0;
    QTail = 0;
    MyCt = 0;
    Semaphore = 0;


    // Show startup 1 sec flash
   

       // Show Active
	   LED_ON    
       DelayMS(1000);
       LED_OFF    
          
    

    while(1)
	{
        nChar = Rec(CodeBuffer);
        p2 = CodeBuffer;
       
        if(nChar > 0)
        { 
           for(i=0; i<nChar; i++)
           {
                
              if(QReset == 1)
              {
                 QReset = 0;
                 // Show reset
                 LED_ON
                 DelayMS(500);
                 LED_OFF
                 goto out;
              }
    

             
              SendCode(p2);
            
              p2++;
            } 
out:
           
            MyCt = 0; 
			memset(CodeBuffer,'\0',sizeof(CodeBuffer));
           	  	
        }
    
        
	}
}





int Rec(char *Mesg)
{

   int i;
   char volatile *p;

	   if (MyCt == 0 && nQueue > 0)
       {   
           MyCt = nQueue;
           p = QBuff;
           memcpy((char *)Mesg,(char *)p, MyCt);
           nQueue = 0;    
           return(MyCt);        		
       }
     	return(0);
    
}

void SendCode(char *p)
{
	
		if (*p ==  '.')
		{
			KEY_ON
            LED_ON 
			Pin15HIGH();
			DelayMS(DitTime);
            KEY_OFF
			LED_OFF
			DelayMS(DitTime);
            return;
            
		}

		if (*p == '-')
		{
            KEY_ON
			LED_ON
			DelayMS(DahTime);
            KEY_OFF
			LED_OFF
			DelayMS(DitTime);
            return;
            
			
		}


         if(*p == ' ')
         {
            // Character Spacing
            DelayMS(CharTime);
            return;
         } 

         if(*p == 'W')
         {  // Word Spacing
		    DelayMS(WordTime);
            return;
         }

         if(*p == 'U')
         {
            // Increase Speed
			Speed ++;
			SetTiming(Speed);
            return;
         }

         if (*p == 'D')
         {
            // Decrease Speed
            Speed --;
            if(Speed < 1)  Speed = 1;
            SetTiming(Speed);
            return;
         }

        

         if (*p == 'F')
         {
            // Set Farnsworth Mode
           if (Farnsworth == 0)
           {
				Farnsworth = 1;
           }
           else
           {
                Farnsworth = 0;
           }

           SetTiming(Speed); 
           return;
         }


}

void SetTiming(int speed)
{

// speeds below 19 WPM use Farnsworth timing


	Tc = 1.2 / speed;
	Tw = Tc * 7;
     
	DitTime = Tc * 1000;
	DahTime = DitTime * 3;
    CharTime = DahTime;
	WordTime = DitTime * 7;

    if(Farnsworth == 1)
    {
	   if (speed < 19)
	   {
		   Ta = 1080 - (37.2 * speed);
           Ta = Ta  / (18 * speed);
		   Tc = Ta  * 3 / 19;
	  	   Tw = Ta * 7 / 19;
		   DitTime = 66;
		   DahTime = DitTime * 3;
		   CharTime = Tc * 1000;
		   WordTime = Tw * 1000;
	   }
    }
}





