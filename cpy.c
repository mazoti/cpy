#include "producer-consumer.h"

int main(int argc, char** argv){
	int err;
	FILE *input, *output;

	/* Command line validation */
	if(argc != 3){
		fprintf(stderr, "Usage: %s <source file> <destination file>\n", argv[0]);
		return 1;
	}

	/* Input and output validation, replace to fopen_s ASAP */
	input = fopen(argv[1], "rb");
	if(input == NULL){
		fprintf(stderr, "ERROR: source file \"%s\" does not exist!\n", argv[1]);
		return 2;
	}

	output = fopen(argv[2], "rb");
	if(output != NULL){
		fclose(output);
		fclose(input);
		fprintf(stderr, "ERROR: destination file \"%s\" exists!\n", argv[2]);
		return 3;
	}

	/* Output does not exists, create a new one */
	output = fopen(argv[2], "wb");
	if(output == NULL){
		fclose(input);
		fprintf(stderr, "ERROR: can't create destination file \"%s\"\n", argv[2]);
		return 4;
	}

	/* Everything is ok, send file pointers to producer-consumer module */
	err = mainpc(input, output);

	fclose(output);
	fclose(input);

	if(!err) return 0;

	remove(argv[2]);

	/* Disk full or hardware problem */
	if(err != 6) fprintf(stderr, "ERROR: can't read or write\n");

	/* User pressed Ctrl+C */
	return err;
}
