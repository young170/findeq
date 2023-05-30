#include "findeq.h"

#define DEFAULT_MIN_SIZE 1024
#define THREAD_MAX_COUNT 64

// Global variables
pthread_mutex_t lock;
pthread_mutex_t subtasks_lock;

int num_of_threads = 0;
int size_of_file = DEFAULT_MIN_SIZE;
char *output_file = NULL;

typedef struct _Equal {
    char *org_path;
    char *equal_path;

    
}

typedef struct _Task {
    char *path;

    struct _Task *next;
    struct _Task *prev;
} Task;

Task *q_front = NULL;
Task *q_end = NULL;

/* cleans up the program, called from signal handler */
void end_program ()
{
    /* if -o output file option is on, write file */
    if (output_file != NULL) {
        FILE* fp = fopen(output_file, "w");

        if (fp != NULL) {
            // Write output to the file
            fclose(fp);
        } else {
            fprintf(stderr, "Unable to open output file: %s\n", output_file);
        }
    }
}

/* handles signals, void foo (int) */
void handle_signal (int sig)
{
    // if interrupt signal, clean up & end program
    if (sig == SIGINT) {
        fprintf(stdout, "\nProgram quitting due to interrupt..\n");
    }

    end_program();
    exit(1);
}

void process_file (const char *path, const char *main_file_path)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "process_file() : Unable to open file: %s\n", path);
        return;
    }

    // get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    struct stat main_file_stat;
    stat(main_file_path, &main_file_stat);

    /* if the file size of the files are the same */
    if (file_size == main_file_stat.st_size) {
        char* buffer = malloc(file_size); // store file in bytes (rb)
        if (!buffer) {
            fprintf(stderr, "process_file() : Memory allocation failed for file: %s\n", path);
            fclose(file);
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

        FILE* main_file = fopen(main_file_path, "rb");
        if (!main_file) {
            fprintf(stderr, "process_file() : Unable to open main file: %s\n", main_file_path);
            free(buffer);
            return;
        }

        char* main_buffer = malloc(file_size); // store main_file in bytes (rb)
        if (!main_buffer) {
            fprintf(stderr, "process_file() : Memory allocation failed for main file: %s\n", main_file_path);
            fclose(main_file);
            free(buffer);
            return;
        }

        size_t main_result = fread(main_buffer, 1, file_size, main_file);
        if (main_result != file_size) {
            fprintf(stderr, "process_file() : Error reading main file: %s\n", main_file_path);
            free(buffer);
            free(main_buffer);
            return;
        }

        fclose(main_file);

        if (memcmp(buffer, main_buffer, file_size) == 0) {
            printf("Identical file found: %s\n", path);
        } else {
            printf("Different file found: %s\n", path);
        }

        free(buffer);
    } else { // file size is different, files are considered different
        printf("Different file found: %s\n", path);
        fclose(file);
    }

    return;
}

void set_global_dir (char *dir_path)
{
	DIR *dir = opendir(dir_path);

	if (dir == NULL) {
		return;
    }

	for (struct dirent *i = readdir(dir); i != NULL; i = readdir(dir)) {
		if (i->d_type != DT_DIR && i->d_type != DT_REG) {
			continue;
        }

        /* concat filepath */
		char *filepath = (char *) malloc(strlen(dir_path) + 1 + strlen(i->d_name) + 1);
		strcpy(filepath, dir_path);
		strcpy(filepath + strlen(dir_path), "/");
		strcpy(filepath + strlen(dir_path) + 1, i->d_name);

		if (i->d_type == DT_DIR) {
			printf("main-d ") ;
			printf("%s\n", filepath) ;

			if (strcmp(i->d_name, ".") != 0 && strcmp(i->d_name, "..") != 0) {
                set_global_dir(filepath);
            }
		} else if (i->d_type == DT_REG) {
			struct stat st;
			stat(filepath, &st);
			printf("main-r ") ;
			printf("%s ", filepath);
			printf("%d\n", (int) st.st_size);

            pthread_mutex_lock(&lock);
                /* enqueue */
                Task *task = (Task *) malloc(sizeof(Task));
                task->path = (char *) malloc(sizeof(char) * strlen(filepath) + 1);
                strcpy(task->path, filepath);

                if (NULL == q_front) {
                    q_front = task;
                }

                if (NULL == q_end) {
                    q_end = task;
                } else {
                    q_end->next = task;
                    task->prev = q_end;
                    q_end = task;
                }

                task->next = NULL;
            pthread_mutex_unlock(&lock);
		}

		free(filepath);
	}

	closedir(dir);
}

