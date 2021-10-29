#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Global variables for background mode
int background = 0;
int foregroundMode = 0;
pid_t newpid;

// If input string contains "$$" expand it into PID
void dollarSignPID(char *input, int pid) {
    // New string to copy over
    char *newInput = malloc(sizeof(char) * strlen(input));
    // Separate iterator for new string
    int j = 0;
    // Iterate through each character of input string
    for (int i = 0; i < strlen(input); i++) {
        // Check if current character and next are "$"
        if (input[i] == '$' && input[i + 1] == '$') {
            // Replace current character with PID converted to string
            char mypid[32768];
            sprintf(mypid, "%d", pid);
            strcat(newInput, mypid);

            // Skip past other $
            i++;
            j += strlen(mypid);
        } else {
            // Otherwise, copy over current character of string
            newInput[j] = input[i];
            j++;
        }
    }
    // Move new string back to input string
    strcpy(input, newInput);
}

// Process input from user, return what to do next
int processInput(char *input, int pid) {
    // Check for blank line or comment
    if (!strcmp(input, "") || (strncmp(input, "#", 1) == 0)) {
        return 1;
    }

    // Check if input contains instance of "$$"
    if (strstr(input, "$$")) {
        dollarSignPID(input, pid);
    }

    // Check for exit command
    if (!strcmp(input, "exit") || !strcmp(input, "exit &")) {
        return 2;
    }

    // Check for cd command with no arguments
    if (!strcmp(input, "cd") || !strcmp(input, "cd &")) {
        return 3;
    }

    // Check for cd command with arguments
    if (strncmp(input, "cd", 2) == 0) {
        return 4;
    }

    // Check for pwd command
    if (!strcmp(input, "pwd") || !strcmp(input, "pwd &")) {
        return 5;
    }

    // Check for status command
    if (!strcmp(input, "status") || !strcmp(input, "status &")) {
        return 6;
    }

    // Another command was used
    return 0;
}

// Print exit status of last command
void showExitStatus(int status) {
    if (WIFEXITED(status)) {
        // Exited by status
        printf("exit value %d\n", WEXITSTATUS(status));
        fflush(stdout);
    } else {
        // Terminated by signal
        printf("terminated by signal %d\n", WTERMSIG(status));
        fflush(stdout);
    }
}

// Show why command was terminated
void sigintHandler(int sig_num) {
    signal(SIGINT, sigintHandler);
    printf("terminated by signal %d\n", sig_num);
    fflush(stdout);
}

// Control-Z changes to foreground only mode
void enterForeground(int sig) {
    // If not in foreground mode, enter it
    if (foregroundMode == 0) {
        printf("Entering foreground-only mode (& is now ignored)\n");
        fflush(stdout);
        foregroundMode = 1;
    }

    // If already in it, exit it
    else {
        printf("Exiting foreground-only mode\n");
        fflush(stdout);
        foregroundMode = 0;
    }
    printf(": ");
    fflush(stdout);
}

// Execute a command with arguments by forking a new process
void execCommand(char *command, int pid, char inputFile[], char outputFile[],
                 struct sigaction sigAct, int *exitStatus) {
    newpid = fork();
    int input, output, result;

    // String of command and arguments
    char *arguments[512];
    // Reset argument string
    memset(arguments, 0, 512);

    // Split input into arguments
    // Format: command [arg1 arg2 ...] [< input_file] [> output_file] [&]
    char *token = strtok(command, " ");
    // Save each argument in array
    for (int i = 0; token; i++) {
        // Check to run in background
        if (!strcmp(token, "&")) {
            background = 1;
        }
        // Check for < for input file
        else if (!strcmp(token, "<")) {
            token = strtok(NULL, " ");
            strcpy(inputFile, token);
        }
        // Check for > for output file
        else if (!strcmp(token, ">")) {
            token = strtok(NULL, " ");
            strcpy(outputFile, token);
        }
        // Otherwise part of command, save in argument array
        else {
            arguments[i] = strdup(token);
        }
        // Next word
        token = strtok(NULL, " ");
    }

    // Execute command
    if (newpid == -1) {
        // Check if fork failed
        perror("fork() failed!");
        exit(1);
    } else if (newpid == 0) {
        // Child process will use Control-C normally to terminate
        sigAct.sa_handler = SIG_DFL;
        sigaction(SIGINT, &sigAct, NULL);

        // Check if input file is specified
        if (strcmp(inputFile, "")) {
            // Open file for read only
            input = open(inputFile, O_RDONLY);
            if (input == -1) {
                printf("cannot open %s for input\n", inputFile);
                exit(1);
            }

            // Redirect and check for error
            result = dup2(input, 0);
            if (result == -1) {
                perror("dup2");
                exit(1);
            }

            // Close file
            fcntl(input, F_SETFD, FD_CLOEXEC);
        }

        else if (background == 1 && foregroundMode == 0) {
            // Input file not specified, but background process
            // Open file for read only at /dev/null/
            input = open("/dev/null", O_RDONLY);
            if (input == -1) {
                fprintf(stderr, "open error");
                exit(1);
            }

            // Redirect and check for error
            result = dup2(input, STDIN_FILENO);
            if (result == -1) {
                perror("dup2");
                exit(1);
            }

            // Close file
            fcntl(input, F_SETFD, FD_CLOEXEC);
        }

        // Check if output file is specified
        if (strcmp(outputFile, "")) {
            // Open file for write only
            output = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (output == -1) {
                printf("Unable to open %s for output\n", outputFile);
                exit(1);
            }

            // Redirect and check for error
            result = dup2(output, 1);
            if (result == -1) {
                perror("dup2");
                exit(1);
            }

            // Close file
            fcntl(output, F_SETFD, FD_CLOEXEC);
        }

        else if (background == 1 && foregroundMode == 0) {
            // Output file not specified, but background process
            // Open file for write only at /dev/null/
            output = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (output == -1) {
                fprintf(stderr, "open error");
                exit(1);
            }

            // Redirect and check for error
            result = dup2(output, STDOUT_FILENO);
            if (result == -1) {
                perror("dup2");
                exit(1);
            }

            // Close file
            fcntl(output, F_SETFD, FD_CLOEXEC);
        }

        // Execute child process, check for error
        if (execvp(arguments[0], arguments)) {
            perror(arguments[0]);
            exit(1);
        }

    } else {
        // Send terminated info if Control-C is used
        signal(SIGINT, sigintHandler);

        if (background == 1 && foregroundMode == 0) {
            // Execute in background and print child PID
            pid_t backgPID = waitpid(newpid, exitStatus, WNOHANG);
            printf("background pid is %d\n", newpid);
            fflush(stdout);
        } else {
            // Wait for child process to terminate in foreground
            newpid = waitpid(newpid, exitStatus, 0);
        }

        // Check for processes that have been terminated
        while ((newpid = waitpid(-1, exitStatus, WNOHANG)) > 0) {
            printf("background pid %d is done: ", newpid);
            fflush(stdout);

            showExitStatus(*exitStatus);
            newpid = waitpid(-1, exitStatus, WNOHANG);
        }
    }
}

