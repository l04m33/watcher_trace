/*
 * =======================================================================
 *
 *       Filename:  watcher_trace.c
 *
 *    Description:  A hooker with a watcher :)
 *                  The watcher(child) watches /proc for new processes, 
 *                  and notifies the hooker(parent).
 *                  The hooker receives SIGUSR1 by default, and hook 
 *                  every bash that pops up.
 *                  Victims that gets hooked are unable to use chdir(), 
 *                  and the "cd" command.
 *                  (works only on Linux systems with the ptrace syscall)
 *
 *        Version:  0.2
 *        Created:  09/22/2008 01:03:53 PM
 *       Compiler:  gcc
 *
 *         Author:  Kay Zheng (l_amee), l04m33@gmail.com
 *
 * =======================================================================
 */

/*
 * TODO: make it use a linked list or hash table(better) instead 
 * of the silly static array
 */


#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

#include "linklist.h"
#include "watcher_trace.h"

static int fds[2];
static unsigned long backup;
static struct __proc_list proc_list;
static struct __new_list  new_list;


//////////////////////////////////////////////////////////////////////
//
// Called by check_proc, to match a /proc directory
//
int file_match(struct dirent *file_ent)
{
    char buf[256];
    FILE *cmd_file;
    if(file_ent->d_type != DT_DIR) return 0;
    if(!isdigit(file_ent->d_name[0])) return 0;
    
    sprintf(buf, "/proc/%s/cmdline", file_ent->d_name);
    if((cmd_file = fopen(buf, "r")) == NULL) return 0;

    fgets(buf, 256, cmd_file);
    // We match the full name, in case some other things have "bash" 
    // in their names
    if(strcmp(buf, "bash")){               
        fclose(cmd_file);
        return 0;
    }
    fclose(cmd_file);
    return 1;
}


////////////////////////////////////////////////////////////////////
// 
// Modifies the parameters passed to chdir()
//
void hk_chdir_param(pid_t traced, const struct user_regs_struct *regs)
{
    union pltval buf;
    unsigned long str_addr;
#ifdef X86_64
    // x86-64 calling convention, parameters are passed in registers
    str_addr = regs->rdi;
#else
    // x86 calling convention, parameters are passed in the stack
    str_addr = ptrace(PTRACE_PEEKDATA, traced, regs->esp+4, NULL);
#endif
    errno = 0;
    buf.val = ptrace(PTRACE_PEEKDATA, traced, str_addr, NULL);
    perror("hk_chdir_param");
    
    // use a string of length 3 at most, to be safe on x86
    memcpy(buf.chars, "WTF", 3);
    buf.chars[3] = 0;
    //buf.chars[0] = 0;
    
    errno = 0;
    ptrace(PTRACE_POKEDATA, traced, str_addr, buf.val);
    perror("hk_chdir_param");
}


////////////////////////////////////////////////////////////////////
//
// Signal handler for the parent, we should attach to new victims
// found by check_proc() (which is called by the child)
//
void sig_hdlr(int sig)
{
    int i, stat;
    union pltval inst_buf;
    typeof(new_list.len) len;
    typeof(new_list.list[0]) pids[PID_BUF_MAX];

    read(fds[0], &len, sizeof(len));

    printf("new process: %d:  \n", len);

    for(i = 0; i < len; i++){
        read(fds[0], &pids[i], sizeof(pids[0]));

        errno = 0;
        ptrace(PTRACE_ATTACH, pids[i], NULL, NULL);
        if(!errno)
            // plist_act: Add the new victim to our list
            get_plist()->list[get_plist()->len++].pid = pids[i];  
        perror("PTRACE_ATTACH");

        waitpid(pids[i], &stat, 0);
        inst_buf.val = ptrace(PTRACE_PEEKDATA, pids[i], PLT_ADDR, NULL);

        if(inst_buf.chars[0] != 0xcc)
            // Since the op code of all instances are the same, 
            // we save only one copy here.
            // And we assume that no other apps are modifying our 
            // victim's code.
            backup = inst_buf.val;

        inst_buf.chars[0] = 0xcc;

        errno = 0;
        ptrace(PTRACE_POKEDATA, pids[i], PLT_ADDR, inst_buf.val);
        perror("PTRACE_POKEDATA");

        errno = 0;
        ptrace(PTRACE_CONT, pids[i], NULL, NULL);
        perror("PTRACE_CONT");

        printf("pid %d\n", pids[i]);
    }

    printf("-------------------------------------------\n");
}



