/*
 * week4_3_struct_database.c
 * Author: [Your Name]
 * Student ID: [Your ID]
 * Description:
 *   Simple in-memory "database" using an array of structs.
 *   Use malloc to allocate space for n Student records,
 *   read each record from the user, print them as a table,
 *   and then free the memory.
 *
 *   Output must match the format in the Week 4 instructions exactly
 *   (it is checked by the autograder).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Define struct Student with fields name (char[50]), id (int), grade (float)
//       (same definition as in Task 2)

int main(void) {
    int n;
    struct Student *students = NULL;

    printf("Enter number of students: ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        printf("Invalid number.\n");
        return 1;
    }

    // TODO: Allocate memory for n Student structs using malloc
    //       Example: students = malloc(n * sizeof(struct Student));

    // TODO: Check allocation success
    // If students is NULL: print "Memory allocation failed." and return 1

    // TODO: Read student data in a loop. For student i (counting from 1):
    //       print "Enter data for student %d: ", then read
    //       name (scanf("%49s", ...)), id and grade.
    //       If a value cannot be read: print "Invalid input.",
    //       free the array and return 1

    // TODO: Print an empty line, then the table:
    //       printf("%-6s %-11s %s\n", "ID", "Name", "Grade");
    //       and for each student:
    //       printf("%-6d %-11s %.1f\n", id, name, grade);

    // Optional (not autograded): after the table, print the average
    // grade or the top student

    // TODO: Free allocated memory
    (void)students;  // remove this line once you use students

    return 0;
}
