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

#define MAX_CMD_BUFFER 255
#define MAX_ARGS 64

// Global variable to track foreground process
pid_t foreground_pid = 0;

// Signal handler for SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
    }
}

// Signal handler for SIGTSTP (Ctrl+Z)
void handle_sigtstp(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGTSTP);
    }
}

int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER] = "";
    FILE *input = stdin;

    // Set up signal handlers
    struct sigaction sa_int, sa_tstp;
    
    // SIGINT handler (Ctrl+C)
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
    
    // SIGTSTP handler (Ctrl+Z)
    sa_tstp.sa_handler = handle_sigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    // Rest of your initialization code...
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            perror("fopen");
            return 1;
        }
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [scriptfile]\n", argv[0]);
        return 1;
    }

    printf("Welcome to icsh!\n");

    while (1) {
        foreground_pid = 0; // Reset before each command
        
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
                fprintf(stderr, "No previous command to execute\n");
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
        
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        // Built-in commands
        if (strcmp(args[0], "exit") == 0) {
            int exit_code = 0;
            if (arg_count > 1) exit_code = atoi(args[1]) & 0xFF;
            printf("bye\n");
            return exit_code;
        }
        else if (strcmp(args[0], "echo") == 0) {
            for (int i = 1; i < arg_count; i++) {
                printf("%s%s", args[i], (i < arg_count - 1) ? " " : "");
            }
            printf("\n");
            continue;
        }

        // External command
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            setpgid(0, 0); // Important for signal handling
            execvp(args[0], args);
            fprintf(stderr, "icsh: %s: command not found\n", args[0]);
            exit(127);
        } else if (pid > 0) {
            // Parent process
            foreground_pid = pid;
            waitpid(pid, NULL, 0);
            foreground_pid = 0;
        } else {
            perror("fork");
        }
    }

    if (input != stdin) fclose(input);
    return 0;
}