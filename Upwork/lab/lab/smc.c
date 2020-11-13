#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "sm.h"

static void process_commands(FILE *file);
FILE *outputFifoPipe, *inputFifoPipe;
int outputFd, inputFd;

void openInput()
{
    inputFd = open(FIFO_OP_FILE_NAME, O_RDONLY);

    if (inputFd == -1)
    {
        printf("Error opening fifo pipe : %s\n", strerror(errno));
        exit(1);
    }

    inputFifoPipe = fdopen(inputFd, "r");

    if (inputFifoPipe == NULL)
    {
        printf("Error opening input stream to fifo pipe\n");
        exit(1);
    }
}

void closeInput()
{
    fclose(inputFifoPipe);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    outputFd = open(FIFO_IP_FILE_NAME, O_WRONLY);

    if (outputFd == -1)
    {
        printf("Error opening fifo pipe : %s\n", strerror(errno));
        exit(1);
    }

    outputFifoPipe = fdopen(outputFd, "a");

    if (outputFifoPipe == NULL)
    {
        printf("Error opening stream to fifo pipe\n");
        exit(1);
    }

    printf("Welcome to sm controller\n");
    process_commands(stdin);

    return 0;
}

static void print_prompt(void)
{
    printf("smc> ");
    fflush(stdout);
}

static void process_commands(FILE *file)
{
    bool exiting = false;
    char *line = NULL;
    size_t line_size = 0;

    print_prompt();
    while (!exiting)
    {
        if (getline(&line, &line_size, file) == -1)
        {
            if (feof(file))
            {
                printf("End of commands; shutting down\n");
            }
            else
            {
                perror("Error while reading command; shutting down\n");
            }
            fprintf(outputFifoPipe, "shutdown\n");
            fflush(outputFifoPipe);
            break;
        }

        if ((strcmp(line, "shutdown\n") == 0))
        {
            fprintf(outputFifoPipe, line);
            fflush(outputFifoPipe);
            exiting = true;
        }
        else if ((strcmp(line, "exit\n") == 0))
        {
            exiting = true;
        }
        else
        {
            fprintf(outputFifoPipe, line);
            fflush(outputFifoPipe);
        }
        int ch;
        openInput();
        while ((ch = getc(inputFifoPipe)) != EOF)
        {
            printf("%c", ch);
        }
        closeInput();
        if (!exiting)
            print_prompt();
    }

    if (line)
    {
        free(line);
    }

    fclose(outputFifoPipe);

    if (ferror(file))
    {
        perror("Failed to read line");
        exit(1);
    }
}
