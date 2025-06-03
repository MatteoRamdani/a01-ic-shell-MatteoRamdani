/* ICCS227: Project 1: icsh
 * Name: Matteo Ramdani
 * StudentID: 6480996
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>

#define MAX_CMD_BUFFER 255
#define MAX_ARGS 64
#define MAX_JOBS 100

// Job status constants
#define JOB_RUNNING 0
#define JOB_STOPPED 1
#define JOB_DONE 2

struct job {
    int job_id;
    pid_t pid;
    char cmd[MAX_CMD_BUFFER];
    int status;
};
struct job jobs[MAX_JOBS];
int next_job_id = 1;
pid_t foreground_pid = 0;
int last_job_id = 0;  // For + marker
int prev_job_id = 0;  // For - marker

// Signal handlers
void handle_sigint(int sig) {
    if (foreground_pid > 0) kill(foreground_pid, SIGINT);
}

void handle_sigtstp(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGTSTP);
        // Update job status if backgrounded
        for (int i = 1; i < next_job_id; i++) {
            if (jobs[i].pid == foreground_pid) {
                jobs[i].status = JOB_STOPPED;
                break;
            }
        }
    }
}

void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    char notification[256];
    
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {
        for (int i = 1; i < next_job_id; i++) {
            if (jobs[i].pid == pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Format notification message
                    char marker = (i == last_job_id) ? '+' : 
                                 (i == prev_job_id) ? '-' : ' ';
                    int len = snprintf(notification, sizeof(notification),
                        "[%d]%c Done\t%s\n", i, marker, jobs[i].cmd);
                    
                    // Async-safe output
                    write(STDOUT_FILENO, notification, len);
                    
                    jobs[i].status = JOB_DONE;
                    jobs[i].pid = 0;
                    

                    // Redisplay prompt
                    write(STDOUT_FILENO, "icsh $ ", 7);
                    fflush(stdout);
                    // Update markers
                    if (i == last_job_id) last_job_id = prev_job_id;
                } else if (WIFSTOPPED(status)) {
                    jobs[i].status = JOB_STOPPED;
                    write(STDOUT_FILENO, "\n", 1);  // Newline after ^Z
                }
                break;
            }
        }
    }
}

void jobs_command() {
    for (int i = 1; i < next_job_id; i++) {
        if (jobs[i].pid != 0 && jobs[i].status != JOB_DONE) {
            char marker = (i == last_job_id) ? '+' : 
                         (i == prev_job_id) ? '-' : ' ';
            printf("[%d]%c %s\t%s\n", 
                  i, 
                  marker,
                  jobs[i].status == JOB_RUNNING ? "Running" : "Stopped",
                  jobs[i].cmd);
        }
    }
}

void fg_command(int job_id) {
    if (job_id <= 0 || job_id >= next_job_id || jobs[job_id].pid == 0) {
        fprintf(stderr, "icsh: fg: no such job\n");
        return;
    }
    
    pid_t pid = jobs[job_id].pid;
    foreground_pid = pid;
    
    // Bring process group to foreground
    tcsetpgrp(STDIN_FILENO, getpgid(pid));
    
    // Continue if stopped
    if (jobs[job_id].status == JOB_STOPPED) {
        kill(pid, SIGCONT);
        jobs[job_id].status = JOB_RUNNING;
    }
    
    waitpid(pid, NULL, WUNTRACED);
    
    // Return terminal control to shell
    tcsetpgrp(STDIN_FILENO, getpgrp());
    foreground_pid = 0;
}

void bg_command(int job_id) {
    if (job_id <= 0 || job_id >= next_job_id || jobs[job_id].pid == 0) {
        fprintf(stderr, "icsh: bg: no such job\n");
        return;
    }
    
    if (jobs[job_id].status == JOB_STOPPED) {
        kill(jobs[job_id].pid, SIGCONT);
        jobs[job_id].status = JOB_RUNNING;
        printf("[%d] %d\n", job_id, jobs[job_id].pid);
    }
}

int main(int argc, char *argv[]) {
    // Signal setup
    struct sigaction sa_int, sa_tstp, sa_chld;
    
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
    
    sa_tstp.sa_handler = handle_sigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
    
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);

    // Main shell loop
    char buffer[MAX_CMD_BUFFER];
    while (1) {
        printf("icsh $ ");
        fflush(stdout);
        
        if (!fgets(buffer, MAX_CMD_BUFFER, stdin)) break;
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strlen(buffer) == 0) continue;

        // Parse command
        char *args[MAX_ARGS];
        int arg_count = 0;
        char *token = strtok(buffer, " ");
        while (token && arg_count < MAX_ARGS-1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        // Handle built-in commands
        if (arg_count == 0) continue;
        
        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        }
        else if (strcmp(args[0], "jobs") == 0) {
            jobs_command();
            continue;
        }
        else if (strcmp(args[0], "fg") == 0) {
            if (arg_count < 2) {
                fprintf(stderr, "Usage: fg %%jobid\n");
                continue;
            }
            int job_id = atoi(args[1]+1); // Skip '%'
            fg_command(job_id);
            continue;
        }
        else if (strcmp(args[0], "bg") == 0) {
            if (arg_count < 2) {
                fprintf(stderr, "Usage: bg %%jobid\n");
                continue;
            }
            int job_id = atoi(args[1]+1);
            bg_command(job_id);
            continue;
        }

        // Handle background/foreground
        int is_background = 0;
        if (arg_count > 0 && strcmp(args[arg_count-1], "&") == 0) {
            is_background = 1;
            args[arg_count-1] = NULL;
        }

        // Fork and execute
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            setpgid(0, 0);
            execvp(args[0], args);
            fprintf(stderr, "icsh: %s: not found\n", args[0]);
            exit(127);
        } else if (pid > 0) {
            if (is_background) {
                // Add to job list
                if (next_job_id < MAX_JOBS) {
                    jobs[next_job_id].job_id = next_job_id;
                    jobs[next_job_id].pid = pid;
                    strcpy(jobs[next_job_id].cmd, buffer);
                    jobs[next_job_id].status = JOB_RUNNING;
                    
                    // Update job markers
                    prev_job_id = last_job_id;
                    last_job_id = next_job_id;
                    
                    printf("[%d] %d\n", next_job_id, pid);
                    next_job_id++;
                }
            } else {
                // Foreground process
                foreground_pid = pid;
                tcsetpgrp(STDIN_FILENO, getpgid(pid));
                waitpid(pid, NULL, WUNTRACED);
                tcsetpgrp(STDIN_FILENO, getpgrp());
                foreground_pid = 0;
            }
        }
    }
    return 0;
}