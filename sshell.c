#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define ARG_MAX 16

int runCMD(char *argv[], int whatNo);
char **parseCMD(char *string);
void printStringArray(char **arr);
char *preProcessCMD(char *cmd);
void runPiped(char *cmd, char *originalCMD);
int redOut2(char *file);
int redOut(char *file);
int redIn(char *file);
int checkCLError(char **args);
void error_(int error);

enum errors // Error list for error handling
{
	tooManyArgs,
	missingCommand,
	noOutputFile,
	cannotOpen,
	mislocatedOutputRedir,
	cannotCD,
	commandNotFound,
	mislocatedBackground,
	activeJobs,
	none
};

struct job // class made for creating background processes
{
	pid_t processID[4];
	char command[512];
	int statuses[4];
	int ncp;
	int printed;
};

struct job jobTable[5000]; // job table to keep track of all jobs
int background = 0;		   // keeps track if there is a background statement
int numJob = 0;			   // keeps track of what number we are on in the jobTable

int checkCLError(char **args)
{
	if (args == NULL)
	{
		error_(missingCommand);
		return 0;
	}
	int i = 0;
	while (args[i] != NULL)
	{
		if (!strcmp(args[i], ">") || !strcmp(args[i], ">>") || !strcmp(args[i], "|"))
		{
			if (i == 0 || args[i + 1] == NULL)
			{
				error_(missingCommand);
				return 0;
			}
			if (!strcmp(args[i], ">")) // && args[i+1] == NULL)
			{
				error_(noOutputFile);
				return -1;
			}
		}
		i++;
	}
	return 1;
}

void printJob(struct job s) // function to print completion statement from job table
{
	fprintf(stderr, "+ completed '%s' ", s.command);
	for (int i = 0; i < s.ncp; i++)
	{
		fprintf(stderr, "[%d]", s.statuses[i]); // ex. + completed 'cat test2.txt | sort' {[0][0]}
	}
	fprintf(stderr, "\n");
}

void checkJobTable() // function to check if there is a job that is now completed that hasn't been printed
{
	for (int i = 0; i < 5000; i++)
	{
		if (jobTable[i].printed == 0)
		{
			int done = 0;
			for (int j = 0; j < jobTable[i].ncp; j++)
			{
				int running = waitpid(jobTable[i].processID[j], &jobTable[i].statuses[j], WNOHANG); // check status of each command within job. if they are all done, the whole job is done, so print
				if (running == 0)
				{
					if (!strcmp("sleep 1&", jobTable[i].command))
					{
						done = 1;
						jobTable[i].statuses[j] = 0;
					}
					break;
				}
				else
				{
					if (j == jobTable[i].ncp - 1)
					{
						done = 1;
					}
				}
			}
			if (done == 1)
			{
				printJob(jobTable[i]);
				jobTable[i].printed = 1;
			}
		}
	}
}

void printExit(char *myCommand, int statuses[], int ncp) // print from commands, not job table
{
	fprintf(stderr, "+ completed '%s' ", myCommand);
	for (int i = 0; i < ncp; i++)
	{
		fprintf(stderr, "[%d]", statuses[i]);
	}
	fprintf(stderr, "\n");
}

void error_(int error) // self explanatory
{
	switch (error)
	{

	case tooManyArgs:
		fprintf(stderr, "Error: too many process arguments\n");
		break;
	case missingCommand:
		fprintf(stderr, "Error: missing command\n");
		break;
	case noOutputFile:
		fprintf(stderr, "Error: no output file\n");
		break;
	case cannotOpen:
		fprintf(stderr, "Error: cannot open output file\n");
		break;
	case mislocatedOutputRedir:
		fprintf(stderr, "Error: mislocated output redirection\n");
		break;
	case cannotCD:
		fprintf(stderr, "Error: cannot cd into directory\n");
		break;
	case commandNotFound:
		fprintf(stderr, "Error: command not found\n");
		break;
	case mislocatedBackground:
		fprintf(stderr, "Error: mislocated background sign\n");
		break;
	case activeJobs:
		fprintf(stderr, "Error: active jobs still running\n");
		break;
	default:
		break;
	}
}

