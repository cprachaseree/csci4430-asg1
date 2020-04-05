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
# include <isa-l.h>
# include <unistd.h>
# include <sys/stat.h>


void print_arg_error(char *role);
int port_num_to_int(char *port_num_string, char *role);
void send_file_header(int destination_sd, int file_size);
void send_file(int destination_sd, int file_size, char *file_name);
void receive_file(int source_sd, int file_size, char *file_name, int block_size);
int check_file_data_header(int source_sd);
int get_file_size(char *file_name);


struct message_s {
	unsigned char protocol[5];   /*protocol string (5 bytes)*/
	unsigned char type;          /*type (1 byte)*/
	unsigned int length;         /*length (header + payload) (4 bytes)*/
} __attribute__ ((packed));

typedef struct stripe {
	int sid;                       /*stripe id*/
	unsigned char **data_block;    /*pointer to the first data block*/
	unsigned char **parity_block;  /*pointer to the first parity block*/
} Stripe;

int chunk_file(char *file_name, int n, int k, int block_size, Stripe **stripe);
void encode_data(int n, int k, int block_size, Stripe **stripe, int num_of_stripes);