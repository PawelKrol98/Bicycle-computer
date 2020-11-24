/*
 * GccApplication4.c
 *
 * Created: 09.05.2020 22:43:08
 * Author : Paweł Król
 */ 
#define 	F_CPU   16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define E PORTD7
#define RS PORTD6
#define DATA PORTB
#define pi 3.1415

void WaitSubroutine1ms(int delay_in_ms) {
	for (volatile int i = delay_in_ms; i > 0; i--) {
		for (volatile int j = 8000; j > 0; j--)
		continue;
	}
}

void LCD_Initialization();                      // funkcja ta służy do zainicjowania wyświetlacza 2x16
void LCD_Command(unsigned char);            //funkcja służy do przekazania komendy do wyświetlacza, przykładowo 0x01 czyści ekran, 0x04 przesuwa kursor
void LCD_Char(unsigned char);            // funkcja ta wyświetla znak na wyświetlaczu LCD
void LCD_Text(char*);                         // funkcja wyświetla cały tekst na wyświetlaczu LCD
void Show_Distance(float);                                  //wyświetla dystans
void Show_Timer(float);                                  //wyświetla stoper
void Show_Clock(float);                                  //wyświetla zegar
void Show_Speed(float);                                  //wyświetla predkosc

int function_number = 0;       //tryb pracy komputera

ISR(INT1_vect)
{
	if (function_number<2) function_number += 1;
	else function_number =0;
	_delay_ms(500);
}




int main(void)
{
	DDRD = 0xE0;
	DDRB = 0xFF;
	PORTB = 0x00;
	PORTD = 0b00101100;
	_delay_ms(20);
	PORTD &= ~(1 << PORTD5);     // piknięcie przy włączeniu
	LCD_Initialization();
	float time = 0;
	float distance = 0;
	float speed = 0;
	
	EICRA = (1<<ISC11 | 1<<ISC10);
	EIMSK = (1<<INT1);
	sei();

	
    while (1) 
    {
		LCD_Command(0x01);  // czyszczenie ekranu
		LCD_Command(0x80);	// przesuniecie na pierwszą linię
		Show_Speed(speed);
		LCD_Command(0xC0);		// przesun na drugą linię
		switch(function_number){
			case 0:
			Show_Distance(distance);
			break;
			case 1:
			Show_Clock(time);
			break;
			case 2:
			Show_Timer(time);
			break;
		}
		
		_delay_ms(100);
    }
}



void LCD_Initialization(){
	LCD_Command(0x33);
	LCD_Command(0x32);	/* Send for 4 bit initialization of LCD  */
	LCD_Command(0x28);	/* 2 line, 5*7 matrix in 4-bit mode */
	LCD_Command(0x0c);	/* Display on cursor off */
	LCD_Command(0x06);	/* Increment cursor (shift cursor to right) */
	LCD_Command(0x01);
	_delay_ms(2);
}

void LCD_Command(unsigned char cmd){                      //funkcja służy do przekazania komendy do wyświetlacza, przykładowo 0x01 czyści ekran, 0x04 przesuwa kursor
	DATA = (DATA & 0xF0)| (cmd >> 4);/* Sending upper nibble */
	PORTD &= ~ (1<<RS);		/* RS=0, command reg. */
	PORTD |= (1<<E);		/* Enable pulse */
	_delay_us(1);
	PORTD &= ~ (1<<E);
	_delay_us(200);
	DATA = (DATA & 0xF0) | (cmd  & 0x0F);/* Sending lower nibble */
	PORTD |= (1<<E);
	_delay_us(1);
	PORTD  &= ~ (1<<E);
	_delay_ms(2);
}

void LCD_Char( unsigned char letter )
{
	DATA = (DATA & 0xF0) | (letter >> 4);/* Sending upper nibble */
	PORTD |= (1<<RS);  /* RS=1, data reg. */
	PORTD|= (1<<E);
	_delay_us(1);
	PORTD &= ~ (1<<E);
	_delay_us(200);
	DATA = (DATA & 0xF0) | (letter & 0x0F);  /* Sending lower nibble */
	PORTD |= (1<<E);
	_delay_us(1);
	PORTD &= ~ (1<<E);
	_delay_ms(2);
}

void LCD_Text (char *text)
{
	for(volatile int j=0;text[j]!=0 ;j++)
	{
		LCD_Char(text[j]);
	}
}

void Show_Distance(float distance){
	LCD_Text("Distance: ");
}                         
void Show_Timer(float time){
	LCD_Text("Time: ");
}                               
void Show_Clock(float time){
	LCD_Text("Clock: ");
}                                 
void Show_Speed(float speed){
	LCD_Text("Speed: ");
}