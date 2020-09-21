#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include<errno.h>

extern int errno;

#define MAX_LINE 80
#define MAX_COMMANDS 20
#define MAX_BUF 2048

char current_path[200];

char *command_output = "/tmp/command_output";

void get_tokens(char *buffer, int num_characters, char* args[MAX_COMMANDS][MAX_LINE/2 + 1], int *num_command, int commands_args_size[MAX_COMMANDS], int is_outbound_redirection[MAX_COMMANDS], int should_run_in_background[MAX_COMMANDS]){
    int i = 0, j = 0;
    //int num_command = 0;
    //int commands_args_size[MAX_COMMANDS];
    while(i < num_characters){
        char* command;
        command = (char*)malloc(num_characters * sizeof(char));
        j = 0;
        while(i < num_characters && buffer[i] != ' '){
            command[j] = buffer[i];
            i++; j++;
        }
        args[*num_command][0] = command;
        int param_num = 1;
        while(i < num_characters && buffer[i + 1] != '|' && buffer[i + 1] != '>' && buffer[i + 1] != '&'){
            char* param;
            param = (char*)malloc(num_characters * sizeof(char));
            i++;
            j = 0;
            while(i < num_characters && buffer[i] != ' '){
                param[j] = buffer[i];
                i++; j++;
            }
            args[*num_command][param_num] = param;
            param_num++;
        }
        args[*num_command][param_num] = '\0';
        if(i < num_characters){
            if(buffer[i + 1] == '&'){
                should_run_in_background[*num_command] = 1;
                i += 2;
            }
            else{
                should_run_in_background[*num_command] = 0;
            }
            if(i < num_characters){
                if(buffer[i + 1] == '>'){
                    is_outbound_redirection[*num_command] = 1;
                }
                else{
                    is_outbound_redirection[*num_command] = 0;
                }
            }
            i = i + 3;
        }
        commands_args_size[*num_command] = param_num;
        *num_command = *num_command + 1;
    }
//    for(i = 0; i < num_command; i++){
//        for(j = 0; j < commands_args_size[i]; j++){
//            printf("%s ", args[i][j]);
//        }
//        printf("\n");
//    }
}

void print_tokens(char* args[MAX_COMMANDS][MAX_LINE/2 + 1], int num_commands, int commands_args_size[MAX_COMMANDS], int is_outbound_redirection[MAX_COMMANDS], int should_run_in_background[MAX_COMMANDS]){
    for(int i = 0; i < num_commands; i++){
        for(int j = 0; j < commands_args_size[i]; j++){
            printf("%s ", args[i][j]);
        }
        if(should_run_in_background[i]){
            printf("& ");
        }
        if(i < num_commands - 1){
            if(is_outbound_redirection[i]){
                printf("> ");
            }
            else{
                printf("| ");
            }
        }
    }
    printf("\n");
}

void execute(char* args[MAX_COMMANDS][MAX_LINE/2 + 1], int num_commands, int commands_args_size[MAX_COMMANDS], int is_outbound_redirection[MAX_COMMANDS], int should_run_in_background[MAX_COMMANDS]){
    if(strcmp("cd", args[0][0]) == 0){
        if(chdir(args[0][1]) == -1){
            printf("ERROR: chdir failed\n");
        }
    }
    else{
        pid_t pid;
        if((pid = fork()) < 0){
            fprintf(stderr, "ERROR: fork failed\n");
            exit(1);
        }
        else{
            if(pid == 0){ /* Child process */
                if(is_outbound_redirection[0]){
                    if(num_commands == 1){
                        printf("ERROR: You must introduce a file name\n");
                        exit(1);
                    }
                    mkfifo(args[1][0], 0666);
                    int fd = open(args[1][0], O_RDWR | O_CREAT);
                    dup2(fd, STDOUT_FILENO);
                    if(execvp(args[0][0], args[0]) < 0){
                        printf("ERROR: exec failed\n");
                        exit(1);
                    }
                }
                else{
                    if(num_commands == 2){
                        pid_t pid2;
                        if((pid2 = fork()) < 0){
                            fprintf(stderr, "ERROR: fork failed\n");
                            exit(1);
                        }
                        else{
                            if(pid2 == 0){
                                args[1][commands_args_size[1]] = "/tmp/process_output";
                                args[1][commands_args_size[1] + 1] = NULL;
                                if(execvp(args[1][0], args[1]) < 0){
                                    printf("ERROR: exec failed\n");
                                    exit(1);
                                }
                            }
                            else{
                                mkfifo("/tmp/process_output", 0666);
                                int fd = open("/tmp/process_output", O_RDWR | O_CREAT);
                                dup2(fd, STDOUT_FILENO);
                                if(execvp(args[0][0], args[0]) < 0){
                                    printf("ERROR: exec failed\n");
                                    exit(1);
                                }
                            }
                        }
                    }
                    else{
                        if(execvp(args[0][0], args[0]) < 0){
                            printf("ERROR: exec failed\n");
                            exit(1);
                        }
                    }
                }
            }
            else{ /* Parent process */
                if(!should_run_in_background[0]){
                    wait(NULL);
                }
            }
        }
    }
}

void main(){
    char *args[MAX_COMMANDS][MAX_LINE/2 + 1];
    char *buffer;
    buffer = (char*)malloc(MAX_LINE * sizeof(char));
    int should_run = 1, num_characters;
    size_t buffer_size = MAX_LINE;
    while(should_run){
        printf("%s> ", getcwd(current_path, 200));
        num_characters = getline(&buffer, &buffer_size, stdin);
        buffer[--num_characters] = '\0';
        if(strlen(buffer) == 0){
            continue;
        }
        if(strcmp("exit", buffer) == 0 || strcmp("logout", buffer) == 0 || strcmp("close", buffer) == 0){
            should_run = 0;
        }
        else{
            int num_commands = 0;
            int commands_args_size[MAX_COMMANDS] = {0};
            int is_outbound_redirection[MAX_COMMANDS] = {0};
            int should_run_in_background[MAX_COMMANDS] = {0};
            get_tokens(buffer, num_characters, args, &num_commands, commands_args_size, is_outbound_redirection, should_run_in_background);
            //print_tokens(args, num_commands, commands_args_size, is_outbound_redirection, should_run_in_background);
            execute(args, num_commands, commands_args_size, is_outbound_redirection, should_run_in_background);
            //printf("%d\n", getppid());
        }
    }
}