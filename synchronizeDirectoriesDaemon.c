#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
#include <dirent.h>


//returns non zero value if path is directory
int isDirectory(const char *path) {
    struct stat buffer;     //<sys/stat.h>
    stat(path, &buffer);
    return S_ISDIR(buffer.st_mode);
}

int isRegularFile(const char *path) {
    struct stat path_stat;  //<sys/stat.h>
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void writeToLog(int *log, char *buffer, size_t bytes) {

}


void daemonize() {

}


int main(int argc, const char *argv[]) {
    
    FILE *testFile;
    testFile= fopen("testFile.txt", "w");
    //fprintf(testFile,"%s\n", argv[1]);

    int isDir = isDirectory(argv[1]);
    printf("\n%s", argv[1]);

    //TODO: Delete dynamic buffer, replace with static, use snprintf to write to log

    if(isDirectory(argv[1]) == 0 && isDirectory(argv[2]) == 0) {
        fprintf(testFile,"ERROR! Invalid arguments: both paths have to be directories");
    } else {
        pid_t pid=0, sid=0; //process ID and session ID
    
        pid = fork();
    
        // Error checking
        if(pid < 0) 
            _exit(EXIT_FAILURE);
        else if(pid > 0)
            _exit(EXIT_SUCCESS);
        umask(0);
    
        int daemonLog = open("daemonLog.txt", O_CREAT | O_WRONLY | O_TRUNC);
        char *buffer;
        buffer = "STARTED SUCCES\n";
        size_t bytes = strlen(buffer);
        write(daemonLog, buffer, bytes);

        sid = setsid();

        // Error checking
        if(sid<0) {
        buffer = "SID ERROR\n" ;
        bytes = strlen(buffer);
        write(daemonLog, buffer, bytes);

        close(daemonLog);
        exit(EXIT_FAILURE); 
        }

        // Change current working directory
        chdir("/");
        
        // Error checking
        if(chdir("/") < 0) {
            buffer="CHANGE DIRECTORY ERROR";
            bytes = strlen(buffer);

            close(daemonLog);
            exit(EXIT_FAILURE);
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

    char buffer2[1024];


    while(1) {

        snprintf(buffer2,1024,"\nENTER INFINITE LOOP\n");
        bytes = strlen(buffer2);
        write(daemonLog, buffer2, bytes);

        if(argv[3] == NULL) {
            snprintf(buffer2,1024,"DAEMON SLEEP 300 sec.\n");
            bytes = strlen(buffer2);
            write(daemonLog, buffer2, bytes);
            sleep(300);
        } else {
            snprintf(buffer2,1024,"DAEMONN SLEEP %s sec.\n", argv[3]);
            bytes = strlen(buffer2);
            write(daemonLog, buffer2, bytes);
            sleep(atoi(argv[3]));
        }
        buffer = "DAEMON WOKE UP\n";
        bytes = strlen(buffer2);
        write(daemonLog, buffer2, bytes);


        DIR *src = NULL, *dest = NULL; // directory pointer, DIR object
        
        src = opendir(argv[1]);       // create source directory stream
        dest = opendir(argv[2]);      // create source directory stream
        // directory stream, opendir returns DIR object, <sys/types.h>, <dirent.h>
        // A directory stream is little more than a file descriptor representing the open directory, 
        // some metadata, and a buffer to hold the directory's contents. 
        // Consequently, it is possible to obtain the file descriptor behind a given directory stream:
        // function for obtaining file descriptor: int dirfd (DIR *dir);
        
        // Error checking 
        if(!src && !dest) {
            buffer = "DIRECTORY STREAM ERROR\n" ;
            bytes = strlen(buffer);
            write(daemonLog, buffer, bytes);
            close(daemonLog);
            exit(EXIT_FAILURE); 
        }

        // if directory stream created successfully, we can read entries from directory
        // struct dirent * readdir (DIR *dir);  - returns entries one by one from given DIR object
        // readdir() obtains entries one by one, untill the one we are searching for is found 
        // or the entire directory is read, at which time it returns NULL
            
        // struct 'dirent' defined in <dirent.h>
        // 'd_name' field in structure is the name of single file within the directory 
        struct  dirent *srcEntry, *destEntry; // entries in directories

        
        
        // Reading/synchronizing directories
            while( (srcEntry = readdir(src)) != NULL) {
                destEntry = readdir(dest);
                buffer = "READING ENTRIES...\n";
                bytes = strlen(buffer);
                write(daemonLog, buffer, bytes);

                //TODO: DT_REG undefined??? Can be fixed with  isRegularFile();
                if(srcEntry->d_type == DT_REG) {
                    char *filename = srcEntry->d_name;
                    snprintf(buffer2, 1024, filename, "\n");
                    bytes = strlen(buffer2);
                    write(daemonLog, buffer2, bytes);
                    
                    snprintf(buffer2, 1024, argv[2], "\n");
                    bytes = strlen(buffer2);
                    write(daemonLog, buffer2, bytes);

                }
                if( destEntry != NULL && destEntry->d_type == DT_REG) {
                    char *fileName = destEntry->d_name;
                    snprintf(buffer2, 1024, fileName, "\n");
                    bytes = strlen(buffer);
                    write(daemonLog, buffer, bytes);
                }
            }
            closedir(src);  // close source directory stream
            closedir(dest); // close destination directory stream
            // TODO: Error checking for closedir()
        }
    }
  
}