int main() {
    // Input string, and strings for input/output files
    char input[2048];
    char inputF[256] = "";
    char outputF[256] = "";
    // Directory strings
    char pwd[2048];
    char *dir;
    int checkExit = 0, foregroundRun = 0, exitStatus = 0;
    int pid = getpid();

    // Ignore Control-C
    struct sigaction ignoreSigInt = {0};
    ignoreSigInt.sa_handler = SIG_IGN;
    sigfillset(&ignoreSigInt.sa_mask);
    ignoreSigInt.sa_flags = 0;
    sigaction(SIGINT, &ignoreSigInt, NULL);

    // Change Control-Z to foreground only mode
    struct sigaction sigStp = {0};
    sigStp.sa_handler = enterForeground;
    sigfillset(&sigStp.sa_mask);
    sigStp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sigStp, NULL);

    while (checkExit == 0) {
        // Check for processes that have been terminated each run
        while ((newpid = waitpid(-1, &exitStatus, WNOHANG)) > 0) {
            printf("background pid %d is done: ", newpid);
            fflush(stdout);

            showExitStatus(exitStatus);
            newpid = waitpid(-1, &exitStatus, WNOHANG);
        }

        // ^C does nothing for now
        ignoreSigInt.sa_handler = SIG_IGN;
        sigfillset(&ignoreSigInt.sa_mask);
        ignoreSigInt.sa_flags = 0;
        sigaction(SIGINT, &ignoreSigInt, NULL);

        // Run shell
        printf(": ");
        fflush(stdout);
        background = 0;
        fgets(input, 2048, stdin);
        // Remove new line character from input
        input[strcspn(input, "\n")] = 0;

        // Handle input each time user inputs
        switch (processInput(input, pid)) {
            case 1:
                // Input is empty or comment starting with #, ignore
                break;

            case 2:
                // Input is exit, exit loop
                checkExit = 1;
                // Kill background processes
                kill(0, SIGINT);

                break;

            case 3:
                // Input is cd with no arguments, change to HOME dir
                dir = getenv("HOME");
                chdir(dir);
                break;

            case 4:
                // Input is cd with arguments
                dir = input;

                // Ignore & character
                char dirLast = dir[(strlen(dir) - 1)];
                if (dirLast == '&') {
                    // Remove & character and extra space if it exists at end
                    dir[strlen(dir) - 2] = '\0';
                }

                // Remove first 3 char from command, "cd "
                dir += 3;

                // Change directory to specified from user
                chdir(dir);

                break;

            case 5:
                // Input is pwd
                printf("%s\n", getcwd(pwd, 2048));
                fflush(stdout);

                break;

            case 6:
                // Input is status
                if (foregroundRun == 0) {
                    // Before foreground external cmds run, print exit value 0
                    printf("exit value %d\n", exitStatus);
                    fflush(stdout);
                } else {
                    // Print exit value from last cmd
                    showExitStatus(exitStatus);
                }

                break;

            case 0:
                // Check if command is foreground
                if (input[strlen(input) - 1] != '&') {
                    foregroundRun = 1;
                }
                // Execute external cmd
                execCommand(input, pid, inputF, outputF, ignoreSigInt,
                            &exitStatus);

                break;

            default:
                break;
        }

        // Reset files each run
        inputF[0] = '\0';
        outputF[0] = '\0';
    }

    return 0;
}