#include "myftp.h"

char *check_arg(int argc, char *argv[]);
void print_arg_error();
int check_port_number(int port_num_string);
void set_message_type(struct message_s *client_message,char *user_cmd, char *argv[]);
void list(int sd);

int main(int argc, char *argv[]) {
	char *user_cmd = check_arg(argc, argv);
	char *SERVER_IP_ADDRESS = argv[1];
	int SERVER_PORT_NUMBER = port_num_to_int(argv[2], "client");
	char *file_name_input = argv[4];
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
        printf("open socket failed: %s (Errno: %d)\n", strerror(errno), errno);
        return -1;
	}
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
	server_addr.sin_port = htons(SERVER_PORT_NUMBER);
	if (connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
		printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	printf("Connected client\n");
	struct message_s client_request_message;
	memset(&client_request_message, 0, sizeof(struct message_s));
	set_message_type(&client_request_message, user_cmd, argv);
	
	int len;
	if ((len = send(sd, (void *) &client_request_message, sizeof(struct message_s), 0)) < 0)	{
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	// send payload if get or put
	if (strcmp(user_cmd, "list") != 0) {
		printf("File name %s\n", file_name_input);
		if ((len = send(sd, file_name_input, strlen(file_name_input), 0)) < 0) {
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
	}
	// wait for the respond headers
	struct message_s server_response;
	memset(&server_response, 0, sizeof(struct message_s));
	if ((len = recv(sd, &server_response, sizeof(struct message_s), 0)) < 0) {
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
	}
	if (server_response.type == 0xB3) {
		printf("File does not exist\n");
	} else if (server_response.type == 0xB2) {
		int file_size = check_file_data_header(sd) - sizeof(struct message_s); 
		char *file_payload = (char *) calloc(file_size, sizeof(char));
		receive_file(sd, file_size, file_payload);
		printf("Received file:\n");
		printf("%s\n", file_payload);
	} else if (server_response.type == 0xC2) {
		FILE *fp;
		if ((fp = fopen(file_name_input, "r")) == NULL) {
        	printf("File does not exist\n");
    	} else {
        	printf("File exists\n");
    	}
		int file_size = get_file_size(fp);
		char *file_payload = (char *) calloc(file_size, sizeof(char *));
        fread(file_payload, file_size, 1, fp);
        send_file(sd, file_size, file_payload);
        fclose(fp);
	}
}

void set_message_type(struct message_s *client_request_message,char *user_cmd, char *argv[]) {
	strncpy((*client_request_message).protocol, "myftp", 5);
	if (strcmp(user_cmd, "list") == 0) {
		client_request_message->type = 0xA1;
	} else if (strcmp(user_cmd, "get") == 0) {
		client_request_message->type = 0xB1;
	} else if (strcmp(user_cmd, "put") == 0) {
		client_request_message->type = 0xC1;
	}
	(*client_request_message).length = sizeof(struct message_s);
	if (strcmp(user_cmd, "list") != 0) {
		client_request_message->length += strlen(argv[4]);
	}
}

char *check_arg(int argc, char *argv[]) {
	if (argc != 4 && argc != 5) {
		printf("0\n");
		print_arg_error("client");
	}
	char *user_cmd = argv[3];
	if (strcmp(user_cmd, "list") != 0 && strcmp(user_cmd, "get") != 0 && strcmp(user_cmd, "put") != 0) {
		printf("1\n");
		print_arg_error("client");
	} 
	if (strcmp(user_cmd, "list") == 0 && argc != 4) {
		printf("2\n");
		print_arg_error("client");
	}
	if (strcmp(user_cmd, "get") == 0 && argc != 5) {
		printf("3\n");
		print_arg_error("client");
	}
	if (strcmp(user_cmd, "put") == 0 && argc != 5) {
		printf("4\n");
		print_arg_error("client");
	}
	return user_cmd;
}

void list(int sd) {
	int len;
	struct message_s header;
	if((len = recv(sd, &header, sizeof(header), 0)) < 0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	int payload_len = header.length - sizeof(header);
	char* buf;
	buf = malloc(sizeof(payload_len));
	if((len = recv(sd, buf, strlen(buf), 0)) < 0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	printf("%s", buf);
}
