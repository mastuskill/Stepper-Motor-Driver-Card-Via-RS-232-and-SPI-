//Author: C.Seguna
//Date Created: 13/03/2015
//Description: LCD Functions

#include <LPC21XX.H>

#define  shift      16
#define  LCD_D0     (1 << (0)) 	// D0 LCD data pin Px.x connected 
#define  LCD_D1     (1 << (1))  // D1 LCD data pin Px.x connected 
#define  LCD_D2     (1 << (2))	// D2 LCD data pin Px.x connected  
#define  LCD_D3     (1 << (3))	// D3 LCD data pin Px.x connected 
#define  LCD_D4     (1 << (4))	// D4 LCD data pin Px.x connected 
#define  LCD_D5     (1 << (5))	// D5 LCD data pin Px.x connected 
#define  LCD_D6     (1 << (6))	// D6 LCD data pin Px.x connected  
#define  LCD_D7     (1 << (7))	// D7 LCD data pin Px.x connected 

#define  LCD_EN     (1 << 20) 	// LCD EN pin  connected 
#define  LCD_RS     (1 << 21) 	// LCD RS pin  connected 

#define  LCD_data_4Bit   (LCD_D7|LCD_D6|LCD_D5|LCD_D4)
#define  LCD_data_8Bit   (LCD_D7|LCD_D6|LCD_D5|LCD_D4|LCD_D3|LCD_D2|LCD_D1|LCD_D0)

long int val; 
void LCDSendData(unsigned char data);
void LCDSendCmd(unsigned char cmd);
void LCD_Init_4_Bit(void);
void LCD_Write_String (const char *string);

// RS = 1 
void SetRS(void){ IOSET1 |= LCD_RS; }
	 				
// RS = 0 
void ClrRS(void){ IOCLR1 |= LCD_RS; }


//EN=1
void SetEN(void){ IOSET1 |= LCD_EN; }

// EN = 0 
void ClrEN(void){ IOCLR1 |= LCD_EN; }




void wait(long int delay)
{
     long int count = 0;
	 
	 while(count<delay)
	 {
	    count++;
	 }
}

void LCD_Init_4_Bit(void)
{
  ClrRS();
  ClrEN();
  wait(5000);	     

  LCDSendCmd(0x33); // not needed??? function set
  LCDSendCmd(0x32);// not needed???
  LCDSendCmd(0x28);//function set
  LCDSendCmd(0xC);//Display on/off and curcor blink off
  LCDSendCmd(0x06);//character entry mode
  LCDSendCmd(0x01); //clear display
  LCDSendCmd(0x80); //display address 1st column 1st row
}

// Send data to LCD

void LCDSendData(unsigned char data)
{

    val = data & 0xF0;
    val &= (LCD_data_4Bit) ;  //Higher Nibble
	  IOCLR1 = (0xFF << shift);	// clear 8-Bit Pin Data
	  IOSET1 = val << (shift-4);// Shift data by 24 to right
	  SetRS();			    		    // RS = 1
	  SetEN();						      // EN = 1, Strobe Signal  
    ClrEN();						      // EN = 0 	   
	  wait(50000); 

		val = (data & 0x0F);
		val &= (LCD_data_4Bit >> 4); //Lower Nibble
		IOCLR1 = (0xFF << shift);	  	 // clear 8-Bit Pin Data
		IOSET1 = val << (shift)   ;	   // Shift data by 24 to right
		SetRS();// RS = 1
		SetEN();// EN = 1, Strobe Signal  
		ClrEN();// EN = 0 	   
		wait(50000); 
}



// Send Command to LCD

void LCDSendCmd(unsigned char cmd)
{
   	val = cmd & 0xF0;
    val &= (LCD_data_4Bit) ;//Higher Nibble
		IOCLR1 = (0xFF << shift);	//clear 8-Bit Pin Data
		IOSET1 = val << (shift-4);//Shift data by 24 to right
		ClrRS();// RS = 0
		SetEN();// EN = 1, Strobe Signal  
		ClrEN();// EN = 0 	   
		wait(50000); 

		val = (cmd & 0x0F);
		val &= (LCD_data_4Bit >> 4) ; //Lower Nibble
		IOCLR1 = (0xFF << shift);	  	// clear 8-Bit Pin Data
		IOSET1 = val << shift   ;	  	// Shift data by 24 to right
		ClrRS();// RS = 0
		SetEN();// EN = 1, Strobe Signal  
		ClrEN();// EN = 0 
		wait(50000); 
}
void LCD_Write_String (const char *string)
{
	while(*string)
		LCDSendData(*string++);
}

