#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "RS.h"
#define TEST_DIR "test/"

int main(int argc, char **argv) {

    char single_ins[32];

    if (argc < 2) {
        printf("Utility:\n\tmain [file]\n");
        exit(0);
    }
    char file[32] = TEST_DIR;
    strcat(file, argv[1]);
    fptr = fopen(file, "r");

    if (fptr == NULL) {
        printf("No file : %s\n", file);
        exit(0);
    }

    bool do_write_res = false;
    bool do_execute = false;
    bool do_issue = false;

    system_init();
    
    while (running) {
        if (fgets(single_ins, sizeof(single_ins), fptr) == NULL)
            memset(single_ins, 0, sizeof(single_ins));
        single_ins[strlen(single_ins) - 1] = '\0';
        do_write_res = write_result();
        do_execute = execute();
        do_issue = issue(single_ins);
        write_back_broadcast();
        clock++;
        show_all_resource();
        running = do_write_res | do_execute | do_issue;
    }

    system_terminate();
    fclose(fptr);
    return 0;
}