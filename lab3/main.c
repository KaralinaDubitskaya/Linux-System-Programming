#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <wchar.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <x86_64-linux-gnu/sys/wait.h>
#include <libgen.h>

// Buffer for reading files.
#define BUFFER_SIZE (16 * 1024)

// Info about periods of bits.
typedef struct
{
    unsigned long len; // Number of bits
    unsigned long num; // Number of periods of size [len]
} prd;

// Info about periods of 0's and 1's of the file's bits.
typedef struct
{
    int num_of_bytes;
    prd *periods_of_0;
    prd *periods_of_1;
    int num_of_periods_of_0;
    int num_of_periods_of_1;
} bits_periods;

int count_periods_of_bits(char *dir_name, struct stat root_dir_stat);
bits_periods *get_periods(char *file_name);
bits_periods *get_periods_of_string(char *str);
bits_periods *count_periods(FILE *file);
int get_bit(char value, char position);
void print_result(int pid, char *path, int num_of_bytes, prd *periods_of_0, prd *periods_of_1, int num_of_periods_0, int num_of_periods_1);
void sort(prd *a, int n);
void print_error(const char *path, const char *error_message);

// The basename of the program
char *program_name;
// Number of concurrent processes.
unsigned char num_of_processes;
// The max number of concurrent processes.
unsigned char max_num_of_processes;

ino_t *visited_inodes = NULL; // Array contains inodes of visited files.
int vst_ind_len = 0;          // Length of the array of visited inodes (the previous line).

int main(int argc, char *argv[])
{
    // Used to display error messages.
    program_name = basename(argv[0]);

    // Check input.
    if (argc != 3)
    {
        print_error("Wrong number of parameters. Usage:", "$./lab3.exe \"path_name\" \"max_num_of_processes\"");
        return 1;
    }

    // Get absolete path name of the directory.
    char max_path_len[PATH_MAX + 1];
    char *path;
    path = realpath(argv[1], max_path_len);

    if (path == NULL)
    {
        print_error(argv[1], "Can't open file ");
        return 1;
    }


    // Get stat structure, which contains info about directory, entered by user.
    // If pathname (argv[1]) is a symbolic link, then it returns information
    // about the link itself, not the file that it refers to.
    struct stat root_dir_stat;
    if (lstat(argv[1], &root_dir_stat) == -1)
    {
        print_error(argv[1], strerror(errno));
        return 1;
    }

    // The number of concurrent processes should not exceed max_num_of_processes.
    max_num_of_processes = (unsigned char)strtol(argv[2], NULL, 10);

    // Parent and 1..254 child processes
    if (max_num_of_processes < 2)
    {
        print_error(argv[1], "Wrong max number of processes.");
        return 1;
    }

    // Number of concurrent processes.
    num_of_processes = 1;

    // Recursively count periods of bits for all files of the directory.
    count_periods_of_bits(path, root_dir_stat);

    // Wait all child processes
    while (wait(NULL) > 0) {}

    return 0;
}

// Recursively count periods of bits for all files of the directory.
int count_periods_of_bits(char *dir_name, struct stat root_dir_stat)
{
    DIR *dir_pointer = NULL;
    struct dirent *dir_entry;

    dir_pointer = opendir(dir_name);
    if (dir_pointer == NULL)
    {
        print_error(dir_name, strerror(errno));
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
        if (lstat(entry_path, &dir_entry_info))
        {
            print_error(entry_path, strerror(errno));
            continue;
        }

        // The file is a directory.
        if (S_ISDIR(dir_entry_info.st_mode) && (dir_entry_info.st_dev == root_dir_stat.st_dev))
        {
            // Multiple hard links
            if ((int) dir_entry_info.st_nlink > 1)
            {
                // If false, inode has been visited by another link and file info has already been processed.
                bool flag = true;

                // Array contains visited inodes numbers.
                for (int i = 0; i < vst_ind_len; i++)
                {
                    if (visited_inodes[i] == dir_entry_info.st_ino)
                    {
                        flag = false;
                    }
                }

                // The directory is visited by the first time.
                if (flag)
                {
                    if ((visited_inodes = (ino_t *) realloc(visited_inodes, (vst_ind_len + 1) * sizeof(ino_t))) == NULL)
                    {
                        print_error(entry_path, strerror(errno));
                    }
                    else
                    {
                        // Note that the file was processed.
                        visited_inodes[vst_ind_len] = dir_entry_info.st_ino;
                        vst_ind_len++;

                        // Recursively calculate for each entry of the directory.
                        count_periods_of_bits(entry_path, root_dir_stat);
                    }

                }
            }
            else
            {
                // Recursively calculate for each entry of the directory.
                count_periods_of_bits(entry_path, root_dir_stat);
            }
            continue;

        }

        // The file is a regular file.
        if (S_ISREG(dir_entry_info.st_mode) && (dir_entry_info.st_dev == root_dir_stat.st_dev))
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
                    print_result(getpid(), entry_path, periods->num_of_bytes, periods->periods_of_0,
                                 periods->periods_of_1, periods->num_of_periods_of_0, periods->num_of_periods_of_1);
                    if (periods->periods_of_1 != NULL)
                        free(periods->periods_of_1);
                    if (periods->periods_of_0 != NULL)
                        free(periods->periods_of_0);
                    free(periods);
                    exit(0);
                }
                exit(1);
            }
            else if (pid < 0)
            {
                print_error(dir_name, strerror(errno));
                exit(1);
            }

            num_of_processes++;
        }

        // The file is a symbolic link.
        if (S_ISLNK(dir_entry_info.st_mode) && (dir_entry_info.st_dev == root_dir_stat.st_dev))
        {
            char link[PATH_MAX + 1];

            if (readlink(entry_path, link, PATH_MAX) == -1)
            {
                print_error(entry_path, strerror(errno));
            }
            else
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
                    periods = get_periods_of_string(link);
                    if (periods != NULL)
                    {
                        print_result(getpid(), entry_path, periods->num_of_bytes, periods->periods_of_0, periods->periods_of_1, periods->num_of_periods_of_0, periods->num_of_periods_of_1);
                        free(periods);
                        exit(0);
                    }
                    exit(1);
                }
                else if (pid < 0)
                {
                    print_error(dir_name, strerror(errno));
                    exit(1);
                }

                num_of_processes++;
            }
        }
        free(entry_path);
    }

    if (closedir(dir_pointer) == -1)
    {
        print_error(dir_name, strerror(errno));
    }

    return 0;
}

