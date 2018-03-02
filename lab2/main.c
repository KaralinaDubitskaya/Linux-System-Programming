#include <stdio.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

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
        print_error(program_name, strerror(errno));
        return 1;
    }

    return 0;
}

void print_error(const char *program_name, const char *error_message)
{
    fprintf(stderr, "%s: %s\n", program_name, error_message);
};