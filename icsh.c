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

#define MAX_CMD_BUFFER 255
#define MAX_ARGS 64
#define MAX_JOBS 100

// Enhanced job tracking structure
struct job {
    int job_id;
    pid_t pid;
    char cmd[MAX_CMD_BUFFER];
    int status; // 0=running, 1=stopped, 2=done
};
struct job jobs[MAX_JOBS];
int next_job_id = 1;

pid_t foreground_pid = 0;

// Signal handlers
void handle_sigint(int sig) {
    if (foreground_pid > 0) kill(foreground_pid, SIGINT);
}
void handle_sigtstp(int sig) {
    if (foreground_pid > 0) kill(foreground_pid, SIGTSTP);
}

int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER] = "";
    FILE *input = stdin;

    // Signal setup
    struct sigaction sa_int, sa_tstp;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
    
    sa_tstp.sa_handler = handle_sigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) { perror("fopen"); return 1; }
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [scriptfile]\n", argv[0]);
        return 1;
    }

    printf("Welcome to icsh!\n");

    while (1) {
        foreground_pid = 0;
        
        if (input == stdin) {
            printf("icsh $ ");
            fflush(stdout);
        }

        if (fgets(buffer, MAX_CMD_BUFFER, input) == NULL) {
            if (input == stdin) printf("\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        if (strlen(buffer) == 0) continue;

        // Handle !!
        if (strcmp(buffer, "!!") == 0) {
            if (strlen(last_command) == 0) {
                fprintf(stderr, "No previous command\n");
                continue;
            }
            printf("%s\n", last_command);
            strcpy(buffer, last_command);
        } else {
            strcpy(last_command, buffer);
        }

        // Parse command
        char *args[MAX_ARGS];
        int arg_count = 0;
        char *token = strtok(buffer, " ");
        while (token && arg_count < MAX_ARGS-1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        // Handle empty command case (fix segfault)
        if (arg_count == 0) {
            continue;
        }

        // Check for background &
        int is_background = 0;
        if (arg_count > 0 && args[arg_count-1] && strcmp(args[arg_count-1], "&") == 0) {
            if (arg_count == 1) {  // Only "&" was entered
                fprintf(stderr, "icsh: syntax error near unexpected token '&'\n");
                continue;  // Skip to next command
            }
            is_background = 1;
            args[arg_count-1] = NULL;
            arg_count--;
        }

        // Built-in commands
        if (strcmp(args[0], "exit") == 0) {
            int exit_code = 0;
            if (arg_count > 1) exit_code = atoi(args[1]) & 0xFF;
            printf("bye\n");
            return exit_code;
        }
        else if (strcmp(args[0], "echo") == 0) {
            for (int i = 1; i < arg_count; i++) printf("%s%s", args[i], (i < arg_count-1) ? " " : "");
            printf("\n");
            continue;
        }
        else if (strcmp(args[0], "jobs") == 0) {
            for (int i = 1; i < next_job_id; i++) {
                if (jobs[i].pid != 0 && jobs[i].status != 2) {
                    printf("[%d] %s\t%s\n", 
                        jobs[i].job_id,
                        jobs[i].status == 0 ? "Running" : "Stopped",
                        jobs[i].cmd);
                }
            }
            continue;
        }

        // Handle I/O redirection
        char *input_file = NULL, *output_file = NULL;
        for (int i = 0; i < arg_count; i++) {
            if (args[i] && strcmp(args[i], "<") == 0 && i+1 < arg_count) {
                input_file = args[i+1];
                args[i] = NULL;
            }
            if (args[i] && strcmp(args[i], ">") == 0 && i+1 < arg_count) {
                output_file = args[i+1];
                args[i] = NULL;
            }
        }

        // Launch process
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            setpgid(0, 0);
            if (input_file) {
                int fd = open(input_file, O_RDONLY);
                if (fd < 0) { perror("open input"); exit(1); }
                dup2(fd, STDIN_FILENO); close(fd);
            }
            if (output_file) {
                int fd = open(output_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                if (fd < 0) { perror("open output"); exit(1); }
                dup2(fd, STDOUT_FILENO); close(fd);
            }
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
                    jobs[next_job_id].status = 0;
                    printf("[%d] %d\n", next_job_id, pid);
                    next_job_id++;
                } else {
                    printf("Maximum jobs reached\n");
                }
            } else {
                foreground_pid = pid;
                waitpid(pid, NULL, 0);
                foreground_pid = 0;
            }
        } else {
            perror("fork");
        }
    }

    if (input != stdin) fclose(input);
    return 0;
}