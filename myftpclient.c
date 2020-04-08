#include "myftp.h"

char *check_arg(int argc, char *argv[]);
void read_clientconfig(char *clientconfig_name, int *n, int *k, int *block_size, char ***serverip_port_addr);
void print_arg_error();
int check_port_number(int port_num_string);
void set_message_type(struct message_s *client_message,char *user_cmd, int file_name_length);
void init_ip_port(char* serverip_port_addr, char **ip, char **port);
void list(int sd, int payload_size);
void put(int sd, int n, int k, Stripe *stripe,
	int num_of_stripes, char *file_name, int file_size, int block_size);
int receive_stripes(int n, int k, int block_size,
	int sd, Stripe **stripe, int num_of_stripes);
void write_to_file(char *file_name,  int k, int block_size, int file_size, int num_of_stripes, Stripe *stripe);

int main(int argc, char *argv[]) {
	int n, k, block_size, num_of_stripes, sd, x;
	int *server_sd, num_of_server_sd;
	int *success_con;
	char **serverip_port_addr;
	char *user_cmd, *file_name, **IP, **PORT;
	int i, len, l;
	struct sockaddr_in *server_addr;
	fd_set fds;
	int maxfd;
	int file_size;
	
	user_cmd = check_arg(argc, argv);
	if (strcmp(user_cmd, "list") != 0) {
		file_name = argv[3];
	}
	
	read_clientconfig(argv[1], &n, &k, &block_size, &serverip_port_addr);

	Stripe *stripe = NULL;

	// set up stripes
	if (strcmp(user_cmd, "put") == 0) {
		if (get_file_size(file_name) == -1) {
			printf("File does not exist\n");
			exit(0);
		}	
		// in myftp.c
		num_of_stripes = chunk_file(file_name, n, k, block_size, &stripe);
		encode_data(n, k, block_size, &stripe, num_of_stripes);
	}
	server_sd = (int *) calloc(n, sizeof(int));
	server_addr = (struct sockaddr_in *) calloc(n, sizeof(struct sockaddr_in));
	success_con = (int *) calloc(n, sizeof(int));
	IP = (char **) calloc(n, sizeof(char *));
	PORT = (char **) calloc(n, sizeof(char *));
	num_of_server_sd = 0;
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
		if (connect(server_sd[i],(struct sockaddr *) &server_addr[i],sizeof(server_addr[i])) < 0) {
			printf("Server %d connection error: %s (Errno:%d)\n", i + 1, strerror(errno), errno);
			success_con[i] = 0;
		}
		else {
			num_of_server_sd++;
			success_con[i] = 1;
			printf("Connected client to server ip and port %s\n", serverip_port_addr[i]);
		}
		free(IP[i]);
		free(PORT[i]);
	}
	printf("\n");
	if ((num_of_server_sd < 1 && strcmp(user_cmd, "list") == 0) ||
		(num_of_server_sd < k && strcmp(user_cmd, "list") != 0) ||
		(num_of_server_sd < n && strcmp(user_cmd, "put") == 0)) {
		printf("Not enough server available\n");
		exit(0);
	}
	int j;
	// make server_sd to be all consecutive
	for(i = 0; i < n; i++) {
		if (success_con[i] == 0 && i != n-1) {
			close(server_sd[i]);
			for(j = i; j < n; j++) {
				//close(server_sd[j]);
				server_sd[j] = server_sd[j + 1];
			}
		}
	}
	// Select multiple server sd descriptors to maintain
	// find max fd
	maxfd = -1;
	for (i = 0; i < num_of_server_sd; i++) {
		if (server_sd[i] > maxfd) {
			maxfd = server_sd[i];
		}
	}
	file_size = 0;
	int serverid;
	// used to check if all server sds are done
	int *done = (int *) calloc(num_of_server_sd, sizeof(int));
	int *validstripes = (int *) calloc(n, sizeof(int));
	// pseudo code from slide 32 Lecture 3 Part 1 and piazza Post 100
	// https://piazza.com/class/k4gkrtwpx1m3yr?cid=100 
	for (;;) {
		FD_ZERO(&fds);
		for (i = 0 ; i < num_of_server_sd; i++) {
			FD_SET(server_sd[i], &fds);
		}
		select(maxfd+1, NULL, &fds, NULL, NULL);
		for (i = 0; i < num_of_server_sd; i++) {
			// check if sd is unique
			if (done[i] == 1) {
				continue;
			}
			sd = server_sd[i];
			if (FD_ISSET(sd, &fds)) {
				// send request
				struct message_s client_request_message;
				memset(&client_request_message, 0, sizeof(struct message_s));
				if (strcmp(user_cmd, "list") != 0) {
					set_message_type(&client_request_message, user_cmd, strlen(file_name));
				}
				else {
					set_message_type(&client_request_message, user_cmd, 0);
				}	
				if ((len = send(sd, (void *) &client_request_message, sizeof(struct message_s), 0)) < 0) {
					printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
					exit(0);
				}
				// send file name
				if (strcmp(user_cmd, "list") != 0) {
					if ((len = send(sd, file_name, strlen(file_name), 0)) < 0) {
						printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
						exit(0);
					}
				}
				// recv response
				struct message_s server_reply;
				memset(&server_reply, 0, sizeof(struct message_s));
				if ((len = recv(sd, &server_reply, sizeof(struct message_s), 0)) < 0) {
					printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			        exit(0);
				}
				// send/recv file from this server sd
				if (server_reply.type == 0xB3) {
					// no file
					printf("File does not exist in server");
					exit(0);
				} else if (server_reply.type == 0xB2) {
					// for get file 
					// have to get filesize from metadata of server
					file_size = check_file_data_header(sd) - sizeof(struct message_s);
					num_of_stripes = ((file_size - 1) / (k * block_size)) + 1;
					if (stripe == NULL) {
						stripe = (Stripe *) calloc(num_of_stripes, sizeof(Stripe));
						for (j = 0; j < num_of_stripes; j++) {
							stripe[j].sid = j;
							stripe[j].data_block = (unsigned char **) calloc(k, sizeof(unsigned char *));
							stripe[j].parity_block = (unsigned char **) calloc(n-k, sizeof(unsigned char *));
							for (x = 0; x < n-k; x++) {
								stripe[j].parity_block[x] = (unsigned char *) calloc(block_size, sizeof(unsigned char));
							}
							for (x = 0; x < k; x++) {
								stripe[j].data_block[x] = (unsigned char *) calloc(block_size, sizeof(unsigned char));
							}
						}
					}
					serverid = receive_stripes(n, k, block_size,sd, &stripe, num_of_stripes);
					validstripes[serverid - 1] = 1;
				} else if (server_reply.type == 0xC2) {
					// for put file
					file_size = get_file_size(file_name);
					put(sd, n, k, stripe, num_of_stripes, file_name, file_size, block_size);
				} else if (server_reply.type == 0xA2) {
					// for list file
					int payload_size = ntohl(server_reply.length) - sizeof(server_reply); 
					list(sd, payload_size);
					int k;
					// skip all other server // should we change j++ to k++?
					for (k = 0; k < num_of_server_sd; k++) {
						done[k] = 1;
					}
				}
				done[i] = 1;
			}
		}
		// check if all servers are done
		int j;
		for (j = 0; j < num_of_server_sd; j++) {
			if (done[j] == 0) {
				break;
			}
		}
		if (j == num_of_server_sd) {
			break;
		}
	}
	// if get have to combine and decode the files
	if (strcmp(user_cmd, "get") == 0) {
		// have to decode
		decode_matrix(n, k, block_size, &stripe, num_of_stripes, validstripes);
		// TODO PUT stripe[i].data_block[j] INTO file_name based on file_size
		write_to_file(file_name, k, block_size, file_size, num_of_stripes, stripe);
		
	}
	printf("\nDone.\n");
}

