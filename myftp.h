# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <dirent.h>

void print_arg_error(char *role);
int port_num_to_int(char *port_num_string, char *role);
void send_file_header(int destination_sd, int file_size);
void send_file(int destination_sd, int file_size, char *file_name);
void receive_file(int source_sd, int file_size, char *file_name);
int check_file_data_header(int source_sd);
int get_file_size(char *file_name);

struct message_s {
	unsigned char protocol[5];   /*protocol string (5 bytes)*/
	unsigned char type;          /*type (1 byte)*/
	unsigned int length;         /*length (header + payload) (4 bytes)*/
} __attribute__ ((packed));

