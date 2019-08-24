#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>


#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

struct process{
        pid_t mypid;
        struct process* next;
        char* name;
        int data;
	int priority;
};
/*Definition of my list nodes*/
struct process *head=NULL;
struct process *newnode=NULL;
//struct process *delete=NULL;
struct process *last=NULL;
//struct process *running=NULL;
int i=0;
int nproc;
/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	//assert(0 && "Please fill me!");
	struct process * temp=head;
	while (temp!=NULL){
		printf("ID: %d PID: %i Name: %s Priority:%d\n",temp->data,temp->mypid,temp->name,temp->priority);
		if (temp->next != NULL)
			temp=temp->next;
		else
			break;
	}
	
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
	//assert(0 && "Please fill me!");
	struct process * temp=head;
	while (temp!=NULL){
		if (temp->data == id ){
			kill(temp->mypid,SIGKILL);
			return 0;
		}
		else	
			if(temp->next!=NULL)
				temp=temp->next;
			else
				break;	
	}
	return -ENOSYS;
}

/*Fix the priorities*/

static int
sched_set_high_p(int id){
	struct process * temp=head;
	//struct process * prev=NULL;
	//set the priority of a desired process to high and put it in the head of my list
        while (temp!=NULL){
                if (temp->data == id ){
                        temp->priority=1;
			return 0;
                }
                else
                        if(temp->next!=NULL){
                                temp=temp->next;
                        }
			else{
                                break;
			}
        }
        return -ENOSYS;
}

static int
sched_set_low_p(int id){
 	struct process * temp=head;
        while (temp!=NULL){
                if (temp->data == id ){
                       temp->priority=0;
                       return 0;
                }
                else
                        if(temp->next!=NULL)
                                temp=temp->next;
                        else
                                break;
        }
        return -ENOSYS;


}




/* Create a new task.  */
static void
sched_create_task(char *executable)//
{
		//assert(0 && "Please fill me!");
		pid_t mypid;
		mypid=fork();
		
		//char executable[] = "prog";
        	char *newargv[] = { executable, NULL, NULL, NULL };
	        char *newenviron[] = { NULL };

                if (mypid<0){
                        printf("Error with forks\n");
                }
                if(mypid==0){
                        raise(SIGSTOP);
                        printf("I am %s, PID = %ld\n",
                        executable, (long)getpid());
                        printf("About to replace myself with the executable %s...\n",
                                executable);
                        sleep(2);

                        execve(executable, newargv, newenviron);

                        /* execve() only returns on error */
                        perror("execve");
                        exit(1);
                        }
                else{  
			//insert the new node
			newnode=(struct process *)malloc(sizeof(struct process));
                        if (newnode==NULL) printf("Error with malloc\n");
                        newnode->mypid=mypid;
                        newnode->next=NULL;
                        newnode->data=i+1; 
			newnode->priority=0;                       
                        //strcpy(newnode->name,executable);                        
                        newnode->name=(char*)malloc(strlen(executable)+1);
			strcpy(newnode->name,executable);
                        last->next=newnode;
                        last=newnode;
                        nproc++;
                        i++;
                }

}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;
//
		case REQ_HIGH_TASK:
			sched_set_high_p(rq->task_arg);
			return 0;

		case REQ_LOW_TASK:
			sched_set_low_p(rq->task_arg);
			return 0;
//
		default:
			return -ENOSYS;
	}
}

