#include "myftp.h"

char *check_arg(int argc, char *argv[]);
void read_clientconfig(char *clientconfig_name, int *n, int *k, int *block_size, char ***serverip_port_addr);
void print_arg_error();
int check_port_number(int port_num_string);
void set_message_type(struct message_s *client_message,char *user_cmd, char *argv[]);
void init_ip_port(char* serverip_port_addr, char **ip, char **port);
void list(int sd, int payload_size);

int main(int argc, char *argv[]) {
	int n, k, block_size, num_of_stripes;
	int *server_sd;
	char **serverip_port_addr;
	char *user_cmd, **IP, **PORT;
	int i;
	struct sockaddr_in *server_addr;
	
	user_cmd = check_arg(argc, argv);
	read_clientconfig(argv[1], &n, &k, &block_size, &serverip_port_addr);

	if (strcmp(user_cmd, "put") == 0) {
		if (get_file_size(argv[3]) == -1) {
			printf("File does not exist\n");
			exit(0);
		}
		Stripe *stripe;
		// in myftp.c
		num_of_stripes = chunk_file(argv[3], n, k, block_size, &stripe);
		encode_data(n, k, block_size, &stripe, num_of_stripes);
	}
	server_sd = (int *) calloc(n, sizeof(int));
	server_addr = (struct sockaddr_in *) calloc(n, sizeof(server_addr));
	IP = (char **) calloc(n, sizeof(char *));
	PORT = (char **) calloc(n, sizeof(char *));
	for (i = 0; i < n; i++) {
		server_sd[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (server_sd[i] < 0) {
        	printf("open socket failed: %s (Errno: %d)\n", strerror(errno), errno);
        	return -1;
		}
		memset(&server_addr[i], 0, sizeof(server_addr[i]));
		server_addr[i].sin_family = AF_INET;
		init_ip_port(serverip_port_addr[i], &IP[i], &PORT[i]);
		server_addr[i].sin_addr.s_addr = inet_addr(IP[i]);
		server_addr[i].sin_port = htons(atoi(PORT[i]));
		printf("IP: %s PORT: %s\n", IP[i], PORT[i]);
		if (connect(server_sd[i],(struct sockaddr *) &server_addr[i],sizeof(server_addr[i])) < 0) {
			printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		printf("Connected client to server ip and port %s\n", serverip_port_addr[i]);
	}
/*
	
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
        printf("open socket failed: %s (Errno: %d)\n", strerror(errno), errno);
        return -1;
	}
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("192.168.0.3");
	server_addr.sin_port = htons(13567);
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
		printf("File name %s\n", file_name);
		if ((len = send(sd, file_name, strlen(file_name), 0)) < 0) {
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
	}

	// wait for the resply headers
	struct message_s server_reply;
	memset(&server_reply, 0, sizeof(struct message_s));
	if ((len = recv(sd, &server_reply, sizeof(struct message_s), 0)) < 0) {
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
	}
	if (server_reply.type == 0xB3) {
		printf("File does not exist\n");
	} else if (server_reply.type == 0xB2) {
		int file_size = check_file_data_header(sd) - sizeof(struct message_s);
		receive_file(sd, file_size, file_name);
	} else if (server_reply.type == 0xC2) {
		int file_size = get_file_size(file_name);
		send_file_header(sd, file_size);
        send_file(sd, file_size, file_name);
	} else if (server_reply.type == 0xA2) {
		int payload_size = ntohl(server_reply.length) - sizeof(server_reply); 
		list(sd, payload_size);
	}
	*/
}

void set_message_type(struct message_s *client_request_message,char *user_cmd, char *argv[]) {
	strncpy(client_request_message->protocol, "myftp", 5);
	if (strcmp(user_cmd, "list") == 0) {
		client_request_message->type = 0xA1;
	} else if (strcmp(user_cmd, "get") == 0) {
		client_request_message->type = 0xB1;
	} else if (strcmp(user_cmd, "put") == 0) {
		client_request_message->type = 0xC1;
	}
	client_request_message->length = sizeof(struct message_s);
	if (strcmp(user_cmd, "list") != 0) {
		client_request_message->length += strlen(argv[4]);
	}
	client_request_message->length = htonl(client_request_message->length);
}

char *check_arg(int argc, char *argv[]) {
	if (argc != 3 && argc != 4) {
		printf("0\n");
		print_arg_error("client");
	}
	char *user_cmd = argv[2];
	if (strcmp(user_cmd, "list") != 0 && strcmp(user_cmd, "get") != 0 && strcmp(user_cmd, "put") != 0) {
		print_arg_error("client");
	} 
	if (strcmp(user_cmd, "list") == 0 && argc != 3) {
		print_arg_error("client");
	}
	if (strcmp(user_cmd, "get") == 0 && argc != 4) {
		print_arg_error("client");
	}
	if (strcmp(user_cmd, "put") == 0 && argc != 4) {
		print_arg_error("client");
	}
	return user_cmd;
}

void read_clientconfig(char *clientconfig_name,int *n, int *k, int *block_size, char ***serverip_port_addr) {
    FILE *clientconfig_fp;
    size_t length;
    int char_length;
    if ((clientconfig_fp = fopen(clientconfig_name, "r")) == NULL) {
        printf("Unable to open file %s\n", clientconfig_name);
        print_arg_error("client");
        exit(0);
    }
    if (fscanf(clientconfig_fp, "%d\n%d\n%d\n", n, k, block_size) != 3) {
        printf("Error in clientconfig.txt.\n");
        exit(0);
    }
    printf("n=%d\nk=%d\nblock_size=%d\n", *n, *k, *block_size);
    *serverip_port_addr = (char **) calloc(*n, sizeof(char*));
    for (int i = 0; i < *n; i++) {
    	char_length = getline(&((*serverip_port_addr)[i]), &length, clientconfig_fp);
    	if ((*serverip_port_addr)[i][char_length-1] == '\n') {
    		(*serverip_port_addr)[i][char_length-1] = '\0';
    	}
    	printf("%s\n", (*serverip_port_addr)[i]);
    }
    fclose(clientconfig_fp);
}

void init_ip_port(char* serverip_port_addr, char **ip, char **port) {
	char *original_p;
	int length, i;
	printf("serverip_port_addr: %s\n", serverip_port_addr);
	length = strlen(serverip_port_addr);
	original_p = (char *) calloc(length, sizeof(char));
	strcpy(original_p, serverip_port_addr);
	for (i = 0; i < length; i++) {
		if (serverip_port_addr[i] == ':') {
			original_p[i] = '\0';
			break;
		}
	}
	i++;
	*ip = (char *) calloc(i, sizeof(char));
	*port = (char *) calloc(length - i, sizeof(char));
	strcpy(*ip, original_p);
	strcpy(*port, &serverip_port_addr[i]);
	printf("IP: %s\n", *ip);
	printf("PORT: %s\n", *port);
}

void list(int sd, int payload_size) {
	int len;
	char* buf;
	buf = malloc(sizeof(payload_size));
	if((len = recv(sd, buf, payload_size, 0)) < 0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	printf("%s", buf);
}
