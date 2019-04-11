#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>

int wr_to_log(char *buffer, int *log);

int main(int argc, const char *argv[]) {
    
    pid_t pid, sid;
    char *buffer;
    pid = fork();
    int daemonLog = open("daemonLog.txt", O_CREAT | O_WRONLY | O_TRUNC);

    if(pid < 0) {
        _exit(EXIT_FAILURE);
    }
    if(pid>0) {
        _exit(EXIT_SUCCESS);
    }
    umask(0);
    
    buffer = "STARTED SUCCES\n";
    size_t bytes = strlen(buffer);
    write(daemonLog, buffer, bytes);

    sid = setsid();

    //session id error
    if(sid<0) {
        buffer = "SID ERROR\n" ;
        bytes = strlen(buffer);
        write(daemonLog, buffer, bytes);

        close(daemonLog);
        exit(EXIT_FAILURE);  //end programme when error
    }

    //change current working directory
    //if(chdir("/") < 0) {
    //    buffer="CHANGE DIRECTORY ERROR";
    //    bytes = strlen(buffer);

    //    close(daemonLog);
    //    exit(EXIT_FAILURE);
    //}

    //close(STDIN_FILENO);
    //close(STDOUT_FILENO);
    //close(STDERR_FILENO);
    //Big Loop 
    char buffer2[1024];

    while(1) {
        snprintf(buffer2,1024,"\nENTER INFINITE LOOP\n");
        //buffer ="\nENTER INFINITE LOOP\n";
        bytes = strlen(buffer2);
        write(daemonLog, buffer2, bytes);

        if(argv[1] == NULL) {
            snprintf(buffer2,1024,"DAEMON SLEEP 300 sec.\n");
            //buffer="DAEMON SLEEP 300 sec.\n";
            bytes = strlen(buffer2);
            write(daemonLog, buffer2, bytes);
            sleep(300);
        } else {
            snprintf(buffer2,1024,"DAEMONN SLEEP %s sec.\n", argv[1]);
            //buffer = "DAEMON SLEEP ELSE\n";
            //strcat(buffer,"DAEMON SLEEP");
            //strcat(strcat(buffer,"DAEMON SLEEP "),argv[1]);
            bytes = strlen(buffer2);
            write(daemonLog, buffer2, bytes);
            sleep(atoi(argv[1]));
        }
        buffer = "DAEMON WOKE UP\n";
        bytes = strlen(buffer);
        write(daemonLog, buffer, bytes);
    }
}
