#include <RTL.h>
#include <LPC21xx.h>
#include "Lcd_Utility.h"

#define CR     		0x0D				//hex value of carriage return
#define BACKSPACE 0x08				//backspace ascii value
#define EN1 			0x00800000  //Enable pin M1
#define EN2 			0x02000000  //Enable pin M2
#define DIR 			0x01400000  //Direction pin of motors
#define LEDM1 		0x00000400  //LED of Motor 1
#define LEDM2 		0x00000800  //LED of Motor 2
#define BUZZER 		0x0001000   //BUZZER pin
#define UARTLED1 	0x10000000	//UART LED 1
#define UARTLED2 	0x08000000  //UART LED 2
#define STEPS 		0x60000000	//Motor clocks	
#define estop 		0x00008000	//Emergeny stop button pin
#define SPI_OK 		0 					// transfer ended No Errors
#define SPI_BUSY 	1 					// transfer busy
#define SPI_ERROR 2						// transfer ended with Errors

void uart_init (void);
void Timer0_init(void);
void SPI_Init(void);
void TST_Button (void) __irq;
void Estop (void) __irq;
/* Global Variables */
unsigned char instruction[20];
int cnt = 0, i = 0;
int dummy = 0;
int flagrun = 0,flaglcd = 0,flagang = 0,flagebuzzer = 0, flagstop = 0;
int step = 0;
int tst = 0;
int MT1 = 0, MT2 = 0;
int angle1 = 0, angle2 = 0;
int steps1 = 0, steps2 = 0;
int hundreds1 = 0, tens1 = 0, units1 = 0,tenths1 = 0,hundreths1 = 0;
int hundreds2 = 0, tens2 = 0, units2 = 0,tenths2 = 0,hundreths2 = 0;
int HZ = 0,BZ = 0;
long int ps1 = 0, ps2 = 0;
float ang1 = 0, ang2 = 0;

static unsigned char state;      // State of SPI driver
static unsigned char spiBuf[4];  // SPI data buffer
static unsigned char *msg;       // pointer to SPI data buffer
static unsigned char count;      // nr of bytes send/received
static unsigned int data;        // data of spi as int
static U64 stk1[10000/8];

typedef struct {
	unsigned char val;/* Message object structure*/
  unsigned int counter;          /* variable counter of type unsigned int*/
} T_COUNT;

/* RTX tasks IDs and Names*/
OS_TID tsk1;
OS_TID tsk2;
OS_TID tsk3;
OS_TID tsk4;
OS_TID tsk5;
OS_TID tsk6;

__task void INIT_TASKS (void);
__task void TIMER (void);
__task void send_task (void);
__task void rec_task (void);
__task void LCD (void);
__task void delay (void);

os_mbx_declare (MsgBox,16);           	  /*Declare an RTX mailbox*/
_declare_box (mpool,sizeof(T_COUNT),16);	/*Dynamic memory pool*/
/*RTX tasks*/
__task void rec_task (void) {
  T_COUNT *rptr;

  while(1) 
	{
    os_mbx_wait (&MsgBox, (void **)&rptr, 0xffff); /*wait for the message*/
    
		if(rptr->counter == 1) 
		{
	    	os_evt_set(0x0001,tsk2);
			  os_dly_wait(2);
			  rptr->counter = 0;
		}
		_free_box (mpool, rptr);           /*free memory allocated for message*/
  }
}

__task void send_task (void) {
  
	T_COUNT *mptr;
  os_mbx_init (&MsgBox, sizeof(MsgBox)); /* initialize the mailbox*/

	while(1)
	{
		mptr = _alloc_box (mpool);           /* Prepare a 1st message*/
		os_dly_wait (50);											
		mptr->counter = 1;
		os_mbx_send (&MsgBox, mptr, 0xffff);
		os_dly_wait(2);
		mptr->counter = 0;
	}
}

