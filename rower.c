/*
 * GccApplication4.c
 *
 * Created: 09.05.2020 22:43:08
 * Author : Paweł Król & Michał Bogoń
 */ 

// definicja taktowania zegara
#define 	F_CPU   16000000UL
// Biblioteki
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h> 
// Definicje portów
#define E PORTD7
#define RS PORTD6
#define DATA PORTB
// Definicje stałych
#define pi 3.1415
#define R 0.35

// Prototypy funkcji
void LCD_Initialization();											// funkcja ta służy do zainicjowania wyświetlacza 2x16
void LCD_Command(unsigned char);									// funkcja służy do przekazania komendy do wyświetlacza, przykładowo 0x01 czyści ekran, 0x04 przesuwa kursor
void LCD_Char(unsigned char);										// funkcja ta wyświetla znak na wyświetlaczu LCD
void LCD_Text(char*);												// funkcja wyświetlająca cały tekst na wyświetlaczu LCD
void Show_Distance(float);											// funkcja wyświetlająca dystans
void Show_Timer(int, int, int);										// funkcja wyświetlająca stoper
void Show_Clock(int, int, int);										// funkcja wyświetlająca zegar
void Show_Speed();													// funkcja wyświetlająca predkość
bool TestPortBit(unsigned char);

ISR(INT1_vect);														
ISR(INT0_vect);
ISR(TIMER0_COMPA_vect);

// Zmienne globalne

float distance = 0;													// zmienna przechowujaca przejechany dystans
int function_number = 0;											// tryb pracy komputera
float magnes_touch = 0;												// variable used to measure distance

// zmienne używane do obliczania prędkości
float magnes_touch_vel = 0;
float delta_t = 0;
float velocity = 0;

// zmienne używane w zegarze
unsigned int hours = 0;
unsigned int minutes = 0;
unsigned int seconds = 0;
float timer = 0;
bool timer_PM = false;
bool timer_exceed = false;

// zmienne używane w stoperze
unsigned int hours_t = 0;
unsigned int minutes_t = 0;
unsigned int seconds_t = 0;
float timer_t = 0;
bool timer_PM_t = false;
bool timer_enable = false;
bool timer_exceed_t = false;


