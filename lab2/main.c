#include <stdio.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

int calc_dir_size(char *dir_name, char *program_name);
void print_error(const char *program_name, const char *error_message);

int main(int argc, char *argv[])
{
    // Used to display error messages.
    char *program_name = basename(argv[0]);

    // Check input.
    if (argc != 2)
    {
        print_error(program_name, "Wrong number of parameters. Usage: ./lab2.exe \"path_name\"");
        return 1;
    }

    // Get stat structure, which contains info about directory, entered by user.
    // If pathname (argv[1]) is a symbolic link, then it returns information
    // about the link itself, not the file that it refers to.
    struct stat root_dir_stat;
    if (lstat(argv[1], &root_dir_stat) == -1)
    {
        char *error_message = "lstat: ";
        strcat(error_message, strerror(errno));
        print_error(program_name, error_message);
        return 1;
    }

    check_dir();

    return 0;
}

int calc_dir_size(char *dir_name, char *program_name)
{
    DIR *dir_pointer = NULL;
    struct dirent *dir_entry;

    dir_pointer = opendir(dir_name);
    if (dir_pointer == NULL)
    {
        strcat(dir_name, ": ");
        strcat(dir_name, strerror(errno));
        print_error(program_name, dir_name);
        return 1;
    }

    // The readdir() function returns a pointer to a dir_entry representing
    // the next directory entry in the directory stream pointed to by
    // dir_pointer. It returns NULL on reaching the end of the directory stream.
    while ((dir_entry = readdir(dir_pointer)))
    {
        // Skip current and parent directories.
        if ((strcmp(dir_entry->d_name, ".") == 0) ||
            (strcmp(dir_entry->d_name, "..") == 0))
        {
            continue;
        }

        
    }
}

void print_error(const char *program_name, const char *error_message)
{
    fprintf(stderr, "%s: %s\n", program_name, error_message);
};