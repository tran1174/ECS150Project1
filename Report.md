Anthony and I chose to break our code up into functions for each section. 
The development of each function for phases is documented below.

Phase 1:
Our basic command runner uses the fork+wait+exit taught to us by 
the Professor and utilizes execvp to search by Path. We called it runCMD.
In our function, it creates a child with fork. 
The child then execs the function using execvp and throws a 
perror if it can't. The parent then waits until the function is 
successfully completed and then gives the completion statement along with 
printing the command that it was asked to run.
If none of this happens, a perror is thrown for a forking error.

Phase 2, Phase 4: 
Parsing our command line happens in our parseCMD function. 
This function allows for us to collect all of the arguments in 
an array of strings that we can use to run later using the runCMD 
that we made in phase 1. We take the inputted commands, then 
tokenize it using strtok() with blank space as the delimiter. This 
allows for the collection of all arguments and commands.

We chose to implement phase 4 before phase 3 here since we were 
working on the parsing of the command line. Phase 4 just detects if 
the next token is either <, >, >>, or | for all of the special cases. 
We implemented the first three, and created three special redirects. 
RedIn, RedOut, and RedOut2 for each respectively. 
RedIn opened the file after for reading only. RedOut opened it for 
overwriting or creating, and RedOut2 opened it for appending. Each 
used the open() command, with RedOut and RedOut2 giving write permission
and opening for write only.
We also detected the pipeline | for later.

Phase 3:
Phase three just detected if the command line was pwd or cd and 
threw an error if cd couldn't happen. This is implemented by parsing 
the command line, checking if it is pwd or cd (DIRECTORY) and calling 
the respective command.

Phase 5:
The implementation of pipelines happened in our runPiped function.
In runPiped, we input the command line as an array of an array of 
strings, broken up by the pipeline delimiter to make it easier to loop
for us. We then track the old FD and STDOUT to make sure we are grabbing
information and sending it to the right place. Then, we loop the 
required amount of times (given by how many arrays are in our array), 
calling pipe each time. What we do each time in the loop the 
amount of arrays that are stored out of a maximum of 3. 

PIPING EXPLANATION // 

Phase 6:
Writing to append was not hard, we just added an extra function 
called redOut2 which was called when the ">>" was seen, and was the 
output redirector but with O_APPEND in the open function instead.

BACKGROUND JOBS //

Anthony and I were able to test our program through copying the examples 
that were given by the Professor on his project 1 html file. We then 
made sure that our file compiled on the CSIF and ran it against the
simple grader that he gave. Then, we generated a few misc test cases
of our own to make sure it threw the correct errors, and tried to 
break our own program. 

We used no code from sources other than the lecture files and 
sample code given by the Professor. 


