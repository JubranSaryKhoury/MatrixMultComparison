// Jubran Khoury 1201264
// Operating system first task

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>

#define SIZE 100

#define numOfThreads 2

//  struct for thread portion ...portion for portion result (portion of the final full result)
struct Node {
    int start; // to contain the start point ... since the rows of the first matrix will be devided between the threads for matrix multiplication.
    int end; // to contain the end point.
    int (*x)[SIZE];
    int (*y)[SIZE];
    int (*result)[SIZE];
};
typedef struct Node ThreadPortion; // to use ThreadPortion instead of Node for better understanding the code.


//----------------------Functions--------------------------------

// Function for matrix multiplication (the native approach or the standard)
void MatrixMultiplication_native(int x[SIZE][SIZE], int y[SIZE][SIZE], int result[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            result[i][j] = 0;
            for (int q = 0; q < SIZE; q++) {
                result[i][j] += x[i][q] * y[q][j];
            }
        }
    }
}

// Functions for matrix multiplication (Multi-process)
void MatrixMultiplication_multiProcess_oneChildProcess(int x[SIZE][SIZE], int y[SIZE][SIZE], int result[SIZE][SIZE]) {
    // Here we need just one fork() to give us a two process (the child process and the parent process)

    int fd[2]; // file descriptor for the pipe (inter process communication)
    if (pipe(fd) == -1) {
        printf("faild to create the pipe!");
        exit(1); // exit if faild
    }

    int child_pid = fork(); // create the child process ---- now there is a child process with id=0 and parent or main process with id!=0

    if (child_pid < 0) { // failed to create a child process then exit
        printf("Failed to create a child process!");
        exit(1); // exit if faild
    } else if (child_pid == 0) { // Child process
        close(fd[0]); // Close the read end in the child process (here we only send or write to the main process, so before beginning we close the read)

        int result1[SIZE / 2][SIZE]; // to contain the first half of the result
        // Perform matrix multiplication for the child process (first half of rows)
        for (int i = 0; i < SIZE / 2; i++) {
            for (int j = 0; j < SIZE; j++) {
                result1[i][j] = 0;
                for (int q = 0; q < SIZE; q++) {
                    result1[i][j] += x[i][q] * y[q][j];
                }
            }
        }

        // Write the result to the pipe
        if (write(fd[1], result1, SIZE / 2 * SIZE * sizeof(int)) == -1) {
            printf("An error occurred when writing to the pipe!");
            close(fd[1]); // if faild then close the write end of the pipe
            exit(1);
        }

        close(fd[1]); // Close the write end (after writing is finished we should close the write, fd[1])

        exit(1);
    } else { // Parent or the main process
        close(fd[1]); // Close the write end in the parent/main process (here we only receive or read from the child process, so before beginning we close the write)

        int result2[SIZE / 2][SIZE]; // to contain the second half of the result
        for (int i = 0; i < SIZE / 2; i++) {
            for (int j = 0; j < SIZE; j++) {
                result2[i][j] = 0;
            }
        }

        // Perform matrix multiplication for the parent process (second half of rows)
        for (int i = SIZE / 2; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                for (int q = 0; q < SIZE; q++) {
                    result2[i - SIZE / 2][j] += x[i][q] * y[q][j];
                }
            }
        }

        // Create a matrix to read the first half from the pipe
        int first_half[SIZE / 2][SIZE];

        // Read the first half from the pipe
        if (read(fd[0], first_half, sizeof(first_half)) == -1) {
            printf("An error occurred when reading from the pipe!");
            close(fd[0]); // Close the read if there is an error
            exit(1);
        }
        close(fd[0]); // Close the read end (after reading is finished we should close the read, fd[0])

        // Combine the partial results into the main result matrix
        for (int i = 0; i < SIZE / 2; i++) {
            for (int j = 0; j < SIZE; j++) {
                result[i][j] = first_half[i][j];
            }
        }

        // Combine the second half of the result
        for (int i = SIZE / 2; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                result[i][j] = result2[i - SIZE / 2][j];
            }
        }

        // Wait for child process to finish
        wait(NULL);

        //printf("The result from multi - processes (one child process)\n");

    }
}

