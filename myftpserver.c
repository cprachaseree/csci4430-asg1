#include "myftp.h"
#include <pthread.h>

void check_arg(int argc);
int check_port_num(int arg_num, char *port_number_string);
void* connection(void* client_sd);
void check_arg(int argc);
void list(int client_sd);
void get_file(int client_sd, int file_name_length);
void put_file(int client_sd, int file_name_length);

int main(int argc, char *argv[]) {
    check_arg(argc);
    int PORT_NUMBER = port_num_to_int(argv[1], "server");
    // open socket
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
        printf("open socket failed: %s (Errno: %d)\n", strerror(errno), errno);
        return -1;
	}
    // bind port to socket
	struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT_NUMBER);
    if(bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		printf("bind failed: %s (Errno: %d)\n", strerror(errno), errno);
		return -1;
	}
    if(listen(sd, 3) < 0) {
		printf("listen failed: %s (Errno: %d)\n",strerror(errno), errno);
		return -1;
	}
    printf("Server is listening to connections\n");
    pthread_t threads[10];
    int thread_count = 0;
    while(1) {
        // keep accepting new connections
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        int* client_sd = malloc(sizeof(int));
	    if((*client_sd = accept(sd, (struct sockaddr *) &client_addr, &addr_len))<0){
		    printf("accept failed: %s (Errno: %d)\n", strerror(errno), errno);
            return -1;
	    }
        // creating thread for transmission stage to support concurrent clients
        pthread_create(&threads[thread_count], NULL, &connection, (void*) client_sd);
        thread_count++;
    }
}

void check_arg(int argc) {
    if (argc != 2) {
    print_arg_error("server");
  }
}

void* connection(void* client_sd) {
    struct message_s client_request_message;
    memset(&client_request_message, 0, sizeof(client_request_message));
    int len;
    if ((len = recv(*((int*) client_sd), &client_request_message, sizeof(client_request_message), 0)) < 0) {
        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
    }
    printf("Message length: %d\n", client_request_message.length);
    if (client_request_message.type == 0xA1) {
        printf("Received list request\n");
		list(*((int*) client_sd));
    } else if (client_request_message.type == 0xB1) {
        printf("Received get request\n");
        get_file(*((int*) client_sd), client_request_message.length - sizeof(client_request_message));
    } else if (client_request_message.type == 0xC1) {
        printf("Received put request\n");
        put_file(*((int*) client_sd), client_request_message.length - sizeof(client_request_message));
    }

}

void put_file(int client_sd, int file_name_length) {
    char *file_name = (char *) calloc(file_name_length, sizeof(char));
    int len;
    if ((len = recv(client_sd, file_name, file_name_length, 0)) < 0) {
        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
    }
    printf("File name received is %s\n", file_name);
    struct message_s put_response;
    memset(&put_response, 0, sizeof(struct message_s));
    strcpy(put_response.protocol, "myftp");
    put_response.type = 0xC2;
    put_response.length = sizeof(put_response);
    if ((len = send(client_sd, &put_response, sizeof(struct message_s), 0)) < 0) {
        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        exit(0);
    }
    int file_size =  check_file_data_header(client_sd) - sizeof(struct message_s); 
    char *file_payload = (char *) calloc(file_size, sizeof(char));
    receive_file(client_sd, file_size, file_payload);
    printf("File received\n");
    printf("%s\n", file_payload);
}

void get_file(int client_sd, int file_name_length) {
    char *file_name = (char *) calloc(file_name_length, sizeof(char));
    int len;
    if ((len = recv(client_sd, file_name, file_name_length, 0)) < 0) {
        printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        exit(0);
    }
    printf("File name received is %s\n", file_name);

    struct message_s get_reponse;
    memset(&get_reponse, 0, sizeof(struct message_s));
    strcpy(get_reponse.protocol, "myftp");
    FILE *fp;
    if ((fp = fopen(file_name, "r")) == NULL) {
        printf("File does not exist\n");
        get_reponse.type = 0xB3;
    } else {
        printf("File exists\n");
        get_reponse.type = 0xB2;
    }
    get_reponse.length = sizeof(struct message_s);
    if ((len = send(client_sd, &get_reponse, sizeof(struct message_s), 0)) < 0) {
        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        exit(0);
    }
    if (get_reponse.type == 0xB2) {
        int file_size = get_file_size(fp);
        char *file_payload = (char *) calloc(file_size, sizeof(char *));
        fread(file_payload, file_size, 1, fp);
        send_file(client_sd, file_size, file_payload);
        
        printf("Sent file contents:\n");
        printf("%s\n", file_payload);
    }
    fclose(fp);
    free(file_name);
}

void list(int client_sd) {
    struct message_s reply;
    memset(&reply, 0, sizeof(reply));
    strncpy(reply.protocol, "myftp", 5);
    reply.type = 0xA2;
	struct dirent *entry;
    DIR *folder;
    folder = opendir("./data");
	char* all_filename;
	int size = 1; // include null-terminated symbol
    while(entry = readdir(folder)) {
		char* filename;
		filename = entry->d_name;
		if(strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0) {
			size += strlen(entry->d_name);
			all_filename = realloc(all_filename, size);
			strcat(all_filename, filename);
			strcat(all_filename, "\n");
		}
    }
	reply.length = sizeof(reply) + strlen(all_filename);
	int len;
	if((len = send(client_sd, &reply, sizeof(reply), 0)) < 0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	if((len = send(client_sd, all_filename, strlen(all_filename), 0)) < 0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
}
