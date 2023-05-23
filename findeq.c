#include "findeq.h"

#define MAX_PATH_LENGTH 256

pthread_mutex_t lock;
pthread_mutex_t lock_n_threads;
pthread_mutex_t subtasks_lock;

sem_t sem_tasks;
int num_of_threads;
int threshold_file_size;
char *output_file;
int output_given;



/* cleans up the program, called from signal handler */
void end_program()
{
    /* if -o output file option is on, write file */
    if (output_file == NULL) {
        return;
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

    /* end_program() failed to end successfully */
    fprintf(stderr, "end_program() : Program end error\n");

	exit(1);
}

void process_subdirectory(const char *path) {
    DIR *dir = opendir(path);

    /* assign each subdirectory to a thread */
    

    closedir(dir);

    return ;
}

void process_file(const char *path) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "process_file() : Unable to open file: %s\n", path);
        return;
    }

    /* get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    /* if file size is less than the threshold, ignore */
    if (file_size < threshold_file_size) {
        fclose(file);
        return;
    }

    char *buffer = (char *)malloc(file_size);

    size_t result = fread(buffer, file_size, 1, file);

    if (result != 1) {
        fprintf(stderr, "process_file() : Error reading file: %s\n", path);

        fclose(file);
        free(buffer);

        return;
    }

    fclose(file);
    free(buffer);

    return;
}

void *worker(void *arg) {
    char *subdirectory;

    /* job for each thread */


    pthread_mutex_lock(&lock_n_threads);
        num_of_threads--;
    pthread_mutex_unlock(&lock_n_threads);

    return NULL;
}

void main_thread(char* dir_path) {
    char *subdirectory = malloc(strlen(dir_path) + 1);
    strcpy(subdirectory, dir_path);

    process_subdirectory(subdirectory);

    free(subdirectory);

    return;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [OPTION] DIR\n", argv[0]);
        return 0;
    }

    char *dir_path = NULL;
    num_of_threads = 0;
    threshold_file_size = 1024;
    output_file = NULL;
    output_given = 0;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-t=", 3) == 0) {
            num_of_threads = atoi(argv[i] + 3);

            if (num_of_threads < 0 || num_of_threads > 64) {
                fprintf(stderr, "main() : Invalid number of threads. It should be between 0 and 64.\n");
                exit(1);
            }
        } else if (strncmp(argv[i], "-m=", 3) == 0) {
            threshold_file_size = atoi(argv[i] + 3);
            if (threshold_file_size < 0) {
                fprintf(stderr, "main() : Invalid file size. It should be a non-negative integer.\n");
                exit(1);
            }
        } else if (strncmp(argv[i], "-o=", 3) == 0) {
            output_file = argv[i] + 3;
            output_given = 1;
        } else {
            dir_path = argv[i];
        }
    }

    pthread_t threads[num_of_threads];

    sem_init(&sem_tasks, 0, 0); // Initialize the semaphore

    for (int i = 0; i < num_of_threads; i++) {
        pthread_create(&(threads[i]), NULL, worker, NULL);
    }

    main_thread(dir_path);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_mutex_lock(&lock_n_threads);
        pthread_join(threads[i], NULL);
        pthread_mutex_unlock(&lock_n_threads);
    }

    sem_destroy(&sem_tasks); // Destroy the semaphore

    return 0;
}
