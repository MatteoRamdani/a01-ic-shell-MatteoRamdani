/* ICCS227: Project 1: icsh
 * Name: Matteo Ramdani
 * StudentID: 6480996
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_BUFFER 255
#define MAX_ARGS 64

int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER] = "";
    FILE *input = stdin;

    // Check for script file
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
        if (input == stdin) {
            printf("icsh $ ");
            fflush(stdout);  // Ensure prompt is displayed
        }

        if (fgets(buffer, MAX_CMD_BUFFER, input) == NULL) {
            if (input == stdin) printf("\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline

        // Skip empty commands
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

        // Parse command and arguments
        char *args[MAX_ARGS];
        int arg_count = 0;
        char *token = strtok(buffer, " ");
        
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        // Handle built-in commands
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

        // Execute external command
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }
    //comment for testing github
    if (input != stdin) fclose(input);
    return 0;
}