#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[])
{
	int runTime, waitTime;
 	for(int i=0; i<argc-1; i++)
 		argv[i] = argv[i+1];
 	argv[argc-1] = 0; 	
	int pid = fork();
	if(pid == 0)
	{
		exec(argv[0], argv);
		printf(1, "Error: cannot exec\n");
	}
	else
		waitx(&waitTime, &runTime);
	printf(1, "Process run time = %d, and wait time = %d\n", runTime, waitTime);
	exit();
}