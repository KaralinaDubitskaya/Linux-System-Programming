#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libgen.h>

int calc_dir_size(char *dir_name, char *program_name, struct stat root_dir_stat, long long *block_count_pointer,
                   long long *size_pointer, ino_t *visited_inodes, int *vst_ind_len_pointer, const int BLOCK_SIZE);
void print_error(const char *program_name, const char *directory, const char *error_message);

int main(int argc, char *argv[])
{
    // Used to display error messages.
    char *program_name = basename(argv[0]);

    // Check input.
    if (argc != 2)
    {
        print_error(program_name, "Wrong number of parameters. Usage", "./lab2.exe \"path_name\"");
        return 1;
    }

    // Get stat structure, which contains info about directory, entered by user.
    // If pathname (argv[1]) is a symbolic link, then it returns information
    // about the link itself, not the file that it refers to.
    struct stat root_dir_stat;
    if (lstat(argv[1], &root_dir_stat) == -1)
    {
        print_error(program_name, argv[1], strerror(errno));
        return 1;
    }

    long long block_count = 0;    // Number of blocks.
    long long size = 0;           // Size in bytes.
    ino_t *visited_inodes = NULL; // Array contains inodes of visited files.
    int vst_ind_len = 0;          // Length of the array of visited inodes (the previous line).
    const int BLOCK_SIZE = 512;   // The st_blocks indicates the number of blocks allocated to the file, 512-byte units.

    // Recursively check info about directory (argv[1] - path name).
    calc_dir_size(argv[1], program_name, root_dir_stat, &block_count, &size, visited_inodes, &vst_ind_len, BLOCK_SIZE);

    if (block_count != 0)
    {
        fprintf(stdout, "Size: %lld \nSize on disk: %lld \nDisk usage rate: %.2f%%\n", size,
                block_count * BLOCK_SIZE, ((float) (size * 100) / (float) (block_count * BLOCK_SIZE)));
    }
    else
    {
        fprintf(stdout, "Empty directory.\n");
    }

    return 0;
}

// Count for the given directory (dir_name) and all its subdirectories the total size of the space
// occupied by the files on the disk in bytes and the total size of the files.
// Calculate the disk usage rate in %.
int calc_dir_size(char *dir_name, char *program_name, struct stat root_dir_stat,
                  long long *block_count_pointer, long long *size_pointer,
                  ino_t *visited_inodes, int *vst_ind_len_pointer, const int BLOCK_SIZE)
{
    DIR *dir_pointer = NULL;
    struct dirent *dir_entry;

    dir_pointer = opendir(dir_name);
    if (dir_pointer == NULL)
    {
        print_error(program_name, dir_name, strerror(errno));
        return 1;
    }

    // The readdir() function returns a pointer to a dir_entry representing
    // the next directory entry in the directory stream pointed to by
    // dir_pointer. It returns NULL on reaching the end of the directory stream.
    while ((dir_entry = readdir(dir_pointer)) != NULL)
    {
        // Skip current and parent directories.
        if ((strcmp(dir_entry->d_name, ".") == 0) ||
            (strcmp(dir_entry->d_name, "..") == 0))
        {
            continue;
        }

        // Get full pathname of the entry.
        char *entry_path = malloc(strlen(dir_name) + strlen(dir_entry->d_name) + 2);
        strcpy(entry_path, dir_name);
        strcat(entry_path, "/");
        strcat(entry_path, dir_entry->d_name);


        // Contains main info about the entry.
        struct stat dir_entry_info;
        if (lstat(entry_path, &dir_entry_info))
        {
            print_error(program_name, entry_path, strerror(errno));
        }

        // The file is a directory.
        if (S_ISDIR(dir_entry_info.st_mode))
        {
            long long dir_block_count = 0;    // Number of blocks in the directory.
            long long dir_size = 0;           // Size in bytes of the directory.

            // Recursively calculate for each entry of the directory.
            calc_dir_size(entry_path, program_name, root_dir_stat, &dir_block_count,
                          &dir_size, visited_inodes, vst_ind_len_pointer, BLOCK_SIZE);

            if (dir_block_count != 0)
            {
                (*size_pointer) += dir_size;
                (*block_count_pointer += dir_block_count);
                fprintf(stdout, "%s: Disk usage rate %.2f%%\n", entry_path,
                        ((float) (dir_size * 100) / (float) (dir_block_count * BLOCK_SIZE)));
            }
            else
            {
                fprintf(stdout, "%s: Empty directory\n", entry_path);
            }

            continue;
        }

        // The file is a regular file.
        if (S_ISREG(dir_entry_info.st_mode))
            {
            // Multiple hard links
            if ((int) dir_entry_info.st_nlink > 1)
            {
                // If false, inode has been visited by another link and file info has already been processed.
                bool flag = true;

                // Array contains visited inodes numbers.
                for (int i = 0; i < (*vst_ind_len_pointer); i++)
                {
                    if (visited_inodes[i] == dir_entry_info.st_ino)
                    {
                        // The file has been processed.
                        flag = false;
                    }
                }

                // The file is visited by the first time.
                if (flag)
                {
                    if ((visited_inodes = (ino_t *) realloc(visited_inodes, ((*vst_ind_len_pointer) + 1) * sizeof(ino_t))) == NULL)
                    {
                        print_error(program_name, entry_path, strerror(errno));
                    }
                    else
                    {
                        // Note that the file was processed.
                        visited_inodes[(*vst_ind_len_pointer)] = dir_entry_info.st_ino;
                        (*vst_ind_len_pointer)++;

                        // Process the file.
                        (*size_pointer) += (long long) dir_entry_info.st_size;
                        (*block_count_pointer) += (long long) dir_entry_info.st_blocks;
                    }

                }
            }
            else
            {
                // Process the file.
                (*size_pointer) += (long long) dir_entry_info.st_size;
                (*block_count_pointer) += (long long) dir_entry_info.st_blocks;
            }

            continue;
        }

        // The file is a symbolic link.
        if (S_ISLNK(dir_entry_info.st_mode))
        {
            char link[PATH_MAX + 1];

            if (readlink(entry_path, link, PATH_MAX) == -1)
            {
                print_error(program_name, entry_path, strerror(errno));
            }
            else
            {
                int link_size;
                link_size = (int) strlen(link);

                (*size_pointer) += link_size;

                if (link_size % BLOCK_SIZE != 0)
                {
                    *block_count_pointer += ((link_size / BLOCK_SIZE) + 1);
                }
                else
                {
                    *block_count_pointer += link_size / BLOCK_SIZE;
                }
            }
        }

    }

    if (closedir(dir_pointer) == -1)
    {
        print_error(program_name, dir_name, strerror(errno));
        return 1;
    }

    return 0;
}

void print_error(const char *program_name, const char *directory, const char *error_message)
{
    fprintf(stderr, "%s: %s: %s\n", program_name, directory, error_message);
};