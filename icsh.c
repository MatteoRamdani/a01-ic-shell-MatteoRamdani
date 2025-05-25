/* ICCS227: Project 1: icsh
 * Name: Matteo Ramdani
 * StudentID: 6480996
 */

#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CMD_BUFFER 255

int main(int argc, char *argv[]) {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER] = {0};

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
        }

        if (fgets(buffer, MAX_CMD_BUFFER, input) == NULL) {
            printf("\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline

        // Skip empty commands
        if (strlen(buffer) == 0) continue;

        // Handle !!
        if (strcmp(buffer, "!!") == 0) {
            if (strlen(last_command) == 0) {
                continue;
            } else {
                printf("%s\n", last_command);
                strcpy(buffer, last_command);
            }
        } else {
            strcpy(last_command, buffer);  // Save new command
        }

        // Parse command
        char *cmd = strtok(buffer, " ");

        if (cmd == NULL) continue;

        // Handle echo
        if (strcmp(cmd, "echo") == 0) {
            char *text = strtok(NULL, "");
            if (text) {
                printf("%s\n", text);
            } else {
                printf("\n");
            }
        }

        // Handle exit <num>
        else if (strcmp(cmd, "exit") == 0) {
            char *num_str = strtok(NULL, " ");
            int exit_code = 0;
            if (num_str) {
                exit_code = atoi(num_str) & 0xFF;
            }
            printf("bye\n");
            return exit_code;
        }

        // Unknown command
        else {
            printf("bad command\n");
        }
    }

    return 0;
}
