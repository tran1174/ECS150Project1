# ECS 150 Project #1 - Simple Shell
**Joshua Hernandez, Anthony Tran**
## Summary
Our program, `sshell`, is a simple shell that accepts input from the user and executes it as a job. It does five things:

1.  Execution of user-supplied commands with optional arguments
2.  Selection of typical builtin commands
3.  Redirection of the standard output of commands to files
4.  Composition of commands via piping
5.  Background jobs

## Jobs
Each command line string is considered a job. Each job is represented by a struct, which keeps track of the generated process ID's, the original string from the command line that needs to be printed out during the completion statement, a place to store the statuses of each small command, how many commands there are to fork, and if the job completion statement was printed out or not. Each job, when created, is then stored in the Job Table, to make sure we keep track of how many jobs are active at a time. One limitation of this Job Table is that we are only able to store 5000 jobs in it at a time. meaning that if somehow, the user inputs 5000 lines into our command line without exiting, a segmentation fault would occur.

## Reading and Parsing

Once the sshell prompt is printed and user input is submitted, the shell parses the command given by the user. It does this through tokenizing a copy of the command string, separating it by whitespace. Each individual "word" in the command is considered an argument to be stored and executed later. 

If the command parser finds ">", or ">>", then we know that our output is being sent to a different file. So we invoke our output redirectors, and set our standard output to the next token (if it can be opened), since that will be the name of the file.  


## Basic Job Running
The cleaned string from pre processing is fed into the runPiped() function which handles the determination of how many commands there are through tokenizing with the delimiter of "|" and creates a job.  In the case that there is just one command and no pipelining, tokenizing the command string by the "|" delimiter will return the whole string.

Running a simple, one command job involves invoking runCMD which executes the fork + wait + exit method we discussed in class.

If the user invokes a builtin function, we sense than in our main function and use the respective C functions accordingly. 

## Pipelines
Pipelines are achieved by tokenizing the command string (after preprocessing) with the "|" delimiter. This allows us to determine how many commands are being pipelined, and process each command individually. The results of each processed command are stored in an array that contains each command in their own array.

Each smaller array is then sent off into its own runCMDPiped, forked, and stored in the job table, with the last one going into the original runCMD which can print the completion statement it if it is not a background task. 

## Background Job
A job is considered a background job if the "&" symbol is found at the end of the inputted command string. When this happens, the variable 'background' is turned to 1, signifying that the next job that is about to be created is a background job. Each job, regardless of being background or not, is stored in the Job Table.  

A job that isn't a background job is printed at the end of the command being run, and is automatically considered written in the job table. 

With background jobs, the completion statement is postponed until, at minimum, the next command is inputted. Once the next command is inputted, the shell runs through the Job Table and checks if there are commands that haven't been printed. If it finds one, it checks the status of all of the job's children. If any have not finished, it does not print the job completion statement. However, if all are finished, then the completion sentence is printed and the job is marked as done. Once the Job Table has finished parsing, then the new command is executed and its completion statement is printed if not a background task. 

## Testing the Program
We were able to test our program through copying the examples that were given by the Professor on his project 1 html file. We then made sure that our file compiled on the CSIF and ran it against the simple grader that he gave. Then, we generated a few misc test cases of our own to make sure it threw the correct errors, and tried to break our own program. 

We used no code from sources other than the lecture files and sample code given by the Professor. 

