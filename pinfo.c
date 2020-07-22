#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "procq.h"

int main(int argc, char *argv[]) {
    if(argc !=2) {
        printf(1, "Invalid number of arguments\n");
    }
    else {
        struct proc_stat p;
        int pid = atoi(argv[1]);
        int status = getpinfo(&p, pid);
        if(status == 0) {
            printf(1, "Invalid pid.....aborting!\n");
        }
        else {
            printf(1, "pid: %d\n", p.pid);
            printf(1, "runtime: %d\n", p.runtime);
            printf(1, "num_run: %d\n", p.num_run);
            printf(1, "current_queue: %d\n", p.current_queue + 1);
            for(int i=0; i<5; i++) {
                printf(1, "Number of ticks spent in queue %d: %d\n", i+1, p.ticks[i]);
            }

        }
    }

    exit();
}