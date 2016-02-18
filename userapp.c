#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "userapp.h"


int main(int argc, char* argv[])
{
	// get pid 
	int pid = getpid();

	// write pid as a string to the proc filesystem
	FILE * statusfile = fopen("/proc/mp1/status", "r+");
	fprintf(statusfile, "%d", pid);
	// read the proc fs

	fclose(statusfile);

	return 0;
}
