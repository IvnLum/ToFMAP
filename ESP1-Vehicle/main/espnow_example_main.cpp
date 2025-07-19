#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "esp_now_base/esp_now_base.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include <payload_types/payload_types.h>
#include <thread>
#include <atomic>
#include <functional>

#include "custom_sockets/custom_sockets.hpp"

namespace UART
{
    constexpr uart_port_t   PORT_NUM = uart_port_t::UART_NUM_2;
    constexpr int           BAUD_RATE = 160000;
    constexpr int           RXD = 16;
    constexpr int           TXD = 17;
    constexpr size_t        TX_TASK_STACK_SIZE = 2048;
    constexpr size_t        RX_TASK_STACK_SIZE = 4096;
    constexpr size_t        BUF_SIZE = 100;
    constexpr char          RX_PACKET_HEAD = 0b01010101;
    constexpr char          RX_PACKET_TAIL = 0b11111111;
}

namespace Data
{
    template <typename S>
    static std::atomic<S> Send;

    template <typename R>
    static std::atomic<R> Recv;

    template <typename R>
    R Recv_;

    static std::atomic<char> Data_connected;
}

static std::atomic<Controller_udata_t> send_;
static std::atomic<Vehicle_data_t> recv_;

static Controller_udata_t send__ = {0};


namespace Packet_Utils
{
    int strncmp_null_bytes(const char *a, const char* b, int len)
    {
        if (!a || !b)
            return 0;
        while (len--) {
            if(*(a+len) != *(b+len))
                return len;
        }
        return 0;
    }
    char* strstr_null_bytes(const char *a, const char* b, int b_len, int limit_len)
    {
        if (!a || !b)
            return 0;
        int i, j;
        
        for (i = 0; i < limit_len; i++) {
            j = 0;
            while ((j < b_len) && (a[i] == b[j]) && (i < limit_len))
                i++, j++;
            if (j == b_len)
                return (char*)(a + i - b_len);
        }
        return 0;      
    }
    char* strstructstr_null_bytes(
        const char *s,
        const char *a,
        const char *b,
        int a_len,
        int b_len,
        int diff_len,
        int limit_len
        )    
    {    
        if (!a || !b || !s)    
            return 0;    
        int i, j, k;
        int max = limit_len - a_len - b_len - diff_len;
            
        for (i = 0; i < max; i++) {    
            j = 0;
            while ((j < a_len) && (s[i] == a[j]) && (i < limit_len)) {
                i++;
                j++;
            }    
            if (j != a_len)    
                continue;    
            k = i + diff_len;    
            if (k > (limit_len - b_len))    
                return 0;    
            j = 0;    
            while ((j < b_len) && (s[k] == b[j]) && (k < limit_len)) {    
                k++;    
                j++;    
            }    
            if (j == b_len)    
                return (char*)(s + k - (a_len + b_len + diff_len));    
        }    
        return 0;    
    } 
    
    template <typename T>
    T async_serial_byte_merge(char *bytes, const char rx_head, const char rx_tail)
    {
        constexpr size_t t_len = sizeof(T);
        char *ptr;
        char end[2];
        end[0] = rx_tail, end[1] = rx_head;
        ptr = strstructstr_null_bytes(bytes, &rx_head, end, 1, 2, t_len, UART::BUF_SIZE);
        if (!ptr)
            return (T){0};
        return *(T*)(ptr + 1);
    }

    template <typename T>
    char not_zero_merged_data(const T *data)
    {
        static T cmp = {0};
        return memcmp(data, &cmp, sizeof(T));
    }
}

// To allow lambdas as tasks
using c_void_ptr = void(*)(void*);
template <typename L>
c_void_ptr lambda_c_void_ptr(L lambda) {
    static auto lf = lambda;
    return [](void *) { lf(); };
}

template <typename T> 
void start_get_uart_rx_then_data_send_task()
{
    xTaskCreate(lambda_c_void_ptr(
    []() {

    char *rx_data = (char *)malloc(UART::BUF_SIZE+1);
    memset(rx_data, UART::RX_PACKET_HEAD, UART::BUF_SIZE);
    T data = {0};
    int len = 0;
    int packet_count = 1;
    int lost_packet_count = 0;
    while (1) {
        len = uart_read_bytes(UART::PORT_NUM, rx_data, UART::BUF_SIZE,  1);
        if (len) {
            data = Packet_Utils::async_serial_byte_merge<T>(rx_data, UART::RX_PACKET_HEAD, UART::RX_PACKET_TAIL);
            if (Packet_Utils::not_zero_merged_data(&data)) {
                packet_count++;
                Data::Send<T>.store(data);
                printf("\t\tPacket count: %d, lost packet count: %d, all time packet loss %.6f%%\n", 
                    packet_count, lost_packet_count, (float)lost_packet_count/(packet_count+lost_packet_count) * 100);
            } else lost_packet_count++;
        }
        taskYIELD();
    }
    free(rx_data);

    }
    ), "get_uart_rx_then_data_send", UART::RX_TASK_STACK_SIZE, NULL, 10, NULL);
}

template <typename T>
void start_data_read_then_uart_tx_task()
{
    xTaskCreate(lambda_c_void_ptr(
    []() {

    constexpr size_t t_len = sizeof(T);
    T tx_data[1];
    while (1) {
        *tx_data = Data::Data_connected.load() ? Data::Recv<T>.load(): (T)(0x0);
        /*ETC*/ {
            *(char*)tx_data |= *(char*)&Data::Recv_<T>;
        }
        uart_write_bytes(UART::PORT_NUM, (const char *) tx_data, t_len);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    }
    ), "data_read_then_uart_tx", UART::TX_TASK_STACK_SIZE, NULL, 10, NULL);
}

void init_remote_sockets()
{
    xTaskCreate(lambda_c_void_ptr(
    []() {

	socket_data_exchange_as_client_init_loop(2403, "192.168.4.2", &Data::Send<Vehicle_data_t>, &Data::Recv_<Controller_udata_t>);

    }
    ), "init_remote_sockets", UART::TX_TASK_STACK_SIZE, NULL, 10, NULL);
}

extern "C"
void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART::BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART::PORT_NUM, UART::BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART::PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART::PORT_NUM, UART::TXD, UART::RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    srand(time(NULL));
    uint8_t target_mac[] = {0x24, 0xdc, 0xc3, 0xa0, 0xb2, 0x8c};
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    esp_wifi_set_ps(WIFI_PS_NONE);
    example_wifi_init();
    example_espnow_init(&Data::Send<Vehicle_data_t>, &Data::Recv<Controller_udata_t>, &Data::Data_connected,*(uint64_t*)target_mac);
    start_data_read_then_uart_tx_task<Controller_udata_t>();
    start_get_uart_rx_then_data_send_task<Vehicle_data_t>();

    usleep(1000000);
	init_remote_sockets();
}