__task void INIT_TASKS (void)
{  
	 //initialization of all tasks then self destructs
	 tsk1 = os_tsk_self (); 
   os_tsk_prio_self(1);
	 tsk2 = os_tsk_create(TIMER,1);
	 tsk3 = os_tsk_create(send_task,1); 
	 tsk4 = os_tsk_create(rec_task,1); 
	 tsk5 = os_tsk_create_user(LCD, 1, &stk1, sizeof(stk1));
	 tsk6 = os_tsk_create(delay,1); 
   os_tsk_delete_self();
}
__task void delay (void)
{
	while(1)
	{
		//delay so RTX is always busy else it crashes
		os_dly_wait (151);	
	}
}
__task void TIMER (void)
{
   while(1)
	 {
		 //Timer of 500ms done using event flags and mailbox
		 //This task handles the buzzer in case estop is pressed as well as the LEDs
		 //and estop counter if pressed for more than 5 sec reset
		 os_evt_wait_and(0x0001,0xFFFF);
		 if (flagebuzzer == 1)
		 {
		   BZ++;
		 }
		 HZ++;	

			if (EN1 == (IOPIN0 & EN1) && HZ == 5)
			{
				IOSET0 = LEDM1;
			}
			if (EN2 == (IOPIN0 & EN2) && HZ == 5)
			{
				IOSET0 = LEDM2;
			}
			if (EN1 == (IOPIN0 & EN1) && HZ == 10)
			{
				IOCLR0 = LEDM1;
			}
			if (EN2 == (IOPIN0 & EN2) && HZ == 10)
			{
				IOCLR0 = LEDM2;
			}
			if (HZ == 10)
			{HZ = 0;}
		
			if (cnt >= 10 && ((IOPIN0 & estop) != 0))
			{
				flagstop = 0;
				flagrun = 0;
				flaglcd = 0;
				PINSEL0 = 0xA0001505;
				cnt = 0;	
				LCDSendCmd(1);
				
			}
			if(cnt >= 10 && ((IOPIN0 & estop) == 0))
			{
				IOCLR0 = UARTLED2;
			}
			else{ IOSET0 = UARTLED2;}
	
			if(flagstop == -1 && ((IOPIN0 & estop) == 0))
			{
				cnt++;
			}
	 
			else if((flagstop == -1 && ((IOPIN0 & estop) != 0)))
			{
				cnt = 0;
			}

			if (flagrun != -1) flagang = 1;
			// Spi is read every 500ms and timer of timer0 is set accordingaly in various ranges
			// note that due to the timer being changed it needed to be stopped and reset 
			// everytime a change is noticed since if it is not stopped and reset the counter 
			// would overflow and crash the program
			
			spiBuf[0] = 0x06; 						//START bit and Single ended mode
			spiBuf[1] = 0x00;							//Channel 0
			msg = spiBuf;
			count = 3; 										// nr of bytes
			state = SPI_BUSY; 						// Status of driver
			IOCLR0 = 0x00000080;
			S0SPDR = *msg; 								// sent first byte
			while (state == SPI_BUSY){}		// wait for end of transfer
			
			if (state == SPI_OK) 					// no error ?
			{
				data = (spiBuf[1]<<8) | spiBuf[2];
				data = data & 0x00000FFF;
				T0TCR = 2;
				if (data > 0&& data < 1000)
				{
					T0MR0 = 4910 * 1.5 ; 	      
				}
				else if (data > 1000&& data < 1500)
				{
					T0MR0 = 4910 * 1.3;
				}
				else if (data > 1500&& data < 2000)
				{
					T0MR0 = 4910 * 1.1; 
				}
				else if (data > 2000&& data < 2500)
				{
					T0MR0 = 4910 * 0.9; 
				}
				else if (data > 2500&& data < 3000)
				{
					T0MR0 = 4910 * 0.7; 
				}
				else if (data > 3000&& data < 3500)
				{
					T0MR0 = 4910 * 0.5; 
				}
				else if (data > 3500&& data < 4100)
				{
					T0MR0 = 4910 * 0.3; 
				}
				T0TCR = 1;
			}
			//reset flag
		os_evt_set(0x0000,tsk2);
	}	
}
__task void LCD (void)
{
		while(1)
	{
		//All LCD commands can be found in this task simple flags
		//are set in order to activate the equivalent if statements
		//angle is also reset every 500ms which updates also the angle
		//displayed on the LCD
		os_dly_wait (10);
		if (flaglcd == -1 && flagrun == -1)
		{
			LCDSendCmd(1);
			LCDSendCmd(0x80);
			LCD_Write_String("System Disabled ");
			flaglcd = 0;
		}
		
		if (flaglcd == 1 && flagrun == 1)
		{
      LCDSendCmd(0x83);
			LCDSendData('R');
      LCDSendCmd(0xC3);
			LCDSendData('R');
			flaglcd = 0; 
		}
		
		if (flaglcd == 2 && flagrun == 2)
		{
      LCDSendCmd(0x83);
			LCDSendData('S');
			flaglcd = 0; 
		}
		
		if (flaglcd == 3 && flagrun == 3)
		{
      LCDSendCmd(0xC3);
			LCDSendData('S');
			flaglcd=0;
		}
		
		if (flaglcd == 6 && flagrun == 6)
		{
			LCDSendCmd (0x84);
			LCDSendData('A');
			LCDSendCmd (0xC4);
			LCDSendData('A');
			flaglcd = 0;
		}
		if (flaglcd == 7 && flagrun == 7)
		{
			LCDSendCmd (0x84);
			LCDSendData('C');
			LCDSendCmd (0xC4);
			LCDSendData('C');
			flaglcd = 0;
		}
		if (flagang == 1 && (flagrun != 2 || flagrun != 3  || flagrun != -1))
		{
			
			LCDSendCmd(0x80);
			LCDSendData('M');
			LCDSendData('1');
			LCDSendData(':');
			LCDSendCmd(0xC0);			
			LCDSendData('M');
			LCDSendData('2');
			LCDSendData(':');
			
			ang1 = steps1;
			ang1 = ang1 * 1.8 / 16 /2;
			if(ang1 < 0)
			{ang1 = ang1+360;}
			
			hundreds1 = ang1/100;
			ang1 = ang1 -(hundreds1 * 100);
			tens1 = ang1/10;
			ang1 = ang1 -(tens1 * 10);
			units1 = ang1/1;
			ang1 = ang1 -(units1 * 1);
			tenths1 = ang1/0.1;
			ang1 = ang1 -(tenths1 * 0.1);
			hundreths1 = ang1/0.01;
			ang1 = ang1 -(hundreths1 * 0.01);
			
			ang2 = steps2;
			ang2 = ang2 * 1.8 / 16 / 2;
			if(ang2 < 0)
			{ang2 = ang2+360;}
			
			hundreds2 = ang2/100;
			ang2 = ang2 -(hundreds2 * 100);
			tens2 = ang2/10;
			ang2 = ang2 -(tens2 * 10);
			units2 = ang2/1;
			ang2 = ang2 -(units2 * 1);
			tenths2 = ang2/0.1;
			ang2 = ang2 -(tenths2 * 0.1);
			hundreths2 = ang2/0.01;
			ang2 = ang2 -(hundreths2 * 0.01);
			
			LCDSendCmd (0x86);
			LCDSendData(hundreds1+48);
			LCDSendData(tens1+48);
			LCDSendData(units1+48);
			LCDSendData('.');
			LCDSendData(tenths1+48);
			LCDSendData(hundreths1+48);
			
			LCDSendCmd (0xC6);
			LCDSendData(hundreds2+48);
			LCDSendData(tens2+48);
			LCDSendData(units2+48);
			LCDSendData('.');
			LCDSendData(tenths2+48);
			LCDSendData(hundreths2+48);
			flagang=0;			
		}
	}
}
void uart (void) __irq
{
	// UART each time a key is pressed and LED is Set either high or low to
  // indicate the key being read. Apart from that UART was programmed to 
	// work even if mid command the user uses backspace. Depending on the
	// command the equivalent if statement is activated which sets the equivalent
	// flags
	ps1 = IOPIN0 & UARTLED1;
	
	if (ps1 == UARTLED1)
	{
		IOCLR0 =UARTLED1;
	}
	else
	{
		IOSET0 =UARTLED1;
	}

  instruction[i] = U0RBR;
	
	if (instruction[i] == BACKSPACE)
		{
			i = i-2;
		}

	if (flagrun == -1){}
	else
	{
	  if(flagstop == 0)
		{
			if (instruction[i] == CR)
			{
				if (instruction[0] == 'R')
				{
					flagrun = 1;
					flaglcd = 1;
				}			
				if (instruction[0] == '1' && instruction[2] == 'S')
				{
					flagrun = 2;
					flaglcd = 2;
				}
				if (instruction[0] == '2' && instruction[2] == 'S')
				{
					flagrun = 3;
					flaglcd = 3;
				}
				if (instruction[0] == '1' && instruction[2] != 'S')
				{
					flagrun = 4;
					flaglcd = 4;
					step = (instruction[2]-48)*160;
				}
				if (instruction[0] == '2' && instruction[2] != 'S')
				{
					flagrun = 5;
					flaglcd = 5;
					step = (instruction[2]-48)*160;
				}
				if (instruction[0] == 'A')
				{
					IOSET0 = DIR;
					flagrun = 6;
					flaglcd = 6;
				}
				if (instruction[0] == 'C')
				{
					IOCLR0 = DIR;
					flagrun = 7;
					flaglcd = 7;
				}
				if (instruction[0] == 'R' && instruction[1] == 'H' && instruction[3] == 'S' && instruction[4] == '1')
				{
					flagrun = 8;
					flaglcd = 8;
					step = (MT1 * 6400) + steps1;
				}
				if (instruction[0] == 'R' && instruction[1] == 'H' && instruction[3] == 'S' && instruction[4] == '2')
				{
					flagrun = 9;
					flaglcd = 9;
					step = (MT2 * 6400) + steps2;
				}	
			i = 0;
			}
	  
			else
			{
				i++;
			}
		}
	}
	dummy = U0IIR;
  VICVectAddr = 0x00000000;	// Dummy write to signal end of interrupt	
}

