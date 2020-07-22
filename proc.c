#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "procq.h"
#include "spinlock.h"

struct {
	struct spinlock lock;
	struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan)
{
	struct proc *p;

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->state == SLEEPING && p->chan == chan)
			p->state = RUNNABLE;
}

int checkFull(int num)
{
	if((front[num]==rear[num]+1)||(front[num]==0&&rear[num]==199))
		return 1;
	return 0;
}

int checkEmpty(int num)
{
	if(front[num]==-1)
		return 1;
	return 0;
}

struct proc*
delet(int num)
{
		// cprintf("deque called ***********\n");
		qsize[num]--;
		struct proc* p;
		p = q[num][front[num]];
		if (front[num] == rear[num]){
				front[num] = -1;
				rear[num] = -1;
		} /* Q has only one element, so we reset the queue after dequeing it. ? */
		else {
				front[num] = (front[num] + 1) % 100;
		}
		// // cprintf("\n Deleted %d\n", element->pid);
		return p;
}

void insert(int num, struct proc* p)
{
	int i;
	for(i=0;i<5;i++)
	{
		int y=rear[i];
		int x=front[i];
		if(front[i]<=rear[i])
		{
			for(int j=x;j>=0&&j<=y;j++)
			{
				if(p==q[i][j])
					return;
			}
		}
		else
		{
			for(int j=0;j<200;j++)
			{
				if(j<=y||j>=x)
					return;
			}
		}
	}

	if(checkFull(num))
		cprintf("\n Queue is full.\n");
	else
	{
		qsize[num]++;
		p->curtime=0;
		if(front[num]==-1)
			front[num]=0;
		rear[num]=(rear[num]+1)%100;
		q[num][rear[num]]=p;
	}
}
void
pinit(void)
{
	initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
	return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
	int apicid, i;
	
	if(readeflags()&FL_IF)
		panic("mycpu called with interrupts enabled\n");
	
	apicid = lapicid();
	// APIC IDs are not guaranteed to be contiguous. Maybe we should have
	// a reverse map, or reserve a register to store &cpus[i].
	for (i = 0; i < ncpu; ++i) {
		if (cpus[i].apicid == apicid)
			return &cpus[i];
	}
	panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
	struct cpu *c;
	struct proc *p;
	pushcli();
	c = mycpu();
	p = c->proc;  
	popcli();
	return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
	struct proc *p;
	char *sp;

	acquire(&ptable.lock);

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->state == UNUSED)
			goto found;

	release(&ptable.lock);
	return 0;

found:
	p->state = EMBRYO;
	p->pid = nextpid++;
	release(&ptable.lock);
	p->curr_q=0;
	p->priority = 60;

	// Allocate kernel stack.
	if((p->kstack = kalloc()) == 0){
		p->state = UNUSED;
		return 0;
	}
	sp = p->kstack + KSTACKSIZE;
	p->num_run=0;
	p->curtime=0;
	// Leave room for trap frame.
	sp -= sizeof *p->tf;
	p->tf = (struct trapframe*)sp;
	for(int i=0;i<5;i++)
		p->time[i]=0;
	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= 4;
	*(uint*)sp = (uint)trapret;
	#ifdef MLFQ
		for(int i=0;i<5;i++)
		{
			if(!checkFull(i)){
				insert(i,p);
				break;
			}
		}
	#endif
	sp -= sizeof *p->context;
	p->context = (struct context*)sp;
	memset(p->context, 0, sizeof *p->context);
	p->context->eip = (uint)forkret;

	//adding time fields
	p->stime = ticks;         // start time
	p->etime = 0;             // end time
	p->rtime = 0;             // run time
	p->iotime = 0;            // I/O time

	return p;
}

