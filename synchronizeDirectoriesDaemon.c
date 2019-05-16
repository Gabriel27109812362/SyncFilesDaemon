#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h> // variadic function which takes undefined number of arguments: //writeToLog()
#include <getopt.h>
#include <sys/mman.h>
#include <signal.h>
#include <utime.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <errno.h>

int sleepValue = 300;
long int sizeThreshold = 100000;
int recursionFlag = 0;
int sigusr1Flag = 0;

struct tm *getCurrentDate();
void signalHandler(int signum);
void daemonize();
void explore(char *srcDirectory, char *destDirectory);
int isDirectory(const char *path);
int isRegularFile(const char *path);
int fileExists(char *path);
void copyFile(char *srcPath, char *destPath, size_t fileSize);
static time_t getFileModificationTime(const char *path);
void updateModificationTime(const char *path, time_t srcTime);
void deleteDirectoryTree(char* path);

int main(int argc, char *argv[]) {

    int opt;
    //int daemonLog = open("daemonLog.txt", O_CREAT | O_WRONLY | O_TRUNC);

    while((opt=getopt(argc,argv, "RS:B:")) != -1) {
        switch(opt) {
            case 'R':
                recursionFlag = 1;
                break;
            case 'S':
                sleepValue = atoi(optarg);
                break;
            case 'B':
                sizeThreshold = atoi(optarg);
                break;
             default:
                break;
        }
    }
    if(isDirectory(argv[argc-2]) == 0 || isDirectory(argv[argc-1]) == 0) { //check if arguments are directories,

        ////writeToLog(daemonLog, "\nERROR: ARGUMENTS ARE NOT DIRECTORIES\n");
        //close(daemonLog);
        exit(EXIT_FAILURE);
    } else {
        setlogmask(LOG_UPTO(LOG_NOTICE)); //set log mask
        syslog(LOG_NOTICE, "\nProgram started by user: %d", getuid());

        daemonize();

        signal(SIGUSR1, signalHandler); //Powiązanie sygnału SIGUSR1 ze zdefiniowanym handlerem
        while(1) {

            sigusr1Flag = 0;

            

            syslog(LOG_NOTICE, "%s daemon falls asleep for %d sec(SIGUSR1 wakes it up)", asctime(getCurrentDate()),sleepValue);
            sleep(sleepValue);
            if( !sigusr1Flag)
                syslog(LOG_NOTICE, "daemon woke up naturally");

            explore(argv[argc-2],argv[argc-1]); // Reading/synchronizing directories
        }
    }
}

struct tm  *getCurrentDate() {

    time_t currentTime;
    struct tm *timeinfo;
    time(&currentTime );
    timeinfo = localtime ( &currentTime );
    return timeinfo;
}

void signalHandler(int signum) {
    if(signum == SIGUSR1) {

        syslog(LOG_NOTICE, "daemon woke up using signal SIGUSR1");

        sigusr1Flag = 1;
    }
}

void daemonize() {
    pid_t pid=0, sid=0; //process ID and session ID

        pid = fork();

        if(pid < 0) {
            ////writeToLog(log, "\nERROR: PROCESS ID\n");
            //close(log);
            _exit(EXIT_FAILURE);
        }
        else if(pid > 0) {
            _exit(EXIT_SUCCESS);
        }
        umask(0);
        //////writeToLog(log, "\nDAEMON STARTED SUCCESSFULLY\n");
        syslog(LOG_NOTICE, "Program became daemon process");

        sid = setsid();

        if(sid<0) {
            //writeToLog(log, "\nERROR: SESSION ID");
            //close(log);
            exit(EXIT_FAILURE);
        }
        // Change current working directory
        chdir("/");

        if(chdir("/") < 0) {
            //writeToLog(log, "\nERROR: CHANGING DIRECTORY FAILED");
            //close(log);
            exit(EXIT_FAILURE);
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
}

void copyFile(char *srcFilePath, char *destFilePath, size_t fileSize) {
    int src, dest;
    long int bufferSize; 
    char *buffer;
    size_t bytesRead;

    src = open(srcFilePath, O_RDONLY);
    dest = open(destFilePath, O_CREAT | O_WRONLY);

    if(fileSize >= sizeThreshold) {
        buffer = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, src,0);
        write(dest, buffer, fileSize);
    } else {
        bufferSize = 100;
        buffer = malloc(bufferSize*sizeof(char));

        while ((bytesRead = read(src, buffer, bufferSize)) > 0)
        write(dest, buffer, bytesRead);

    }

    while ((bytesRead = read(src, buffer, bufferSize)) > 0) 
        write(dest, buffer, bytesRead);
    close(dest);
    close(src);
}


int isDirectory(const char *path) {
    struct stat sb;
    stat(path, &sb);
    return S_ISDIR(sb.st_mode);
}

int isRegularFile(const char *path) {
    struct stat sb;
    stat(path, &sb);
    return S_ISREG(sb.st_mode);
}

int fileExists (char *path) {
  struct stat sb;   
  return (stat (path, &sb) == 0);
}

static time_t getFileModificationTime(const char *path) {
    struct stat sb;
    if (stat(path, &sb) == 0)
        return sb.st_mtime;
    return 0;
}