void Timer0(void) __irq
{
	//the speed of this interrupt timer is set by the spi
	//this takes care of the PWM outputs as well as records
  //each step the motors take in order to be able to use the RH command later on
	if (steps1 == 6400) {steps1 = 0;MT1++;}
	if (steps1 == -6400) {steps1 = 0;MT1--;}
	if (steps2 == 6400) {steps2 = 0;MT2++;}
	if (steps2 == -6400) {steps2 = 0;MT2--;}
	
	ps1 = IOPIN0 & STEPS;
	if (flagrun == -1)
	{
	}
	else
	{
		if(flagrun == 1)
		{
			IOCLR0 = EN1 | EN2;
			if ((IOPIN0 & DIR) == DIR)
			{
				steps1--;
				steps2--;
			}
			else
			{
				steps1++;
				steps2++;
			}
			IOSET0 = LEDM1 | LEDM2;
		}
		if (flagrun == 2)
		{
			IOSET0 = EN1; //Disabling EN1	
			if ((IOPIN0 & DIR) == DIR && (IOPIN0 & EN2)  == 0)
			{
				steps2--;
			}
			else if((IOPIN0 & DIR) == 0 && (IOPIN0 & EN2)  == 0)
			{
				steps2++;
			}
		}
		if (flagrun == 3)
		{
			IOSET0 = EN2; //Disabling EN2	
			if ((IOPIN0 & DIR) == DIR && (IOPIN0 & EN1)  == 0)
			{
				steps1--;
			}
			else if ((IOPIN0 & DIR) == 0 && (IOPIN0 & EN1)  == 0)
			{
				steps1++;
			}
		}
	
		if (flagrun == 4)
		{
			IOCLR0 = 0X20000000;
			IOCLR0 = EN1; //Enabling MOTOR 1
			IOSET0 = LEDM1;
			if (step > 0)
			{
				step--;
				if ((IOPIN0 & DIR) == DIR)
				{
					steps1--;
				}
				else if ((IOPIN0 & DIR) == 0)
				{
					steps1++; 
				}	
			}
			else
			{
				IOSET0 = EN1;
				step = 0;
			}
		}
		if (flagrun == 5)
		{
			IOCLR0 = 0x40000000;
			IOCLR0 = EN2;
			IOSET0 = LEDM2;
			if (step > 0)
			{
				step--;
				if ((IOPIN0 & DIR) == DIR)
				{
					steps2--;
				}
				else if ((IOPIN0 & DIR) == 0)
				{
					steps2++; 
				}
			}
			else
			{
				IOSET0 = EN2;
				step = 0;
			}
		}
		if (flagrun == 6)
		{
			if ((IOPIN0 & EN1) != EN1)
			{steps1--;}
			if ((IOPIN0 & EN2)!= EN2)
			{steps2--;}
		}
		if (flagrun == 7)
		{
			if ((IOPIN0 & EN1) != EN1)
			{steps1++;}
			if ((IOPIN0 & EN2) != EN2)
			{steps2++;}
		}
		if (flagrun == 8)
		{
			IOCLR0 = 0X20000000;
			IOCLR0 = EN1; //Enabling MOTOR 1
			if (step > 0)
			{
				step--;
				IOSET0 = DIR;
			}
			else if( step < 0)
			{
				step++;
				IOCLR0 = DIR;
			}
			else if( step == 0)
			{
				IOSET0 = EN1;
				step = 0;
				steps1 = 0;
				MT1 = 0;
				if((IOPIN0 & DIR) == DIR)
				{
					IOCLR0 = DIR;
				}
				else if ((IOPIN0 & DIR) != DIR)
				{
					IOSET0 = DIR;
				}
			}
		}
		if (flagrun == 9)
			{
				IOCLR0 = 0X40000000;
				IOCLR0 = EN2; //Enabling MOTOR 1
				if (step > 0)
				{
					step--;
					IOSET0 = DIR;
				}
				else if( step < 0)
				{
					step++;
					IOCLR0 = DIR;
				}
				else if( step == 0)
				{
					IOSET0 = EN2;
					step = 0;
					steps2 = 0;
					MT2 = 0;
					if((IOPIN0 & DIR) == DIR)
					{
						IOCLR0 = DIR;
					}
					else if ((IOPIN0 & DIR) != DIR)
					{
						IOSET0 = DIR;
					}
				}
			}
		}
		if(flagrun == 10)
		{
		IOCLR0 = EN1 | EN2;
		if (tst > 0)
		{
			tst--;
			if ((IOPIN0 & DIR) == DIR)
			{
				steps1--;
				steps2--;
			}
			else if ((IOPIN0 & DIR) == 0)
			{
				steps1++;
				steps2++;
			}
		}
		else
		{
			IOSET0 = EN2 | EN1;
			tst=0;
		}
	}
	if (ps1 == 0)
	{IOSET0 = STEPS;}
	else {IOCLR0 = STEPS;}
	//buzzer set if estop pressed
	if (flagebuzzer == 1 && ((IOPIN0 & BUZZER) == BUZZER))
	{IOCLR0 = BUZZER;}
  
	else if (flagebuzzer == 1 && ((IOPIN0 & BUZZER) != BUZZER))
	{IOSET0 = BUZZER;}
	//reseting buzzer flag after 2 sec (4 since 500ms timer)
	if (BZ == 4)
	{
		BZ=0;
		flagebuzzer =0 ;
	}
	
	T0IR = 1;
	VICVectAddr = 0x00000000;	// Dummy write to signal end of interrupt
}
void SPI_Isr(void) __irq
{
	if ((S0SPSR & 0xF8) == 0x80)
	{
		*msg++ = S0SPDR; // read byte from slave
		if (--count > 0)
		S0SPDR = *msg; // sent next byte
		else
		{
			state = SPI_OK; // transfer completed
			IOSET0 = 0x00000080;
		}
	}
	else // SPI error
	{
		*msg = S0SPDR; // dummy read to clear flags
		state = SPI_ERROR;
	}
	S0SPINT = 0x01; // reset interrupt flag
	VICVectAddr = 0; // reset VIC
}

