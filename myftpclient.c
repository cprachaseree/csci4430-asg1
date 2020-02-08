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
	memset(&client_request_message, 0, sizeof(client_request_message));
	set_message_type(&client_request_message, user_cmd, argv);
	
	printf("user cmd: %s\n", user_cmd);
	printf("Message protocol: %s\n", client_request_message.protocol);
	printf("Message type: %c\n", client_request_message.type);
	printf("Message length: %d\n", client_request_message.length);
	int len;
	if (len = send(sd, (void *) &client_request_message, sizeof(client_request_message), 0) < 0)	{
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	// send payload if get or put
	if (strcmp(user_cmd, "list") != 0) {
		printf("sending more payload\n");
	}
	// wait for the respond headers
	printf("Waiting for more payload\n");
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
	printf("User cmd is %s\n", user_cmd);
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
