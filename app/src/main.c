// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>

#include "study_helpers.h"

int main()
{
    printf("Starting Study Session Timer...\n");

    // Initialization
    if (!study_init()) {
        fprintf(stderr, "ERROR: initialization failed.\n");
        return 1;
    }

    // Main execution loop
    study_run();

    // Cleanup
    study_cleanup();

    printf("Exited cleanly.\n");
    return 0;
}
