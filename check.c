#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include <stddef.h>

int main(int argc, char *argv[]) {
    int k, n, id;
    double x = 0, z;
    // set_priority(4, 70);
    if (argc < 2)
        n = 1;  // default value
    else
        n = atoi(argv[1]);  // from command line

    if (n < 0 || n > 20)
        n = 2;

    x = 0;
    id = 0;
    for (k = 0; k < n; k++) {
        id = fork();

        // printf(1, "%d\n", o);
        if (id < 0) {
            printf(1, "%d failed in fork!\n", getpid());
        } else if (id > 0) {  // parent
            // printf(1, "Parent %d creating child %d\n", getpid(), id);
            // wait();
        } else {  // child
            // printf(1, "Child %d created\n", getpid());
            for (z = 0; z < 3000000.0; z += 0.1)
                x = x +
                    3.14 * 89.64;  // useless calculations to consume CPU time
            exit();
        }
        // set_priority(6, 80);
    }
    for (k = 0; k < n; k++) {
        // struct proc_stat q, *r;
        // int pd = 4;
        // waitx(&a, &b);
        wait();
        // printf(1, "%d %d\n", a, b);
        // struct proc_stat *r = &q;
        // printf(1, "et\n");

        // int c = getpinfo(&q, pd);
        // if (c == 1) {
        //     printf(1, "%p %p\n", &q, q.pid);
        //     r = &q;
        //     printf(1, "%p\n", r->pid);
        //     // printf(1, "%d \n", *(r->pid));
        // }

        // printf(1, "%p %p %p %p\n", (q.pid), (q.current_queue), (q.num_run),
        //        (q.runtime));
        // waitx(&a, &b);
        // printf(1, "%d %d\n", a, b);
    }
    exit();
}