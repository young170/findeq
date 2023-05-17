#include "findeq.h"



/* cleans up the program, called from signal handler */
void end_program()
{
    /* if -o output file option is on, write file */
}

/* handles signals */
void handle_signal(int sig)
{
	// if interrupt signal, clean up & end program
	else if (sig == SIGINT)
    {
        fprintf(stderr, "\nProgram quitting due to interrupt..");
    }

	end_program();
	exit(1);
}

int main (int argc, char *argv[])
{
    // signal(SIGINT, handle_signal);

    return 0;
}