void MatrixMultiplication_multiProcess_fourChildrenProcess(int x[SIZE][SIZE], int y[SIZE][SIZE], int result[SIZE][SIZE]) {
    // This function is same as above, but this time with four children process instead of just one.
    // Each child process will calculate a portion of the final result and pass it to the parent process by a pipe (inter process communication)
    // Finally the parent process will receive the portion results from the childs and produce the final full result matrix.


    int fd[4][2]; // file descriptors for four pipes each have read and write ends
    int child_pid[4]; // array to store child process id's

    for (int i = 0; i < 4; i++) { // Checks for all pipes if succesfully created
        if (pipe(fd[i]) == -1) {
            printf("Failed to create the pipe fd[%d] ",i+1);
            exit(1); // exit if failed
        }
    }

    for (int i = 0; i < 4; i++) {
        child_pid[i] = fork(); // create a child process in each loop (4 in total)

        if (child_pid[i] < 0) { // failed to create a child process then exit
            printf("Failed to create a child process!");
            exit(1); // Exit if failed
        } else if (child_pid[i] == 0) { // Child process always has id=0
            close(fd[i][0]); // Close the read end in the child process

            int Portion_Of_Result[SIZE / 4][SIZE]; // matrix to contain the portion of the result (each child will contain part of the final result)
                                                  // This martix has same number of coloumn but with quarter number of rows


            // Calculate the portion of the result for the child process
            int start = i * SIZE / 4;        // Every child process takes start row and end row so that the matrix rows is devided into four parts each child a quarter
            int end = (i + 1) * SIZE / 4;    // First child will take the rows from index 0 (first row) till the SIZE / 4 (the quarter), and the second from the quarter till the half , and so on ......
                                            // Then every child process is responsable for calculate its result and pass it to the parent process
            for (int j = start; j < end; j++) {
                for (int k = 0; k < SIZE; k++) {
                    Portion_Of_Result[j - start][k] = 0;
                    for (int q = 0; q < SIZE; q++) {
                        Portion_Of_Result[j - start][k] += x[j][q] * y[q][k];
                    }
                }
            }

            // Write the result to the pipe
            if (write(fd[i][1], Portion_Of_Result, SIZE / 4 * SIZE * sizeof(int)) == -1) {
                printf("An error occurred when writing to the pipe fd[%d]",i+1);
                close(fd[i][1]); // if faild then close the write end of the pipe
                exit(1);
            }

            close(fd[i][1]); // Close the write end (after finished writing))

            exit(0);
        }else{ // parent process

            close(fd[i][1]); // Close the write ends in the parent process (because we dont need to write , just read)

            int Portion_Of_Result[SIZE / 4][SIZE]; // to receive the portion result from the child


            // Read the portion from the pipe
            if (read(fd[i][0], Portion_Of_Result, SIZE / 4 * SIZE * sizeof(int)) == -1) {
                printf("An error occurred when reading from the pipe fd[%d]",i+1);
                close(fd[i][0]); // if faild close the read end
                exit(1);
            }

            close(fd[i][0]); // Close the read end (after finished reading)

            // Combine the portion result into the main/full result matrix
            int start = i * SIZE / 4;  // start row

            for (int j = start; j < start + SIZE / 4; j++) {  // the end for each will be the start + Size/4, because quarter for each
                for (int k = 0; k < SIZE; k++) {
                    result[j][k] = Portion_Of_Result[j - start][k];
                }
            }
            // Wait for child process to finish
            wait(NULL);
        }
    }
    //printf("The result from multi - processes (four children processes)\n");
}

