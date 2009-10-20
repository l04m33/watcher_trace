/*
 * =======================================================================
 *
 *       Filename:  watcher_trace.h
 *
 *    Description:  Main header file for watcher_trace
 *
 *        Version:  0.2
 *        Created:  10/01/2008 12:49:02 PM
 *       Compiler:  gcc
 *
 *         Author:  Kay Zheng (l_amee), l04m33@gmail.com
 *
 * =======================================================================
 */

#ifndef __WATCHER_TRACE_H__
#define __WATCHER_TRACE_H__


// Time between /proc checks, in u seconds.
#define CHECK_INTERVAL 1000000
// Signal used to communicate between child/parent.
#define COM_SIG        SIGUSR1        

// Max number of pids that can be stored.
#define PID_BUF_MAX    512
// stack alignment
#define STK_ALIGN      (sizeof(long))

// macro to retrieve proc_list
#define get_plist()    (&proc_list)

union pltval{
    unsigned long val;
    unsigned char chars[sizeof(unsigned long)];
};

struct __proc_list{ 
    struct {
        pid_t pid;
        long touched;
    } list[ PID_BUF_MAX ];
    int len;
};

struct __new_list {
    pid_t list[PID_BUF_MAX];
    int   len;
};


int   file_match(struct dirent *file_ent);
void  hk_chdir_param(pid_t traced, const struct user_regs_struct *regs);
void  sig_hdlr(int sig);
void  terminator(int sig);
void  shrink_plist(struct __proc_list *proc_list);
int   check_proc(struct __proc_list *proc_list);
void  untouch_plist(struct __proc_list *proc_list);
void  init_plist(struct __proc_list *proc_list);

#endif  // ifndef __WATCHER_TRACE_H__

