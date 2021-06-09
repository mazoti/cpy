#include <stdio.h>
#include <signal.h>
#include <threads.h>

/* 
For better performance, find the best values here
and keep aligned with your CPU architecture (ex. 32 bits = 4*n, 64 bits = 8*n...)

OBS: total memory usage will be BUFFERS * BUFFER_SIZE bytes
*/

#define BUFFERS 8
#define BUFFER_SIZE 4096

int mainpc(FILE* input, FILE* output);
