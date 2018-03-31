#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libgen.h>

#define ERR_LOG_PATH "/tmp/err.log"

ino_t *visited_inodes = NULL; // Array contains inodes of visited files.
int vst_ind_len = 0;          // Length of the array of visited inodes (the previous line).

int calc_dir_size(char *dir_name, char *program_name, struct stat root_dir_stat, long long *block_count_pointer,
                   long long *size_pointer, FILE *err_log, const int BLOCK_SIZE);
void save_error_to_log(FILE *err_log, const char *program_name, const char *directory, const char *error_message);
void print_error_log(FILE *err_log);

int main(int argc, char *argv[])
{
    // Used to display error messages.
    char *program_name = basename(argv[0]);

    // Create temporary file to save error messages.
    FILE *err_log = NULL;
    if ((err_log = fopen(ERR_LOG_PATH ,"w+")) == NULL)
    {
        fprintf(stderr, "%s: Unable create error log (%s)\n", program_name, ERR_LOG_PATH);
        return 1;
    }

    // Check input.
    if (argc != 3)
    {
        save_error_to_log(err_log, program_name, "Wrong number of parameters. Usage", "./lab2.exe \"path_name\" \"max_num_of_processes\"");
        print_error_log(err_log);
        return 1;
    }

    // Get stat structure, which contains info about directory, entered by user.
    // If pathname (argv[1]) is a symbolic link, then it returns information
    // about the link itself, not the file that it refers to.
    struct stat root_dir_stat;
    if (lstat(argv[1], &root_dir_stat) == -1)
    {
        save_error_to_log(err_log, program_name, argv[1], strerror(errno));
        print_error_log(err_log);
        return 1;
    }

    long long block_count = 0;    // Number of blocks.
    long long size = 0;           // Size in bytes.
    const int BLOCK_SIZE = 512;   // The st_blocks indicates the number of blocks allocated to the file, 512-byte units.

    // Get absolete path name of the directory.
    char max_path_len[PATH_MAX + 1];
    char *path;
    path = realpath(argv[1], max_path_len);

    // Recursively check info about the directory.
    calc_dir_size(path, program_name, root_dir_stat, &block_count, &size, err_log, BLOCK_SIZE);

    if (block_count != 0)
    {
        fprintf(stdout, "Size: %lld \nSize on disk: %lld \nDisk usage rate: %.2f%%\n", size,
                block_count * BLOCK_SIZE, ((float) (size * 100) / (float) (block_count * BLOCK_SIZE)));
    }
    else
    {
        fprintf(stdout, "Empty directory.\n");
    }

    // Print error messages from err_log to stderr.
    print_error_log(err_log);

    return 0;
}

// Count for the given directory (dir_name) and all its subdirectories the total size of the space
// occupied by the files on the disk in bytes and the total size of the files.
// Calculate the disk usage rate in %.
int calc_dir_size(char *dir_name, char *program_name, struct stat root_dir_stat,
                  long long *block_count_pointer, long long *size_pointer, FILE *err_log, const int BLOCK_SIZE)
{
    DIR *dir_pointer = NULL;
    struct dirent *dir_entry;

    dir_pointer = opendir(dir_name);
    if (dir_pointer == NULL)
    {
        save_error_to_log(err_log, program_name, dir_name, strerror(errno));
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
        if (entry_path[strlen(dir_name) - 1] != '/') {
            strcat(entry_path, "/"); }
        strcat(entry_path, dir_entry->d_name);

        // Contains main info about the entry.
        struct stat dir_entry_info;
        if (lstat(entry_path, &dir_entry_info))
        {
            save_error_to_log(err_log, program_name, entry_path, strerror(errno));
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
                        save_error_to_log(err_log, program_name, entry_path, strerror(errno));
                    }
                    else
                    {
                        // Note that the file was processed.
                        visited_inodes[vst_ind_len] = dir_entry_info.st_ino;
                        vst_ind_len++;

                        long long dir_block_count = 0;    // Number of blocks in the directory.
                        long long dir_size = 0;           // Size in bytes of the directory.

                        // Recursively calculate for each entry of the directory.
                        calc_dir_size(entry_path, program_name, root_dir_stat, &dir_block_count,
                                      &dir_size, err_log, BLOCK_SIZE);

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
                    }

                }
            }
            else
            {
                long long dir_block_count = 0;    // Number of blocks in the directory.
                long long dir_size = 0;           // Size in bytes of the directory.

                // Recursively calculate for each entry of the directory.
                calc_dir_size(entry_path, program_name, root_dir_stat, &dir_block_count,
                              &dir_size, err_log, BLOCK_SIZE);

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
            }

            continue;
        }

        // The file is a regular file.
        if (S_ISREG(dir_entry_info.st_mode) && (dir_entry_info.st_dev == root_dir_stat.st_dev))
            {
            // Multiple hard links
            if ((int) dir_entry_info.st_nlink > 1)
            {
                // If false, inode has been visited by another link and file info has already been processed.
                bool flag = true;

                // Array contains visited inodes numbers.
                int i = 0;
                for (i = 0; i < vst_ind_len; i++)
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
                    if ((visited_inodes = (ino_t *) realloc(visited_inodes, (vst_ind_len + 1) * sizeof(ino_t))) == NULL)
                    {
                        save_error_to_log(err_log, program_name, entry_path, strerror(errno));
                    }
                    else
                    {
                        // Note that the file was processed.
                        visited_inodes[vst_ind_len] = dir_entry_info.st_ino;
                        vst_ind_len++;

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
        if (S_ISLNK(dir_entry_info.st_mode) && (dir_entry_info.st_dev == root_dir_stat.st_dev))
        {
            char link[PATH_MAX + 1];

            if (readlink(entry_path, link, PATH_MAX) == -1)
            {
                save_error_to_log(err_log, program_name, entry_path, strerror(errno));
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
        save_error_to_log(err_log, program_name, dir_name, strerror(errno));
        return 1;
    }

    return 0;
}

// Print error message to temporary file err_log. 
void save_error_to_log(FILE *err_log, const char *program_name, const char *directory, const char *error_message)
{
    fprintf(err_log, "%s: %s: %s\n", program_name, directory, error_message);
};

// Print all error messages to stream stderr from temporary file err_log and remove the file.
void print_error_log(FILE *err_log)
{
    fseek(err_log, 0, SEEK_SET);

    int ch = fgetc(err_log);
    while (ch != EOF)
    {
        fputc(ch, stderr);
        ch = fgetc(err_log);
    }

    fclose(err_log);

    remove(ERR_LOG_PATH);
}