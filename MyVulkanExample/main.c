#include <stdlib.h>
#include "example.h"
#include "application.h"

int main(int argc, char *argv[]) {
    example_init();
    my_application *app = my_application_new();
    my_application_run(app);
    my_application_delete(app);
    return EXIT_SUCCESS;
}
