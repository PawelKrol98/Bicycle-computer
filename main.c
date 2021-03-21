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
#include "LCD_Functions.c"
// Definicje stałych
#define pi 3.1415

// Struktura clock
typedef struct Clock{
	unsigned int hours;
	unsigned int minutes;
	unsigned int seconds;
} Clock;


// Prototypy funkcji
void Show_Distance(float);											// funkcja wyświetlająca dystans
void Show_Timer(Clock);												// funkcja wyświetlająca stoper
void Show_Clock(Clock);												// funkcja wyświetlająca zegar
void Show_Speed();													// funkcja wyświetlająca predkość
bool TestPortBit(unsigned char);

ISR(INT1_vect);														
ISR(INT0_vect);
ISR(TIMER0_COMPA_vect);


// Zmienne globalne

const float R = 0.35;
int function_number = 0;											// tryb pracy komputera
float magnes_touch = 0;												// variable used to measure distance

// zmienne używane do obliczania prędkości
float magnes_touch_vel = 0;
float delta_t = 0;
float velocity = 0;


// zmienne używane w zegarze
float timer = 0;


// zmienne używane w stoperze
float timer_t = 0;
bool timer_enable = false;



int main(void)
{
	DDRD = 0xE0;
	DDRB = 0xFF;
	PORTB = 0x00;
	PORTD = 0b00111100;
	
/*	      Podłączenie portów ARDUINO UNO 328p | Port connections ARDUINO UNO 328p
__________________________________________________________________________
					PORTD	  |		PORTB
	PIN(P)	0    1   2   3   4   5   6   7   8   9  10  11  12  13
	ARD    PD0  PD1 PD2 PD3 PD4 PD5 PD6 PD7 PB0 PB1 PB2 PB3 PB4 PB5
	CONN	X    X kont tac tac buz  RS  E   D4  D5	D6  D7	X   X
	type	X    X  in  in  in  out  out out out out out out X  X
__________________________________________________________________________	
	key: 
		kont =kontaktron (reed switch)
		tac = tact switch
		buz = buzzer
		RS, E, D4, D5, D6, D7 = wyjścia do wyświetlacza LCD (LCD screen outs)
		X = brak (nothing connected)
		CONN = podłączenia (connections)
*/

	
	_delay_ms(20);
	PORTD &= ~(1 << PORTD5);															// piknięcie przy włączeniu
	LCD_Initialization();
	
	// Zmienne podczas ustawiania czasu
	int settings_val = 0;																// tryb pracy podczas ustawiania czasu
	bool settings = true;																// zmienna, ktora mowi o tym, czy ustawiamy zegar
	
	
	
	bool timer_PM = false;
	bool timer_exceed = false;
	
	// zmienne używane w stoperze
	bool timer_PM_t = false;
	volatile bool timer_exceed_t = false;
	
	
	// zmienne zegara
	Clock zegar;
	zegar.hours = 0;
	zegar.minutes = 0;
	zegar.seconds = 0;
	
	Clock stoper;
	stoper.hours = 0;
	stoper.minutes = 0;
	stoper.seconds = 0;
	
	// Setting up time
	while(settings) {
		LCD_Clear();																	// Czysci ekran
		Show_Clock(zegar);
		LCD_Second_Line();																// Przesuwa tekst na druga linie
		switch(settings_val){
			case 0: 
				LCD_Text("Setting hours");                                            
				if (TestPortBit(1 << PIND4)) {											// jesli przycisk pod pinem 5 jest wcisniety, to zmienna ++
					zegar.hours++;
					if (zegar.hours > 23)
						zegar.hours = 0; 
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
					zegar.minutes++;
					if (zegar.minutes > 59)
						zegar.minutes = 0;
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
	timer += (float)zegar.hours*3600;
	timer += (float)zegar.minutes*60;
	
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
		LCD_Clear();
		LCD_First_Line();
		Show_Speed();
		LCD_Second_Line();
		
		float distance;													// zmienna przechowujaca przejechany dystans
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
					zegar.seconds = (unsigned int)timer;
					zegar.minutes = zegar.seconds / 60;
					zegar.hours = zegar.minutes / 60;
					if(timer_PM)
						zegar.hours += 12;
				}
				
				else {
					if (timer_PM)
						timer_exceed = true;
					timer -= 43200.0;
					zegar.seconds = (unsigned int)timer;
					zegar.minutes = zegar.seconds / 60;
					zegar.hours = zegar.minutes / 60;
					zegar.hours += 12;
					timer_PM = true;
				}
				
				if(timer_exceed) {
					timer = 0;
					zegar.seconds = 0;
					zegar.minutes = 0;
					zegar.hours = 0;
					timer_PM = false;
					timer_exceed = false;
				}
				
				Show_Clock(zegar);
				break;
				
			case 2:
				if(timer_t < 43200.0) {
					stoper.seconds = (unsigned int)timer_t;
					stoper.minutes = stoper.seconds / 60;
					stoper.hours = stoper.minutes / 60;
					if(timer_PM_t)
						stoper.hours += 12;
				}
				
				else {
					if (timer_PM_t)
						timer_exceed_t = true;
					timer_t -= 43200.0;
					stoper.seconds = (unsigned int)timer_t;
					stoper.minutes = stoper.seconds / 60;
					stoper.hours = stoper.minutes / 60;
					stoper.hours += 12;
					timer_PM_t = true;
				}
				
				if(stoper.hours > 23) {
					timer_t = 0;
					stoper.seconds = 0;
					stoper.minutes = 0;
					stoper.hours = 0;
					timer_PM_t = false;
					timer_exceed_t = false;
				}
				
				Show_Timer(stoper);
				break;
		}
		
		_delay_ms(125);
    }
}

void Show_Distance(float distance){
	LCD_Text("Distance: ");
	char str[8];
	dtostrf(distance, 5,0, str);
	LCD_Text(str);
	LCD_Text("m");
}        
                 
void Show_Timer(Clock stoper){
	unsigned int hours_t = stoper.hours;
	unsigned int minutes_t = stoper.minutes % 60;
	unsigned int seconds_t = stoper.seconds % 60;
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
                       
void Show_Clock(Clock zegar){
	unsigned int hours = zegar.hours;
	unsigned int minutes = zegar.minutes % 60;
	unsigned int seconds = zegar.seconds % 60;
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