// Function for matrix multiplication (Multi-thread)
void* thread_function(void* arg) { // In case of two threads, this function will be used twice (when i=0 and i=1)... i=0 for the first half and i=1 for the remaining half.(in case of two threads)
    ThreadPortion* portion = (ThreadPortion*)arg;

    for (int i = portion->start; i < portion->end; i++) { // from start point till end point.
        for (int j = 0; j < SIZE; j++) {
            portion->result[i][j] = 0;
            for (int q = 0; q < SIZE; q++) {
                portion->result[i][j] += portion->x[i][q] * portion->y[q][j];
            }
        }
    }
}

void MatrixMultiplication_Joinablethreads(int x[SIZE][SIZE], int y[SIZE][SIZE], int result[SIZE][SIZE],int Number_Of_Threads) {
    // lets take the case of used two threads (for explain the function)

    pthread_t threads[Number_Of_Threads]; // two threads
    ThreadPortion* portion[Number_Of_Threads];

    for (int i = 0; i < Number_Of_Threads; i++) {
        portion[i] = malloc(sizeof(struct Node)); // memory allocation for portion[0] and portion[1] which are a struct of ThreadPortion
        if (portion[i] == NULL) {
            printf("Out of space"); // faild to reserve a place in the memory
            exit(1);
        }

        portion[i]->start = i * SIZE / Number_Of_Threads;  // i between 0 and 1 ...... if i=0 then the start will be from 0 till the half of the matrix (the end point).
        portion[i]->end = (i + 1) * SIZE / Number_Of_Threads;                       // if i=1 then the start will be from the half till the end of the matrix (the end point).
        portion[i]->x = x;
        portion[i]->y = y;
        portion[i]->result = result;

        if (pthread_create(&threads[i], NULL, thread_function, (void*)portion[i]) != 0) {  // The fourth argument (void*)portion[i] is a pointer to the struct to be passed to the thread_function
            // Note that pthread_create function return 0 if success, so if return non-zero => faild
            printf("faild to create thread ");
            exit(1);
        }
    }

    for (int i = 0; i < Number_Of_Threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) { // wait for each thread to finish its execution
            // Same here, pthread_join return 0 if success, so if return non-zero => faild
            printf("faild to join thread");
            exit(1);
        }

        free(portion[i]); // free the memory that was allocated before
    }
    printf("The result from multi - threads (%d thread/threads)\n",numOfThreads);
}

void MatrixMultiplication_Detachthreads(int x[SIZE][SIZE], int y[SIZE][SIZE], int result[SIZE][SIZE],int Number_Of_Threads) {
    // lets take the case of used two threads (for explain the function)

    pthread_t threads[Number_Of_Threads]; // two threads
    ThreadPortion* portion[Number_Of_Threads];

    for (int i = 0; i < Number_Of_Threads; i++) {
        portion[i] = malloc(sizeof(struct Node)); // memory allocation for portion[0] and portion[1] which are a struct of ThreadPortion
        if (portion[i] == NULL) {
            printf("Out of space"); // faild to reserve a place in the memory
            exit(1);
        }

        portion[i]->start = i * SIZE / Number_Of_Threads;  // i between 0 and 1 ...... if i=0 then the start will be from 0 till the half of the matrix (the end point).
        portion[i]->end = (i + 1) * SIZE / Number_Of_Threads;                       // if i=1 then the start will be from the half till the end of the matrix (the end point).
        portion[i]->x = x;
        portion[i]->y = y;
        portion[i]->result = result;

        if (pthread_create(&threads[i], NULL, thread_function, (void*)portion[i]) != 0) {  // The fourth argument (void*)portion[i] is a pointer to the struct to be passed to the thread_function
            // Note that pthread_create function return 0 if success, so if return non-zero => faild
            printf("faild to create thread ");
            exit(1);
        }
    }

    for (int i = 0; i < Number_Of_Threads; i++) {
        pthread_detach(threads[i]);  // by using detach then => not wait for thread to finish its execution
                                     // if the main thread completes before the threads, the program may exit and leave the threads unfinished.
                                     // That's why the result matrix is zero's (The program ends before each thread calculate its partial result)
        //sleep(1); // sleep for one second
        free(portion[i]); // free the memory that was allocated before
    }
}

