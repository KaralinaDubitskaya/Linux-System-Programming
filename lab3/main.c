#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <locale.h>
#include <wchar.h>

#include <wctype.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <stdbool.h>



#include <x86_64-linux-gnu/sys/wait.h>

#define ARGS_COUNT 3
#define BUFFER_SIZE (16 * 1024)


//char *prog_name;
//int max_working_processes_amount;
//int working_processes_amount;

//void print_error(char *prog_name, char *error_message, char *error_file)
//{
//    fprintf(stderr, "\n%s: %s %s\n", prog_name, error_message, error_file ? error_file : "");
//}
//








// The structure contains info about periods of 0's and 1's of the file's bits.
typedef struct
{
    int num_of_bytes;
    int *periods_of_0;
    int *periods_of_1;
    int num_of_periods_of_0;
    int num_of_periods_of_1;
} bits_periods;

int count_periods_of_bits(char *dir_name);
bits_periods *get_periods(char *file_name);
bits_periods *count_periods(FILE *file);
int get_bit(char value, char position);
void print_result(int pid, char *full_path, int bytes_amount, int *array0, int *array1, int size0, int size1);
void print_error(const char *error_message);

// The basename of the program
char *program_name;
// Number of concurrent processes.
char num_of_processes;
// The max number of concurrent processes.
char max_num_of_processes;

int main(int argc, char *argv[])
{
    // Used to display error messages.
    program_name = basename(argv[0]);

    // Check input.
    if (argc != 3)
    {
        print_error("Wrong number of parameters. Usage: $./lab3.exe \"path_name\" \"max_num_of_processes\"");
        return 1;
    }

    // Get absolete path name of the directory.
    char max_path_len[PATH_MAX + 1];
    char *path;
    path = realpath(argv[1], max_path_len);

    if (path == NULL)
    {
        char *error_msg = "";
        strcpy(error_msg, "Can't open file ");
        error_msg = strcat(error_msg, argv[1]);
        print_error(error_msg);
        return 1;
    }

    // The number of concurrent processes should not exceed max_num_of_processes.
    max_num_of_processes = (char)strtol(argv[2], NULL, 10);

    // TODO why 1?
    if (max_num_of_processes < 2)
    {
        print_error("Wrong max number of processes.");
        return 1;
    }

    //todo Number of concurrent processes.
    num_of_processes = 1;

    count_periods_of_bits(path);

//todo ?
    while (wait(NULL) > 0) {}

    return 0;
}

//todo comment
int count_periods_of_bits(char *dir_name)
{
    DIR *dir_pointer = NULL;
    struct dirent *dir_entry;

    dir_pointer = opendir(dir_name);
    if (dir_pointer == NULL)
    {
        char *error_msg = "";
        strcpy(error_msg, dir_name);
        error_msg = strcat(error_msg, strerror(errno));
        print_error(error_msg);
        return 1;
    }

    // The readdir() function returns a pointer to a dir_entry representing
    // the next directory entry in the directory stream pointed to by
    // dir_pointer. It returns NULL on reaching the end of the directory stream.
    while ((dir_entry = readdir(dir_pointer)) != NULL) {

        // Skip current and parent directories.
        if ((strcmp(dir_entry->d_name, ".") == 0) ||
            (strcmp(dir_entry->d_name, "..") == 0)) {
            continue;
        }

        // Get full pathname of the entry.
        char *entry_path = malloc(strlen(dir_name) + strlen(dir_entry->d_name) + 2);
        strcpy(entry_path, dir_name);
        if (entry_path[strlen(dir_name) - 1] != '/') {
            strcat(entry_path, "/");
        }
        strcat(entry_path, dir_entry->d_name);

        // Contains main info about the entry.
        struct stat dir_entry_info;
        if (lstat(entry_path, &dir_entry_info)) {
            char *error_msg = "";
            strcpy(error_msg, entry_path);
            error_msg = strcat(error_msg, strerror(errno));
            print_error(error_msg);
            continue;
        }

        // The file is a directory.
        if (S_ISDIR(dir_entry_info.st_mode))
        {
            count_periods_of_bits(dir_name);
        }

        // The file is a regular file.
        if (S_ISREG(dir_entry_info.st_mode))
        {
            // Waiting for the child process to complete.
            if (num_of_processes >= max_num_of_processes)
            {
                wait(NULL);
                num_of_processes--;
            }

            pid_t pid = fork();

            // Child Process
            if (pid == 0)
            {
                bits_periods *periods;
                periods = get_periods(entry_path);
                if (periods != NULL)
                {
                    print_result(getpid(), entry_path, periods->num_of_bytes, periods->periods_of_0, periods->periods_of_1, periods->num_of_periods_of_0, periods->num_of_periods_of_1);
                    exit(0);
                }
                exit(1);
            }
            else if (pid < 0)
            {
                char *error_msg = "";
                strcpy(error_msg, dir_name);
                error_msg = strcat(error_msg, strerror(errno));
                print_error(error_msg);
                exit(1);
            }

            num_of_processes++;
        }


    }

    if (closedir(dir_pointer) == -1)
    {
        char *error_msg = "";
        strcpy(error_msg, dir_name);
        error_msg = strcat(error_msg, strerror(errno));
        print_error(error_msg);
    }

    return 0;
}