void Estop (void) __irq
{
	IOSET0 = EN1 | EN2;
  flagrun = -1;
	flaglcd = -1;
	flagebuzzer = 1;
	flagstop = -1;
	PINSEL0 = 0X00000000;
	
	EXTINT=4;   //clearning the external interrupt flag 0 bit in EINT0 register
  VICVectAddr=0x00;	
}
void TST_Button (void) __irq
{
	flagrun = 10;
	flaglcd = 10;
	tst = 1;
	EXTINT=1;   //clearning the external interrupt flag 0 bit in EINT0 register
  VICVectAddr=0x00;
}

int main(void)
{
  PINSEL0 = 0xA0000005;   //Initialise P0.14 as EINT1 and P0.15 as EINT2, UART 0.0, 0.1
	PINSEL1 = 0x00000001;   //Initialise P0.16 as EINT0
	PINSEL2 = 0;																				// SET as GPIO
	IODIR0 = 0xFFFE3FFF;																// SET as OUTPUT
	IODIR1 = 0xFFFFFFFF;																// SET as OUTPUT
	
	VICVectCntl3= 32 | 16; //channel 16 is for ext int1 enable bit 5 for highest priority
  VICVectCntl4= 32 | 14;
	VICVectCntl5= 32 | 15;
	VICVectAddr3= (unsigned long)Estop;
  VICVectAddr4= (unsigned long)TST_Button;
	
	uart_init();
	SPI_Init();
	LCD_Init_4_Bit();	
	Timer0_init();
  VICIntEnable |= 0x0001C050;   //// change accordingaly
	
	IOSET0 = 0x18000000;
	IOSET0 = EN1 | EN2;
	IOSET0 = LEDM1 | LEDM2;
	IOCLR0 = DIR;
	
	_init_box (mpool, sizeof(mpool),    /* initialize the 'mpool' memory for   */
  sizeof(T_COUNT));        					  /* the membox dynamic allocation       */
	os_sys_init(INIT_TASKS);
}