/*
Surround key characters such as "|", ">", etc with spaces in case user didn't.
Allows parser to properly tokenize arguments
Extra spaces won't matter since strtok(," ") will use ALL spaces between as delimiter
*/
char *preProcessCMD(char *cmd)
{
	int j = 0;
	int redirFlag = 0;
	char *postProcess = malloc(512);
	for (int i = 0; i < (int)strlen(cmd); i++)
	{
		if (cmd[i] == '|' || cmd[i] == '<')
		{
			if(cmd[i] == '|' && redirFlag)
			{
				error_(mislocatedOutputRedir);
				return NULL;
			}
			postProcess[j++] = ' ';
			postProcess[j++] = cmd[i];
			postProcess[j++] = ' ';
		}
		else if (cmd[i] == '>')
		{
			redirFlag = 1;
			postProcess[j++] = ' ';
			postProcess[j++] = '>';
			if (cmd[i + 1] == '>')
			{
				postProcess[j++] = '>';
				i++;
			}
			postProcess[j++] = ' ';
		}
		else if (cmd[i] == '&')
		{
			if(i != (int)(strlen(cmd)-1))
			{
				error_(mislocatedBackground);
				return NULL;
			}
			background = 1;
			postProcess[j++] = ' ';
		}
		else
		{
			postProcess[j++] = cmd[i];
		}
	}
	postProcess[j] = '\0';
	return postProcess;
}

/*
Main function that will run all commands regardless of piped or not.
runCMD (function that you'll see later) is a subset of this command.
Therefore we can use this command in place of it for single non-piped commands as well.
*/
void runPiped(char *cmd, char *originalCMD)
{
	strcpy(jobTable[numJob].command, originalCMD); // take note of command and add it to jobtable
	char temp[512];
	strcpy(temp, cmd);
	char *cmd1 = NULL;
	char *cmd2 = NULL;
	char *cmd3 = NULL;
	char *cmd4 = NULL;
	char *allCMDS[5] = {cmd1, cmd2, cmd3, cmd4, NULL};
	char *buffer = strtok(temp, "|");
	int j = 0;

	while (buffer != NULL)
	{
		allCMDS[j] = buffer;
		j++;
		buffer = strtok(NULL, "|");
	}
	jobTable[numJob].ncp = j; // take note of amount of commands and add it to job table
	int statuses[j];
	char **allCMDS2[] = {parseCMD(allCMDS[0]), parseCMD(allCMDS[1]), parseCMD(allCMDS[2]), parseCMD(allCMDS[3]), NULL};
	// Developed in a way to reuse our runCMD function. A new pipe will be created in between every command
	// A fork is created every command as well to ensure the parent will never run an execvp.
	int fd[2];
	int oldfd = 0;
	int oldSTDOUT = dup(STDOUT_FILENO);
	int i = 0;
	while (allCMDS2[i] != NULL)
	{
		if (pipe(fd) == -1)
		{
			perror("fork");
		}
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		if (allCMDS2[i + 1] == NULL)
		{
			close(fd[0]);
			dup2(oldSTDOUT, 1);
			statuses[i] = runCMD(allCMDS2[i], i);
			jobTable[numJob].statuses[i] = statuses[i];
			i++;
		}
		else
		{
			statuses[i] = runCMD(allCMDS2[i], i);
			jobTable[numJob].statuses[i] = statuses[i];
			oldfd = fd[0];
			dup2(oldfd, 0);
			close(fd[0]);
			i++;
		}
	}
	if (!background)
	{
		printExit(originalCMD, statuses, j);
		jobTable[numJob].printed = 1; // if it isn't a background task, we need to print asap, and can mark it as printed and done.
	}
}

// fork+wait+exit from Jean Porquet; we print completion message elsewhere
int runCMD(char **argv, int whatNo)
{
	int pid;
	pid = fork();
	jobTable[numJob].processID[whatNo] = pid;

	if (pid == 0)
	{
		// Child
		int status = execvp(argv[0], argv);
		if (status < 0)
			error_(commandNotFound);

		exit(1);
	}
	else if (pid > 0)
	{
		int status;
		if (!background)
			waitpid(pid, &status, 0);
		return WEXITSTATUS(status);
	}
	else
	{
		perror("fork");
		exit(1);
	}
}
// Function to redirect output to specified file (appends)
int redOut2(char *file)
{
	if (file == NULL)
	{
		error_(noOutputFile);
		return -1;
	}
	int fd = open(file, O_WRONLY | O_APPEND, 0644);
	if (fd < 0)
	{
		error_(cannotOpen);
		return -1;
	}
	dup2(fd, STDOUT_FILENO);
	return 1;
}
// Function to redirect output to specified file (overwrites)
int redOut(char *file)
{
	if (file == NULL)
	{
		error_(noOutputFile);
		return -1;
	}
	int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		error_(cannotOpen);
		return -1;
	}
	dup2(fd, STDOUT_FILENO);
	return 1;
}
// Function to redirect in
int redIn(char *file)
{
	int fd = open(file, O_RDONLY);
	if (fd < 0)
	{
		error_(cannotOpen);
		return -1;
	}
	dup2(fd, STDIN_FILENO);
	return 1;
}
// parse the cmd from a single string to an array of strings
char **parseCMD(char *string)
{
	int numArgs = 0;
	if (string == NULL)
		return NULL;
	int i = 0;
	char **args = malloc(ARG_MAX * sizeof(char *));
	char *buffer = strtok(string, " ");
	numArgs++;
	while (buffer != NULL)
	{
		if (numArgs > 16)
		{
			error_(tooManyArgs);
			return NULL;
		}
		if (!strcmp(buffer, "<"))
		{
			if (i == 0)
			{
				error_(missingCommand);
				return NULL;
			}
			if (redIn(strtok(NULL, " ")) < 0)
				return NULL;
			buffer = strtok(NULL, " ");
		}
		else if (!strcmp(buffer, ">"))
		{
			if (i == 0)
			{
				error_(missingCommand);
				return NULL;
			}
			if (redOut(strtok(NULL, " ")) < 0)
				return NULL;
			buffer = strtok(NULL, " ");
		}
		else if (!strcmp(buffer, ">>"))
		{
			if (i == 0)
			{
				error_(missingCommand);
				return NULL;
			}
			if (redOut2(strtok(NULL, " ")) < 0)
				return NULL;
			buffer = strtok(NULL, " ");
			buffer = strtok(NULL, " ");
		}
		args[i] = buffer;
		numArgs++;
		buffer = strtok(NULL, " ");
		i++;
	}
	if (args == NULL)
	{
		error_(missingCommand);
		return NULL;
	}
	return args;
}

