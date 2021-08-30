#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include <avr/pgmspace.h>

#include "ssd1306_attiny85.c"

#define		PORT_BUTTON				PB4
// #define	PORT_LED				PB3		// led not in use

static char button_pressed = 0;
static char button_previously_pressed = 1; // avoid start with the button already pressed

static char buffer_0[SSD1306_INTERNAL_BUFFER_SIZE];
static char buffer_1[SSD1306_INTERNAL_BUFFER_SIZE];

static char current_buffer = 0;

static char force_reset = 0;

volatile unsigned int seed = 0;

// Game of Life
// ------------

void gol_display_title() {
	ssd1306_reset_buffer(buffer_0);
	ssd1306_reset_buffer(buffer_1);

	ssd1306_send_progmem_multiple_data(title_image_length, title_image);
}

void gol_init() {

	// reset the buffers
	ssd1306_reset_buffer(buffer_0);
	ssd1306_reset_buffer(buffer_1);

	// always start with buffer 0 as the current one
	current_buffer = 0;

	srand(seed);

	for (int i = 0; i < SSD1306_INTERNAL_WIDTH; i++) {
		for (int j = 0; j < SSD1306_INTERNAL_HEIGHT; j++) {
			ssd1306_set_buffer_pixel(buffer_0, i, j, (rand() % 2));
		}
	}
}

// returns 1 if the next step is DIFFERENT than the current one
// otherwise returns 0 (to avoid get stucked in static states)
char gol_step() {

	char new_step_different = 0;

	char *initial_buffer;
	char *final_buffer;

	if (!current_buffer) {
		initial_buffer = buffer_0;
		final_buffer = buffer_1;
	} else {
		initial_buffer = buffer_1;
		final_buffer = buffer_0;
	}

	// count the neighbours
	// 	if the cell is live
	// 		dies if < 2 neighbours
	// 		lives if == 2 || == 3 neighbours
	// 		dies if > 3 neighbours
	// if the cell is dead
	// 		lives if == 3 neighbours

	for (int i = 0; i < SSD1306_INTERNAL_WIDTH; i++) {
		for (int j = 0; j < SSD1306_INTERNAL_HEIGHT; j++) {
			char current_cell = ssd1306_get_buffer_pixel(initial_buffer, i, j);
			char new_cell;
			char neighbours_count = 0;

			// when reaching the grid limits just loop to the opposite side
			char i_left = i-1 < 0 ? SSD1306_INTERNAL_WIDTH-1 : i-1;
			char i_right = i+1 > SSD1306_INTERNAL_WIDTH-1 ? 0 : i+1;

			char j_left = j-1 < 0 ? SSD1306_INTERNAL_HEIGHT-1 : j-1;
			char j_right = j+1 > SSD1306_INTERNAL_HEIGHT-1 ? 0 : j+1;

			// LEFT
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i_left, j_left); // TOP - LEFT
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i_left, j); // MIDDLE - LEFT
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i_left, j_right); // BOTTOM - LEFT

			// MIDDLE
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i, j_left); // TOP - MIDDLE
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i, j_right); // BOTTOM - MIDDLE

			// BOTTOM
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i_right, j_left); // TOP - RIGHT
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i_right, j); // MIDDLE - RIGHT
			neighbours_count += ssd1306_get_buffer_pixel(initial_buffer, i_right, j_right); // BOTTOM - RIGHT

			if (
				(current_cell && (neighbours_count < 2 || neighbours_count > 3)) ||
				(!current_cell && neighbours_count != 3)
			) {
				new_cell = 0;
			} else {
				new_cell = 1;
			}

			ssd1306_set_buffer_pixel(final_buffer, i, j, new_cell);

			new_step_different |= (new_cell != current_cell);
		}
	}

	current_buffer ^= 1;

	return new_step_different;
}

void gol_draw_current_buffer(char disable_while_drawing) {
	ssd1306_draw_buffer(!current_buffer ? buffer_0 : buffer_1, disable_while_drawing);
}

// ----------------------

int main(void) {

	// led not in use
	// DDRB |= (1 << PORT_LED);   // led port as output
	// PORTB |= (1 << PORT_LED);

	PORTB |= (1 << PORT_BUTTON); // pull-up on button

	ssd1306_init();

	gol_display_title(buffer_0);

	// wait 'til first push to hide the title and start with the game
	while (1) {
		button_pressed = !(PINB & (1 << PORT_BUTTON));

		if (!button_previously_pressed && button_pressed) break;

		button_previously_pressed = button_pressed;
	}

	while(1) {
		button_pressed = !(PINB & (1 << PORT_BUTTON));

		if (force_reset || (!button_previously_pressed && button_pressed)) {
			gol_init();
			gol_step(); // avoid showing the first step since it's usually so dense that the change between step 0 and 1 is weeeeird
			gol_draw_current_buffer(1);
			// _delay_ms(500); // there's no need for delay between steps since the micro speed is "slow enough" to show the display for some time

			force_reset = 0;
		}

		gol_draw_current_buffer(0);
		if (!gol_step()) {
			_delay_ms(500);
			force_reset = 1;
		} else {
			_delay_ms(200);
		}

		seed++; // doesn't matter the overflow, just change fast enough to be used as a "random seed"
		button_previously_pressed = button_pressed;
	}

}