void discover_dir (char *dir_path)
{
	DIR *dir = opendir(dir_path);

	if (dir == NULL) {
		return;
    }

	for (struct dirent *i = readdir(dir); i != NULL; i = readdir(dir)) {
		if (i->d_type != DT_DIR && i->d_type != DT_REG) {
			continue;
        }

        /* concat filepath */
		char *filepath = (char *) malloc(strlen(dir_path) + 1 + strlen(i->d_name) + 1);
		strcpy(filepath, dir_path);
		strcpy(filepath + strlen(dir_path), "/");
		strcpy(filepath + strlen(dir_path) + 1, i->d_name);

		if (i->d_type == DT_DIR) {
			printf("d ");
			printf("%s\n", filepath);

			if (strcmp(i->d_name, ".") != 0 && strcmp(i->d_name, "..") != 0) {
                discover_dir(filepath);
            }
		} else if (i->d_type == DT_REG) {
			struct stat st;
			stat(filepath, &st);
			printf("r ");
			printf("%s ", filepath);
			printf("%d\n", (int) st.st_size);

            // use process_file() to compare files
            pthread_mutex_lock(&subtasks_lock);
                if (q_front != NULL) {
                    Task *task = q_front;
                    q_front = q_front->next;

                    if (q_front != NULL) {
                        q_front->prev = NULL;
                    } else {
                        q_end = NULL;
                    }

                    process_file(filepath, task->path);

                    free(task->path);
                    free(task);
                }
            pthread_mutex_unlock(&subtasks_lock);
		}

		free(filepath);
	}

	closedir(dir);
}

void *worker (void *arg)
{
    while(1) {
        if (q_front == NULL) { // change to cond
            continue;
        }

        discover_dir((char *) arg);
        break;
    }

    return NULL;
}

void main_thread (char *dir_path)
{
    char* main_dir_path = strdup(dir_path);

    set_global_dir(main_dir_path);

    return;
}

int main (int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s [OPTION] DIR\n", argv[0]);
        return 0;
    }

    char* dir_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-t=", 3) == 0) {
            num_of_threads = atoi(argv[i] + 3);

            if (num_of_threads < 0 || num_of_threads > THREAD_MAX_COUNT) {
                fprintf(stderr, "Invalid number of threads. It should be between 0 and 64.\n");
                return 0;
            }
        } else if (strncmp(argv[i], "-m=", 3) == 0) {
            size_of_file = atoi(argv[i] + 3);

            if (size_of_file < 0) {
                fprintf(stderr, "Invalid file size. It should be a non-negative integer.\n");
                return 0;
            }
        } else if (strncmp(argv[i], "-o=", 3) == 0) {
            output_file = argv[i] + 3;
        } else {
            dir_path = argv[i];
        }
    }

    signal(SIGINT, handle_signal);

    pthread_t threads[num_of_threads];
    // pthread_mutex_init(&task_pool_lock, NULL);
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&subtasks_lock, NULL);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_create(&(threads[i]), NULL, worker, (void *) dir_path);
    }

    main_thread(dir_path);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // pthread_mutex_destroy(&task_pool_lock);
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&subtasks_lock);

    return 0;
}
