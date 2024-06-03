#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSG_BUFFER_SIZE sizeof(message)

typedef struct flight_info {
    // flight_info structure definition
} flight_info;

typedef struct airport_details {
    // airport_details structure definition
} airport_details;

typedef struct flight_detail {
    flight_info flight;
    airport_details airport;
} flight_detail;

typedef struct message {
    long mtype;
    flight_detail data;
} message;

int main() {
    message m;
    key_t key=ftok("plane.c",'A');
	if (key == -1){
        printf("error in creating unique key\n");
        exit(1);
    }
    int msg_id = msgget(key, 0666); // Creating message queue if not exists
    if (msg_id == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    char yes;
    while (1) {
        printf("Do you want the Air Traffic Control System to terminate? (Y for Yes and N for No)\n");
        scanf(" %c", &yes); // Adding a space before %c to consume any whitespace characters

        if (yes == 'N') {
            continue;
        } else if (yes == 'Y') {
            m.mtype = 100;
            if (msgsnd(msg_id, &m, sizeof(message), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
            break;
        } else {
            printf("Invalid input. Please enter Y or N.\n");
        }
    }

    return 0;
}
