/* Code for a Slot Machine Controller*/
#include <stdio.h>
#include <Arduino.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "HD44780.hpp"
#include "uart.hpp"
#include "libADC.hpp"

#define UP 1
#define DOWN 2
#define	LEFT 3
#define RIGHT 4
#define SELECT 5
#define NONE 0

#define REEL_SYMBOLS 10
#define TOTAL_REEL_SYMBOL_ROWS (REEL_SYMBOLS  * 8)
const byte reel[TOTAL_REEL_SYMBOL_ROWS] =
		{

				//Heart
				0B00000, 0B01010, 0B11111, 0B11111, 0B11111, 0B01110, 0B00100,
				0B00000,

				//Diamond
				0B00000, 0B00100, 0B01110, 0B11111, 0B11111, 0B01110, 0B00100,
				0B00000,

				//Spade
				0B00000, 0B00100, 0B01110, 0B11111, 0B11111, 0B00100, 0B01110,
				0B00000,

				//Club
				0B00000, 0B01110, 0B01110, 0B11111, 0B11111, 0B00100, 0B01110,
				0B00000,

				//7
				0B00000, 0B11111, 0B10001, 0B11101, 0B01010, 0B10100, 0B11100,
				0B00000,

				//#
				0B00000, 0B00000, 0B01010, 0B11111, 0B01010, 0B11111, 0B01010,
				0B00000,

				//3 Bars
				0B00000, 0B00000, 0B11111, 0B00000, 0B11111, 0B00000, 0B11111,
				0B00000,

				//2 Bars
				0B00000, 0B00000, 0B11111, 0B00000, 0B00000, 0B11111, 0B00000,
				0B00000,

				//1 Bar
				0B00000, 0B00000, 0B11111, 0B10001, 0B11111, 0B00000, 0B00000,
				0B00000,

				//Face
				0B00000, 0B11011, 0B11011, 0B00000, 0B10001, 0B01110, 0B00000,
				0B00000, };

char buf[7];
int num1 = 0;
int num2 = 0;
int num3 = 0;
byte wheel1[8];
byte wheel2[8];
byte wheel3[8];
int cash = 1000;
int bet = 5;
int raw = 0;

void CreateCustomCharacter(unsigned char *Pattern, const char Location) {
	LCD_WriteCommand(0x40 + (Location * 8));
	for (int i = 0; i < 8; i++)
		LCD_WriteData(Pattern[i]);
}

int read_LCD_buttons() {
	while (1) {
		ADCSRA |= (1 << ADSC);
		while (ADCSRA & (1 << ADSC))
			;
		raw = ADC;

		if ((100 < raw) && (raw < 250)) {
			return UP;
		}
		if ((250 < raw) && (raw < 350)) {
			return DOWN;
		}

		if ((0 >= raw) && (raw < 100)) {
			return RIGHT;
		}

		if ((420 < raw) && (raw < 550)) {
			return LEFT;
		}

		if ((600 < raw) && (900 > raw)) {
			return SELECT;
		}
		return NONE;
	}

}

int rollReel(byte wheel[8], int x, int y, int pos) {

	boolean stop = false;
	int row = 0;

	while (!stop) {
		for (int i = 0; i < 8; i++) {
			wheel[i] = reel[(row + i) % TOTAL_REEL_SYMBOL_ROWS];
		}

		CreateCustomCharacter(wheel, pos);
		LCD_GoTo(x, y);
		LCD_WriteData(pos);

		row = (row + 1) % TOTAL_REEL_SYMBOL_ROWS;

		if ((read_LCD_buttons() == UP) && (row % 8 == 0)) {
			stop = true;
			for (int i = 0; i < 8; i++) {
				wheel[i] = reel[(row + i) % TOTAL_REEL_SYMBOL_ROWS];
			}
			CreateCustomCharacter(wheel, pos);
			LCD_GoTo(x, y);
			LCD_WriteData(pos);

		}
		_delay_ms(10);
	}

	return row / 8;
}


void rolls() {

	LCD_GoTo(0, 1);
	LCD_WriteText("Spin!");

	num1 = rollReel(wheel1, 2, 0, 0);
	_delay_ms(400);

	num2 = rollReel(wheel2, 7, 0, 1);
	_delay_ms(400);

	num3 = rollReel(wheel2, 12, 0, 2);
	_delay_ms(400);
}

