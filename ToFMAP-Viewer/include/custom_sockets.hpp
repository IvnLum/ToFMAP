#ifndef _CUSTOM_SOCKETS_
#define _CUSTOM_SOCKETS_

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>

static int lost = 0;
static int server_fd, new_socket;

void close_socket_on_lost(int __a)
{
	lost = 1;
	close(new_socket);
	close(server_fd);
	printf("Closed connection!!!\n");
	signal(SIGPIPE, SIG_DFL);
}

template <typename S, typename R>
int socket_data_exchange_as_server_init_loop(const int PORT, std::atomic<S> *send_, std::atomic<R> *recv)
{
	constexpr size_t send_size = sizeof(S);
	constexpr size_t recv_size = sizeof(R);
	char s_buff[send_size] = {'\0'};
	char r_buff[recv_size] = {'\0'};

	int i = 0;
	struct sockaddr_in address;
	int opt = 1;
	socklen_t addrlen = sizeof(address);

	for (;;) {
		signal(SIGPIPE, &close_socket_on_lost);
			// Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);  
    }                      
  
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
          SO_REUSEADDR | SO_REUSEPORT, &opt,
          sizeof(opt))) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
  
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,
        sizeof(address))
      < 0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    if ((new_socket
      = accept(server_fd, (struct sockaddr*)&address,
          &addrlen))
      < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
		lost = 0;
		while (!lost) {
			memset(r_buff, '\0', recv_size);
			read(new_socket, (char*)r_buff, recv_size);
			recv->store( *(R*)r_buff );

			printf("Received Vehicle_data_t %d\n", i++);
			//print_Vehicle_data_t(*(R*)r_buff);

			*(S*)s_buff = send_->load();
			send(new_socket, (char*)s_buff, send_size, 0);

			//printf("Sent Controller_data_t\n");
		}
	}
	return 0;
}

#endif /* !_CUSTOM_SOCKETS_ */