//----------------------The main function--------------------------------
int main() {
    // Define the id matrix
    int a[] = {1, 2, 0, 1, 2, 6, 4}; // My id = 1201264
    int id[SIZE][SIZE];

    // Fill the id matrix with my id in a circular pattern
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            id[i][j] = a[j % 7]; // the size of matrix a, since my id is 7 integers
        }
    }

    // Define the id*birthYear matrix
    int b[] = {2, 4, 0, 7, 6, 8, 5, 2, 8}; // 240,768,528
    int idBy[SIZE][SIZE]; // idBy => id * birth year

    // Fill the idBy matrix with 240,768,528 in a circular pattern
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            idBy[i][j] = b[j % 9]; // the size of matrix b, 240,768,528 is 9 integers
        }
    }

    // Now the matrices are ready

    //Print the matrices
    printf("ID Matrix:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", id[i][j]);
        }
    }
    printf("\n\n");

    printf("\nID*BirthYear Matrix:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", idBy[i][j]);
        }
    }
     printf("\n\n");

    int result_Matrix[SIZE][SIZE]; // to have the multiplication result


    // Measure the execution time for all approches

    // The native approches
    // Measure the start time (before the multiplication)
    clock_t start_time1 = clock();

    // Calling the function for multiplication
    MatrixMultiplication_native(idBy, id, result_Matrix);

    // Measure the end time (striaght ahead after the multiplication)
    clock_t end_time1 = clock();

    // The multi process approches (one child process)
    clock_t start_time2 = clock();
    MatrixMultiplication_multiProcess_oneChildProcess(idBy, id, result_Matrix);
    clock_t end_time2 = clock();


    // The multi process approches (four child process)
    clock_t start_time3 = clock();
    MatrixMultiplication_multiProcess_fourChildrenProcess(idBy, id, result_Matrix);
    clock_t end_time3 = clock();

    // The multi threads (joined)
    clock_t start_time4 = clock();
    MatrixMultiplication_Joinablethreads(idBy, id, result_Matrix,numOfThreads);
    clock_t end_time4 = clock();

    // The multi threads(detached)
    clock_t start_time5 = clock();
    MatrixMultiplication_Detachthreads(idBy, id, result_Matrix,numOfThreads);
    clock_t end_time5 = clock();

    // Print the result matrix (Matrix multiplication result)
    printf("Result Matrix:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", result_Matrix[i][j]);
        }
    }
    printf("\n\n");

    // Calculate the execution time of each matrix multiplication approches
    double cpu_time_used1 = ((double) (end_time1 - start_time1)) / CLOCKS_PER_SEC;
    printf("\nThe execution time of native multiplication : %f seconds\n", cpu_time_used1);

    double cpu_time_used2 = ((double) (end_time2 - start_time2)) / CLOCKS_PER_SEC;
    printf("\nThe execution time of multi process (one child + parent) : %f seconds\n", cpu_time_used2);

    double cpu_time_used3 = ((double) (end_time3 - start_time3)) / CLOCKS_PER_SEC;
    printf("\nThe execution time of multi process (four children + parent) : %f seconds\n", cpu_time_used3);

    double cpu_time_used4 = ((double) (end_time4 - start_time4)) / CLOCKS_PER_SEC;
    printf("\nThe execution time of joinable threads (%d Thread/Threads): %f seconds\n", cpu_time_used4,numOfThreads);

    double cpu_time_used5 = ((double) (end_time5 - start_time5)) / CLOCKS_PER_SEC;
    printf("\nThe execution time of detached threads (%d Thread/Threads): %f seconds\n", cpu_time_used5,numOfThreads);

    return 0;
}
