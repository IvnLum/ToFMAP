#ifndef __OLED__
#define __OLED__

#ifdef __cplusplus
extern "C"
{
#endif
#include <string.h>
#include "vec3.h"
#include "oled.h"
#include "payload_types/payload_types.h"
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "driver/gpio.h"

#define PACK8 __attribute__((aligned( __alignof__( uint8_t ) ), packed ))
typedef union out_column_t {
	uint32_t u32;
	uint8_t  u8[4];
} PACK8 out_column_t;

//Vec3(float);


char text_ii[][7][20] = {
        {
            "[    ] %s",
            "[    ] %s",
            "[|   ] %s",
            "[||  ] %s",
            "[||| ] %s",
            "[||||] %s",
            "[ DC ] %s",
        },
        {
            "[ NA ]  %.2f m/s",
            "[    ]  %.2f m/s",
            "[|   ]  %.2f m/s",
            "[||  ]  %.2f m/s",
            "[||| ]  %.2f m/s",
            "[||||]  %.2f m/s",
        },
	};

uint8_t mm[8][128];

void poly_vertex_draw(Vec3_float *vertices, uint32_t len, char *signal, int rate)
{
	int i, j;
	Vec3_float next, current;

	do {
		for (i = 0; i < len-1; i++) {
			next = vertices[i+1];
			current = vertices[i];
			mm[(int)vertices[i].y / 8][127 - (int)vertices[i].x] = 1<<((int)vertices[i].y % 8);
			vec3_subv_float(&next, vertices[i]);
			vec3_set_mod_float(&next, 0.5);
		
			while (!vec3_is_close_w_float(current, vertices[i+1], 0.5)) {
				mm[(int)current.y / 8][127 - (int)current.x] |= 1<<((int)current.y % 8);
				vec3_addv_float(&current, next);
			}
		}
		vTaskDelay(5);//e6 / rate);
		for (i = 2; i < 8; i++)
			memset(mm[i], 0, 128);
	} while (!*signal);
	printf("th_poly_vertex_draw end reached!!\n");
}


// Modificacion de ssd1306_display_text_x3 a x2

void ssd1306_display_text_x2(SSD1306_t * dev, int page, const char * text, int text_len, bool invert)
{
	if (page >= dev->_pages) return;
	int _text_len = text_len;
	if (_text_len > 8) _text_len = 8;

	uint8_t seg = 0;

	for (uint8_t nn = 0; nn < _text_len; nn++) {

		uint8_t const * const in_columns = font8x8_basic_tr[(uint8_t)text[nn]];

		// make the character 2x as high
		out_column_t out_columns[8];
		memset(out_columns, 0, sizeof(out_columns));

		for (uint8_t xx = 0; xx < 8; xx++) { // for each column (x-direction)

			uint32_t in_bitmask = 0b1;
			uint32_t out_bitmask = 0b11;

			for (uint8_t yy = 0; yy < 8; yy++) { // for pixel (y-direction)
				if (in_columns[xx] & in_bitmask) {
					out_columns[xx].u32 |= out_bitmask;
				}
				in_bitmask <<= 1;
				out_bitmask <<= 2;
			}
		}

		// render character in 8 column high pieces, making them 2x as wide
		for (uint8_t yy = 0; yy < 2; yy++)	{ // for each group of 8 pixels high (y-direction)

			uint8_t image[16];
			for (uint8_t xx = 0; xx < 8; xx++) { // for each column (x-direction)
				image[xx*2+0] = 
				image[xx*2+1] = out_columns[xx].u8[yy];
			}
			if (invert) ssd1306_invert(image, 16);
			if (dev->_flip) ssd1306_flip(image, 16);
			if (dev->_address == SPIAddress) {
				spi_display_image(dev, page+yy, seg, image, 16);
			} else {
				i2c_display_image(dev, page+yy, seg, image, 16);
			}
			memcpy(&dev->_page[page+yy]._segs[seg], image, 16);
		}
		seg = seg + 16;
	}
}

// Layout personalizado oled

void ssd1306_display_text_rev(SSD1306_t * dev, int page, char * text, int text_len, bool invert)
{
	if (page >= dev->_pages) return;
	int _text_len = text_len;
	if (_text_len > 16) _text_len = 16;

	uint8_t seg = 0;
	uint8_t image[8];
	uint8_t i, j, k;
	for (i = 0; i < _text_len; i++) {
		k = text_len-i-1;
		//for (j = 7; j >= 0; j--)
		image[7] = font8x8_basic_tr[(uint8_t)text[k]][0];
		image[6] = font8x8_basic_tr[(uint8_t)text[k]][1];
		image[5] = font8x8_basic_tr[(uint8_t)text[k]][2];
		image[4] = font8x8_basic_tr[(uint8_t)text[k]][3];
		image[3] = font8x8_basic_tr[(uint8_t)text[k]][4];
		image[2] = font8x8_basic_tr[(uint8_t)text[k]][5];
		image[1] = font8x8_basic_tr[(uint8_t)text[k]][6];
		image[0] = font8x8_basic_tr[(uint8_t)text[k]][7];
		//memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		if (invert) ssd1306_invert(image, 8);
		if (dev->_flip) ssd1306_flip(image, 8);
		ssd1306_display_image(dev, page, seg, image, 8);
#if 0
		if (dev->_address == SPIAddress) {
			spi_display_image(dev, page, seg, image, 8);
		} else {
			i2c_display_image(dev, page, seg, image, 8);
		}
#endif
		seg = seg + 8;
	}
}

void disp_upd(SSD1306_t *dev, Vehicle_data_t data, char c_batt, char conn)
{
	int i;
    float v;

	char raw[16] = {'\0'};

	memset(raw, '\0', strlen(raw));
	if (data.batt > 0x05)
		sprintf(raw, text_ii[0][6], conn?"CONECTADO":"NO CONECT");
	else sprintf(raw, text_ii[0][(int)c_batt], conn?"CONECTADO":"NO CONECT");
	ssd1306_display_text_rev   (dev, 0, raw, 16, true);

	memset(raw, '\0', 16);
    v = sqrt(data.dir_x*data.dir_x + data.dir_y*data.dir_y) / 1000;
	sprintf(raw, text_ii[1][(int)(data.batt>5?4:data.batt)], v>100? 100:v);
	ssd1306_display_text_rev   (dev, 1, raw, strlen(raw), true);
	//ssd1306_display_text   (dev, 4, " Bateria baja!! ", 16, true);

	memset(raw, '\0', strlen(raw));
	sprintf(raw, "Loc: %.2f,%.2f", (float)data.loc_x / 1000, (float)data.loc_y / 1000);
	ssd1306_display_text_rev   (dev, 2, raw, 16, 0);
	memset(raw, '\0', strlen(raw));
	sprintf(raw, "Dir: %.2f,%.2f", (float)data.dir_x / 1000, (float)data.dir_y / 1000);
	ssd1306_display_text_rev   (dev, 3, raw, 16, 0);
	memset(raw, '\0', strlen(raw));
	sprintf(raw, "ToF: %.2f,%.2f", (float)data.inc / 1000 , (float)data.azm / 1000);
	ssd1306_display_text_rev   (dev, 4, raw, 16, 0);
	memset(raw, '\0', strlen(raw));
	sprintf(raw, "Obj dist: %.2f", (float)data.dist / 1000);
	ssd1306_display_text_rev   (dev, 5, raw, 16, 0);
}
void disp_upd_2(SSD1306_t *dev)
{
	int i;
    float v;

	for (i = 0; i < 8; i++)
		i2c_display_image(dev, i, 0, mm[i], 128);
}

void disp_upd_3(SSD1306_t *dev)
{
	int i;
    float v;
	char raw[16] = {'\0'};
	memset(raw, '\0', strlen(raw));
	sprintf(raw, "Sample NETSTAT");
	ssd1306_display_text_rev   (dev, 2, raw, 16, 0);
}


#ifdef __cplusplus
}
#endif

#endif /* !__OLED__ */