int main(void)
{
	DDRD = 0xE0;
	DDRB = 0xFF;
	PORTB = 0x00;
	PORTD = 0b00111100;
	
/*		Podłączenie portów ARDUINO UNO 328p
__________________________________________________________________________
						PORTD			  |			PORTB
	PIN(P)	0	1	2	3	4	5	6	7	8	9	10	11	12	13
	ARD	   PD0 PD1 PD2 PD3 PD4 PD5 PD6 PD7 PB0 PB1 PB2 PB3 PB4 PB5
	CONN	X	X kont tac tac buz  RS  E   D4  D5	D6	D7	X	X
	type	X	X  in  in  in  out  out out out out out out X	X
__________________________________________________________________________	
	key: 
		kont =kontaktron
		tac = tact switch
		buz = buzzer
		RS, E, D4, D5, D6, D7 = wyjścia do wyświetlacza LCD
		X = brak
		CONN = podłączenia
*/
	
	_delay_ms(20);
	PORTD &= ~(1 << PORTD5);															// piknięcie przy włączeniu
	LCD_Initialization();
	
	// Zmienne podczas ustawiania czasu
	int settings_val = 0;																// tryb pracy podczas ustawiania czasu
	bool settings = true;																// zmienna, ktora mowi o tym, czy ustawiamy zegar
	
	// Setting up time
	while(settings) {
		LCD_Command(0x01);																// czyszczenie ekranu
		Show_Clock(hours, minutes, seconds);
		LCD_Command(0xC0);																// przesun na druga linie
		switch(settings_val){
			case 0: 
				LCD_Text("Setting hours");                                            
				if (TestPortBit(1 << PIND4)) {											// jesli przycisk pod pinem 5 jest wcisniety, to zmienna ++
					hours++;
					if (hours > 23)
						hours = 0; 
					_delay_ms(500);
				}
				if (TestPortBit(1 << PIND3)) {
					settings_val++;
					_delay_ms(1000);
				}
				_delay_ms(100);
				break;
				
			case 1:
				LCD_Text("Setting minutes");
				if (TestPortBit(1 << PIND4)) {											// jesli przycisk pod pinem 5 jest wcisniety, to zmienna ++
					minutes++;
					if (minutes > 59)
						minutes = 0;
					_delay_ms(500);
				}
				if (TestPortBit(1 << PIND3)) {
					settings_val++;
					_delay_ms(1000);
				}
				_delay_ms(100);
				break;
			case 2:
				LCD_Text("Done?Y(P3)/N(P4)");
				if (TestPortBit(1 << PIND3)) {
					settings = false;
					_delay_ms(500);
				}
				if (TestPortBit(1 << PIND4)) {
					settings_val = 0;
					_delay_ms(500);
				}
				_delay_ms(100);
				break;
		}
		
	} 
	
	// Skalowanie zegara
	timer += (float)hours*3600;
	timer += (float)minutes*60;
	
	EICRA = (1<<ISC11 | 0<<ISC10);
	EIMSK = (1<<INT0);
	EIMSK |= (1<<INT1);
	
	 
	TCCR0A |= (1 << WGM01);
	OCR0A = 0xF9;												// przerwanie ma się włączać co 4ms, OCRn =  [ (czestotliwosc zegara / dzielnik) * Czas jaki chcemy otrzymać ] -1
	TIMSK0 |= (1 << OCIE0A);									// Włączam przerwanie czasowe
	sei();														

	TCCR0B |= (1 << CS02);										//dzielnik zegara ustawiony na 256
	
	
	//	MAIN PROGRAM
	
    while (1) 
    {
		LCD_Command(0x01);										// czyszczenie ekranu
		LCD_Command(0x80);										// przesuniecie na pierwszą linię
		Show_Speed();
		LCD_Command(0xC0);										// przesun na drugą linię
		switch(function_number){
			case 0:
				distance = magnes_touch * 2.0 * pi * R;
				Show_Distance(distance);
				break;
				
			case 1:
				/* 
				Ze względu na zasięg unsigned int postanowiliśmy stworzyć stuczną flagę "PM", gdybyśmy użyli zmiennej unsigned int bylibyśmy zmuszeni do przyjęcia limitu godziny 18:12:15. Nie wiemy dlaczego, ale konwersja na long int nie działała
			
				Postanowiliśmy więc stworzyć flagę PM. Sposób działania jest prosty: po 12 godzinach flaga PM włącza się i usuwa 43200 jednostek z timera (12godzin), zwiększając liczbę godzin o 12. Wiemy, że można to zrobić w znacznie lepszy sposób,
				jednak to rozwiązanie działa poprawnie, więc postanowiliśmy go nie zmieniać. Jednym ze sposobów, w jaki mogliśmy to zrobić, było użycie wskaźników, jednak naszym celem było uczynienie zegara funkcyjnego tak łatwym, jak to możliwe do zrozumienia.
				*/
				
				if(timer < 43200.0) {
					seconds = (unsigned int)timer;
					minutes = seconds / 60;
					hours = minutes / 60;
					if(timer_PM)
						hours += 12;
				}
				
				else {
					if (timer_PM)
						timer_exceed = true;
					timer -= 43200.0;
					seconds = (unsigned int)timer;
					minutes = seconds / 60;
					hours = minutes / 60;
					hours += 12;
					timer_PM = true;
				}
				
				if(timer_exceed) {
					timer = 0;
					seconds = 0;
					minutes = 0;
					hours = 0;
					timer_PM = false;
					timer_exceed = false;
				}
				
				Show_Clock(hours % 24, minutes % 60, seconds % 60);
				break;
				
			case 2:
				if(timer_t < 43200.0) {
					seconds_t = (unsigned int)timer_t;
					minutes_t = seconds_t / 60;
					hours_t = minutes_t / 60;
					if(timer_PM_t)
						hours_t += 12;
				}
				
				else {
					if (timer_PM_t)
						timer_exceed_t = true;
					timer_t -= 43200.0;
					seconds_t = (unsigned int)timer_t;
					minutes_t = seconds_t / 60;
					hours_t = minutes_t / 60;
					hours_t += 12;
					timer_PM_t = true;
				}
				
				if(hours_t > 23) {
					timer_t = 0;
					seconds_t = 0;
					minutes_t = 0;
					hours_t = 0;
					timer_PM_t = false;
					timer_exceed_t = false;
				}
				
				Show_Timer(hours_t % 24, minutes_t % 60, seconds_t % 60);
				break;
		}
		
		_delay_ms(125);
    }
}



void LCD_Initialization(){
	LCD_Command(0x33);
	LCD_Command(0x32);									// Ustawiamy tryb pracy wyświetlacza na 4-bitowy
	LCD_Command(0x28);									// dwie linie 2*
	LCD_Command(0x0c);									// wyłączenie kursora
	LCD_Command(0x06);									// inkrementacja kursora (w prawo)
	LCD_Command(0x01);									// wyczyszczenie ekranu
	_delay_ms(2);
}

