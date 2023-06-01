#include "findeq.h"

#define DEFAULT_MIN_SIZE 1024
#define THREAD_MAX_COUNT 64
#define EXEC_TIME 5

// Global variables
struct itimerval timer;

pthread_mutex_t lock;
pthread_mutex_t subtasks_lock;
pthread_cond_t queue_cond;

int num_of_threads = 0;
int size_of_file = DEFAULT_MIN_SIZE;
char *output_file = NULL;

typedef struct _Task {
    char *path;

    struct _Task *next;
    struct _Task *prev;
} Task;

Task *q_front = NULL;
Task *q_end = NULL;

typedef struct _Equal_File {
    char *equal_path;
    char *main_path;

    struct _Equal_File *next;
} Equal_File;

Equal_File *equal_file_list = NULL;
int num_of_equal_files = 0;

void free_equal_file_list ()
{
    Equal_File *curr = equal_file_list;
    Equal_File *save;

    while (curr != NULL) {
        save = curr->next;

        free(curr->equal_path);
        free(curr->main_path);
        free(curr);

        curr = save;
    }
}

void print_list()
{
    Equal_File *curr = equal_file_list;
    int i = 1;

    while (curr != NULL) {
        printf("list(%d): %s, %s\n", i, curr->equal_path, curr->main_path);

        curr = curr->next;
        i++;
    }

    printf("num: %d\n", num_of_equal_files);
}

/* cleans up the program, called from signal handler */
void end_program ()
{
    /* if -o output file option is on, write file */
    if (output_file != NULL) {
        FILE* fp = fopen(output_file, "w");

        if (fp != NULL) {
            // write output to the file
            char program_name[] = "findeq\n";
            fwrite(program_name, 1, sizeof(program_name), fp);

            fclose(fp);
        } else {
            fprintf(stderr, "Unable to open output file: %s\n", output_file);
        }
    } else {
        /* print the output in format */
        print_list();
    }

    free_equal_file_list();
}

/* handles signals, syntax: void foo (int) */
void handle_signal (int sig)
{
    /* if interrupt signal, clean up & end program */
    if (SIGINT == sig) {
        fprintf(stderr, "\nProgram quitting due to interrupt..\n");
        end_program();
    } else if (SIGALRM == sig) {
        /* print the data:
            findeq must print the search progress to standard 
            error every 5 seconds. A search progress must show the number 
            of files known to have at least one other identical file, and other 
            information about the program execution. */
        fprintf(stderr, "Number of found identical files: %d\n", num_of_equal_files);

        /* reset timer */
        timer.it_value.tv_sec = EXEC_TIME;
        timer.it_value.tv_usec = 0;
        setitimer(ITIMER_REAL, &timer, NULL);

        return;
    }
}

int find_duplicate_entry (char *path, char *main_file_path)
{
    Equal_File *curr = equal_file_list;
    int flag_dup = 0;

    while (curr != NULL) {
        if (strcmp(curr->equal_path, path) == 0 && strcmp(curr->main_path, main_file_path) == 0) {
            flag_dup = 1;
            break;   
        }

        if (strcmp(curr->equal_path, main_file_path) == 0 && strcmp(curr->main_path, path) == 0) {
            flag_dup = 1;
            break;   
        }

        curr = curr->next;
    }

    return flag_dup;
}

Task *enqueue (char *filepath)
{
    Task *task = (Task *) malloc(sizeof(Task));

    if (NULL == filepath) {
        task->path = NULL;
    } else {
        task->path = (char *) malloc(sizeof(char) * strlen(filepath) + 1);
        strcpy(task->path, filepath);
    }

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

    return task;
}

