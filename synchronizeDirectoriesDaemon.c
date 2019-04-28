#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
#include<dirent.h>
#include<stdarg.h> //unknown number of arguments

void writeToLog(int log, char* format, ...);
int isDirectory(const char *path);
void copyFile(char *srcPath, char *destPath);
int isRegularFile(const char *path);

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
        writeToLog(daemonLog, "STARTED SUCCES\n");
        size_t bytes;

        sid = setsid();

        // Error checking
        if(sid<0) {
        writeToLog(daemonLog, "\nSID ERROR");
        close(daemonLog);
        exit(EXIT_FAILURE); 
        }

        // Change current working directory
        chdir("/");
        
        // Error checking
        if(chdir("/") < 0) {
            writeToLog(daemonLog, "\nCHANGE DIRECTORY ERROR");
            close(daemonLog);
            exit(EXIT_FAILURE);
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

    char buffer2[1024];

    while(1) {
       
        writeToLog(daemonLog, "\nENTER INFINITE LOOP\n");

        if(argv[3] == NULL) {
            writeToLog(daemonLog, "DAEMON SLEEP 300 sec.\n");
            sleep(300);
        } else {
            writeToLog(daemonLog, "DAEMON SLEEP %s sec.\n", argv[3]);
            sleep(atoi(argv[3]));
        }
        writeToLog(daemonLog, "DAEMON WOKE UP\n");

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
            writeToLog(daemonLog, "\nDIRECTORY STREAM ERROR");
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

                char srcFilePath[1024];
                char destFilePath[1024];

                destEntry = readdir(dest);
                writeToLog(daemonLog, "READING ENTRIES...\n");

                if(srcEntry->d_type == DT_REG) {
                    char *fileName = srcEntry->d_name;

                    snprintf(srcFilePath,1024,"%s/%s", argv[1], fileName);
                    snprintf(destFilePath,1024,"%s/%s", argv[2], fileName);

                    struct stat sb;
                    
                    //write to log stat of current entry, testing/educational purpose mainly, size of file will be used later
                    if(stat(srcFilePath, &sb) != -1) {
                        writeToLog(daemonLog, "\nDirectory:      %s\nFile:           %s\nFile Full Path: %s\nSize:           %ld\n",
                        argv[1], fileName, srcFilePath, sb.st_size);
                    }

                    copyFile(srcFilePath,destFilePath);
                }

                /*if( destEntry != NULL && destEntry->d_type == DT_REG) {
                    char *fileName = destEntry->d_name;
                    snprintf(buffer2, 1024, fileName, "\n");
                    bytes = strlen(buffer);
                    write(daemonLog, buffer, bytes);
                }*/
            }
            closedir(src);  // close source directory stream
            closedir(dest); // close destination directory stream
            // TODO: Error checking for closedir()
        }
    }
}

void writeToLog(int log, char* format, ...) {
    size_t bytes;
    char buffer[1024];
    va_list argPtr;
    va_start(argPtr, format);
    vsnprintf(buffer,1024, format, argPtr);
    bytes = strlen(buffer);
    write(log, buffer, bytes);
    va_end(argPtr);
}

void copyFile(char *srcFilePath, char *destFilePath)
{
    int src, dest;
    int bufferSize = 1000;
    char *buffer = malloc(bufferSize*sizeof(char));
    
    src = open(srcFilePath, O_RDONLY);
    dest = open(destFilePath, O_CREAT | O_WRONLY);  
    
    size_t bytesRead;   

    while ((bytesRead = read(src, buffer, bufferSize)) > 0){
        write(dest, buffer, bytesRead);
        printf("bytes: %ld\n",bytesRead); 
    }
}

//returns non zero value if path is directory
int isDirectory(const char *path) {
    struct stat sb;     //<sys/stat.h>
    stat(path, &sb);
    return S_ISDIR(sb.st_mode);
}

int isRegularFile(const char *path) {
    struct stat sb;  //<sys/stat.h>
    stat(path, &sb);
    return S_ISREG(sb.st_mode);
}