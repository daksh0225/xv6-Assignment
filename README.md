# xv6-OS Assignment
As part of this assignment, I have tweaked the xv6 OS to implement new features.

## Getting Started
Requirements: QEMU 3.0 or above
1. ```make clean```
2. ```make SCHEDPOL <flag>```
3. ```make qemu -nox SCHEDPOL <flag>```

*flag values* : DEFAULT, PBS, FCFS, MLFQ

## Task 1
### waitx:

 - waitx syscall returns the time spent in running and waiting by a particular process.
 - Both times are given in terms of ticks defined in xv6.
 - To check the command use time.c. It takes an argument which is the command whose runtime and waititme we want to find. Type 'time <command-name>'.

### getpinfo:

 - getpinfo command is used to get information like pid, runtime, number of times it was executed, current queue in which the process is currently waiting, and number of ticks spent in each queue.
 - Different queues are implemented in *Task 2*

## Task 2

Different scheduling policies are implemented for xv6 in task 2.

### FCFS:

 - In FCFS, the process which arrives first is implemented first.
 - There is no preemption in this scheduling.
 - This scheduling just selects the process from *proc table* with least start time i.e. the process which arrived earliest and executes it until it finishes. Only then a new process is selected for execution.

### Priority Based Scheduling:

 - In this scheduling, processes are selected based on priority.
 - process with highest priority is selected from *proc table*
 - preemption is done if a process with higher property arrives.
 - when there are processes with same priority then "round robin" policy is used.
 - both the above functionalities are achieved in *trap.c* by calling checkpriority() with suitable argument and checking if there is a need of preemption. If there is a need than *yield()* is called

### Multi-level Feedback queue scheduling

 - Processes are allocated different queues. Each queue has a different priority.
 - A *RUNNABLE* process is selected from the queue with highest priority possible. 
 - In *update_proc_time()* we check if the *wtime* (wait time) of a process has crossed a particular value. If it has than it is promoted to a queue of higher priority to prevent starvation. 
 - *yield()*  in case of MLFQ is only called if a process has been running for a very long time in a queue (specific number of allowed clock ticks for different queue). If it has, than it is demoted to a queue of lower priority and scheduler is called again. Calling the *yield()* function  at suitable times is taken care of in *trap.c*