void updateModificationTime(const char *path, time_t srcTime) {
    struct stat sb;
    struct utimbuf ub;
    if(stat(path, &sb)==0)
        sb.st_mtime = srcTime;
    if (utime(path, &ub) == 0) {
        srcTime = sb.st_mtime;
    
        ub.actime = sb.st_atime; // keep access time unchanged 
        ub.modtime = srcTime;   // set modification time of destination file so it's up to date with source file
        utime(path,&ub);
    }
}

void deleteDirectoryTree(char* path) {
    DIR *dest;
    struct dirent *entry;
    char newPath[1024] = {0};
    dest = opendir(path);

    if(!dest) {
        ////writeToLog(log, "\nERROR: DIRECTORY STREAM\n");
        //close(log);
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(dest)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;       //skip symbolic attachments to avoid infinite recursion

        sprintf(newPath, "%s/%s", path, entry->d_name);
        if (isDirectory(newPath)) {
            deleteDirectoryTree(newPath);
        } else {
            unlink(newPath);
            syslog(LOG_NOTICE, "File deleted successfully: %s", newPath);
        }
    }
    closedir(dest);
    rmdir(path);
    syslog(LOG_NOTICE, "Directory deleted successfully: %s", path);
    
}

void explore(char *srcDirectory, char *destDirectory) {
    
    DIR *src = NULL, *dest = NULL; 

    src = opendir(srcDirectory);
    dest = opendir(destDirectory);

    if(!src && !dest) {
        //////writeToLog(log, "\nERROR: DIRECTORY STREAM");
        //close(log);
        exit(EXIT_FAILURE);
    }

    struct  dirent *srcEntry, *destEntry;
    char srcFilePath[1024], destFilePath[1024];
    char srcFileName[1024], destFileName[1024];
    struct stat sb;
    char srcRecursionPath[1024], destRecursionPath[1024];

    while( (destEntry = readdir(dest)) != NULL ) {

        if(destEntry->d_type == DT_REG) {
            snprintf(destFileName, 1024, "%s", destEntry->d_name);
            snprintf(destFilePath,1024,"%s/%s", destDirectory, destFileName);

                snprintf(srcFilePath,1024,"%s/%s", srcDirectory, destFileName);
                if (fileExists(srcFilePath) ) {
                    
                } else {
                    int status;
                    status = unlink(destFilePath);
                    if(status == 0) {
                        syslog(LOG_NOTICE, "File deleted successfully: %s", destFilePath);
                    } else {
                        ////writeToLog(log, "\nERROR: UNABLE TO DELETE THE FILE");
                        //close(log);
                        exit(EXIT_FAILURE);
                    }
                }
        } else if(destEntry->d_type == DT_DIR && recursionFlag == 1) {

            if (strcmp(destEntry->d_name, ".") == 0 || strcmp(destEntry->d_name, "..") == 0)
                continue;       //skip symbolic attachments to avoid infinite recursion
            
            char destDirectoryRemove[1024];
            char srcDirectoryExists[1024];
            snprintf(destDirectoryRemove,1024,"%s/%s", destDirectory, destEntry->d_name);
            snprintf(srcDirectoryExists,1024,"%s/%s", srcDirectory, destEntry->d_name);
            
            if(isDirectory(srcDirectoryExists) == 0){
                deleteDirectoryTree(destDirectoryRemove);
            }
        } else 
            continue;
    }
    rewinddir(src);
    rewinddir(dest);

    while( (srcEntry = readdir(src)) != NULL ) {

        if(srcEntry->d_type == DT_REG) {
            snprintf(srcFileName, 1024, "%s", srcEntry->d_name);
            snprintf(srcFilePath,1024,"%s/%s", srcDirectory, srcFileName);
                    
            snprintf(destFilePath,1024,"%s/%s", destDirectory, srcFileName);

            if (fileExists(destFilePath) ) { // check if similar file exists in destination directory
                if(isRegularFile(destFilePath)){ 
                    time_t srcTime = getFileModificationTime(srcFilePath);
                    time_t destTime = getFileModificationTime(destFilePath);
                    
                    if(srcTime > destTime || srcTime < destTime) {

                        copyFile(srcFilePath,destFilePath, sb.st_size);
                        syslog(LOG_NOTICE, "File copied successfully: %s", srcFilePath);
                        updateModificationTime(destFilePath, srcTime);
                    }
                }
            } else {
                copyFile(srcFilePath,destFilePath, sb.st_size);
                syslog(LOG_NOTICE, "File copied successfully: %s", srcFilePath);
            }
        } else if(srcEntry->d_type == DT_DIR && recursionFlag == 1) { 

            if (!strcmp(srcEntry->d_name, ".") || !strcmp(srcEntry->d_name, "..")) 
                continue;       //skip symbolic attachments to avoid infinite recursion
                
            struct stat st = {0};
            
            snprintf(srcRecursionPath,1024,"%s/%s", srcDirectory, srcEntry->d_name);
            snprintf(destRecursionPath,1024,"%s/%s", destDirectory, srcEntry->d_name);

            if (stat(destRecursionPath, &st) == -1) {
                mkdir(destRecursionPath, 0777);
                syslog(LOG_NOTICE, "Directory created successfully: %s", destRecursionPath);
            }
        
            explore(srcRecursionPath,destRecursionPath);
        } else 
            continue;
    }
    closedir(src);  // close source directory stream
    closedir(dest); // close destination directory stream
}