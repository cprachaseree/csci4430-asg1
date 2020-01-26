#include "myftp.h"

char *check_arg(int argc, char *argv[]);
void print_arg_error();
int check_port_number(int port_num_string);

int main(int argc, char *argv[]) {
	char *user_cmd = check_arg(argc, argv);
	char *SERVER_IP_ADDRESS = argv[1];
	int SERVER_PORT_NUMBER = port_num_to_int(argv[2], "client");
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