#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
#include<dirent.h>
#include<stdarg.h> // variadic function which takes undefined number of arguments: writeTolog()

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
            char srcFilePath[1024], destFilePath[1024];
            char srcFileName[1024], destFileName[1024];
            struct stat sb;
            struct dirent_extra *extra;
            long srcLoc, destLoc;
            int fileDeletionflag; //helps to indentify when the file should be deleted
            // Reading/synchronizing directories

            rewinddir(dest);
             while( (destEntry = readdir(dest)) != NULL ) {

                fileDeletionflag = 0;
                writeToLog(daemonLog, "READING DESTINATION ENTRIES...\n");
                if(destEntry->d_type != DT_REG)
                  continue;

                snprintf(destFileName, 1024, "%s", destEntry->d_name);

                snprintf(destFilePath,1024,"%s/%s", argv[2], destFileName);
                if(stat(destFilePath, &sb) != -1) {
                    writeToLog(daemonLog, "\nDirectory:      %s\nFile:           %s\nFile Full Path: %s\nSize:           %ld\n",
                    argv[2], destFileName, destFilePath, sb.st_size);
                }

                //extra = _DEXTRA_FIRST(&destEntry);
                while ((srcEntry = readdir(src)) != NULL) {

                    writeToLog(daemonLog, "READING SOURCE ENTRIES...\n");

                    if(srcEntry->d_type != DT_REG)
                        continue;

                    snprintf(srcFileName, 1024, "%s", srcEntry->d_name);

                    // files in both directories are certainly regular
                    if((strcmp(srcFileName,destFileName)) == 0){
                        destLoc = telldir(dest);
                        seekdir(dest, destLoc);
                        writeToLog(daemonLog, "\nFILES ARE EQUAL, BREAK\n\n");
                        fileDeletionflag = 1;
                        break;
                    }
                    snprintf(srcFilePath,1024,"%s/%s", argv[1], srcFileName);

                    //write to log stat of current entry, testing/educational purpose mainly, size of file will be used later
                    if(stat(srcFilePath, &sb) != -1) {
                        writeToLog(daemonLog, "\nDirectory:      %s\nFile:           %s\nFile Full Path: %s\nSize:           %ld\n",
                        argv[1], srcFileName, srcFilePath, sb.st_size);
                    }
                }

                rewinddir(src);
                
                if(fileDeletionflag == 0) {
                    //remove instead of copy

                    writeToLog(daemonLog, "\nDELETING FILES\n");
                    
                    int status;
                    status = unlink(destFilePath);
                    if(status == 0) {
                        writeToLog(daemonLog, "\nFILE DELETED SUCCESSFULLY");
                        destEntry = readdir(dest);
                    }
                    else {
                        writeToLog(daemonLog, "\nERROR: UNABLE TO DELETE THE FILE");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            rewinddir(src);
            rewinddir(dest);

            while( (srcEntry = readdir(src)) != NULL ) {

                writeToLog(daemonLog, "READING SOURCE ENTRIES...\n");
                if(srcEntry->d_type != DT_REG)
                  continue;

                snprintf(srcFileName, 1024, "%s", srcEntry->d_name);
                
                snprintf(srcFilePath,1024,"%s/%s", argv[1], srcFileName);
                
                if(stat(srcFilePath, &sb) != -1) {
                    writeToLog(daemonLog, "\nDirectory:      %s\nFile:           %s\nFile Full Path: %s\nSize:           %ld\n",
                    argv[1], srcFileName, srcFilePath, sb.st_size);
                }

                //extra = _DEXTRA_FIRST(&destEntry);
                while ((destEntry = readdir(dest)) != NULL) {

                    writeToLog(daemonLog, "READING DESTINATION ENTRIES...\n");

                    if(destEntry->d_type != DT_REG)
                        continue;

                    snprintf(destFileName, 1024, "%s", destEntry->d_name);

                    // files in both directories are certainly regular
                    if(strcmp(srcEntry->d_name,destEntry->d_name) != 0)
                        continue;
                    //files in both directories have the same name

                    snprintf(destFilePath,1024,"%s/%s", argv[2], srcFileName);

                    //write to log stat of current entry, testing/educational purpose mainly, size of file will be used later
                    
                    writeToLog(daemonLog, "\n\n\n\nCOPYING FILES\n");
                    copyFile(srcFilePath,destFilePath);

                    if(stat(destFilePath, &sb) != -1) {
                        writeToLog(daemonLog, "\nDirectory:      %s\nFile:           %s\nFile Full Path: %s\nSize:           %ld\n",
                        argv[2], destFileName, destFilePath, sb.st_size);
                    }
                    break;
                }
                srcLoc = telldir(src);
                seekdir(src, srcLoc);
                rewinddir(dest);
                continue;
            }
            
            closedir(src);  // close source directory stream
            closedir(dest); // close destination directory stream
            //TODO: Error checking for closedir()
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

void copyFile(char *srcFilePath, char *destFilePath) {
    int src, dest;
    int bufferSize = 1000;
    char *buffer = malloc(bufferSize*sizeof(char));

    src = open(srcFilePath, O_RDONLY);
    dest = open(destFilePath, O_CREAT | O_WRONLY);

    size_t bytesRead;

    while ((bytesRead = read(src, buffer, bufferSize)) > 0) {
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