void prizes() {

	cash -= bet;
	if ((num1 == num2) && (num2 == num3) && (num3 == num1)) {
		int jackWin = num1 * 100;
		cash += jackWin;
		sprintf(buf,
				"Jackpot! (%d %d %d). 3 numbers are equal: %d*100 = %d$\n\r",
				num1, num2, num3, num1, num1 * 100);
		uart_send_string(buf);
		sprintf(buf, "Balance: %d\n\r", cash);
		uart_send_string(buf);
		LCD_GoTo(0, 1);
		LCD_WriteText("JACKPOT!");
		LCD_GoTo(10, 1);
		sprintf(buf, "$%u", cash);
		LCD_WriteText("     ");
		LCD_GoTo(10, 1);
		LCD_WriteText(buf);

	} else if (((num1 == num2) && (num2 != num3))
			|| ((num3 == num1) && (num1 != num2))
			|| ((num2 == num3) && (num3 != num1))) {
		if ((num1 == num2) || (num1 == num3)) {
			cash += num1 * bet;
			sprintf(buf,"You Won! (%d %d %d). 2 numbers are equal: You won %d$x5 = %d$\n\r",
					num1, num2, num3, num1, bet*num1);
					uart_send_string(buf);
		}
		if (num2 == num3) {
			cash += num2 * bet;
			sprintf(buf,"You Won! (%d %d %d). 2 numbers are equal: You won %d$x5 = %d$\n\r",
					num1, num2, num3, num2, bet*num2);
					uart_send_string(buf);
		}

		sprintf(buf, "Balance: %d\n\r", cash);
		uart_send_string(buf);
		LCD_GoTo(0, 1);
		LCD_WriteText("WIN!");
		LCD_GoTo(10, 1);
		sprintf(buf, "$%u", cash);
		LCD_WriteText("     ");
		LCD_GoTo(10, 1);
		LCD_WriteText(buf);

	} else {
		sprintf(buf,
				"Miss! (%d %d %d). None of them are equal, better luck next time!\n\r",
				num1, num2, num3);
		uart_send_string(buf);
		sprintf(buf, "Balance: %d\n\r", cash);
		uart_send_string(buf);
		LCD_GoTo(0, 1);
		LCD_WriteText("MISS :(");
		LCD_GoTo(10, 1);
		sprintf(buf, "$%u", cash);
		LCD_WriteText("     ");
		LCD_GoTo(10, 1);
		LCD_WriteText(buf);
	}

	_delay_ms(3000);
	LCD_GoTo(0, 1);
	LCD_WriteText("          ");

}

int main(void) {

	uint8_t cash_uart = 0;
	uint8_t bet_uart = 0;

	LCD_Initalize();
	uart_init(9600, 0);
	ADC_Init();
	sei();

	uart_send_string("Welcome to the Slot Machines! You start with 1000$\n\r");
	uart_send_string("Your bets are 5$\n\r");
	uart_send_string("Hold SELECT button to start the slot\n\r");
	uart_send_string("Let's roll!\n\r");

	//Initial state
	LCD_GoTo(10, 1);
	sprintf(buf, "$%u", cash);
	LCD_WriteText(buf);

	LCD_GoTo(0, 0);
	LCD_WriteText("[ X ][ X ][ X ]");

	while (read_LCD_buttons() != SELECT) {
		LCD_GoTo(0, 1);
		LCD_WriteText("PRESS");
		_delay_ms(1000);
		LCD_GoTo(0, 1);
		LCD_WriteText("     ");
		_delay_ms(1000);
	}

	_delay_ms(1000);

	while (1) {

		rolls();
		prizes();

		while (read_LCD_buttons() == NONE) {

			LCD_GoTo(0, 1);
			LCD_WriteText("     ");
			LCD_GoTo(0, 1);
			LCD_WriteText("EXIT?");
			_delay_ms(100);

			if (read_LCD_buttons() == DOWN) {
				LCD_Clear();
				if (cash < 1000) {
					cash = 1000 - cash;
					sprintf(buf, "You lost: $%u", cash);
					LCD_GoTo(0, 0);
					LCD_WriteText(buf);
					LCD_GoTo(0, 1);
					LCD_WriteText("Try next time!");
				} else {
					cash = cash - 1000;
					sprintf(buf, "You won: $%u", cash);
					LCD_GoTo(0, 0);
					LCD_WriteText(buf);
					LCD_GoTo(0, 1);
					LCD_WriteText("Congratulations!");
				}
				return 0;
			}
		}

	}
}

