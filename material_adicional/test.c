#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int using_dup2_example1(){
    int fd = open("file.txt", O_RDWR | O_CREAT);
    dup2(fd, STDOUT_FILENO);
    printf("Hola\n");
    printf("Hola\n");
}


int using_chdir_example1(){
    char s[100]; 
  
    // printing current working directory 
    printf("%s\n", getcwd(s, 100)); 
  
    // using the command 
    chdir(".."); 
  
    // printing current working directory 
    printf("%s\n", getcwd(s, 100)); 
  
    // after chdir is executed 
    return 0; 
} 

int create_processes(){
    pid_t  pid, pid2;
    int    status;
    if ((pid = fork()) == 0) {
        if((pid2 = fork()) == 0){
            sleep(3);
            printf("Second child\n");
            exit(1);
        }
        else{
            char *args[3];
            args[0] = "ls";
            args[1] = "-a";
            args[2] = NULL;
            if(execvp(args[0], args) < 0){
                printf("ERROR: exec failed\n");
                exit(1);
            }
        }
    }
    else {
        wait(NULL);
        sleep(3);
        printf("Children finished\n");
    }
    return 0;
}

int main(){
    FILE *fp = NULL;
    char current_path[200];
    getcwd(current_path, 200);
    strcat(current_path, "/test.txt");
    fp = fopen(current_path, "w");
    fputs("Hola", fp);
    fclose(fp);
    return 0;
}