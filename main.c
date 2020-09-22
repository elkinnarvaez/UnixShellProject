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
#define MAX_COMMAND_OUTPUT_BUFFER_SIZE 8192

char current_path[200];

void get_tokens(char *buffer, int num_characters, char* args[MAX_COMMANDS][MAX_LINE/2 + 1], int *num_command, int commands_args_size[MAX_COMMANDS], int is_outbound_redirection[MAX_COMMANDS], int should_run_in_background[MAX_COMMANDS]){
    int i = 0, j = 0;
    //int num_command = 0;
    //int commands_args_size[MAX_COMMANDS];
    while(i < num_characters){
        char* command;
        command = (char*)malloc(num_characters * sizeof(char));
        command[0] = '\0';
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
            param[0] = '\0';
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

void write_command_in_history_file(char* args[MAX_COMMANDS][MAX_LINE/2 + 1], int num_commands, int commands_args_size[MAX_COMMANDS], int is_outbound_redirection[MAX_COMMANDS], int should_run_in_background[MAX_COMMANDS]){
    FILE *fp;
    fp = fopen("history.txt", "a");
    for(int i = 0; i < num_commands; i++){
        for(int j = 0; j < commands_args_size[i]; j++){
            //fprintf(fp, "%s ", args[i][j]);
            fputs(args[i][j], fp);
            fputs(" ", fp);
        }
        if(should_run_in_background[i]){
            //fprintf(fp, "& ");
            fputs("& ", fp);
        }
        if(i < num_commands - 1){
            if(is_outbound_redirection[i]){
                //fprintf(fp, "> ");
                fputs("> ", fp);
            }
            else{
                //fprintf(fp, "| ");
                fputs("| ", fp);
            }
        }
    }
    //fprintf(fp, "\n");
    fputs("\n", fp);
    fclose(fp);
}

void print_file_content(char *file_name){
   FILE *fp;
   int c;

   fp = fopen(file_name, "r");
   while(1) {
      c = fgetc(fp);
      if(feof(fp)){
         break;
      }
      printf("%c", c);
   }
   fclose(fp);
}

void execute(char* args[MAX_COMMANDS][MAX_LINE/2 + 1], int num_commands, int commands_args_size[MAX_COMMANDS], int is_outbound_redirection[MAX_COMMANDS], int should_run_in_background[MAX_COMMANDS]){
    if(strcmp("cd", args[0][0]) == 0){
        if(chdir(args[0][1]) == -1){
            printf("ERROR: chdir failed\n");
        }
    }
    else{
        pid_t pid, pid2;
        int a[2], b[2], readbytes;
        char buffer[MAX_COMMAND_OUTPUT_BUFFER_SIZE];
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
                    int fd = open(args[1][0], O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    dup2(fd, STDOUT_FILENO);
                    if(strcmp("history", args[0][0]) == 0){
                        print_file_content("history.txt");
                        exit(1);
                    }
                    else{
                        if(execvp(args[0][0], args[0]) < 0){
                            printf("ERROR: exec failed\n");
                            exit(1);
                        }
                    }
                }
                else{
                    if(num_commands == 2){
                        pipe(a);
                        pipe(b);
                        if((pid2 = fork()) < 0){
                            fprintf(stderr, "ERROR: fork failed\n");
                            exit(1);
                        }
                        else{
                            if(pid2 == 0){
                                close(a[1]); /* Cerramos la lectura para el proceso padre */
                                close(b[0]); /* Cerramos la escritura para el proceso padre */
                                dup2(a[0], STDIN_FILENO);
                                if(execvp(args[1][0], args[1]) < 0){
                                    printf("ERROR: exec failed\n");
                                    exit(1);
                                }
                            }
                            else{
                                close(a[0]); /* Cerramos la lectura para el proceso hijo */
                                close(b[1]); /* Cerramos la escritura para el proceso hijo */
                                dup2(a[1], STDOUT_FILENO);
                                if(strcmp("history", args[0][0]) == 0){
                                    print_file_content("history.txt");
                                    exit(1);
                                }
                                else{
                                    if(execvp(args[0][0], args[0]) < 0){
                                        printf("ERROR: exec failed\n");
                                        exit(1);
                                    }
                                }
                            }
                        }
                    }
                    else{
                        if(strcmp("history", args[0][0]) == 0){
                            print_file_content("history.txt");
                            exit(1);
                        }
                        else{
                            if(execvp(args[0][0], args[0]) < 0){
                                printf("ERROR: exec failed\n");
                                exit(1);
                            }
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
    int should_run = 1, num_characters;
    size_t buffer_size = MAX_LINE;
    while(should_run){
        char *args[MAX_COMMANDS][MAX_LINE/2 + 1];
        char *buffer;
        buffer = (char*)malloc(MAX_LINE * sizeof(char));
        buffer[0] = '\0';
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
            write_command_in_history_file(args, num_commands, commands_args_size, is_outbound_redirection, should_run_in_background);
            execute(args, num_commands, commands_args_size, is_outbound_redirection, should_run_in_background);
        }
    }
}