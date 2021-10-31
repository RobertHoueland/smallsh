# smallsh
Small shell assignment built in C

# Compiling and Running The Shell
* First run `make` in a command line
* Then run `./smallsh` to open the shell
* You are now running the small shell.

# Project Requirements

## Overview
This is a custom wrote shell I wrote in C. The shell runs command line instructions and returns results similar to other shells.

The shell allows you to redirect standard input and standard output and it supports foreground and background processes (controllable by the command line and by receiving signals).

There are three built in commands: `exit`, `cd`, and `status`. It also supports comments, which are lines beginning with the # character.

## Specifications
### The Prompt

The colon : symbol is a prompt for each command line. 

The general syntax of a command line is:
`command [arg1 arg2 ...] [< input_file] [> output_file] [&]`
... where items in square brackets are optional.  A command is made up of words separated by spaces. The special symbols <, >, and & are recognized, but they must be surrounded by spaces like other words. If the command is to be executed in the background, the last word must be &. If standard input or output is to be redirected, the > or < words followed by a filename word must appear after all the arguments. Input redirection can appear before or after output redirection.

The  shell does not need to support any quoting; so arguments with spaces inside them are not possible.

The shell supports command lines with a maximum length of 2048 characters, and a maximum of 512 arguments. There is no error checking done on the syntax of the command line.

Finally, the shell allows blank lines and comments.  Any line that begins with the # character is a comment line and is ignored (mid-line comments, such as the C-style //, are not supported).  A blank line (one without any commands) will also do nothing. The shell will just re-prompt for another command when it receives either a blank line or a comment line.

### Command Execution

The shell uses fork(), exec(), and waitpid() to execute commands. 

Both stdin and stdout for a command can be redirected at the same time (see example below).

The program expands any instance of "$$" in a command into the process ID of the shell itself. The shell does not otherwise perform variable expansion.

### Background and Foreground

The shell will wait() for completion of foreground commands (commands without the &) before prompting for the next command. If the command given was a foreground command, then the parent shell does NOT return command line access and control to the user until the child terminates.

The shell will not wait for background commands to complete. If the command given was a background process, then the parent returns command line access and control to the user immediately after forking off the child.

Background commands have their standard input redirected from /dev/null if the user did not specify some other file to take standard input from.

The shell will print the process id of a background process when it begins. When a background process terminates, a message showing the process id and exit status will be printed.

### Signals

A CTRL-C command from the keyboard will send a SIGINT signal to the parent process and all children at the same time. SIGINT does not terminate the shell, but only terminates the foreground command if one is running.

Background processes will also not be terminated by a SIGINT signal. They will terminate themselves, continue running, or be terminated when the shell exits (see below).

A CTRL-Z command from the keyboard will send a SIGTSTP signal to the shell. When this signal is received, the shell will display an informative message (see below) and then enter a state where new commands can no longer be run in the background. In this state, the & operator will be ignored - run all such commands as if they were foreground processes. If the user sends SIGTSTP again, the shell will return back to the normal condition where the & operator is once again honored, allowing commands to be placed in the background. 

See the example below for usage and the exact syntax which you must use for these two informative messages.

### Built-in Commands

The shell has support for three built in commands: `exit`, `cd`, and `status`. There is no support for input/output redirection for these built in commands and they do not set any exit status. These three built-in commands are the only ones that the shell will handle itself.

The `exit` command exits the shell. It takes no arguments. When this command is run, the shell kills any other processes or jobs that the shell has started before it terminates itself.

The `cd` command changes directories. By itself, it changes to the directory specified in the HOME environment variable. It can also take one argument, the path of the directory to change to.

The `status` command prints out either the exit status or the terminating signal of the last foreground process (not both, processes killed by signals do not have exit statuses).

### Example

Here is an example:

```
$ smallsh
: ls
junk   smallsh    smallsh.c
: ls > junk
: status
exit value 0
: cat junk
junk
smallsh
smallsh.c
: wc < junk > junk2
: wc < junk
       3       3      23
: test -f badfile
: status
exit value 1
: wc < badfile
cannot open badfile for input
: status
exit value 1
: badfile
badfile: no such file or directory
: sleep 5
^Cterminated by signal 2
: status &
terminated by signal 2
: sleep 15 &
background pid is 4923
: ps
PID TTY          TIME CMD
4923 pts/0    00:00:00 sleep
4564 pts/0    00:00:03 bash
4867 pts/0    00:01:32 smallsh
4927 pts/0    00:00:00 ps
:
: # that was a blank command line, this is a comment line
:
background pid 4923 is done: exit value 0
: # the background sleep finally finished
: sleep 30 &
background pid is 4941
: kill -15 4941
background pid 4941 is done: terminated by signal 15
: pwd
/nfs/stak/faculty/b/brewsteb/CS344/prog3
: cd
: pwd
/nfs/stak/faculty/b/brewsteb
: cd CS344
: pwd
/nfs/stak/faculty/b/brewsteb/CS344
: echo 4867
4867
: echo $$
4867
: ^C
: ^Z
Entering foreground-only mode (& is now ignored)
: date
Mon Jan  2 11:24:33 PST 2017
: sleep 5 &
: date
Mon Jan  2 11:24:38 PST 2017
: ^Z
Exiting foreground-only mode
: date
Mon Jan  2 11:24:39 PST 2017
: sleep 5 &
background pid is 4963
: date
Mon Jan 2 11:24:39 PST 2017
: exit $
```
