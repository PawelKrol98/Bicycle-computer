#ifndef _LCD_Functions_c
#define _LCD_Functions_c

// Definicje portów
#define E PORTD7
#define RS PORTD6
#define DATA PORTB

#include "LCD_Functions.h"


// LCD_Command								"Wiersz polecen wyswietlacza"
// LCD_Initialization							Inicjalizuje ekran
// LCD_Char								Wyswietla pojedynczy znak							
// LCD_Text								Wyswietla tablice znakow
// LCD_Clear								Czyści ekran
// LCD_First_Line							Przesuwa tekst na pierwsza linijke wyswietlacza
// LCD_Second_Line							Przesuwa tekst na druga linijke wyswietlacza

void LCD_Command(unsigned char cmd){					//funkcja służy do przekazania komendy do wyświetlacza, przykładowo 0x01 czyści ekran, 0x04 przesuwa kursor
	DATA = (DATA & 0xF0)| (cmd >> 4);				// wysłanie wyzszej połowy bajtu
	PORTD &= ~ (1<<RS);
	PORTD |= (1<<E);
	_delay_us(1);
	PORTD &= ~ (1<<E);
	_delay_us(200);
	DATA = (DATA & 0xF0) | (cmd  & 0x0F);				// wysłanie niższej połowy bajtu
	PORTD |= (1<<E);
	_delay_us(1);
	PORTD  &= ~ (1<<E);
	_delay_ms(2);
}

void LCD_Initialization(){
	LCD_Command(0x33);
	LCD_Command(0x32);						// Ustawiamy tryb pracy wyświetlacza na 4-bitowy
	LCD_Command(0x28);						// dwie linie 2*
	LCD_Command(0x0c);						// wyłączenie kursora
	LCD_Command(0x06);						// inkrementacja kursora (w prawo)
	LCD_Command(0x01);						// wyczyszczenie ekranu
	_delay_ms(2);
}

void LCD_Char( unsigned char letter )
{
	DATA = (DATA & 0xF0) | (letter >> 4);				// wysłanie wyzszej połowy bajtu
	PORTD |= (1<<RS);  
	PORTD|= (1<<E);
	_delay_us(1);
	PORTD &= ~ (1<<E);
	_delay_us(200);
	DATA = (DATA & 0xF0) | (letter & 0x0F);				// wysłanie niższej połowy bajtu
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

void LCD_Clear () {
	LCD_Command(0x01);						// czyszczenie ekranu
}

void LCD_First_Line() {
	LCD_Command(0x80);						// przesuniecie na pierwszą linię
}

void LCD_Second_Line() {
	LCD_Command(0xC0);						// przesun na drugą linię
}

#endif	/* _LCD_Functions.c */