///////////////////////////////////////////////////////////////////////
//
// This is called when the parent receives a TERM or INT signal, to 
// clean up the mess.
//
void terminator(int sig)
{
    int i, stat;
    pid_t ret, cur_pid;

    fprintf(stderr, "\nterminator called\n");

    for(i = 0; i < get_plist()->len; i++){
        // plist_act: retrieve pid from proc_list
        cur_pid = get_plist()->list[i].pid;

        // signal the victim to give us chance to detach
        kill(cur_pid, SIGTRAP);
        fprintf(stderr, "waiting for %d\n", cur_pid);
        ret = waitpid(cur_pid, &stat, 0); 
        if(ret < 0){
            perror("wait");
            continue;
        }
        if(WIFEXITED(stat)) continue;

        errno = 0;
        ptrace(PTRACE_POKEDATA, cur_pid, PLT_ADDR, backup);
        perror("PTRACE_POKEDATA");

        errno = 0;
        ptrace(PTRACE_DETACH, cur_pid, NULL, NULL);
        perror("PTRACE_DETACH");
    }
    fprintf(stderr, "done terminating\n");
    exit(0);
}


////////////////////////////////////////////////////////////////////
//
// Internal thing, used by check_proc to remove dead processes
// 
void shrink_plist(struct __proc_list *proc_list)
{
    int i = 0, j, anchor;
    while(i < proc_list->len){
        while(proc_list->list[i].touched) i++;
        if(i == proc_list->len) break;
        
        anchor = j = i;
        while((!proc_list->list[j].touched) && (j<proc_list->len)) j++;
        for( ; j < proc_list->len; i++, j++){
            proc_list->list[i] = proc_list->list[j];
        }
        proc_list->len -= (j-i);
        i = anchor;
    }
}


void untouch_plist(struct __proc_list *proc_list)
{
    int i;
    for(i = 0; i < proc_list->len; i++)
        proc_list->list[i].touched = 0;
}

void init_plist(struct __proc_list *proc_list)
{
    proc_list->list[0].pid = 0;
    proc_list->list[0].touched = 0;
    proc_list->len = 1;
}



/////////////////////////////////////////////////////////////////////
//
// Called by the child, to check the /proc fs. It modifies proc_list,
// and returns the number of new process found.
//
int check_proc(struct __proc_list *proc_list)
{   
    pid_t tmpid;
    int  i;

    errno = 0;
    DIR *proc = opendir("/proc");
    if(!proc){
        perror("opendir");
        return -1;
    }

    struct dirent *proc_ent = NULL;
    int count = 0;
    int plist_ptr = 0;

    // plist_act: "untouch" the elements in proc_list
    untouch_plist(proc_list);           

    while((proc_ent = readdir(proc)) != NULL){

        if(!file_match(proc_ent)) continue;

        tmpid = (pid_t)atoi(proc_ent->d_name);

        // XXX: We assume that the /proc entries we get here are sorted.
        //      And maybe a linked list or even hash table here will be
        //      more suitable.

        // plist_act: add new process to proc_list, sorted
        while((proc_list->list[plist_ptr].pid < tmpid) && 
              (plist_ptr < proc_list->len)){    
            plist_ptr++;
        }
        if(plist_ptr == proc_list->len){
            proc_list->list[plist_ptr].pid = tmpid;
            proc_list->list[plist_ptr].touched = 1;
            new_list.list[count++] = tmpid;
            proc_list->len++;
        }else{
            if(proc_list->list[plist_ptr].pid == tmpid)
                proc_list->list[plist_ptr].touched = 1;
            else
                ; 
            // FIXME: This rarely happens (wrapping around of pid value),
            //        ignore for now.
        }
        plist_ptr++;

    } // while((proc_ent = readdir(proc)) != NULL)

    // plist_act: keep the "touched" ones, remove the "untouched"
    shrink_plist(proc_list); 

    for(i = 0; i < proc_list->len; i++)
        printf("pid %d, touched %ld\n", proc_list->list[i].pid, 
               proc_list->list[i].touched);

    closedir(proc);
    new_list.len = count;
    return count;
}



