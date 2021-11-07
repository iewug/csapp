/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
//int builtin_cmd(char **argv); //为了思路流畅，我写在了eval中
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    int isBG = parseline(cmdline, argv); //build argv

    // 处理输入！
    /**
     * 输入为内建命令，直接运行
     * 虽然可能如同jobs wulala这种错误输入，但是这里按tshref实现
     */ 
    const char *command = argv[0];
    if (command == NULL) //输入为空
        return;
    else if(!strcmp(command,"quit")) //recall：相等时strcmp返回0
        exit(0);
    //如果输入第一个字符为&，跳过该命令（按tshref实现）
    else if(!strcmp(command,"&"))
        return;
    else if(!strcmp(command,"bg")||!strcmp(command,"fg"))
        do_bgfg(argv);
    else if(!strcmp(command,"jobs"))
        listjobs(jobs);
    /**
     * 输入不是内建命令，要fork后exec
     */ 
    else
    {
        sigset_t mask_all, mask_one, prev; //创建信号集，来作为掩码
        sigfillset(&mask_all); //初始化set为全集，两种初始化手段之一
        sigemptyset(&mask_one); //初始化set为空集合，两种初始化手段之一
        sigaddset(&mask_one, SIGCHLD); //SIGCHILD加入mask_one集合
        /**
         * block SIGCHILD! 
         * 我们在sigchld_handler中remove了job，在父进程add了job
         * 为了使先父进程add job后再有remove job
         * 需要在fork前block SIGCHILD，在父进程add job后再unblock SIGCHILD
         */ 
        //block SIGCHILD, mask_one is stored in prev (for later recovery)
        sigprocmask(SIG_BLOCK, &mask_one, &prev);
        pid_t pid = fork();
        /**
         * 这里就举个处理系统调用函数的返回值的例子吧
         * 按理说sigprocmask、exit、wait等系统调用函数的返回值都要处理
         * handout说不处理要扣5分。。。
         * 可以用csapp提供的包装函数，但是不想修改了（就是把系统调用函数的首字母大写
         */ 
        if (pid < 0) 
            unix_error("fork error");
        /**
         * 子进程！
         */ 
        else if (pid == 0)
        {
            //子进程继承了父进程的阻塞向量，需要解除阻塞
            //避免收不到它本身的子进程的信号
            sigprocmask(SIG_SETMASK, &prev, NULL);
            /**
             * shell进程fork出来的子进程是自己进程组的领导，否则无前后台之说
             * 默认子进程的pgid就是父进程的pid，所以这里需要修改
             * setpgid(pid,pgid): 把pid的pgid设置为函数调用者的pgid
             * 如果pid为0，则pid使用调用者的pid；
             * 如果pgid为0，则pid的pgid会设置为pid
             */ 
            setpgid(0,0);
            //正常运行execve函数会替换内存，不会返回到调用者进程
            //但是有错误的话，会返回-1
            if (execve(argv[0], argv, environ) < 0)
            {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }
        /**
         * 父进程！
         */ 
        else
        {
            //访问全局变量阻塞所有信号
            sigprocmask(SIG_BLOCK, &mask_all, NULL);
            if(isBG) 
                addjob(jobs, pid, BG, cmdline);
            else
                addjob(jobs, pid, FG, cmdline);
            
            //addjob完成后就可以unblock SIGCHLD了
            sigprocmask(SIG_SETMASK, &prev, NULL);
            if(isBG)
                printf("[%d] (%d) %s",pid2jid(pid), pid, cmdline);
            else
                waitfg(pid);
        }
    }
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * 4 builtin_cmd: quit, fg, bg, and jobs
 */
/*
int builtin_cmd(char **argv) 
{
    const char *command = argv[0];
    if(!strcmp(command,"quit")) //相等时strcmp返回0
        exit(0);
    if(!strcmp(command,"&"))
        return 1;
    if(!strcmp(command,"bg")||!strcmp(command,"fg"))
    {
        do_bgfg(argv);
        return 1;
    }
    if(!strcmp(command,"jobs"))
    {
        listjobs(jobs);
        return 1;
    }
    return 0;
}
*/

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 * bg/fg jobs 给jobs发送SIGCONT信号，并分别在后台和前台运行
 * jobs既可以是PID，也可以是JID，JID前面要加上%
 * 按参考tshref，我们也不检查bg 1 wulala这种错误的输入
 */
void do_bgfg(char **argv) 
{
    if (argv[1] == NULL)
    {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    struct job_t *job;
    int id;

    // 读到jid
    if (sscanf(argv[1], "%%%d", &id) > 0)
    {
        job = getjobjid(jobs, id);
        if (job == NULL)
        {
            printf("%%%d: No such job\n", id);
            return;
        }
    }
    // 读到pid
    else if (sscanf(argv[1], "%d", &id) > 0)
    {
        job = getjobpid(jobs, id);
        if (job == NULL)
        {
            printf("(%d): No such process\n", id);
            return;
        }
    }
    // 格式错误
    else
    {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }
    /**
     * kill应该发送到job中的所有进程
     * 所以传入参数是pgid
     * 而eval函数保证了job中的pid就是pgid
     */ 
    if (!strcmp(argv[0], "bg"))
    {
        //进程组是pgid的相反数
        kill(-(job->pid), SIGCONT);
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
    else
    {
        kill(-(job->pid), SIGCONT);
        job->state = FG;
        waitfg(job->pid);
    }

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
/*高效并正确(?)的版本*/
void waitfg(pid_t pid)
{
    sigset_t mask, prev;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prev);
    /**
     * int sigsuspend(const sigset_t *mask)相当于
     * sigprocmask(SIG_SETMASK, &mask, &prev)
     * pause()
     * sigprocmask(SIG_SETMASK, &prev, NULL)
     * 的原子版本
     */ 
    while (pid == fgpid(jobs))
        sigsuspend(&prev);
    sigprocmask(SIG_SETMASK, &prev, NULL);
}
/*忙等是可行的，但是效率差
void waitfg(pid_t pid)
{
	while (pid == fgpid(jobs))
		sleep(1);
    return;
} 
*/
/*我不认为以下是可行的。
void waitfg(pid_t pid)
{
    sigset_t m;
    sigemptyset(&m);
    while (pid == fgpid(jobs))
    当此时传入sigchild信号，并接下来无任何信号传入，会永远pause
        sigsuspend(&m);//有信号时被唤醒检查前台进程pid是否变化，变化则说明前台进程结束。
    return;
}
*/


/*****************
 * Signal handlers
 *****************/
/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid;
    int status;
    sigset_t mask_all, prev;
    sigfillset(&mask_all);
    /**
     * pid_t waitpid(pid_t __pid, int *__stat_loc, int __options)的options：
     * WNOHANG：若等待集合中没有子进程被终止，父进程不会被挂起，而是waitpid函数返回0
     * WUNTRACED：除了返回终止子进程的信息外，还返回因信号而停止的子进程信息
     * WNOHANG|WUNTRACED：如果等待集合无子进程终止或者停止，立刻返回0，否则返回该终止或停止的子进程的pid
    */
    while ((pid = waitpid(-1,&status,WNOHANG|WUNTRACED)) > 0)
    {
        sigprocmask(SIG_BLOCK, &mask_all, &prev);
        if(WIFEXITED(status))//return true if the child terminated normally, via a call to exit or a return
        {
            deletejob(jobs,pid);
        }
        if(WIFSTOPPED(status))//return true if the child that caused the return is currently stopped
        {
            struct job_t *job = getjobpid(jobs, pid); //Find a job (by PID) on the job list
            int jid = pid2jid(pid); //Map process ID to job ID
            printf("Job [%d] (%d) stopped by signal %d\n",jid,pid,WSTOPSIG(status));
            job->state = ST;
        }
        if(WIFSIGNALED(status))//return true if the child process terminated because of a signal that was not caught
        {
            int jid = pid2jid(pid);
            printf("Job [%d] (%d) terminated by signal %d\n",jid,pid,WTERMSIG(status));
            deletejob(jobs,pid);
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    /*
    if (errno != ECHILD)
        Sio_error("waitpid error");
    */
    errno = olderrno;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask_all, prev;
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev);
    pid_t fpid = fgpid(jobs);
    if(fpid)
        kill(-fpid, sig);
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = olderrno;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    sigset_t mask_all, prev;
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev);
    pid_t fpid = fgpid(jobs);
    if(fpid)
        kill(-fpid, sig);
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = olderrno;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



