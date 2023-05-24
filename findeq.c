#include "findeq.h"

#define DEFAULT_MIN_SIZE 1024

// Global variables
pthread_mutex_t lock;
pthread_mutex_t lock_n_threads;
pthread_mutex_t subtasks_lock;

int num_of_threads = 0;
int size_of_file = DEFAULT_MIN_SIZE;
char *output_file = NULL;

/* cleans up the program, called from signal handler */
void end_program()
{
    /* if -o output file option is on, write file */
    if (output_file != NULL) {
        FILE *fp;
    }
}

/* handles signals */
void handle_signal(int sig)
{
	// if interrupt signal, clean up & end program
	if (sig == SIGINT)
    {
        fprintf(stdout, "\nProgram quitting due to interrupt..");
    }

	end_program();
	exit(1);
}

void process_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "process_file() : Unable to open file: %s\n", path);

        return;
    }

    /* get the file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *buffer = malloc(file_size);
    if (!buffer) {
        fprintf(stderr, "process_file() : Memory allocation failed for file: %s\n", path);

        fclose(file);
        free(buffer);

        return;
    }

    size_t result = fread(buffer, 1, file_size, file);
    if (result != file_size) {
        fprintf(stderr, "process_file() : Error reading file: %s\n", path);

        fclose(file);
        free(buffer);

        return;
    }

    fclose(file);
    free(buffer);

    return;
}

void process_subdirectory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "process_subdirectory() : Unable to open directory: %s\n", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char *entry_path = (char *) malloc(10000);
        printf("%s\n", path);
        snprintf(entry_path, sizeof(entry_path), "%s/%s", path, entry->d_name);

        DIR *subdir = opendir(entry_path);
        if (subdir) { // if it is a sub directory
            closedir(subdir);

            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // if a new directory exists
                //
            }
        } else { // if it is a file
            FILE *file = fopen(entry_path, "rb");

            if (file) {
                fclose(file);

                process_file(entry_path);
            }
        }
    }

    closedir(dir);
}

void *worker(void *arg)
{


    pthread_mutex_lock(&lock_n_threads);
    num_of_threads--;
    pthread_mutex_unlock(&lock_n_threads);

    return NULL;
}

void main_thread(char *dir_path)
{
    process_subdirectory(dir_path);

    return;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [OPTION] DIR\n", argv[0]);
        return 0;
    }

    char *dir_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-t=", 3) == 0) {
            num_of_threads = atoi(argv[i] + 3);

            if (num_of_threads < 0 || num_of_threads > 64) {
                fprintf(stderr, "Invalid number of threads. It should be between 0 and 64.\n");
                return 0;
            }
        } else if (strncmp(argv[i], "-m=", 3) == 0) {
            size_of_file = atoi(argv[i] + 3);

            if (size_of_file < 0) {
                fprintf(stderr,     "Invalid file size. It should be a non-negative integer.\n");
                return 0;
            }
        } else if (strncmp(argv[i], "-o=", 3) == 0) {
            output_file = argv[i] + 3;
        } else {
            dir_path = argv[i];
        }
    }

    pthread_t threads[num_of_threads];

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&lock_n_threads, NULL);
    pthread_mutex_init(&subtasks_lock, NULL);

    for (int i = 0; i < num_of_threads; i++)
    {
        pthread_create(&(threads[i]), NULL, worker, NULL);
    }

    // main thread work starting point
    main_thread(dir_path);

    for (int i = 0; i < num_of_threads; i++)
    {
        pthread_mutex_lock(&lock_n_threads);
            pthread_join(threads[i], NULL);
        pthread_mutex_unlock(&lock_n_threads);
    }

    return 0;
}
