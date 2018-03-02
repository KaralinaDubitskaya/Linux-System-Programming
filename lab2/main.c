#include <stdio.h>
#include <libgen.h>

void print_error(const char *program_name, const char *error_message);

int main(int argc, char *argv[])
{
    // Used to display error messages
    char *program_name = basename(argv[0]);

    if (argc != 2)
    {
        print_error(program_name, "Wrong number of parameters. Usage: ./lab2.exe \"path_name\"");
        return 1;
    }

    return 0;
}

void print_error(const char *program_name, const char *error_message)
{
    fprintf(stderr, "%s: %s\n", program_name, error_message);
};