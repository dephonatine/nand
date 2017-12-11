#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

void main(int argc, char *argv[]) {
	char str2[] = "(LEWP)";
	uint8_t *tok = strtok(str2, ")");
	++tok;
	printf("%s", tok);

}