bits_periods *get_periods_of_string(char *str)
{
    int curr_bit = -1;
    int prev_bit;
    unsigned long period = 1;

    prd *periods_of_0 = NULL;
    prd *periods_of_1 = NULL;
    int num_of_periods_of_0 = 0;
    int num_of_periods_of_1 = 0;

    char ch;
    int len = (int)strlen(str);
    for (int i = 0; i < len; i++)
    {
        ch = str[i];
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
                bool flag = true;
                for (int j = 0; (j < num_of_periods_of_0) && flag; j++)
                {
                    if ((periods_of_0 != NULL) && (periods_of_0[j].len == period))
                    {
                        periods_of_0[j].num++;
                        period = 1;
                        flag = false;
                    }
                }

                if (flag)
                {
                    num_of_periods_of_0++;
                    periods_of_0 =
                        (prd *)realloc(periods_of_0, num_of_periods_of_0 * sizeof(prd));
                    periods_of_0[num_of_periods_of_0 - 1].len = period;
                    periods_of_0[num_of_periods_of_0 - 1].num = 1;
                    period = 1;
                }
            }
            else if ((curr_bit == 0) && (prev_bit == 1))
            {
                bool flag = true;
                for (int j = 0; (j < num_of_periods_of_1) && flag; j++)
                {
                    if ((periods_of_1 != NULL) && (periods_of_1[j].len == period))
                    {
                        periods_of_1[j].num++;
                        period = 1;
                        flag = false;
                    }
                }

                if (flag)
                {
                    num_of_periods_of_1++;
                    periods_of_1 =
                            (prd *)realloc(periods_of_1, num_of_periods_of_1 * sizeof(prd));
                    periods_of_1[num_of_periods_of_1 - 1].len = period;
                    periods_of_1[num_of_periods_of_1 - 1].num = 1;
                    period = 1;
                }

            }
        }
    }

    if (curr_bit == 0)
    {
        bool flag = true;
        for (int j = 0; (j < num_of_periods_of_0) && flag; j++)
        {
            if ((periods_of_0 != NULL) && (periods_of_0[j].len == period))
            {
                periods_of_0[j].num++;
                flag = false;
            }
        }

        if (flag)
        {
            num_of_periods_of_0++;
            periods_of_0 =
                    (prd *)realloc(periods_of_0, num_of_periods_of_0 * sizeof(prd));
            periods_of_0[num_of_periods_of_0 - 1].len = period;
            periods_of_0[num_of_periods_of_0 - 1].num = 1;
        }
    }

    if (curr_bit == 1) {
        bool flag = true;
        for (int j = 0; (j < num_of_periods_of_1) && flag; j++)
        {
            if ((periods_of_1 != NULL) && (periods_of_1[j].len == period))
            {
                periods_of_1[j].num++;
                flag = false;
            }
        }

        if (flag)
        {
            num_of_periods_of_1++;
            periods_of_1 =
                    (prd *)realloc(periods_of_1, num_of_periods_of_1 * sizeof(prd));
            periods_of_1[num_of_periods_of_1 - 1].len = period;
            periods_of_1[num_of_periods_of_1 - 1].num = 1;
        }
    }

    bits_periods *periods = malloc(sizeof(bits_periods));
    periods->num_of_bytes = len;
    periods->periods_of_0 = periods_of_0;
    periods->periods_of_1 = periods_of_1;
    periods->num_of_periods_of_0 = num_of_periods_of_0;
    periods->num_of_periods_of_1 = num_of_periods_of_1;

    return periods;
}