int main()
{
    if(pipe(fds)){
        perror("pipe");
        exit(1);
    }

    init_plist(&proc_list);  // plist_act: initialization

    pid_t worker = fork();
    if(worker)
    {
        close(fds[1]);

        signal(COM_SIG, sig_hdlr);
        signal(SIGTERM, terminator);
        signal(SIGINT, terminator);

        proc_list.len = 0;  // plist_act: parent side initialization

        pid_t cpid;
        int stat;

        while(1){
            errno = 0;
            cpid = wait(&stat);
            if(cpid < 0){
                perror("wait");
                continue;
            }
            if(WIFEXITED(stat)){ 
                 // Removing the exiting process from the proc_list here? 
                 // May need a lock or sth.
                int i, j;
                for(i = 0; i < proc_list.len; i++){
                    if(proc_list.list[i].pid == cpid){
                        for(j = i; j < proc_list.len-1; j++)
                            proc_list.list[j] = proc_list.list[j+1];
                        proc_list.len--;
                        break;
                    }
                }
                continue;
            }

//////////////////////////////////////////////////////////////////////////
            siginfo_t si;
            struct user_regs_struct regs;
            union pltval buf;

            ptrace(PTRACE_GETSIGINFO, cpid, NULL, &si);
            ptrace(PTRACE_GETREGS, cpid, NULL, &regs);

#ifdef X86_64
  #define XIP (regs.rip)
#else
  #define XIP (regs.eip)
#endif
            if((si.si_signo != SIGTRAP) || (XIP != (long)PLT_ADDR+1)){
                ptrace(PTRACE_CONT, cpid, NULL, NULL);
                continue;
            }

            hk_chdir_param(cpid, &regs); // HOOK HOOK HOOK HOOK HOOOOO0000K

            buf.val = backup;
            ptrace(PTRACE_POKEDATA, cpid, PLT_ADDR, buf.val);

            XIP = XIP - 1;
#undef XIP
            ptrace(PTRACE_SETREGS, cpid, NULL, &regs);

            ptrace(PTRACE_SINGLESTEP, cpid, NULL, NULL);
            // We have to wait after each call of ptrace(), 
            // to make sure the process actually moved.
            waitpid(cpid, NULL, 0);  
        
            ptrace(PTRACE_GETREGS, cpid, NULL, &regs);
        
            buf.chars[0] = 0xcc;
            ptrace(PTRACE_POKEDATA, cpid, PLT_ADDR, buf.val);
            ptrace(PTRACE_CONT, cpid, NULL, NULL);
//////////////////////////////////////////////////////////////////////////

        }
    }else{
        //signal(SIGUSR1, SIG_IGN);
        close(fds[0]);
        while(1){
            usleep(CHECK_INTERVAL);
            check_proc(&proc_list);
            write(fds[1], &new_list.len, sizeof(new_list.len));
            int i;
            for(i = 0; i < new_list.len; i++)
                write(fds[1], &new_list.list[i], sizeof(new_list.list[i]));
            kill(getppid(), COM_SIG);
        }
    }

    return 0;
}

