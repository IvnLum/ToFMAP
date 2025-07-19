#ifndef _CUSTOM_SOCKETS_
#define _CUSTOM_SOCKETS_

#include "sdkconfig.h"
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"
#include <atomic>

//#include "payload_types/payload_types.h"

static volatile int lost = 0;
static volatile int client_fd;


template <typename S, typename R>
int socket_data_exchange_as_client_init_loop(const int PORT, const char *address, std::atomic<S> *send_, R *recv_)
{
	constexpr size_t send_size = sizeof(S);
	constexpr size_t recv_size = sizeof(R);
	char s_buff[sizeof(S)] = {'\0'};
	char r_buff[sizeof(R)] = {'\0'};

    int addr_family = 0;
    int ip_protocol = 0;

	int sock;

    for (;;) {
		printf("Socks IN\n");
		struct sockaddr_in dest_addr;
		inet_pton(AF_INET, address, &dest_addr.sin_addr);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;

        sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGW(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", address, PORT);
        ESP_LOGE(TAG, "HERE 1");

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGW(TAG, "Socket unable to connect: errno %d", errno);
			usleep(4000000);
            //break;
        }
        ESP_LOGE(TAG, "HERE 2");
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
			*(S*)s_buff = send_->load();
            int err = send(sock, s_buff, send_size, 0);
            if (err < 0) {
                ESP_LOGW(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, r_buff, recv_size, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGW(TAG, "recv failed: errno %d", errno);
				*recv_ = {0};
                break;
            }
            // Data received
            else {
                r_buff[len] = 0; // Null-terminate whatever we received and treat like a string
				*recv_ = *(R*)r_buff;
				//print_Controller_udata_t(recv_->load());
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, address);
                ESP_LOGI(TAG, "%x", *(uint8_t*)r_buff);
            }
        }

        *recv_ = (R){0};

        if (sock != -1) {
            close(sock);
        }
        
    }

    ESP_LOGE(TAG, "Shutting down socket and restarting...");
	shutdown(sock, 0);

	return 0;
}

/*

Unit-test


int main(int argc, char const* argv[])
{
	Vehicle_data_t dat = get_Vehicle_data_t(1, 0, 3, 4, 5, 6, 7, 8);
	Controller_udata_t contr;

	socket_data_exchange_as_client_init_loop(2403, "127.0.0.1", &dat, &contr);
	return 0;
}

*/

#endif /* !_CUSTOM_SOCKETS_ */