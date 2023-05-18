#include "findeq.h"

#define DEFAULT_MIN_SIZE 1024

/* cleans up the program, called from signal handler */
void end_program()
{
    /* if -o output file option is on, write file */
}

/* handles signals */
void handle_signal(int sig)
{
	// if interrupt signal, clean up & end program
	if (sig == SIGINT)
    {
        fprintf(stderr, "\nProgram quitting due to interrupt..");
    }

	end_program();
	exit(1);
}

// Global variables
int num_threads = 0;
int min_size = DEFAULT_MIN_SIZE;
char *output_file = NULL;

// Function to handle file processing
void process_file(const char *file_path) {
    struct stat st;
    if (stat(file_path, &st) == 0) {
        if (S_ISREG(st.st_mode) && st.st_size >= min_size) {
            // Print or write to output file
            FILE *output = stdout;
            if (output_file != NULL) {
                output = fopen(output_file, "a");
                if (output == NULL) {
                    perror("Error opening output file");
                    exit(EXIT_FAILURE);
                }
            }
            fprintf(output, "%s\n", file_path);
            if (output_file != NULL) {
                fclose(output);
            }
        }
    } else {
        perror("Error accessing file");
        exit(EXIT_FAILURE);
    }
}

// Function to traverse directories recursively
void traverse_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char file_path[1024];
                snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

                if (entry->d_type == DT_DIR) {
                    // Recursively traverse subdirectory
                    traverse_directory(file_path);
                } else if (entry->d_type == DT_REG) {
                    // Process regular file
                    process_file(file_path);
                }
            }
        }
        closedir(dir);
    } else {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
}

// Thread entry point
void *thread_entry(void *arg) {
    char *dir_path = (char *)arg;
    traverse_directory(dir_path);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments
    if (argc < 2) {
        printf("Usage: %s [OPTION] DIR\n", argv[0]);
        return 0;
    }

    char *dir_path = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "t:m:o:")) != -1) {
        switch (opt) {
            case 't':
                num_threads = atoi(optarg);
                if (num_threads <= 0 || num_threads > 64) {
                    printf("Invalid number of threads. Must be between 1 and 64.\n");
                    return 0;
                }
                break;
            case 'm':
                min_size = atoi(optarg);
                if (min_size < 0) {
                    printf("Invalid minimum size. Must be a non-negative integer.\n");
                    return 0;
                }
                break;
            case 'o':
                output_file = optarg;
                break;
            default:
                printf("Invalid option.\n");
                return 0;
        }
    }

    if (optind < argc) {
        dir_path = argv[optind];
    } else {
        printf("No directory path provided.\n");
        return 0;
    }

    // Create threads
    pthread_t threads[num_threads + 1]; // +1 for the main thread

    return 0;
}