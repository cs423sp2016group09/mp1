#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "userapp.h"

long long unsigned fac(int n) {
	long long unsigned f = 1;
	int i;
	for (i = 2; i <= n; i++) {
		f *= n;
	}

	return f;
}

int main(int argc, char* argv[])
{
	// get pid 
	int pid = getpid();

	// write pid as a string to the proc filesystem
	FILE * statusfile = fopen("/proc/mp1/status", "r+");
	fprintf(statusfile, "%d", pid);
	// read the proc fs

	fclose(statusfile);
	
	long long x = 0;

	int i, j;
	for (j=0; j < 30; j++) {
		for (i = 0; i < 10000; i++) {
			x = fac(33);		
			printf("%llu\n", x);
		}

		sleep(10);
	}	
	return 0;
}