/* 
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	//assert(0 && "Please fill me!");
        kill(head->mypid,SIGSTOP);
        //alarm(SCHED_TQ_SEC);
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
        //assert(0 && "Please fill me!");
        pid_t p;
        int status;
	int flag=0;
        while(1){
                p=waitpid(-1,&status,WNOHANG|WUNTRACED);
                if (p<0){
                        perror("waitpid");
                        exit(1);
                }
                if(p==0) break;         /////////

                explain_wait_status(p,status);
                //p has the pid of our child that has changed its status

                if (WIFEXITED(status) || WIFSIGNALED(status)){//if terminated or killed by a signal remove it from the list
                        struct process* delete=NULL;
                        struct process * temp;
                        temp=head;

                        while(temp!=NULL){
                                if (temp->mypid==p && temp==head){ //if i got to delete the head
                                                if (temp->next == NULL){
                                                         free(temp);
                                                         printf("I am done. Waiting for new processes\n");
                                                         //exit(0);
                                                }
                                                else{
                                                        head=temp->next;
                                                        free(temp);
                                                }
                                }
                                else if (temp->mypid==p && temp==last){ //if i got to delete the last element of the list
                                        last=delete;
                                        last->next=NULL;
                                        free(temp);
                                }
                                else if(temp->mypid==p){ //if i delete a random element in the list         
                                        delete->next=temp->next;
                                        free(temp);

                                }
                                else{ //if i got to continue searching , will never access any node after the last element
                                        delete=temp;
                                        temp=temp->next;
                                        continue;
                                }
                                break;
                        }
			temp=head;
			struct process *current = head;
			//struct process * prev = NULL;
                        //search for high priorities
                        while (temp!=NULL){
                        	if (temp->priority!=1){ //move this node to the end
                                	last->next=head;
                                        last=head;
                                        head=head->next;
                                        last->next=NULL;
                                        temp=head;
                                        if (temp == current) break;
                                        	continue;
                                        }
                                        else{
                                                flag=1;
                                                break;
                                      }
			}


                }

                if (WIFSTOPPED(status)){
                        //if our process is stopped by the scheduler, continue with the next one according to priorities
			if (head==NULL) {printf("Empty list\n"); exit(0);}
			if (head->next != NULL){
				last->next=head;//send my current process to the end
                        	last=head;
				head=head->next;
				last->next=NULL;
				struct process * current = head;
                        	struct process * temp = head;
				//struct process * prev = NULL;
				//search for high priorities
				while (temp!=NULL){
					if (temp->priority!=1){ //move this node to the end
						last->next=head;
						last=head;
						head=head->next;
						last->next=NULL;
						temp=head;
						if (temp == current) break;
						continue;
					}
					else{
						flag=1;
						break;
					}
						
					
				}
			}
			

                }

		if (WIFSTOPPED(status)){
			if(head->data==0 && (head->priority == 1 || flag == 0)){
				printf("****Welcome to the shell again****\n");
				//printf("Shell>");
				alarm(6);
			}
			else
                		alarm(SCHED_TQ_SEC);
        	}
                kill(head->mypid,SIGCONT);


        }

}


/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
	}
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
	//insert shell as head of my list
	head->data=0;
	head->mypid=p;
	head->priority=0;
	head->name="shell";
	head->next=NULL;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;
	
	head=(struct process *)malloc(sizeof(struct process));
	last=head;
	/* Create the shell. */
	sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	/* TODO: add the shell to the scheduler's tasks */

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	nproc = argc-1; /* number of proccesses goes here */
		
	char executable[] = "prog";
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };

        nproc = argc-1; /* number of proccesses goes here */
        if (nproc < 0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

        pid_t mypid;
        //int i;
        /*Fork and create the list*/
        for (i=0;i<nproc;i++){
                mypid=fork();
                if (mypid<0){
                        printf("Error with forks\n");
                }
                if(mypid==0){
                        raise(SIGSTOP);
                        printf("I am %s, PID = %ld\n",
                        argv[0], (long)getpid());
                        printf("About to replace myself with the executable %s...\n",
                                executable);
                        sleep(2);

                        execve(executable, newargv, newenviron);

                        /* execve() only returns on error */
                        perror("execve");
                        exit(1);
                        }
                else{
                        if (head==NULL){//will never enter though
                                //printf("lala\n");
				head=(struct process *)malloc(sizeof(struct process));
                                if (head==NULL) printf("Error with malloc\n");				
                                head->mypid=mypid;
                                head->next=NULL;
                                head->data=i+1;
				head->priority=0;
                                head->name=argv[i+1];
				last=head;
                        }
			else{
                                newnode=(struct process *)malloc(sizeof(struct process));
                                if (newnode==NULL) printf("Error with malloc\n");
                                newnode->mypid=mypid;
                                newnode->next=NULL;
                                newnode->data=i+1;
				newnode->priority=0;
                                newnode->name=argv[i+1];
                                last->next=newnode;
                                last=newnode;
                        }
                }


        }

		
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();
	
        /*Set the alarm on*/
        alarm(SCHED_TQ_SEC);

        /*Start the first process*/
	kill(head->mypid,SIGCONT); //wake up the shell
	
	shell_request_loop(request_fd, return_fd);
	
	//kill(head->mypid,SIGCONT);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
