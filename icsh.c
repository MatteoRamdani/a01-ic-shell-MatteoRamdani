/* ICCS227: Project 1: icsh
 * Name: Matteo Ramdani
 * StudentID: 6480996
 */

#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CMD_BUFFER 255

int main() {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER] = {0};

    printf("Welcome to icsh!\n");

    while (1) {
        printf("icsh $ ");
        if (fgets(buffer, MAX_CMD_BUFFER, stdin) == NULL) {
            printf("\n");
            break;
        }
        buffer[strcspn(buffer, "\n")] = '\0';

        // Skip if command is empty
        if (strlen(buffer) == 0) continue;

        // Handle !!
        if (strcmp(buffer, "!!") == 0) {
            if (strlen(last_command) == 0) {
                continue;  // No previous command
            } else {
                printf("%s\n", last_command);
                strcpy(buffer, last_command);  // Replace buffer with last command
            }
        } else {
            // Store last command (but not "!!")
            strcpy(last_command, buffer);
        }

        // Parse command
        char *cmd = strtok(buffer, " ");

        // Handle echo
        if (strcmp(cmd, "echo") == 0) {
            char *text = strtok(NULL, "");  // Get the rest of the line
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
                exit_code = atoi(num_str) & 0xFF; // truncate to 8 bits
            }
            printf("bye\n");
            exit(exit_code);
        }

        // Unknown command
        else {
            printf("bad command\n");
        }
    }
    return 0;
}
