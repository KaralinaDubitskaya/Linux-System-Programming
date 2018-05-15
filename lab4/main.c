#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>
#include <malloc.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_SIGNAL_COUNT 101

void proc0_handler(int sig);
void proc1_handler(int sig);
void proc2_handler(int sig);
void proc3_handler(int sig);
void proc4_handler(int sig);
void proc5_handler(int sig);
void proc6_handler(int sig);
void proc7_handler(int sig);
void proc8_handler(int sig);


void save_pid_to_file(char *filename, int pid);
int get_pid_from_file(char *filename);
long get_time();
void print_action_details(uint proc_num, pid_t pid, pid_t ppid, const char *event, long time);
void print_error(int pid, const char *error_message);

int sig_count = 0;
int sigusr1_count = 0, sigusr2_count = 0;

// The basename of the program
char *program_name;

// Number of processes at the moment
char proc_count;

// Used to change signals handlers
struct sigaction sig_action;

int main(int argc, char *argv[]) {
    // Used to display error messages.
    program_name = basename(argv[0]);
    proc_count = 1;

    // Main process pid
    pid_t proc0_pid = getpid();

    sig_action.sa_handler = &proc0_handler;
    if (sigaction(SIGUSR1, &sig_action, 0) == -1) {
        print_error(proc0_pid, strerror(errno));
        proc0_pid = -1;
        exit(1);
    }

    int pid;
    switch (pid = fork()) {
        case -1:   // Error
        {
            print_error(getpid(), strerror(errno));
            exit(1);
        }
        case 0:    // 1 process (child)
        {
            // Create file with the first process pid
            save_pid_to_file("proc1_pid.txt", getpid());

            // Set handlers
            sig_action.sa_handler = &proc1_handler;
            if (sigaction(SIGUSR1, &sig_action, 0) == -1) {
                print_error(getpid(), strerror(errno));
                exit(1);
            }
            if (sigaction(SIGUSR2, &sig_action, 0) == -1) {
                print_error(getpid(), strerror(errno));
                exit(1);
            }

            // Notify the main process
            if (kill(proc0_pid, SIGUSR1) == -1) {
                print_error(getpid(), strerror(errno));
                exit(1);
            }

            switch (pid = fork()) {
                case -1:   // Error
                {
                    print_error(getpid(), strerror(errno));
                    exit(1);
                }
                case 0:    // 2 process (child)
                {
                    // Create file with the second process pid
                    save_pid_to_file("proc2_pid.txt", getpid());

                    // Set handlers
                    sig_action.sa_handler = &proc2_handler;
                    if (sigaction(SIGUSR2, &sig_action, 0) == -1) {
                        print_error(getpid(), strerror(errno));
                        exit(1);
                    }
                    if (sigaction(SIGTERM, &sig_action, 0) == -1) {
                        print_error(getpid(), strerror(errno));
                        exit(1);
                    }

                    // Set process group
                    if (setpgid(getpid(), getpid()) == -1) {
                        print_error(getpid(), strerror(errno));
                        exit(1);
                    }

                    // Notify the main process
                    if (kill(proc0_pid, SIGUSR1) == -1) {
                        print_error(getpid(), strerror(errno));
                        exit(1);
                    }

                    // Just wait
                    while (1) {
                        pause();
                    }
                }
                default:   // 1 process (parent)
                {
                    switch (pid = fork()) {
                        case -1:    // Error
                        {
                            print_error(getpid(), strerror(errno));
                            exit(1);
                        }
                        case 0:    // 3 process (child)
                        {
                            // Create file with the third process pid
                            save_pid_to_file("proc3_pid.txt", getpid());

                            // Set handlers
                            sig_action.sa_handler = &proc3_handler;
                            if (sigaction(SIGUSR2, &sig_action, 0) == -1) {
                                print_error(getpid(), strerror(errno));
                                exit(1);
                            }
                            if (sigaction(SIGTERM, &sig_action, 0) == -1) {
                                print_error(getpid(), strerror(errno));
                                exit(1);
                            }

                            // Set process group
                            if (setpgid(getpid(), get_pid_from_file("proc2_pid.txt")) == -1) {
                                print_error(getpid(), strerror(errno));
                                exit(1);
                            }

                            // Notify the main process
                            if (kill(proc0_pid, SIGUSR1) == -1) {
                                print_error(getpid(), strerror(errno));
                                exit(1);
                            }

                            switch (pid = fork()) {
                                case -1:   // Error
                                {
                                    print_error(getpid(), strerror(errno));
                                    exit(1);
                                }
                                case 0:    // 4 process (child)
                                {
                                    // Create file with the fourth process pid
                                    save_pid_to_file("proc4_pid.txt", getpid());

                                    // Set handlers
                                    sig_action.sa_handler = &proc4_handler;
                                    if (sigaction(SIGUSR2, &sig_action, 0) == -1) {
                                        print_error(getpid(), strerror(errno));
                                        exit(1);
                                    }
                                    if (sigaction(SIGTERM, &sig_action, 0) == -1) {
                                        print_error(getpid(), strerror(errno));
                                        exit(1);
                                    }

                                    // Set process group
                                    if (setpgid(getpid(),  get_pid_from_file("proc2_pid.txt")) == -1) {
                                        print_error(getpid(), strerror(errno));
                                        exit(1);
                                    }

                                    // Notify the main process
                                    if (kill(proc0_pid, SIGUSR1) == -1) {
                                        print_error(getpid(), strerror(errno));
                                        exit(1);
                                    }

                                    switch (pid = fork()) {
                                        case -1:   // Error
                                        {
                                            print_error(getpid(), strerror(errno));
                                            exit(1);
                                        }
                                        case 0:   // 5 process (child)
                                        {
                                            // Create file with the fifth process pid
                                            save_pid_to_file("proc5_pid.txt", getpid());

                                            // Set handlers
                                            sig_action.sa_handler = &proc5_handler;
                                            if (sigaction(SIGUSR1, &sig_action, 0) == -1) {
                                                print_error(getpid(), strerror(errno));
                                                exit(1);
                                            }
                                            if (sigaction(SIGTERM, &sig_action, 0) == -1) {
                                                print_error(getpid(), strerror(errno));
                                                exit(1);
                                            }

                                            // Set process group
                                            if (setpgid(getpid(), getpid()) == -1) {
                                                print_error(getpid(), strerror(errno));
                                                exit(1);
                                            }

                                            // Notify the main process
                                            if (kill(proc0_pid, SIGUSR1) == -1) {
                                                print_error(getpid(), strerror(errno));
                                                exit(1);
                                            }

                                            // Just wait
                                            while (1) {
                                                pause();
                                            }
                                        }
                                        default:   // 4 process (parent)
                                        {
                                            switch (pid = fork()) {
                                                case -1:   // Error
                                                {
                                                    print_error(getpid(), strerror(errno));
                                                    exit(1);
                                                }
                                                case 0:    // 6 process (child)
                                                {
                                                    // Create file with the sixth process pid
                                                    save_pid_to_file("proc6_pid.txt", getpid());

                                                    // Set handlers
                                                    sig_action.sa_handler = &proc6_handler;
                                                    if (sigaction(SIGUSR1, &sig_action, 0) == -1) {
                                                        print_error(getpid(), strerror(errno));
                                                        exit(1);
                                                    }
                                                    if (sigaction(SIGTERM, &sig_action, 0) == -1) {
                                                        print_error(getpid(), strerror(errno));
                                                        exit(1);
                                                    }

                                                    // Set process group
                                                    if (setpgid(getpid(), get_pid_from_file("proc5_pid.txt")) == -1) {
                                                        print_error(getpid(), strerror(errno));
                                                        exit(1);
                                                    }

                                                    // Notify the main process
                                                    if (kill(proc0_pid, SIGUSR1) == -1) {
                                                        print_error(getpid(), strerror(errno));
                                                        exit(1);
                                                    }

                                                    switch (pid = fork()) {
                                                        case -1:   // Error
                                                        {
                                                            print_error(getpid(), strerror(errno));
                                                            exit(1);
                                                        }
                                                        case 0:    // 7 process (child)
                                                        {
                                                            // Create file with the seventh process pid
                                                            save_pid_to_file("proc7_pid.txt", getpid());

                                                            // Set handlers
                                                            sig_action.sa_handler = &proc7_handler;
                                                            if (sigaction(SIGUSR1, &sig_action, 0) == -1) {
                                                                print_error(getpid(), strerror(errno));
                                                                exit(1);
                                                            }
                                                            if (sigaction(SIGTERM, &sig_action, 0) == -1) {
                                                                print_error(getpid(), strerror(errno));
                                                                exit(1);
                                                            }

                                                            // Set process group
                                                            if (setpgid(getpid(), getpid()) == -1) {
                                                                print_error(getpid(), strerror(errno));
                                                                exit(1);
                                                            }

                                                            // Notify the main process
                                                            if (kill(proc0_pid, SIGUSR1) == -1) {
                                                                print_error(getpid(), strerror(errno));
                                                                exit(1);
                                                            }

                                                            switch (pid = fork()) {
                                                                case -1:   // Error
                                                                {
                                                                    print_error(getpid(), strerror(errno));
                                                                    exit(1);
                                                                }
                                                                case 0:    // 8 process (child)
                                                                {
                                                                    // Create file with the eighth process pid
                                                                    save_pid_to_file("proc8_pid.txt", getpid());

                                                                    // Set handlers
                                                                    sig_action.sa_handler = &proc8_handler;
                                                                    if (sigaction(SIGUSR1, &sig_action, 0) == -1) {
                                                                        print_error(getpid(), strerror(errno));
                                                                        exit(1);
                                                                    }
                                                                    if (sigaction(SIGTERM, &sig_action, 0) == -1) {
                                                                        print_error(getpid(), strerror(errno));
                                                                        exit(1);
                                                                    }

                                                                    // Set process group
                                                                    if (setpgid(getpid(), get_pid_from_file("proc7_pid.txt")) == -1) {
                                                                        print_error(getpid(), strerror(errno));
                                                                        exit(1);
                                                                    }

                                                                    // Notify the main process
                                                                    if (kill(proc0_pid, SIGUSR1) == -1) {
                                                                        print_error(getpid(), strerror(errno));
                                                                        exit(1);
                                                                    }

                                                                    // Just wait
                                                                    while (1) {
                                                                        pause();
                                                                    }
                                                                }
                                                                default:   // 7 process (parent)
                                                                {
                                                                    while (1) {
                                                                        pause();
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        default:   // 6 process (parent)
                                                        {
                                                            // Just wait
                                                            while (1) {
                                                                pause();
                                                            }
                                                        }
                                                    }
                                                }
                                                default:   // 4 process (parent)
                                                {
                                                    // Just wait
                                                    while (1) {
                                                        pause();
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                default:   // 3 process (parent)
                                {
                                    // Just wait
                                    while (1) {
                                        pause();
                                    }
                                }
                            }
                        }
                        default:   // 1 process (parent)
                        {
                            // Just wait
                            while (1) {
                                pause();
                            }
                        }
                    }
                }
            }
        }
        default:    // 0 process (main)
        {
            while (proc_count != 8) {
                pause(); // wait for signal
            }

	    char buffer[15];
	    sprintf(buffer, "pstree %d", getpid());
	    system(buffer);

            if (kill(get_pid_from_file("proc1_pid.txt"), SIGUSR1) == -1) {
                print_error(getpid(), strerror(errno));
                exit(1);
            }

            while (wait(NULL) != -1) {}
        }
    }
}

// Print  error message to stream stderr
void print_error(const int pid, const char *error_message)
{
    fprintf(stderr, "%i: %s: %s\n", pid, program_name, error_message);
}

// Save pid of the process to the file
void save_pid_to_file(char *filename, int pid)
{
    FILE *fp; // File pointer

    // Get file path
    char *path = malloc(strlen("/tmp/osisp/") + strlen(filename) + 1);
    strcpy(path, "/tmp/osisp/");
    strcat(path, filename);

    if ((fp = fopen(path,"w")))
    {
        fprintf (fp, "%d", pid);
        fflush(fp);
        fclose(fp);
    }
    else
    {
        // Error
        char *err_msg = malloc(strlen("Cannot create file ") + strlen(filename));
        strcpy(path, "Cannot create file ");
        strcat(path, filename);
        print_error(pid, err_msg);
        exit(1);
    }

}

int get_pid_from_file(char *filename)
{
    FILE *file_ptr;
    int pid;

    // Get file path
    char *path = malloc(strlen("/tmp/osisp/") + strlen(filename) + 1);
    strcpy(path, "/tmp/osisp/");
    strcat(path, filename);

    if (!(file_ptr = fopen(path,"r")))
    {
        print_error(getpid(), strerror(errno));
        exit(1);
    }

    fscanf(file_ptr, "%d", &pid);
    fclose(file_ptr);
    return pid;
}

// Return microseconds / 1000
long get_time()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_usec / 1000;
}

void print_action_details(uint proc_num, pid_t pid, pid_t ppid, const char *event, long time)
{
    printf ("%d %d %d %s %lu\n", proc_num, pid, ppid, event, time);
    fflush(stdout);
}

void proc0_handler(int sig)
{
    proc_count++;
}

void proc1_handler(int sig)
{
    if (sig == SIGUSR1)
    {
        // Start sending signals
        print_action_details(1, getpid(), getppid(), "послал USR1", get_time());
        if (kill(-get_pid_from_file("proc7_pid.txt"), SIGUSR1) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        sigusr1_count++;
    }

    if (sig == SIGUSR2) {
        print_action_details(1, getpid(), getppid(), "получил USR2", get_time());
        sig_count++;
        if (sig_count == MAX_SIGNAL_COUNT) {
            // Send SIGTERM to process 2
            if (kill(get_pid_from_file("proc2_pid.txt"), SIGTERM) == -1) {
                print_error(getpid(), strerror(errno));
                exit(1);
            }
            // Send SIGTERM to process 3
            if (kill(get_pid_from_file("proc3_pid.txt"), SIGTERM) == -1) {
                print_error(getpid(), strerror(errno));
                exit(1);
            }

            // Wait for processes 2 and 3 to complete
            while (wait(NULL) != -1) {}

            // Remove temp file with process pid
            if (remove("/tmp/osisp/proc1_pid.txt") == -1) {
                print_error(getpid(), "3");
                exit(1);
            }

            exit(0);
        }
        else
        {
            // Send SIGUSR1 to processes 7 and 8
            if (kill(-get_pid_from_file("proc7_pid.txt"), SIGUSR1) == -1) {
                print_error(getpid(), strerror(errno));
                exit(1);
            }
            print_action_details(1, getpid(), getppid(), "послал USR1", get_time());
            sigusr1_count++;
        }
    }
}

void proc2_handler(int sig)
{
    if (sig == SIGTERM)
    {
        printf("2 %d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(),
               getppid(), sigusr1_count, sigusr2_count);
        fflush(stdout);

        // Remove temp file with process pid
        if (remove("/tmp/osisp/proc2_pid.txt") == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }
        exit(0);
    }

    if (sig == SIGUSR2)
    {
        print_action_details(2, getpid(), getppid(), "получил USR2", get_time());

        // Send SIGUSR2 to process 1
        if (kill(get_pid_from_file("proc1_pid.txt"), SIGUSR2) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        print_action_details(2, getpid(), getppid(), "послал USR2", get_time());
        sigusr2_count++;
    }
}

void proc3_handler(int sig)
{
    if (sig == SIGTERM)
    {
        // Send SIGTERM to process 4
        if (kill(get_pid_from_file("proc4_pid.txt"), SIGTERM) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        // Wait for process 4 to complete
        while (wait(NULL) != -1)
        {  }

        printf("3 %d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(),
               getppid(), sigusr1_count, sigusr2_count);
        fflush(stdout);

        // Remove temp file with process pid
        if (remove("/tmp/osisp/proc3_pid.txt") == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        exit(0);
    }

    if (sig == SIGUSR2)
    {
        print_action_details(3, getpid(), getppid(), "получил USR2", get_time());
    }
}

void proc4_handler(int sig)
{
    if (sig == SIGTERM)
    {
        // Send SIGTERM to process 5
        if (kill(get_pid_from_file("proc5_pid.txt"), SIGTERM) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        // Send SIGTERM to process 6
        if (kill(get_pid_from_file("proc6_pid.txt"), SIGTERM) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        // Wait for processes 5 and 6 to complete
        while (wait(NULL) != -1) {  }

        printf("4 %d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(),
               getppid(), sigusr1_count, sigusr2_count);
        fflush(stdout);

        // Remove temp file with process pid
        if (remove("/tmp/osisp/proc4_pid.txt") == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        exit(0);
    }

    if (sig == SIGUSR2)
    {
        print_action_details(4, getpid(), getppid(), "получил USR2", get_time());
    }
}

void proc5_handler(int sig)
{
    if (sig == SIGTERM)
    {
        printf("5 %d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(),
               getppid(), sigusr1_count, sigusr2_count);
        fflush(stdout);

        // Remove temp file with process pid
        if (remove("/tmp/osisp/proc5_pid.txt") == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }
        exit(0);
    }

    if (sig == SIGUSR1)
    {
        print_action_details(5, getpid(), getppid(), "получил USR1", get_time());

        // Send SIGUSR2 to processes 2, 3 and 4
        print_action_details(5, getpid(), getppid(), "послал USR2", get_time());
        if (kill(-get_pid_from_file("proc2_pid.txt"), SIGUSR2) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }
        sigusr2_count++;
    }
}

void proc6_handler(int sig)
{
    if (sig == SIGTERM)
    {
        // Send SIGTERM to process 7
        if (kill(get_pid_from_file("proc7_pid.txt"), SIGTERM) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        // Wait for process 7 to complete
        while (wait(NULL) != -1) {  }

        printf("6 %d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(),
               getppid(), sigusr1_count, sigusr2_count);
        fflush(stdout);

        // Remove temp file with process pid
        if (remove("/tmp/osisp/proc6_pid.txt") == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        exit(0);
    }

    if (sig == SIGUSR1)
    {
        print_action_details(6, getpid(), getppid(), "получил USR1", get_time());
    }
}

void proc7_handler(int sig)
{
    if (sig == SIGTERM)
    {
        // Send SIGTERM to process 8
        if (kill(get_pid_from_file("proc8_pid.txt"), SIGTERM) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        // Wait for process 8 to complete
        while (wait(NULL) != -1) {  }

        printf("7 %d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(),
               getppid(), sigusr1_count, sigusr2_count);
        fflush(stdout);

        // Remove temp file with process pid
        if (remove("/tmp/osisp/proc7_pid.txt") == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        exit(0);
    }

    if (sig == SIGUSR1)
    {
        print_action_details(7, getpid(), getppid(), "получил USR1", get_time());
    }
}

void proc8_handler(int sig)
{
    if (sig == SIGTERM)
    {
        printf("8 %d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(),
               getppid(), sigusr1_count, sigusr2_count);
        fflush(stdout);

        // Remove temp file with process pid
        if (remove("/tmp/osisp/proc8_pid.txt") == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        exit(0);
    }

    if (sig == SIGUSR1)
    {
        print_action_details(8, getpid(), getppid(), "получил USR1", get_time());

        // Send SIGUSR1 to processes 5 and 6
        if (kill(-get_pid_from_file("proc5_pid.txt"), SIGUSR1) == -1) {
            print_error(getpid(), strerror(errno));
            exit(1);
        }

        print_action_details(8, getpid(), getppid(), "послал USR1", get_time());
        sigusr1_count++;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//long long get_time()
//{
//    struct timeval time;
//    gettimeofday(&time, NULL);
//    return time.tv_usec / 1000;
//}
//
//void process_signal_event(int N, pid_t pid, pid_t ppid, char *event, time_t time)
//{
//    printf ("%d %d %d %s %llu\n",N, getpid(),getppid(), event, get_time());
//    fflush(stdout);
//}
//
//void print_error(pid_t procpid)
//{
//    char *buf = (char*) malloc(sizeof(char) * (strlen(progname) + 9));
//    sprintf(buf, "%d %s", procpid, progname);
//    perror(buf);
//}

//int get_pid_from_file(char *filename)
//{
//    FILE *file_ptr;
//    char *buf;
//    int pid;
//    if (!(file_ptr=fopen(filename,"r")))
//    {
//        char *buf = (char*) malloc(sizeof(char) * (strlen(progname) + strlen(filename) + 12));
//        sprintf(buf, "%d %s %s", getpid(), progname, filename);
//        perror(buf);
//        exit(1);
//    }
//
//    fscanf (file_ptr,"%d",&pid);
//    fclose(file_ptr);
//    return pid;
//}
//
//int file_exists(char *filename)
//{
//    FILE *file_ptr;
//    if (!(file_ptr=fopen(filename,"r")))
//    {
//        return 0;
//    }
//    fclose(file_ptr);
//    return 1;
//}
//
//
//void proc1_handler (int sig)
//{
//    if (sig == SIGUSR1)
//    {
//        process_signal_event(1, getpid(), getppid(), "sent SIGUSR1", time(NULL));
//        if (kill(-get_pid_from_file("p7.txt"), SIGUSR1) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//
//        sigusr1_count++;
//    }
//    else
//    {
//        process_signal_event(1, getpid(), getppid(), "got SIGUSR2", time(NULL));
//        total_signal_count++;
//        fflush(stdout);
//        if (total_signal_count == MAX_SIGNAL_COUNT)
//        {
//            if (kill(get_pid_from_file("p2.txt"), SIGTERM) == -1) {
//                print_error(getpid());
//                exit(1);
//            }
//            while (wait(NULL) != -1)
//            {  }
//            if (remove("p1.txt") == -1) {
//                print_error(getpid());
//                exit(1);
//            }
//            exit(0);
//        }
//        else
//        {
//
//            if (kill(-get_pid_from_file("p7.txt"), SIGUSR1) == -1) {
//                print_error(getpid());
//                exit(1);
//            }
//            process_signal_event(1, getpid(), getppid(), "sent SIGUSR1", time(NULL));
//            sigusr1_count++;
//        }
//    }
//
//
//}
//
//void proc2_handler (int sig)
//{
//    if (sig == SIGTERM)
//    {
//        if (kill(get_pid_from_file("p3.txt"), SIGTERM) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        if (kill(get_pid_from_file("p4.txt"), SIGTERM) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        while (wait(NULL) != -1)
//        {  }
//        printf ("2 %d %d finished after %d SIGUSR1 and %d SIGUSR2 %llu\n",getpid(),
//                getppid(), sigusr1_count, sigusr2_count, get_time());
//        fflush(stdout);
//        if (remove("p2.txt") == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        exit(0);
//    }
//    else
//    {
//
//        process_signal_event(2, getpid(), getppid(), "got SIGUSR2", time(NULL));
//
//
//        if (kill(get_pid_from_file("p1.txt"),SIGUSR2) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        process_signal_event(2, getpid(), getppid(), "sent SIGUSR2", time(NULL));
//        sigusr2_count++;
//
//
//    }
//}
//
//void proc3_handler (int sig)
//{
//    if (sig == SIGTERM)
//    {
//        if (kill(get_pid_from_file("p6.txt"), SIGTERM) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        while (wait(NULL) != -1)
//        {  }
//        printf ("3 %d %d finished after %d SIGUSR1 and %d SIGUSR2 %llu\n",getpid(),
//                getppid(), sigusr1_count, sigusr2_count, get_time());
//        fflush(stdout);
//        if (remove("p3.txt") == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        exit(0);
//    }
//    else
//    {
//        process_signal_event(3, getpid(), getppid(), "got SIGUSR2", time(NULL));
//    }
//
//}
//
//void proc4_handler (int sig)
//{
//    if (sig == SIGTERM)
//    {
//        if (kill(get_pid_from_file("p5.txt"), SIGTERM) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        while (wait(NULL) != -1)
//        {  }
//        printf ("4 %d %d finished after %d SIGUSR1 and %d SIGUSR2 %llu\n",getpid(),
//                getppid(), sigusr1_count, sigusr2_count, get_time());
//        fflush(stdout);
//        if (remove("p4.txt") == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        exit(0);
//    }
//    else
//    {
//        process_signal_event(4, getpid(), getppid(), "got SIGUSR2", time(NULL));
//    }
//
//}
//
//void proc5_handler (int sig)
//{
//    if (sig == SIGTERM)
//    {
//        while (wait(NULL) != -1)
//        {  }
//        printf ("5 %d %d finished after %d SIGUSR1 and %d SIGUSR2 %llu\n",getpid(),
//                getppid(), sigusr1_count, sigusr2_count, get_time());
//        fflush(stdout);
//        if (remove("p5.txt") == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        exit(0);
//    }
//    else
//    {
//        process_signal_event(5, getpid(), getppid(), "got SIGUSR1", time(NULL));
//
//
//        if (kill(-get_pid_from_file("p2.txt"), SIGUSR2) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//
//        process_signal_event(5, getpid(), getppid(), "sent SIGUSR2", time(NULL));
//        sigusr2_count++;
//    }
//}
//
//void proc6_handler (int sig)
//{
//    if (sig == SIGTERM)
//    {
//        if (kill(get_pid_from_file("p7.txt"), SIGTERM) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        while (wait(NULL) != -1)
//        {  }
//        printf ("6 %d %d finished after %d SIGUSR1 and %d SIGUSR2 %llu\n",getpid(),
//                getppid(), sigusr1_count, sigusr2_count, get_time());
//        fflush(stdout);
//        if (remove("p6.txt") == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        exit(0);
//    }
//    else
//    {
//        process_signal_event(6, getpid(), getppid(), "got SIGUSR1", time(NULL));
//    }
//
//}
//
//void proc7_handler (int sig)
//{
//    if (sig == SIGTERM)
//    {
//        if (kill(get_pid_from_file("p8.txt"), SIGTERM) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        while (wait(NULL) != -1)
//        {  }
//        printf ("7 %d %d finished after %d SIGUSR1 and %d SIGUSR2 %llu\n",getpid(),
//                getppid(), sigusr1_count, sigusr2_count, get_time());
//        fflush(stdout);
//        if (remove("p7.txt") == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        exit(0);
//    }
//    else
//    {
//        process_signal_event(7, getpid(), getppid(), "got SIGUSR1", time(NULL));
//    }
//
//}
//
//void proc8_handler (int sig)
//{
//    if (sig == SIGTERM)
//    {
//        while (wait(NULL) != -1)
//        {  }
//        printf ("8 %d %d finished after %d SIGUSR1 and %d SIGUSR2 %llu\n",getpid(),
//                getppid(), sigusr1_count, sigusr2_count, get_time());
//        fflush(stdout);
//        if (remove("p8.txt") == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//        exit(0);
//    }
//    else
//    {
//        process_signal_event(8, getpid(), getppid(), "got SIGUSR1", time(NULL));
//
//
//        if (kill(-get_pid_from_file("p5.txt"), SIGUSR1) == -1) {
//            print_error(getpid());
//            exit(1);
//        }
//
//        process_signal_event(8, getpid(), getppid(), "sent SIGUSR1", time(NULL));
//        sigusr1_count++;
//    }
//}
