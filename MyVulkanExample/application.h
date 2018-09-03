#ifndef VK_EXAMPLE_APPLICATION_H
#define VK_EXAMPLE_APPLICATION_H

typedef struct my_application my_application;

extern my_application * my_application_new(void);

extern void my_application_delete(my_application *app);

extern void my_application_run(my_application *app);

#endif //VK_EXAMPLE_APPLICATION_H