void print_result(int pid, char *path, int num_of_bytes, prd *periods_of_0, prd *periods_of_1, int num_of_periods_0, int num_of_periods_1)
{
    sort(periods_of_0, num_of_periods_0);
    sort(periods_of_1, num_of_periods_1);

    printf("%d %s %d 0: ", pid, path, num_of_bytes);

    for (int i = 0; i < num_of_periods_0; i++)
    {
        printf("%lu=%lu ", periods_of_0[i].len, periods_of_0[i].num);
    }

    printf("1: ");

    for (int i = 0; i < num_of_periods_1; i++)
    {
        printf("%lu=%lu ", periods_of_1[i].len, periods_of_1[i].num);
    }

    printf("\n");
}


void sort(prd *a, int n)
{
    int j, nn;

    do {
        nn = 0;
        for (j = 1; j < n; ++j)
            if (a[j-1].len > a[j].len)
            {
                prd temp;
                temp.len = a[j-1].len;
                temp.num = a[j-1].num;

                a[j-1].len = a[j].len;
                a[j-1].num = a[j].num;

                a[j].len = temp.len;
                a[j].num = temp.num;
                nn = j;
            }
        n = nn;
    } while (n);
}



bits_periods *get_periods(char *file_name)
{
    char buffer[BUFFER_SIZE];

    int fd = open(file_name, O_RDONLY);

    if (fd == -1)
    {
        print_error(file_name, strerror(errno));
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
        print_error(file_name, strerror(errno));
        return NULL;
    }

    FILE *file_ptr  = fopen(file_name,"r");


    bits_periods *periods;
    periods = count_periods(file_ptr);
    periods->num_of_bytes = num_of_bytes;


    if (fclose(file_ptr) == -1)
    {
        print_error(file_name, strerror(errno));
        return NULL;
    }

    return periods;
}


bits_periods *count_periods(FILE *file)
{
    int curr_bit = -1;
    int prev_bit;
    unsigned long period = 1;

    prd *periods_of_0 = NULL;
    prd *periods_of_1 = NULL;
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
                bool flag = true;
                for (int j = 0; (j < num_of_periods_of_0) && flag; j++)
                {
                    if ((periods_of_0 != NULL) && (periods_of_0[j].len == period))
                    {
                        periods_of_0[j].num++;
                        period = 1;
                        flag = false;
                    }
                }

                if (flag)
                {
                    num_of_periods_of_0++;
                    periods_of_0 =
                            (prd *)realloc(periods_of_0, num_of_periods_of_0 * sizeof(prd));
                    periods_of_0[num_of_periods_of_0 - 1].len = period;
                    periods_of_0[num_of_periods_of_0 - 1].num = 1;
                    period = 1;
                }
            }
            else if ((curr_bit == 0) && (prev_bit == 1))
            {
                bool flag = true;
                for (int j = 0; (j < num_of_periods_of_1) && flag; j++)
                {
                    if ((periods_of_1 != NULL) && (periods_of_1[j].len == period))
                    {
                        periods_of_1[j].num++;
                        period = 1;
                        flag = false;
                    }
                }

                if (flag)
                {
                    num_of_periods_of_1++;
                    periods_of_1 =
                            (prd *)realloc(periods_of_1, num_of_periods_of_1 * sizeof(prd));
                    periods_of_1[num_of_periods_of_1 - 1].len = period;
                    periods_of_1[num_of_periods_of_1 - 1].num = 1;
                    period = 1;
                }

            }
        }
    }

    if (curr_bit == 0)
    {
        bool flag = true;
        for (int j = 0; (j < num_of_periods_of_0) && flag; j++)
        {
            if ((periods_of_0 != NULL) && (periods_of_0[j].len == period))
            {
                periods_of_0[j].num++;
                flag = false;
            }
        }

        if (flag)
        {
            num_of_periods_of_0++;
            periods_of_0 =
                    (prd *)realloc(periods_of_0, num_of_periods_of_0 * sizeof(prd));
            periods_of_0[num_of_periods_of_0 - 1].len = period;
            periods_of_0[num_of_periods_of_0 - 1].num = 1;
        }
    }

    if (curr_bit == 1) {
        bool flag = true;
        for (int j = 0; (j < num_of_periods_of_1) && flag; j++)
        {
            if ((periods_of_1 != NULL) && (periods_of_1[j].len == period))
            {
                periods_of_1[j].num++;
                flag = false;
            }
        }

        if (flag)
        {
            num_of_periods_of_1++;
            periods_of_1 =
                    (prd *)realloc(periods_of_1, num_of_periods_of_1 * sizeof(prd));
            periods_of_1[num_of_periods_of_1 - 1].len = period;
            periods_of_1[num_of_periods_of_1 - 1].num = 1;
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
void print_error(const char *path, const char *error_message)
{
    fprintf(stderr, "%s: %s %s\n", program_name, path, error_message);
}