void uart_init (void)
{	
	VICIntSelect=(0x00000000);  //CLASSIFIES INTERRUPTS AS EITHER FIQ or IRQ.Put a 1 for FIQ or 0 for IRQ.
  U0IER = 1;	
	
	VICVectCntl1 = 32 | 6;
	VICVectAddr1 = (unsigned long)uart;
	VICIntEnable= 0x00000050 ; 	//Enabling channels 14-16 and 6
	
	PINSEL0 &= 0xFFFFFFF0;			// Reset P0.0,P0.1 Pin Config
  PINSEL0 |= 0x00000001;			// Select P0.0 = TxD(UART0)
  PINSEL0 |= 0x00000004;			// Select P0.1 = RxD(UART0)
															// one character is written.Trigger level can be adjusted to 4,8,14 characters.
  U0LCR &= 0xFC;							// Reset Word Select(1:0) //Line control register
  U0LCR |= 0x03;							// Data Bit = 8 Bit
  U0LCR &= 0xFB;							// Stop Bit = 1 Bit
  U0LCR &= 0xF7;							// Parity = Disable
  U0LCR &= 0xBF;							// Disable Break Control. Unix-like systems can use the long "break" level 
															// as a request to change the signaling rate,
															// to support dial-in access at multiple signaling rates.
  U0LCR |= 0x80;							// Enable Programming of Divisor Latches
															// U0DLM:U0DLL = 29.915MHz / [16 x Baud]
															//             = 29.915MHz / [16 x 9600]
															//             = 0x00C0
  U0DLM = 0x00;								// Program Divisor Latch(32) for 9600 Baud
  U0DLL = 0xC0;								//	0x60 for 9600bps
  U0LCR &= 0x7F;							// Disable Programming of Divisor Latches
  U0FCR |= 0x01;							// FIF0 Enable
  U0FCR |= 0x02;							// RX FIFO Reset so to clear bytes from UART0 RX FIFO Register
  U0FCR |= 0x04;							// TX FIFO Reset so to clear bytes from UART0 TX FIFO Register
  U0FCR &= 0x3F;       	  		// Configuring trigger level.i.e.Interrupt is generated after 
}