void updateProc(void)
{
	acquire(&ptable.lock);
	struct proc *p;
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->state == RUNNING){  
			#ifdef MLFQ
				p->time[p->curr_q]++;
				p->curtime++;     
			#endif
			p->rtime++;
		}
		else
		{ 
			#ifdef MLFQ
				if((p->wtime > MAXAGE) && ( p->curr_q !=0))
				{
					p->curtime = 0;
					p->curr_q--;
					p->wtime = 0;
					insert(p->curr_q, p);
					// cprintf("AGING %d\n",p->pid);
				}
			#endif
			p->wtime++;
		}  
	}
	release(&ptable.lock);
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];

	p = allocproc();
	
	initproc = p;
	if((p->pgdir = setupkvm()) == 0)
		panic("userinit: out of memory?");
	inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
	p->sz = PGSIZE;
	memset(p->tf, 0, sizeof(*p->tf));
	p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
	p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
	p->tf->es = p->tf->ds;
	p->tf->ss = p->tf->ds;
	p->tf->eflags = FL_IF;
	p->tf->esp = PGSIZE;
	p->tf->eip = 0;  // beginning of initcode.S

	safestrcpy(p->name, "initcode", sizeof(p->name));
	p->cwd = namei("/");

	// this assignment to p->state lets other cores
	// run this process. the acquire forces the above
	// writes to be visible, and the lock is also needed
	// because the assignment might not be atomic.
	acquire(&ptable.lock);

	p->state = RUNNABLE;

	release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
	uint sz;
	struct proc *curproc = myproc();

	sz = curproc->sz;
	if(n > 0){
		if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
			return -1;
	} else if(n < 0){
		if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
			return -1;
	}
	curproc->sz = sz;
	switchuvm(curproc);
	return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
	int i, pid;
	struct proc *np;
	struct proc *curproc = myproc();

	// Allocate process.
	if((np = allocproc()) == 0){
		return -1;
	}

	// Copy process state from proc.
	if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
		kfree(np->kstack);
		np->kstack = 0;
		np->state = UNUSED;
		return -1;
	}
	np->sz = curproc->sz;
	np->parent = curproc;
	*np->tf = *curproc->tf;

	// Clear %eax so that fork returns 0 in the child.
	np->tf->eax = 0;

	for(i = 0; i < NOFILE; i++)
		if(curproc->ofile[i])
			np->ofile[i] = filedup(curproc->ofile[i]);
	np->cwd = idup(curproc->cwd);

	safestrcpy(np->name, curproc->name, sizeof(curproc->name));

	pid = np->pid;

	acquire(&ptable.lock);

	np->state = RUNNABLE;

	release(&ptable.lock);

	return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
	struct proc *curproc = myproc();
	struct proc *p;
	int fd;
	if(curproc == initproc)
		panic("init exiting");
	curproc->etime=ticks;
	// Close all open files.
	for(fd = 0; fd < NOFILE; fd++){
		if(curproc->ofile[fd]){
			fileclose(curproc->ofile[fd]);
			curproc->ofile[fd] = 0;
		}
	}

	begin_op();
	iput(curproc->cwd);
	end_op();
	curproc->cwd = 0;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(curproc->parent);

	// Pass abandoned children to init.
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->parent == curproc){
			p->parent = initproc;
			if(p->state == ZOMBIE)
				wakeup1(initproc);
		}
	}

	// Jump into the scheduler, never to return.
	curproc->state = ZOMBIE;
	sched();
	panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
	struct proc *p;
	int havekids, pid;
	struct proc *curproc = myproc();
	
	acquire(&ptable.lock);
	for(;;){
		// Scan through table looking for exited children.
		havekids = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->parent != curproc)
				continue;
			havekids = 1;
			if(p->state == ZOMBIE){
				// Found one.
				pid = p->pid;
				kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->state = UNUSED;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if(!havekids || curproc->killed){
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock);  //DOC: wait-sleep
	}
}
int
waitx(int* wtime, int* rtime)
{
	struct proc *p;
	int havekids, pid;
	struct proc *curproc = myproc();
	
	acquire(&ptable.lock);
	for(;;){
		// Scan through table looking for exited children.
		havekids = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->parent != curproc)
				continue;
			havekids = 1;
			if(p->state == ZOMBIE){
				// Found one.

			 *wtime = p->etime - p->stime - p->rtime;
			 *rtime = p->rtime;
				pid = p->pid;
				kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->state = UNUSED;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if(!havekids || curproc->killed){
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock);  //DOC: wait-sleep
	}
}
//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
scheduler(void)
{
	struct proc *p;
	struct cpu *c = mycpu();
	c->proc = 0;
	
	#ifdef DEFAULT
		for(;;)
		{
			// Enable interrupts on this processor.
			sti();

			// Loop over process table looking for process to run.
			acquire(&ptable.lock);
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state != RUNNABLE)
					continue;

				// Switch to chosen process.  It is the process's job
				// to release ptable.lock and then reacquire it
				// before jumping back to us.
				if(p!=0)
				{
					c->proc = p;
					switchuvm(p);
					p->state = RUNNING;

					swtch(&(c->scheduler), p->context);
					switchkvm();

					// Process is done running for now.
					// It should have changed its p->state before coming back.
					c->proc = 0;
				}
			}
			release(&ptable.lock);
		}
	#else
	#ifdef PBS
		for(;;)
		{
			sti();
			acquire(&ptable.lock);
			for(p=ptable.proc; p<&ptable.proc[NPROC];p++)
			{
				struct proc *minP = 0;
				struct proc *p1 = 0;

				if(p->state != RUNNABLE)
					continue;
				// Choose the process with highest priority (among RUNNABLEs)
				minP = p;
				for(p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++){
					if((p1->state == RUNNABLE) && (minP->priority > p1->priority))
						minP = p1;
				}

				if(minP != 0)
					p = minP; 
				// Switch to chosen process.  It is the process's job
				// to release ptable.lock and then reacquire it
				// before jumping back to us.
				if(p!=0)
				{
					cprintf("%d %d\n",p->priority,p->pid);
					c->proc = p;
					switchuvm(p);
					p->state = RUNNING;

					swtch(&(c->scheduler), p->context);
					switchkvm();

					// Process is done running for now.
					// It should have changed its p->state before coming back.
					c->proc = 0;
				}
			}
			release(&ptable.lock);
		}
	#else
	#ifdef FCFS
		for(;;)
		{
			sti();
			acquire(&ptable.lock);
			for(p=ptable.proc;p<&ptable.proc[NPROC];p++)
			{
				struct proc *minP = 0;

				if(p->state != RUNNABLE)
					continue;

				// ignore init and sh processes from FCFS
				if(p->pid > 1)
				{
					if (minP != 0){
						// here I find the process with the lowest creation time (the first one that was created)
						if(p->stime < minP->stime)
							minP = p;
					}
					else
							minP = p;
				}

				// If I found the process which I created first and it is runnable I run it
				//(in the real FCFS I should not check if it is runnable, but for testing purposes I have to make this control, otherwise every time I launch
				// a process which does I/0 operation (every simple command) everything will be blocked
				if(p!=0)
				{
					// cprintf("%d\n",p->pid );
						if(minP != 0 && minP->state == RUNNABLE)
							p = minP;
						c->proc = p;
						switchuvm(p);
						p->state = RUNNING;

						swtch(&(c->scheduler), p->context);
						switchkvm();

						// Process is done running for now.
						// It should have changed its p->state before coming back.
						c->proc = 0;
					}
			}
			release(&ptable.lock);
		}
	#else
	#ifdef MLFQ
		int flag = 0;
		for(;;){
			// Enable interrupts on this processor.
			sti();
			acquire(&ptable.lock);
			if(!checkEmpty(0)){
				int cnt = 0;
				while(1){
					if(cnt==qsize[0])
						break;
					cnt++;
					p = delet(0);
					flag = 1;
					if(p->state != RUNNABLE){
						insert(0, p);
						flag = 0;
					}
					else{
						cnt=qsize[0];
					}
				}
			}
			if(!checkEmpty(1) && flag==0){
				int cnt = 0;
				while(1){
					if(cnt==qsize[1])
						break;
					cnt++;
					p = delet(1);
					flag = 1;
					if(p->state != RUNNABLE){
						insert(1, p);
						flag = 0;
					}
					else
						cnt=qsize[1];
				}
			}
			if(!checkEmpty(2) && flag==0){
				int cnt = 0;
				while(1){
					if(cnt==qsize[2])
						break;
					cnt++;
					p = delet(2);
					flag = 1;
					if(p->state != RUNNABLE){
						insert(2, p);
						flag = 0;
					}
					else
						cnt=qsize[2];
				}
			}
			if(!checkEmpty(3) && flag==0){
				int cnt = 0;
				while(1){
					if(cnt==qsize[3])
						break;
					cnt++;
					p = delet(3);
					flag = 1;
					if(p->state != RUNNABLE){
						insert(3, p);
						flag = 0;
					}
					else
						cnt=qsize[3];
				}
			} 
			if(!checkEmpty(4) && flag==0){
				int cnt = 0;
				while(1){
					if(cnt==qsize[4])
						break;
					cnt++;
					p = delet(4);
					flag = 1;
					if(p->state != RUNNABLE){
						insert(4, p);
						flag = 0;
					}
					else
						cnt=qsize[4];
				}
			}
			if(flag == 1 && p->state == RUNNABLE){
				// cprintf("%d %d\n",p->pid,p->curr_q);
				p->num_run++;
				p->wtime = 0;
				flag = 0;  
				c->proc = p;
				switchuvm(p);
				p->state = RUNNING;

				// cprintf("pid = %d, curr_q = %d, waittime = %d size = %d runtime = %d\n", p->pid, p->curr_q, p->wtime, qsize[p->curr_q], p->rtime);

				swtch(&(c->scheduler), p->context);
				switchkvm();
				p = 0;      
				c->proc = 0;
			}

			if(flag == 0)
			{
				for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
					if(p->state != RUNNABLE || p -> pid == 0)
						continue;
				insert(0, p);
			 }
			}
			release(&ptable.lock);
		}
	#endif
	#endif
	#endif
	#endif
}

