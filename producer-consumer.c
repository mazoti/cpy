#include "producer-consumer.h"

#define EMPTY     BUFFER_SIZE+1
#define IO_ERROR  BUFFER_SIZE+2
#define TERMINATE BUFFER_SIZE+3

/* Shared variables between threads */
static mtx_t  mutex[BUFFERS];
static char   buffer[BUFFERS][BUFFER_SIZE];
static size_t bytes_produced[BUFFERS];

/*
Insert TERMINATE code on first unlocked buffer and the
threads will stop; this way the program ends gracefully
*/
static void quit(int sig){
	for(int i=0;;){
		if(mtx_trylock(&mutex[i]) == thrd_busy) {
			i=(i+1)%BUFFERS;
			continue;
		}

		bytes_produced[i] = TERMINATE;
		printf("\nTerminated BY USER with signal %d\n", sig);
		mtx_unlock(&mutex[i]);
		break;
	}
}

static int producer(void* input){
	for(int i=0;;){
		if(mtx_trylock(&mutex[i]) == thrd_busy) continue;

		if(bytes_produced[i] == EMPTY){
			bytes_produced[i] = fread(&buffer[i][0], 1, BUFFER_SIZE, (FILE*)input);

			/* Last read */
			if(bytes_produced[i] != BUFFER_SIZE){
				if(ferror((FILE*)input)){
					bytes_produced[i] = IO_ERROR;
					mtx_unlock(&mutex[i]);
					return 1;
				}

				/* No errors */
				mtx_unlock(&mutex[i]);
				return 0;
			}

			mtx_unlock(&mutex[i]);
			i=(i+1)%BUFFERS;
			continue;
		}

		/* Ctrl + c or terminate signal */
		if(bytes_produced[i] == TERMINATE){
			mtx_unlock(&mutex[i]);
			return 3;
		}

		/* Disk full, bad block or something worse */
		if(bytes_produced[i] == IO_ERROR){
			mtx_unlock(&mutex[i]);
			return 0;
		}

		/* Empty buffer, still consuming */
		mtx_unlock(&mutex[i]);
	}

	return 0;
}

static int consumer(void* output){
	int i;
	size_t wrote;

	for(i=0;;){
		if(mtx_trylock(&mutex[i]) == thrd_busy) continue;

		if(bytes_produced[i] < EMPTY){

			wrote = fwrite(&buffer[i][0], 1, bytes_produced[i], (FILE*)output);

			/* Last write */
			if(wrote != BUFFER_SIZE){
				if(ferror((FILE*)output)){
					bytes_produced[i] = IO_ERROR;
					mtx_unlock(&mutex[i]);
					return 2;
				}

				/* No errors */
				mtx_unlock(&mutex[i]);
				return 0;
			}

			bytes_produced[i] = EMPTY;
			mtx_unlock(&mutex[i]);
			i=(i+1)%BUFFERS;
			continue;
		}

		/* Ctrl + c or terminate signal */
		if(bytes_produced[i] == TERMINATE){
			mtx_unlock(&mutex[i]);
			return 3;
		}

		/* Disk full, bad block or something worse */
		if(bytes_produced[i] == IO_ERROR){
			mtx_unlock(&mutex[i]);
			return 0;
		}

		/* Empty buffer, still producing */
		mtx_unlock(&mutex[i]);
	}

	return 0;
}

int mainpc(FILE* input, FILE* output){
	int i, cons_res, prod_res;
	thrd_t cons, prod;

	for(i=0; i<BUFFERS; ++i){
		mtx_init(&mutex[i], mtx_plain);
		bytes_produced[i] = EMPTY;
	}

	thrd_create(&prod, producer, input);
	thrd_create(&cons, consumer, output);

	signal(SIGINT,  quit);
	signal(SIGTERM, quit);

	thrd_join(prod, &prod_res);
	thrd_join(cons, &cons_res);

	for(i=0; i<BUFFERS;++i) mtx_destroy(&mutex[i]);

	return prod_res + cons_res;
}
