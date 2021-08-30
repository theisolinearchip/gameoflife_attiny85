#ifndef SSD1306_attiny85_c
#define SSD1306_attiny85_c

#include "i2cattiny85.c"
#include "ssd1306_attiny85_constants.h"

void ssd1306_send_single_command(char command) {
	i2c_start();
	i2c_write_byte(SSD1306_I2C_WRITE_ADDRESS);
	i2c_write_byte(SSD1306_CONTROL_BYTE_ONE_COMMAND);
	i2c_write_byte(command);
	i2c_stop(); 
}

void ssd1306_init() {
	i2c_start();
	i2c_write_byte(SSD1306_I2C_WRITE_ADDRESS);
	for (int i = 0; i < 29; i++) {
		i2c_write_byte(SSD1306_CONTROL_BYTE_MULTIPLE_COMMANDS);
		i2c_write_byte(pgm_read_byte(init_options + i)); // progmem access
	}
	i2c_stop();
}

void ssd1306_send_multiple_commands(int length, char commands[]) {
	if (length <= 0) return;

	i2c_start();
	i2c_write_byte(SSD1306_I2C_WRITE_ADDRESS);
	for (int i = 0; i < length; i++) {
		i2c_write_byte(SSD1306_CONTROL_BYTE_MULTIPLE_COMMANDS);
		i2c_write_byte(commands[i]);
	}
	i2c_stop();
}

void ssd1306_send_single_data(char data) {
	i2c_start();
	i2c_write_byte(SSD1306_I2C_WRITE_ADDRESS);
	i2c_write_byte(SSD1306_CONTROL_BYTE_ONE_DATA);
	i2c_write_byte(data);
	i2c_stop(); 
}

void ssd1306_send_multiple_data(int length, char data[]) {
	i2c_start();
	i2c_write_byte(SSD1306_I2C_WRITE_ADDRESS);
	for (int i = 0; i < length; i++) {
		i2c_write_byte(SSD1306_CONTROL_BYTE_MULTIPLE_DATA);
		i2c_write_byte(data[i]);
	}
	i2c_stop();
}

void ssd1306_send_progmem_multiple_data(const int length, const char *data) {
	i2c_start();
	i2c_write_byte(SSD1306_I2C_WRITE_ADDRESS);
	for (int i = 0; i < length; i++) {
		i2c_write_byte(SSD1306_CONTROL_BYTE_MULTIPLE_DATA);
		i2c_write_byte(pgm_read_byte(data + i));
	}
	i2c_stop();
}

// ---------------------------------------

static char ssd1306_page_buffer[SSD1306_PAGE_BUFFER_SIZE]; 

void ssd1306_reset_buffer(char *buffer) {
	for (int i = 0; i < SSD1306_INTERNAL_BUFFER_SIZE; i++) {
		*buffer = 0x00;
		buffer++;
	}
}

void ssd1306_set_buffer_pixel(char *buffer, char x, char y, char status) {
	// advance Y lines, then point to the proper char (X / pagesize)
	buffer += y * (SSD1306_INTERNAL_WIDTH / SSD1306_PAGESIZE) + (x / SSD1306_PAGESIZE);

	// save the (X % pagesize) bit
	if (status) {
		*buffer |= 0x01 << (x % SSD1306_PAGESIZE);
	} else {
		*buffer &= ~(0x01 << (x % SSD1306_PAGESIZE));
	}
}

char ssd1306_get_buffer_pixel(char *buffer, char x, char y) {
	buffer += y * (SSD1306_INTERNAL_WIDTH / SSD1306_PAGESIZE) + (x / SSD1306_PAGESIZE);

	return (*buffer >> (x % SSD1306_PAGESIZE)) & 0x01;
}

void ssd1306_draw_buffer(char *buffer, char disable_while_drawing) {

	int internal_buffer_i = 0;
	int internal_buffer_column = 0;
	int page_buffer_i = 0;

	int bits_used = 0;

	char bit_1;
	char bit_2;

	if (disable_while_drawing) ssd1306_send_single_command(SSD1306_DISPLAYOFF);

	while (internal_buffer_i < SSD1306_INTERNAL_BUFFER_SIZE) {

		while (bits_used < SSD1306_PAGESIZE) {
			// use the bits 2 by 2 (since the div is 4)

			bit_1 = (*(buffer + internal_buffer_i)) & (0x01 << bits_used);
			bit_2 = (*(buffer + internal_buffer_i + (SSD1306_INTERNAL_WIDTH / SSD1306_PAGESIZE))) & (0x01 << bits_used); // fetch the next line

			for (int k = 0; k < 4; k++) { // repeat the pixel "horizontally"
				// "top" pixel is the least significant bit
				if (bit_1) ssd1306_page_buffer[page_buffer_i + k] = 0x0F; 
				else ssd1306_page_buffer[page_buffer_i + k] = 0x00;
	
				if (bit_2) ssd1306_page_buffer[page_buffer_i + k] |= 0xF0;
				else ssd1306_page_buffer[page_buffer_i + k] &= 0x0F;
			}

			page_buffer_i += 4;

			bits_used++;
		}

		bits_used = 0;

		internal_buffer_i++;
		internal_buffer_column += SSD1306_PAGESIZE;
		if (internal_buffer_column >= SSD1306_INTERNAL_WIDTH) {
			internal_buffer_column = 0;
			page_buffer_i = 0;
			internal_buffer_i += (SSD1306_INTERNAL_WIDTH / SSD1306_PAGESIZE);

			// now send the page_buffer
			ssd1306_send_multiple_data(SSD1306_PAGE_BUFFER_SIZE, ssd1306_page_buffer);
		}

	}

	if (disable_while_drawing) ssd1306_send_single_command(SSD1306_DISPLAYON);
}

#endif