int process_file (char *path, char *main_file_path)
{
    struct stat st;
    if (lstat(path, &st) != 0) {
        fprintf(stderr, "process_file(): Unable to get file status: %s\n", path);
        return -1;
    }

    if (S_ISLNK(st.st_mode)) {
        // skip processing symbolic links
        #ifdef DEBUG
            fprintf(stderr, "Ignore link: %s\n", path);
        #endif
        return 0;
    }

    if (strcmp(path, main_file_path) == 0) {
        // printf("Exact file found: %s = %s\n", path, main_file_path);
        return 0;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "process_file() : Unable to open file: %s\n", path);
        return -1;
    }

    // get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    /* ignore file if size is less than size_of_file */
    if (file_size < size_of_file) {
        #ifdef DEBUG
            printf("Ignore file: %s\n", path);
        #endif
        fclose(file);
        return 0;
    }

    struct stat main_file_stat;
    stat(main_file_path, &main_file_stat);

    int exit_condition = 0;

    /* if the file size of the files are the same */
    if (file_size == main_file_stat.st_size) {
        char* buffer = malloc(file_size); // store file in bytes (rb)
        if (!buffer) {
            fprintf(stderr, "process_file() : Memory allocation failed for file: %s\n", path);
            fclose(file);
            return -1;
        }

        size_t result = fread(buffer, 1, file_size, file);
        if (result != file_size) {
            fprintf(stderr, "process_file() : Error reading file: %s\n", path);
            fclose(file);
            free(buffer);
            return -1;
        }

        fclose(file);

        FILE* main_file = fopen(main_file_path, "rb");
        if (!main_file) {
            fprintf(stderr, "process_file() : Unable to open main file: %s\n", main_file_path);
            free(buffer);
            return -1;
        }

        char* main_buffer = malloc(file_size); // store main_file in bytes (rb)
        if (!main_buffer) {
            fprintf(stderr, "process_file() : Memory allocation failed for main file: %s\n", main_file_path);
            fclose(main_file);
            free(buffer);
            return -1;
        }

        size_t main_result = fread(main_buffer, 1, file_size, main_file);
        if (main_result != file_size) {
            fprintf(stderr, "process_file() : Error reading main file: %s\n", main_file_path);
            free(buffer);
            free(main_buffer);
            return -1;
        }

        fclose(main_file);

        if (memcmp(buffer, main_buffer, file_size) == 0) {
            /* check if same entry exists in equal_file_list
                if not, insert an entry to the equal_file_list */
            if (find_duplicate_entry(path, main_file_path) == 0) {
                Equal_File *equal_file = (Equal_File *) malloc(sizeof(Equal_File));
                equal_file->equal_path = (char *) malloc(sizeof(char) * (strlen(path) + 1));
                equal_file->main_path = (char *) malloc(sizeof(char) * (strlen(main_file_path) + 1));

                strcpy(equal_file->equal_path, path);
                strcpy(equal_file->main_path, main_file_path);

                equal_file->next = equal_file_list;
                equal_file_list = equal_file;

                num_of_equal_files++;

                #ifdef DEBUG
                    print_list();
                #endif
            } else {
                #ifdef DEBUG
                    printf("Duplicate entry found: %s, %s\n", path, main_file_path);
                #endif
                exit_condition = 0;
            }

            
        } else {
            #ifdef DEBUG
                printf("Different file found: %s\n", path);
            #endif
            exit_condition = 0;
        }

        free(buffer);
    } else { // file size is different, files are considered different
        #ifdef DEBUG
            printf("Different file found: %s\n", path);
        #endif
        exit_condition = 0;

        fclose(file);
    }

    return exit_condition;
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
            #ifdef DEBUG
                printf("main-d ");
                printf("%s\n", filepath);
            #endif

			if (strcmp(i->d_name, ".") != 0 && strcmp(i->d_name, "..") != 0) {
                set_global_dir(filepath);
            }
		} else if (i->d_type == DT_REG) {
            // pthread_mutex_lock(&lock);

			struct stat st;
			stat(filepath, &st);

            #ifdef DEBUG
                printf("main-r ");
                printf("%s ", filepath);
                printf("%d\n", (int) st.st_size);
            #endif

            /* enqueue */
            Task *task = enqueue(filepath);
            pthread_cond_signal(&queue_cond); // signal a filepath is enqueued

            // pthread_mutex_unlock(&lock);

		}

		free(filepath);
	}

	closedir(dir);
}

int discover_dir (char *dir_path, char *target_file_path)
{
	DIR *dir = opendir(dir_path);

	if (dir == NULL) {
		return -1;
    }

    int exit_condition = 0;

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
            #ifdef DEBUG
                printf("d ");
                printf("%s\n", filepath);
            #endif

			if (strcmp(i->d_name, ".") != 0 && strcmp(i->d_name, "..") != 0) {
                exit_condition = discover_dir(filepath, target_file_path);
            }
		} else if (i->d_type == DT_REG) {
			struct stat st;
			stat(filepath, &st);

            #ifdef DEBUG
                printf("r ");
                printf("%s ", filepath);
                printf("%d\n", (int) st.st_size);
            #endif

            // use process_file() to compare files
            #ifdef DEBUG
                printf("file: %s = %s\n", filepath, target_file_path);
            #endif

            pthread_mutex_lock(&subtasks_lock);
                exit_condition = process_file(filepath, target_file_path);
            pthread_mutex_unlock(&subtasks_lock);

		}

		free(filepath);
	}

	closedir(dir);

    return exit_condition;
}

void *worker(void *arg)
{
    int exit_condition = 0;
    int tasks_remaining = 1;

    while (tasks_remaining) {
        pthread_mutex_lock(&lock);
            while (NULL == q_front) {  // Wait until the queue is non-empty
                pthread_cond_wait(&queue_cond, &lock);
            }

            /* dequeue */
            Task *task = q_front;
            q_front = q_front->next;

            if (q_front != NULL) {
                q_front->prev = NULL;
            } else {
                q_end = NULL;
            }

            // Signal the condition variable
            pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&lock);

        if (NULL == task->path) {
            free(task);
            break;
        }

        #ifdef DEBUG
            printf("Start: %s, Task: %s\n", (char *)arg, task->path);
        #endif

        exit_condition = discover_dir((char *)arg, task->path);

        free(task->path);
        free(task);

        if (exit_condition < 0) {
            fprintf(stderr, "worker() : Error reading directory\n");
            return NULL;
        }

        pthread_mutex_lock(&lock);
            tasks_remaining = (q_front != NULL || q_end != NULL); // Check if there are still tasks in the queue
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

#ifdef DEBUG
    void print_que()
    {
        Task *curr = q_front;

        while (curr != NULL) {
            printf("que: %s\n", curr->path);

            curr = curr->next;
        }
    }
#endif

void main_thread (char *dir_path)
{
    char* main_dir_path = strdup(dir_path);

    set_global_dir(main_dir_path);

    #ifdef DEBUG
        print_que();
    #endif

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

    timer.it_value.tv_sec = EXEC_TIME;	// 5 seconds
	timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);

    signal(SIGINT, handle_signal);
	signal(SIGALRM, handle_signal);

    pthread_t threads[num_of_threads];
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&subtasks_lock, NULL);
    pthread_cond_init(&queue_cond, NULL);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_create(&(threads[i]), NULL, worker, (void *) dir_path);
    }

    main_thread(dir_path);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&subtasks_lock);
    pthread_cond_destroy(&queue_cond);

    end_program();

    return 0;
}