void set_message_type(struct message_s *client_request_message,char *user_cmd, int file_name_length) {
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
		client_request_message->length += file_name_length;
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
    }
    fclose(clientconfig_fp);
}

void init_ip_port(char* serverip_port_addr, char **ip, char **port) {
	char *original_p;
	int length, i;
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
	free(original_p);
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

void put(int sd, int n, int k, Stripe *stripe,
	int num_of_stripes, char *file_name, int file_size, int block_size) {
	int j, len;
	char c;
	// send file size for server to store in metadata
	send_file_header(sd, file_size);
	int serverid = check_file_data_header(sd) - sizeof(struct message_s);
	// send size of stripes
	send_file_header(sd, num_of_stripes);
	// check if should send data_block or parity_block
	// send all data_block or parity_block along the column
	for (j = 0; j < num_of_stripes; j++) {
		fflush(stdout);
		if (serverid <= k) {
			if ((len = send(sd, stripe[j].data_block[serverid - 1], block_size, 0)) < 0) {
	        	printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
	        	exit(0);
	    	}
		} else {
			if ((len = send(sd, stripe[j].parity_block[serverid - 1 - k], block_size, 0)) < 0) {
	        	printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
	        	exit(0);
	    	}
		}
	}
}

int receive_stripes(int n, int k, int block_size,
	int sd, Stripe **stripe, int num_of_stripes) {
	
	int len, i, j;
	int serverid =  check_file_data_header(sd) - sizeof(struct message_s);
	char* buffer = (char *)calloc(block_size, sizeof(char));
	
	for (i = 0; i < num_of_stripes; i++) {
		if ((len = recv(sd, buffer, block_size, MSG_WAITALL)) < 0) {
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
	        exit(0);
		}
		// get the data and put them into the stripes
		if (serverid <= k) {
			memcpy(((*stripe)[i]).data_block[serverid - 1], buffer, block_size);
		} else {
			memcpy(((*stripe)[i]).parity_block[serverid - k - 1], buffer, block_size);
		}
	}

	free(buffer);
	return serverid;
}

void write_to_file(char *file_name, int k, int block_size, int file_size, int num_of_stripes, Stripe *stripe) {
	int written = 0, out = 0, i, j;
	FILE *fw;
	fw = fopen(file_name, "w");
	for (i = 0; i < num_of_stripes; i++) {
		for (j = 0; j < k; j++) {
			if (written + block_size > file_size) {
				out = 1;
				break;
			}
			fwrite(stripe[i].data_block[j], sizeof(unsigned char), block_size, fw);
			written += block_size;
		}
		if (out == 1) {
			break;
		}
	}
	if (out == 1) {
		unsigned char *last = (unsigned char *) calloc(file_size - written, sizeof(unsigned char));
		fwrite(stripe[i].data_block[j], sizeof(unsigned char), file_size - written, fw);
	}
	fclose(fw);
}