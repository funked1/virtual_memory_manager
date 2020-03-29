#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char** argv)
{
	char filename[] = "addresses.txt";
	FILE *fp;
	char str[5];


	/* Open file for reading */
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("Error opening file");
		return(-1);
	}

	/* Read address file line by line */
	while (fgets(str, 6, fp) != NULL) {
		puts(str);
	}
	fclose(fp);

	return(0);
}