void Timer0_init(void)
{
	VPBDIV = 0x02;														// Clock same as CPU freq
	VICVectCntl0 = 32 | 4;										// Enable Timer0 Channel Interrupt
	VICVectAddr0 = (unsigned long) Timer0;	  // IRQ Address
	VICIntEnable = 0x00000070;								// Enable Timer 0 Interrupt
	// Resonator of 8MHz --> PLL x 7 --> 56MHz
	// For 1 mS at 56MHz -> 1/56Mhz = 17.8571nS, then multiply by 3 because of pre-scaler = 53.5714nS = tick time
	// 1mS / 53.5714nS = 18666.67 ticks
	T0PR = 2;																	// Prescaler of 3
	T0MCR = 3;   				  										// Interrupt on MR0
	T0MR0 = 4910; 	      										// 1 ms delay
	T0TCR = 1;   				 									 		// Enable Counter
}

void SPI_Init(void)
{
	VICVectAddr2 = (unsigned int) &SPI_Isr;
	VICVectCntl2 = 32 | 10; 	// Channel0 on Source#10 ... enabled
	VICIntEnable |= 0x400; 		// 10th bit is the SPI
	IODIR0 |= 0x00000080; 		// P0.7 defined as SLAVE PIN CONFIGURATION
	IOSET0 |= 0x00000080; 		//CLEAR SLAVE PIN
	PINSEL0 |= 0x00001500; 		// configure SPI0 pins (except SSEL0)
	S0SPCCR = 26; 						// SCK = 1 MHz, counter > 8 and even
	S0SPCR = 0xA0; 						// CPHA=0, CPOL=0, master mode, MSB first, interrupt enabled A0 or 11 B8
}
