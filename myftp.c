#include "myftp.h"
#define BUFFER_SIZE 1024

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

void send_file_header(int destination_sd, int file_size) {
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
}

// used in client when get and in server when put
// need to loop
void send_file(int destination_sd, int file_size, char *file_name) {
	int len;
    // send file payload
    FILE *fp;
    fp = fopen(file_name, "r");
    char buffer[BUFFER_SIZE];
    for (int i = 0; i < file_size / BUFFER_SIZE; i++) {
    	fread(buffer, BUFFER_SIZE, 1, fp);
    	if ((len = send(destination_sd, buffer, BUFFER_SIZE, 0)) < 0) {
        	printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        	exit(0);
    	}
    	printf("Sent file contents:\n");
        printf("%s\n", buffer);
    }
    int last_contents_size = file_size % BUFFER_SIZE;
    if (last_contents_size != 0) {
    	char *last_contents = (char *) calloc (last_contents_size, sizeof(char));
	    fread(last_contents, last_contents_size, 1, fp);
	    if ((len = send(destination_sd, last_contents, last_contents_size, 0)) < 0) {
	        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
	        exit(0);
	    }
	    printf("Sent file contents last:\n");
	    printf("%s\n", last_contents);
    }
    fclose(fp);
    printf("Sent file done.\n");
}

// used in server when get and in client when put
void receive_file(int source_sd, int file_size, char *file_name) {
	int len;
	// receive file payload
	FILE *fp;
    fp = fopen(file_name, "w");
    char buffer[BUFFER_SIZE];
    for (int i = 0; i < file_size / BUFFER_SIZE; i++) {
    	if ((len = recv(source_sd, buffer, BUFFER_SIZE, 0)) < 0) {
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
	        exit(0);
		}
    	printf("Received file contents:\n");
		printf("%s\n", buffer);
		fprintf(fp, "%s", buffer);
    }
    int last_contents_size = file_size % BUFFER_SIZE;
    if (last_contents_size != 0) {
    	char *last_contents = (char *) calloc (last_contents_size, sizeof(char));
    	if ((len = recv(source_sd, last_contents, last_contents_size, 0)) < 0) {
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        	exit(0);
		}
    	printf("Received file contents last:\n");
    	printf("%s\n", last_contents);
		fprintf(fp, "%s", last_contents);
    }
    fclose(fp);
    printf("Received file done.\n");
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
int get_file_size(char *file_name) {
	FILE *fp;
	if ((fp = fopen(file_name, "r")) == NULL) {
		return -1;
    }
	fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    rewind(fp);
    fclose(fp);
    return file_size;
}