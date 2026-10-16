// week5_task1_file_io.c
// Name: Fuad  Surname: Salahli Student ID: 241ADB065
//
// Task 1: Read and write data from text files
// Week 5 – Files & Modular Programming
// TODO: Fill in the missing parts marked below.
//
// Build: make lab5   Run: ./bin/week5_task1_file_io

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_FILENAME "data.txt"
#define LINE_LEN 256

int main(int argc, char *argv[]) {
    FILE *fp = NULL;
    const char *filename = DEFAULT_FILENAME;
    char line[LINE_LEN];
    int line_count = 0;
    if(argc > 1) {
        filename = argv[1];
    }

    printf("Writing lines to %s...\n", filename);
    fp = fopen(filename, "w");
    if(fp == NULL) {
        perror(filename);
        return EXIT_FAILURE;
    }
    fprintf(fp, "New Data File\n");
    fprintf(fp, "Second Line of Date\n");
    fclose(fp);
    fp = fopen(filename, "r");
    if(fp == NULL) {
        perror(filename);
        return EXIT_FAILURE;
    }
    
    printf("Reading contents:\n");
    while(fgets(line, sizeof line, fp) != NULL) {
        printf("%s", line);
        line_count++;
    }
    fclose(fp);
    printf("Lines read: %d\n", line_count);
   
    return EXIT_SUCCESS;
}