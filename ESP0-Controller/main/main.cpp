#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "oled/oled.h"
#include "payload_types/payload_types.h"
#include "font8x8_basic.h"
#include "oled/vec3.h"
#include <thread>
#include <chrono>
#include <ps3/ps3.h>
#include <ps3/ps3_int.h>
#include <atomic>
#include <unistd.h>
#include <csignal>
#include <functional>
#include "esp_now_base/esp_now_base.hpp"
#include "custom_sockets/custom_sockets.hpp"

#define tag "SSD1306"

static char bat = 0x0;
static uint32_t control_payload;
static uint32_t tof_payload;
static const int rate = 10;
std::atomic<char> vehicle_connected;

void controller_th();
void print_stat(ps3_t *);
void controller_event_cb(ps3_t, ps3_event_t);
static void gpio_conf(void);

using c_void_ptr = void(*)(void*);
template <typename L>
c_void_ptr lambda_c_void_ptr(L lambda) {
    static auto lf = lambda;
    return [](void *) {
        lf();
    };
}


void controller_task()
{
    ps3SetEventCallback(controller_event_cb);
	for (;;) {
    	ps3Init();
    	while (!ps3IsConnected()){
     	   ps3SetLed(1);
           vTaskDelay(10 / portTICK_PERIOD_MS);
    	}
    	while (ps3IsConnected()) {
     	   ps3SetLed(1); vTaskDelay(12);ps3SetLed(2);vTaskDelay(12); ps3SetLed(3);vTaskDelay(12);ps3SetLed(4);
      	  ps3SetLed(4); vTaskDelay(12);ps3SetLed(3);vTaskDelay(12);ps3SetLed(2);vTaskDelay(12);ps3SetLed(1);
    	}
        vTaskDelay(100);
		printf("Desconectado\n");
		ps3Deinit();
	}
}

enum OLED_PAGES
{
    ENVIRON,
    VEHDATA,
    NETSTAT
};

static std::atomic<uint8_t> current_page;

static std::atomic<Controller_udata_t> send_;
static std::atomic<Vehicle_data_t> recv_;
static std::atomic<char> ap_event_;

Controller_udata_t send__;
void print_stat(ps3_t const& ps3)
{
    control_payload = (
          (ps3.button.square        << 0)
        | (ps3.button.triangle      << 1)
        | (ps3.button.circle        << 2)
        | (ps3.button.cross         << 3)
        | (ps3.button.select        << 4)
        | (ps3.button.ps            << 5)
        | (ps3.button.start         << 6)
        | (ps3.button.l3            << 7)
        | (ps3.button.r3            << 8)
        | (ps3.button.l2            << 9)
        | (ps3.button.r2            << 10)
        | (ps3.button.l1            << 11)
        | (ps3.button.r1            << 12)
        | (ps3.button.up            << 13)
        | (ps3.button.down          << 14)
        | (ps3.button.right         << 15)
        | (ps3.button.left          << 16)
        | (((ps3.analog.stick.lx < -100) ? 1:((ps3.analog.stick.lx > 100) ? 2:0))     << 17)
        | (((ps3.analog.stick.ly < -100) ? 1:((ps3.analog.stick.ly > 100) ? 2:0))     << 19)
        | (((ps3.analog.stick.rx < -100) ? 1:((ps3.analog.stick.rx > 100) ? 2:0))     << 21)
        | (((ps3.analog.stick.ry < -100) ? 1:((ps3.analog.stick.ry > 100) ? 2:0))     << 23)
    );
    //static uint8_t val = current_page.load();
    if (ps3.button.left)
        current_page.store((current_page.load()+1)%3);
    else if (ps3.button.right)
        current_page.store((current_page.load()-1)%3);
	Controller_udata_t data[2];
	*(char*)data = (
			 (ps3.button.r2<<0)
			|(ps3.button.cross<<1)
			|(((ps3.analog.stick.lx < -100) ? 1:((ps3.analog.stick.lx > 100) ? 2:0))     << 2)
        	|(((ps3.analog.stick.ly < -100) ? 1:((ps3.analog.stick.ly > 100) ? 2:0))     << 4)
	);
	data[1] = send__;
	*(char*)data |= *(char*)(data+1);
    print_Controller_udata_t(send__);
    printf("\t\t\tV: %x", *(char*)(data+1));
	send_.store(data[0]);
    puts("");
}

void controller_event_cb( ps3_t ps3, ps3_event_t event )
{
    static int i;
    if (i++ < rate)
        return;
    i = 0;
	bat = (char)ps3.status.battery;
    print_stat(ps3);
}

void esp_now_task()
{
	uint8_t target_mac[8] = {0x24, 0xdc, 0xc3, 0xa0, 0x68, 0x38};
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
	esp_wifi_set_ps(WIFI_PS_NONE);

	example_wifi_init();
    example_espnow_init(&send_, &recv_, &vehicle_connected, *(uint64_t*)target_mac);
    set_sap_event_atomic_ptr(&ap_event_);

	for (;;)
		vTaskDelay(100);
}

void init_remote_sockets()
{
	socket_data_exchange_as_client_init_loop(2403, "192.168.4.2", &recv_, &send__, &ap_event_);
}

extern "C"
void app_main(void)
{
	srand(time(NULL));

	SSD1306_t dev[1] = {{._flip = true}};

	ESP_LOGI(tag, "INTERFACE is i2c");
	ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
	
	ESP_LOGI(tag, "Panel is 128x64");

	ssd1306_init(dev, 128, 64);
    ssd1306_clear_screen(dev, false);
	ssd1306_contrast(dev, 0xff);

	// Lectura
	int i;
	uint32_t data = 0;
    uint32_t data_buff = UINT32_MAX;
    const uint32_t loop_period = 100;
    uint32_t current_loop_period = 0;

	Vec3_float p[] = {
		get_Vec3_float(61, 60, 0),
		get_Vec3_float(67, 60, 0),
		get_Vec3_float(64, 54, 0),
		get_Vec3_float(61, 60, 0)
	};
	char signal = 0;

    ap_event_.store(0);

	auto vertex_draw_th = std::thread(poly_vertex_draw, p, 4, &signal, 60);
	auto t = std::thread(controller_task);

	auto f = std::thread(esp_now_task);


	usleep(1000000);
	auto g = std::thread(init_remote_sockets);

    for(;; current_loop_period++) {
        switch((OLED_PAGES)current_page.load()) {
        case VEHDATA:
            disp_upd(dev, recv_.load(), bat, vehicle_connected.load());
            break;
        case ENVIRON:
            disp_upd_2(dev);
            break;
        case NETSTAT:
            disp_upd_3(dev);
            break;
        default:
            disp_upd(dev, recv_.load(), bat, vehicle_connected.load());
        }
    }
	esp_restart();
}