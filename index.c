#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
void* student_actions(void* student_id);
void* ta_actions();
#define NUM_WAITING_CHAIRS 3
#define DEFAULT_NUM_STUDENTS 5
sem_t sem_students;
sem_t sem_ta;
pthread_mutex_t mutex_thread;
int waiting_room_chairs[NUM_WAITING_CHAIRS];
int number_students_waiting = 0;
int next_seating_position = 0;
int next_teaching_position = 0;
int ta_sleeping_flag = 0;
int num_executions = 1; 
int main(int argc, char** argv) {
    int i;
    int student_num;

    if (argc > 1) {
        if (isdigit(argv[1][0])) {
            student_num = atoi(argv[1]);
        } else {
            printf("Invalid input. Quitting program.\n");
            return 0;
        }
    } else {
        student_num = DEFAULT_NUM_STUDENTS;
    }

    int student_ids[student_num];
    pthread_t students[student_num];
    pthread_t ta;

    sem_init(&sem_students, 0, 0);
    sem_init(&sem_ta, 0, 1);

    // Create threads
    pthread_mutex_init(&mutex_thread, NULL);
    pthread_create(&ta, NULL, ta_actions, NULL);
    for (i = 0; i < student_num; i++) {
        student_ids[i] = i + 1;
        pthread_create(&students[i], NULL, student_actions, (void*)&student_ids[i]);
    }

    // Join threads
    pthread_join(ta, NULL);
    for (i = 0; i < student_num; i++) {
        pthread_join(students[i], NULL);
    }

    return 0;
}

void* ta_actions() {
    printf("Checking for students.\n");

    while (num_executions > 0) {
        // If students are waiting
        if (number_students_waiting > 0) {
            ta_sleeping_flag = 0;
            sem_wait(&sem_students);
            pthread_mutex_lock(&mutex_thread);

            int help_time = rand() % 5;

            // TA helping student.
            printf("Helping a student for %d seconds. Students waiting = %d.\n", help_time, (number_students_waiting - 1));
            printf("Student %d receiving help.\n", waiting_room_chairs[next_teaching_position]);

            waiting_room_chairs[next_teaching_position] = 0;
            number_students_waiting--;
            next_teaching_position = (next_teaching_position + 1) % NUM_WAITING_CHAIRS;

            usleep(help_time * 1000000);

            pthread_mutex_unlock(&mutex_thread);
            sem_post(&sem_ta);

            num_executions--;
        }

        else {
            if (ta_sleeping_flag == 0) {
                printf("No students waiting. Sleeping.\n");
                ta_sleeping_flag = 1;
            }
        }
    }
}
void* student_actions(void* student_id) {
    int id_student = *(int*)student_id;
    int executions = num_executions;
    while (executions > 0) {
        if (waiting_room_chairs[id_student - 1] == id_student) {
            continue;
        }
        int time = rand() % 5;
        printf("\tStudent %d is programming for %d seconds.\n", id_student, time);
        usleep(time * 1000000);
        pthread_mutex_lock(&mutex_thread);
        if (number_students_waiting < NUM_WAITING_CHAIRS) {
            waiting_room_chairs[next_seating_position] = id_student;
            number_students_waiting++;
            printf("\t\tStudent %d takes a seat. Students waiting = %d.\n", id_student, number_students_waiting);
            next_seating_position = (next_seating_position + 1) % NUM_WAITING_CHAIRS;
            pthread_mutex_unlock(&mutex_thread);
            sem_post(&sem_students);
            sem_wait(&sem_ta);
        } else {
            pthread_mutex_unlock(&mutex_thread);
            printf("\t\t\tStudent %d will try later.\n", id_student);
        }
        executions--;
    }
}