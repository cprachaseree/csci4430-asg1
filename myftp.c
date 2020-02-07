#include "myftp.h"

void print_arg_error(char *role) {
	printf("Invalid Arguments\n");
	if (strcmp(role, "server") == 0) {
		printf("Usage: ./server <PORT_NUMBER>\n");
	} else if (strcmp(role, "client") == 0) {
		printf("Usage: ./client <SERVER_IP_ADDRESS> <SERVER_PORT_NUMBER> <list|get|put> <FILE_NAME>\n");
	}
	printf("Program terminated.\n");
	exit(0);
}

int port_num_to_int(char *port_num_string, char *role) {
	int port_num_int;
  	char *end_ptr;
  	port_num_int = (int) strtol(port_num_string, &end_ptr, 0);
  	if (*end_ptr != '\0') {
    	print_arg_error(role);
  	}
  	return port_num_int;
}

// used in client when get and in server when put
void send_file(int destination_sd, int file_size, char *file_payload) {
	int len;
	// send file data header
	struct message_s file_data;
    memset(&file_data, 0, sizeof(struct message_s));
    strcpy(file_data.protocol, "myftp");
    file_data.type = 0xFF;
    file_data.length = file_size + sizeof(struct message_s);
    if ((len = send(destination_sd, &file_data, sizeof(struct message_s), 0)) < 0) {
        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        exit(0);
    }
    // send file payload
    if ((len = send(destination_sd, file_payload, file_size, 0)) < 0) {
        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        exit(0);
    }
}

// used in server when get and in client when put
void receive_file(int source_sd, int file_size, char *file_payload) {
	int len;
	// receive file payload
    if ((len = recv(source_sd, file_payload, file_size, 0)) < 0) {
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
	}
}

// returns the file length after checking file data header
int check_file_data_header(int source_sd) {
	int len;
	// receive file header
	struct message_s file_data;
	memset(&file_data, 0, sizeof(struct message_s));
	if ((len = recv(source_sd, &file_data, sizeof(struct message_s), 0)) < 0) {
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
	    exit(0);
	}
	// check if file header protocol is correct
	if (file_data.type == 0xFF) printf("YES\n");
	if (strncmp(file_data.protocol, "myftp", 5) != 0 || file_data.type != 0xFF) {
		printf("Invalid header type or protocol.\n");
		exit(0);
	}

	return file_data.length;
}

// used in client when put and in server when get
int get_file_size(FILE *fp) {
	fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp) + 1;
    rewind(fp);
    return file_size;
}