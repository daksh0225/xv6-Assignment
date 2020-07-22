#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[])
{
	if(argc != 3){
		printf(1, "Invalid number of arguments\n");
		exit();
	}
	int priority = atoi(argv[1]);
	int pid = atoi(argv[2]);
	int pre_priority = set_priority(priority, pid);
	if(pre_priority == -1){
		printf(1, "Invalid pid\n");
		exit();
	}
	printf(1, "Previous priority: %d\n", pre_priority);
	exit();
}