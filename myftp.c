#include "myftp.h"
#define BUFFER_SIZE 1024

void print_arg_error(char *role) {
	printf("Invalid Arguments\n");
	if (strcmp(role, "server") == 0) {
		printf("Usage: ./server <server config file>\n");
	} else if (strcmp(role, "client") == 0) {
		printf("Usage: ./client <client config file> <list|get|put> <file>\n");
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
    file_data.length = htonl(file_size + sizeof(struct message_s));
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
	int i;
    for (i = 0; i < file_size / BUFFER_SIZE; i++) {
    	fread(buffer, BUFFER_SIZE, 1, fp);
    	if ((len = send(destination_sd, buffer, BUFFER_SIZE, 0)) < 0) {
        	printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        	exit(0);
    	}
    }
    int last_contents_size = file_size % BUFFER_SIZE;
    if (last_contents_size != 0) {
    	char *last_contents = (char *) calloc (last_contents_size, sizeof(char));
	    fread(last_contents, last_contents_size, 1, fp);
	    if ((len = send(destination_sd, last_contents, last_contents_size, 0)) < 0) {
	        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
	        exit(0);
	    }
    }
    fclose(fp);
    printf("Sent file done.\n");
}

// used in server when get and in client when put
void receive_file(int source_sd, int payload_size, char *file_name, int block_size) {
	int len;
	// receive file payload
	FILE *fp;
    fp = fopen(file_name, "w");
    char* buffer = (char *)calloc(block_size, sizeof(char));
	int i;
    for (i = 0; i < payload_size / block_size + 1; i++) {
    	if ((len = recv(source_sd, buffer, block_size, 0)) < 0) {
    		memset(buffer, 0, sizeof(char));
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
	        exit(0);
		}
		fwrite(buffer, sizeof(buffer[0]), block_size, fp);
    }
    /*
    int last_contents_size = file_size % block_size;
    if (last_contents_size != 0) {
    	char *last_contents = (char *) calloc (last_contents_size, sizeof(char));
    	if ((len = recv(source_sd, last_contents, last_contents_size, 0)) < 0) {
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
        	exit(0);
		}
		fwrite(last_contents, sizeof(last_contents[0]), last_contents_size, fp);
    }*/
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
	if (file_data.type != 0xFF) {
		printf("Transferring error.\n");
		exit(0);
	}
	if (strncmp(file_data.protocol, "myftp", 5) != 0 || file_data.type != 0xFF) {
		printf("Invalid header type or protocol.\n");
		exit(0);
	}
	int ret = ntohl(file_data.length);
	return ret;
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

// init stripes by read data from the file into data_block in stripes
// parity block will be used when encoding, calloc corresponding parity sizes
// tested from client side
int chunk_file(char *file_name, int n, int k, int block_size, Stripe **stripe) {
	int file_size, num_of_stripes, data_block_no;
	int i, j;
	char *read_buffer;
	FILE *fp;
	file_size = get_file_size(file_name);
	if (file_size == -1) {
		printf("Invalid file: %s\n", file_name);
	}
	fp = fopen(file_name, "r");
	data_block_no = n - k;
	// get num of stripes based on file size, k, and block size
	num_of_stripes = ((file_size - 1) / (k * block_size)) + 1;
	*stripe = (Stripe *) calloc(num_of_stripes, sizeof(Stripe));
	for (i = 0; i < num_of_stripes; i++) {
		// to be used so when the stripes are seperated we know which stripe id it is
		((*stripe)[i]).sid = i;
		((*stripe)[i]).data_block = (unsigned char **) calloc(k, sizeof(unsigned char *));
		((*stripe)[i]).parity_block = (unsigned char **) calloc(n-k, sizeof(unsigned char *));
		for (j = 0; j < n-k; j++) {
			((*stripe)[i]).parity_block[j] = (unsigned char *) calloc(block_size, sizeof(unsigned char));
		}
		for (j = 0; j < k; j++) {
			((*stripe)[i]).data_block[j] = (unsigned char *) calloc(block_size, sizeof(unsigned char));
			fread(((*stripe)[i]).data_block[j], block_size, 1, fp);
		}
	}
	fclose(fp);
	return num_of_stripes;
}

// put parity info into each parity block for all stripes
// print to check if parity has k blocks
void encode_data(int n, int k, int block_size, Stripe **stripe, int num_of_stripes) {
	uint8_t *encode_matrix , *errors_matrix, *invert_matrix, *tables;
	int i, j;
	encode_matrix = (uint8_t *) calloc(n * k, sizeof(uint8_t));
	
	tables = (uint8_t *) calloc(32 * k * (n-k), sizeof(uint8_t));
	// generate encoded matrix
	gf_gen_rs_matrix(encode_matrix, n, k);
	// generate expanded matrix (includes parity matrix)
	ec_init_tables(k, n-k, &encode_matrix[k*k], tables);
	// generate parity for all stripes
	for (i = 0; i < num_of_stripes; i++) {
		ec_encode_data(block_size, k, n-k, tables, ((*stripe)[i]).data_block, ((*stripe)[i]).parity_block);
	}
	free(encode_matrix);
	free(tables);
}

void decode_matrix(int n, int k, int block_size, Stripe **stripe, int num_of_stripes, int *validstripes) {
	int i, j, m, l, o;
	uint8_t *encode_matrix , *errors_matrix, *invert_matrix, *decode_matrix, *tables;
	encode_matrix = (uint8_t *) calloc(n * k, sizeof(uint8_t));
	errors_matrix = (uint8_t *) calloc(k * k, sizeof(uint8_t));
	invert_matrix = (uint8_t *) calloc(k * k, sizeof(uint8_t));
	decode_matrix = (uint8_t *) calloc(k * k, sizeof(uint8_t));
	tables = (uint8_t *) calloc(32 * k * (n-k), sizeof(uint8_t));

	gf_gen_rs_matrix(encode_matrix, n, k);
	unsigned char **src, **dest;
	
	j = 0;
	for (i = 0; i < n; i++) {
		if (validstripes[i] == 1) {
			for (m = 0; m < k; m++) {
				errors_matrix[k * j + m] = encode_matrix[k * i + m];
			}
			j++;
		}
	}
	
	gf_invert_matrix(errors_matrix, invert_matrix, k);
	
	
	j = 0;
	for (i = 0; i < k; i++) {
		if (validstripes[i] == 0) {
			for (m = 0; m < k; m++) {
				decode_matrix[k * j + m] = invert_matrix[k * i + m];
			}
			j++;
		}
	}
	
	// IF DECODE MATRIX IS ALL ZEROES 
	//  IT MEANS WE ALREADY GET K DATA BLOCKS 
	// SO JUST RETURN
	for (i = 0; i < k * k; i++) {
		if (decode_matrix[i] != 0) {
			break;
		}
	}
	if (i == k*k) {
		free(encode_matrix);
		free(errors_matrix);
		free(invert_matrix);
		free(tables);
		return;
	}
	// INIT TABLES	
	ec_init_tables(k, n-k, decode_matrix, tables);

	// count number of fail/good column
	int fail = 0, good = 0;
	for (i = 0; i < n; i++) {
		if (validstripes[i] == 1)
			good++;
		else
			fail++;
	}
	for (i = 0; i < num_of_stripes; i++) {
		src  = (unsigned char **) calloc(good, sizeof(unsigned char **));
		dest = (unsigned char **) calloc(fail, sizeof(unsigned char **));
		for (j = 0; j < good; j++) {
			src[j] = (unsigned char *) calloc(block_size, sizeof(unsigned char *));
		}
		for (j = 0; j < fail; j++) {
			dest[j] = (unsigned char *) calloc(block_size, sizeof(unsigned char *));
		}
		m = 0;
		for (j = 0; j < n; j++) {
			if (validstripes[j] == 1) {
				if (j < k) {
					memcpy(src[m], ((*stripe)[i]).data_block[j], block_size);
				} else {
					memcpy(src[m], ((*stripe)[i]).parity_block[k-j], block_size);
				}
				m++;
			}
		}
		//ec_encode_data(block_size, good, fail, tables, src, dest);
		ec_encode_data(block_size, k, n-k, tables, src, dest);
		o = 0;
		
		for (l = 0; l < n; l++) {
			if (validstripes[l] == 0) {
				//printf("dest[%d]: %s\n", o, dest[o]);
				memcpy(((*stripe)[i]).data_block[l], dest[o], block_size);
				o++;
			}
		}
		for (j = 0; j < n-k; j++) {
			free(dest[j]);
		}
		for (j = 0; j < k; j++) {
			free(src[j]);
		}
		free(src);
		free(dest);
	}
	
	free(encode_matrix);
	free(errors_matrix);
	free(invert_matrix);
	free(tables);
}