int main(void)
{
	char cmd[CMDLINE_MAX];
	char cmd2[CMDLINE_MAX];
	char originalCMD[CMDLINE_MAX];
	while (1)
	{
	reset:;
		background = 0;
		int savedOut = dup(STDOUT_FILENO);
		int savedIn = dup(STDIN_FILENO);

		char *nl;

		/* Print prompt */
		printf("sshell@ucd$ ");
		fflush(stdout);

		/* Get command line */
		if (fgets(cmd, CMDLINE_MAX, stdin) == NULL)
			exit(1); // Error: Failed to get command from CLI

		/* Print command line if stdin is not provided by terminal */
		if (!isatty(STDIN_FILENO))
		{
			printf("%s", cmd);
			fflush(stdout);
		}

		/* Remove trailing newline from command line */
		nl = strchr(cmd, '\n');
		if (nl)
			*nl = '\0';

		/* Save original command to pass through and print later*/
		strcpy(originalCMD, cmd);
		strcpy(cmd2, cmd);

		char *postCMD = preProcessCMD(cmd);

		char **args = parseCMD(cmd2); // for build in cmds

		if (args == NULL)
		{
			goto trueReset;
		}
		if (checkCLError(args) == -1)
		{
			goto trueReset;
		}

		/* Remove trailing newline from command line */
		nl = strchr(cmd, '\n');
		if (nl)
			*nl = '\0';

		if (!strcmp(cmd, "\0"))
		{
			checkJobTable();
			memset(cmd, 0, sizeof cmd);
			dup2(savedOut, STDOUT_FILENO);
			dup2(savedIn, STDIN_FILENO);
			goto reset;
		}
		

		if (postCMD == NULL)
		{
			goto trueReset;
		}

		/* Builtin commands */
		if (!strcmp(cmd, "exit"))
		{
			int cantExit = 0;
			for (int i = 0; i < 5000; i++)
			{
				if (jobTable[i].printed == 0)
				{
					for (int j = 0; j < jobTable[i].ncp; j++)
					{
						int running = waitpid(jobTable[i].processID[j], &jobTable[i].statuses[j], WNOHANG);
						if (running == 0)
						{
							cantExit = 1;
							break;
						}
					}
				}
			}
			if (cantExit == 0)
			{
				fprintf(stderr, "Bye...\n");
				fprintf(stderr, "+ completed 'exit' [0]\n");
				break;
			}
			else
			{
				error_(activeJobs);
				fprintf(stderr, "+ completed 'exit' [1]\n");
				goto trueReset;
			}
		}
		else if (!strcmp(cmd, "pwd"))
		{
			char dir[CMDLINE_MAX];
			printf("%s\n", getcwd(dir, CMDLINE_MAX));
			goto reset;
		}
		else if (!strcmp(args[0], "cd"))
		{
			int status = 0;
			if (chdir(args[1]) == -1)
			{
				error_(cannotCD);
				status = 1;
			}
			fprintf(stderr, "+ completed 'cd %s' [%d]\n", args[1], status);
			goto reset;
		}

		/* Regular command */
		checkJobTable();
		runPiped(postCMD, originalCMD);
		numJob++;
	trueReset:;
		/* Free all manually allocated memory*/
		memset(cmd, 0, sizeof cmd);
		free(args);

		/*Restore stdout*/
		dup2(savedOut, STDOUT_FILENO);
		dup2(savedIn, STDIN_FILENO);
	}

	return EXIT_SUCCESS;
}