void print_result(int pid, char *path, int num_of_bytes, int *periods_of_0, int *periods_of_1, int num_of_periods_0, int num_of_periods_1)
{
    printf("%d %s %d 0:", pid, path, num_of_bytes);

    for (int i = 0; i < num_of_periods_0; i++)
    {
        printf("%d ", periods_of_0[i]);
    }

    printf("1: ");

    for (int i = 0; i < num_of_periods_1; i++)
    {
        printf("%d ", periods_of_1[i]);
    }

    printf("\n");
}



bits_periods *get_periods(char *file_name)
{
    char buffer[BUFFER_SIZE];

    int fd = open(file_name, O_RDONLY);

    if (fd == -1)
    {
        char *error_msg = "";
        strcpy(error_msg, file_name);
        error_msg = strcat(error_msg, strerror(errno));
        print_error(error_msg);
        return NULL;
    }

    int num_of_bytes = 0;
    int bytes_read = 0;
    while ((bytes_read = (int)read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        num_of_bytes += bytes_read;
    }

    if (bytes_read < 0)
    {
        char *error_msg = "";
        strcpy(error_msg, file_name);
        error_msg = strcat(error_msg, strerror(errno));
        print_error(error_msg);
        return NULL;
    }

    FILE *file_ptr  = fopen(file_name,"r");


    bits_periods *periods;
    periods = count_periods(file_ptr);
    periods->num_of_bytes = num_of_bytes;


    if (fclose(file_ptr) == -1) {
        char *error_msg = "";
        strcpy(error_msg, file_name);
        error_msg = strcat(error_msg, strerror(errno));
        print_error(error_msg);
        return NULL;
    }

    return periods;
}


bits_periods *count_periods(FILE *file)
{
    int curr_bit = -1;
    int prev_bit;
    int period = 1;

    int *periods_of_0 = NULL;
    int *periods_of_1 = NULL;
    int num_of_periods_of_0 = 0;
    int num_of_periods_of_1 = 0;

    char ch;
    while ((ch = (char)getc(file)) != EOF)
    {
        for (char position = 7; position >= 0; position--)
        {
            prev_bit = curr_bit;
            curr_bit = get_bit(ch, position);

            if ((curr_bit == 0) && (prev_bit == 0))
            {
                period++;
            }
            else if ((curr_bit == 1) && (prev_bit == 1))
            {
                period++;
            }
            else if ((curr_bit == 1) && (prev_bit == 0))
            {
                num_of_periods_of_0++;
                periods_of_0 =
                        (int *)realloc(periods_of_0, num_of_periods_of_0 * sizeof(int));
                periods_of_0[num_of_periods_of_0 - 1] = period;
                period = 1;
            }
            else if ((curr_bit == 0) && (prev_bit == 1))
            {
                num_of_periods_of_1++;
                periods_of_1 =
                        (int *)realloc(periods_of_1, num_of_periods_of_1 * sizeof(int));
                periods_of_1[num_of_periods_of_1 - 1] = period;
                period = 1;

            }
        }
    }

    bits_periods *periods = malloc(sizeof(bits_periods));
    periods->num_of_bytes = 0;
    periods->periods_of_0 = periods_of_0;
    periods->periods_of_1 = periods_of_1;
    periods->num_of_periods_of_0 = num_of_periods_of_0;
    periods->num_of_periods_of_1 = num_of_periods_of_1;

    return periods;
}


int get_bit(const char value, const char position)
{
    if ((value & ((char)1 << position)) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


// Print  error message to stream stderr
void print_error(const char *error_message)
{
    fprintf(stderr, "%s: %s\n", program_name, error_message);
}