int 
checkpriority(int priority, int flag)
{
	struct proc *p;
	if(flag==1)
	{
		acquire(&ptable.lock);
		for(p=ptable.proc;p<&ptable.proc[NPROC];p++)
		{
			if(p->priority>priority)
			{
				release(&ptable.lock);
				return 1;
			}
		}
		release(&ptable.lock);
	}
	else
	{
		acquire(&ptable.lock);
		for(p=ptable.proc;p<&ptable.proc[NPROC];p++)
		{
			if(p->priority>=priority)
			{
				release(&ptable.lock);
				return 1;
			}
		}
		release(&ptable.lock);
	}
	return 0;
}

int
set_priority(int new_priority, int pid)
{
	struct proc *p;
	int prev_priority=-1;
	acquire(&ptable.lock);
	for(p=ptable.proc;p<&ptable.proc[NPROC];p++)
	{
		if(p->pid==pid)
		{
			cprintf("New priority: %d\n",new_priority);
			prev_priority=p->priority;
			p->priority=new_priority;
			break;
		}
	}
	release(&ptable.lock);
	return prev_priority;
}


void
sched(void)
{
	int intena;
	struct proc *p = myproc();

	if(!holding(&ptable.lock))
		panic("sched ptable.lock");
	if(mycpu()->ncli != 1)
		panic("sched locks");
	if(p->state == RUNNING)
		panic("sched running");
	if(readeflags()&FL_IF)
		panic("sched interruptible");
	intena = mycpu()->intena;
	swtch(&p->context, mycpu()->scheduler);
	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
	acquire(&ptable.lock);  //DOC: yieldlock
	myproc()->state = RUNNABLE;
	// cprintf("Yield CAlled");
	#ifdef MLFQ
		// cprintf("Premption %d with curtime %d\n",myproc()->pid,myproc()->curtime);
		myproc()->curtime=0;
		myproc()->curr_q++;
		insert(myproc()->curr_q,myproc());
	#endif
	sched();
	release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
	static int first = 1;
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	if (first) {
		// Some initialization functions must be run in the context
		// of a regular process (e.g., they call sleep), and thus cannot
		// be run from main().
		first = 0;
		iinit(ROOTDEV);
		initlog(ROOTDEV);
	}

	// Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
	struct proc *p = myproc();
	
	if(p == 0)
		panic("sleep");

	if(lk == 0)
		panic("sleep without lk");

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.
	if(lk != &ptable.lock){  //DOC: sleeplock0
		acquire(&ptable.lock);  //DOC: sleeplock1
		release(lk);
	}
	// Go to sleep.
	p->chan = chan;
	p->state = SLEEPING;

	sched();

	// Tidy up.
	p->chan = 0;

	// Reacquire original lock.
	if(lk != &ptable.lock){  //DOC: sleeplock2
		release(&ptable.lock);
		acquire(lk);
	}
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
// static void
// wakeup1(void *chan)
// {
//   struct proc *p;

//   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     if(p->state == SLEEPING && p->chan == chan)
//       p->state = RUNNABLE;
// }

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
	struct proc *p;

	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->pid == pid){
			p->killed = 1;
			// Wake process from sleep if necessary.
			if(p->state == SLEEPING)
				p->state = RUNNABLE;
			release(&ptable.lock);
			return 0;
		}
	}
	release(&ptable.lock);
	return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
	static char *states[] = {
	[UNUSED]    "unused",
	[EMBRYO]    "embryo",
	[SLEEPING]  "sleep ",
	[RUNNABLE]  "runble",
	[RUNNING]   "run   ",
	[ZOMBIE]    "zombie"
	};
	int i;
	struct proc *p;
	char *state;
	uint pc[10];

	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->state == UNUSED)
			continue;
		if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		cprintf("%d %s %s", p->pid, state, p->name);
		if(p->state == SLEEPING){
			getcallerpcs((uint*)p->context->ebp+2, pc);
			for(i=0; i<10 && pc[i] != 0; i++)
				cprintf(" %p", pc[i]);
		}
		cprintf("\n");
	}
}

int getpinfo(struct proc_stat* p, int pid)
{
	struct proc* p1 = 0;
	acquire(&ptable.lock);
	for(p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++){
		if(p1->pid == pid)
		{
			p->pid = p1->pid;
			p->current_queue = p1->curr_q;
			p->runtime = p1->rtime;
			p->num_run = p1->num_run;
			
			for(int i=5; i>=0; i--){
				p->ticks[i] = p1->time[i]; 
			}

			release(&ptable.lock);
			return 1;
		}
	}
	
	release(&ptable.lock);

	return 0;
}