void LCD_Command(unsigned char cmd){					//funkcja służy do przekazania komendy do wyświetlacza, przykładowo 0x01 czyści ekran, 0x04 przesuwa kursor
	DATA = (DATA & 0xF0)| (cmd >> 4);					// wysłanie wyzszej połowy bajtu
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

void Show_Distance(float distance){
	LCD_Text("Distance: ");
	char str[8];
	dtostrf(distance, 5,0, str);
	LCD_Text(str);
	LCD_Text("m");
}        
                 
void Show_Timer(int hours_t, int minutes_t, int seconds_t){
	
	char time_t[9];
	
	//Kwestia estetyki, nie jest to konieczne
	if (hours_t < 10) {
		if (minutes_t < 10) {
			if (seconds_t < 10) { sprintf(time_t, "0%d:0%d:0%d", hours_t, minutes_t, seconds_t); }
			else { sprintf(time_t, "0%d:0%d:%d", hours_t, minutes_t, seconds_t); }
		}
		else {
			if (seconds_t < 10) { sprintf(time_t, "0%d:%d:0%d", hours_t, minutes_t, seconds_t); }
			else { sprintf(time_t, "0%d:%d:%d", hours_t, minutes_t, seconds_t); }
		}
	}
	else {
		if (minutes_t < 10) {
			if (seconds_t < 10) { sprintf(time_t, "%d:0%d:0%d", hours_t, minutes_t, seconds_t); }
			else { sprintf(time_t, "%d:0%d:%d", hours_t, minutes_t, seconds_t); }
		}
		
		else {
			if (seconds_t < 10) { sprintf(time_t, "%d:%d:0%d", hours_t, minutes_t, seconds_t); }
			else { sprintf(time_t, "%d:%d:%d", hours_t, minutes_t, seconds_t); }
		}
	}
	
	LCD_Text("Time:  ");
	LCD_Text(time_t);
}        
                       
void Show_Clock(int hours, int minutes, int seconds){
	
	char time[9];
	
	//Kwestia estetyki, nie jest to konieczne
	if (hours < 10) {
		if (minutes < 10) {
			if (seconds < 10) { sprintf(time, "0%d:0%d:0%d", hours, minutes, seconds); }
			else { sprintf(time, "0%d:0%d:%d", hours, minutes, seconds); }
		}
		else {
			if (seconds < 10) { sprintf(time, "0%d:%d:0%d", hours, minutes, seconds); }
			else { sprintf(time, "0%d:%d:%d", hours, minutes, seconds); }
		}
	}
	else {
		if (minutes < 10) {
			if (seconds < 10) { sprintf(time, "%d:0%d:0%d", hours, minutes, seconds); }
			else { sprintf(time, "%d:0%d:%d", hours, minutes, seconds); }
		}
		
		else {
			if (seconds < 10) { sprintf(time, "%d:%d:0%d", hours, minutes, seconds); }
			else { sprintf(time, "%d:%d:%d", hours, minutes, seconds); }
		}
	}
	
	LCD_Text("Clock: ");
	LCD_Text(time);
}    
                             
void Show_Speed(){
	LCD_Text("Speed: ");
	int dt = (int)delta_t;
	if (dt == 3) {
		velocity = 2*pi*R*magnes_touch_vel/3*3.6;
		char str[8];
		dtostrf(velocity, 5,0, str);
		LCD_Text(str);
		LCD_Text("km/h");
		magnes_touch_vel = 0;
		delta_t = 0;
	}
	else {
		char str[8];
		dtostrf(velocity, 5,0, str);
		LCD_Text(str);
		LCD_Text("km/h");
	}
}

bool TestPortBit(unsigned char bit_number) {
	if (!(PIND & bit_number))
		return true;
	else
		return false;
}

ISR(INT1_vect)
{
	cli();
	if (function_number<2) function_number += 1;
	else function_number = 0;
	_delay_ms(200);
	
	timer += 0.2;									// kompensacja opóźnienia spowodowana funkcją _delay_ms
	if(timer_enable)
		timer_t += 0.2;
	
	sei();
}

ISR(INT0_vect)
{
	cli();
	magnes_touch++;
	magnes_touch_vel++;
	timer_enable = true;
	_delay_ms(100);
	
	timer += 0.1;									// kompensacja opóźnienia spowodowana funkcją _delay_ms
	if(timer_enable)
		timer_t += 0.1;
	
	sei();
}

ISR(TIMER0_COMPA_vect){
	timer += 0.004;
	delta_t += 0.004;
	if (timer_enable)
		timer_t += 0.004;
}
