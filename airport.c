#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>

#define ATC_tag 2
#define flight_tag 3
#define MAX_SIZE 100
#define MSG_BUFFER_SIZE sizeof(message)

int num_runways;

long id_variable = -1;
long airport_num;

typedef struct flight_info {
    long plane_id;
    int is_passenger_plane;
    int num_occupied_seats;
    int num_cargo_items;
    int total_weight;
    long departure_airport;
    long arrival_airport;
    bool for_departure;
    bool ready_for_termination;
    bool for_init;
} flight_info;

typedef struct airport_details {
    long airport_num;
    int num_runways;
    int *load_cap;
} airport_details;

typedef struct flight_detail {
    flight_info flight;
    airport_details airport;
} flight_detail;

typedef struct message {
    long mtype;
    flight_detail data;
} message;

// Structure for the FCFS semaphore
typedef struct {
    sem_t sem;
    pthread_mutex_t mutex;
    int waiting;
} FCFS_Semaphore;

FCFS_Semaphore fcfs_sem; // Global FCFS semaphore

// Initialize the FCFS semaphore
void initializeFCFSSemaphore(FCFS_Semaphore* sem) {
    sem_init(&sem->sem, 0, 1); // Initialize semaphore to 1
    pthread_mutex_init(&sem->mutex, NULL); // Initialize mutex
    sem->waiting = 0; // Initialize waiting count
}

// Wait on the FCFS semaphore
void waitFCFS(FCFS_Semaphore* sem) {
    pthread_mutex_lock(&sem->mutex); // Lock the mutex
    sem->waiting++; // Increment the waiting count
    pthread_mutex_unlock(&sem->mutex); // Unlock the mutex
    sem_wait(&sem->sem); // Wait on the semaphore
    pthread_mutex_lock(&sem->mutex); // Lock the mutex
    sem->waiting--; // Decrement the waiting count
    pthread_mutex_unlock(&sem->mutex); // Unlock the mutex
}

// Signal the FCFS semaphore
void signalFCFS(FCFS_Semaphore* sem) {
    sem_post(&sem->sem); // Signal the semaphore
}
void destroyFCFSSemaphore(FCFS_Semaphore* sem) {
    sem_destroy(&sem->sem); // Destroy the semaphore
    pthread_mutex_destroy(&sem->mutex); // Destroy the mutex
}
void *flight_runner(void *arg) {
    flight_detail *detail = (flight_detail *)arg;
    key_t key=ftok("plane.c",'A');
	if (key == -1){
        printf("error in creating unique key\n");
        exit(1);
    }
    int msg_id = msgget(key, 0666);
    if (msg_id == -1) {
        perror("msgget");
        exit(1);
    }

    int best_fit = -1;
    int best_fit_cap = 15000;
    message m;
    m.data.flight = detail->flight;
    
    m.mtype = 20 + detail->airport.airport_num;
    
    for (int i = 0; i < detail->airport.num_runways; i++) {
        if (detail->flight.total_weight < detail->airport.load_cap[i] && best_fit_cap > detail->airport.load_cap[i]) {
            best_fit = i;
            best_fit_cap = detail->airport.load_cap[i];
        }
    }
   
    

    if (best_fit == -1) best_fit = detail->airport.num_runways;
    
    if (m.data.flight.for_departure == false) sleep(30);
    
    
    waitFCFS(&fcfs_sem); // Wait on the FCFS semaphore
    
    
    sleep(5);

    if (m.data.flight.for_departure) {
        printf("Plane %ld has completed boarding/loading and taken off from Runway No. %d of Airport No. %ld. \n", m.data.flight.plane_id, best_fit+1, m.data.flight.departure_airport);
        
        
    } else {
        printf("Plane %ld has landed on Runway No. %d of Airport No. %ld and has completed deboarding/unloading.\n", m.data.flight.plane_id, best_fit+1, m.data.flight.arrival_airport);
    }
    if (m.data.flight.for_departure) m.mtype = m.data.flight.departure_airport + 20;
    else m.mtype = m.data.flight.arrival_airport + 20;
    
    if (msgsnd(msg_id, (void *)&m, sizeof(message), 0) == -1) {
        printf("error in sending message");
        exit(1);
    }


    signalFCFS(&fcfs_sem); // Signal the FCFS semaphore

    pthread_exit(NULL);
}

int main() {
    
    printf("Enter Airport Number:");
    scanf("%ld", &airport_num);
    printf("Enter number of Runways:");
    scanf("%d", &num_runways);

    int load_cap[num_runways + 1];

    
	
    printf("Enter loadCapacity of Runways (give as a space separated list in a single line):");
    for (int i = 0; i < num_runways; i++)
        scanf("%d", &load_cap[i]);

    load_cap[num_runways] = 15000;

    initializeFCFSSemaphore(&fcfs_sem); // Initialize the FCFS semaphore

    key_t key=ftok("plane.c",'A');
	if (key == -1){
        printf("error in creating unique key\n");
        exit(1);
    }

	pthread_t flight_threads[MAX_SIZE];
    int thread_count = 0;
    int msg_id = msgget(key, 0666);
    if (msg_id == -1) {
        perror("msgget");
        exit(1);
    }



    message msg;
    

    while (true) {
        
        if (msgrcv(msg_id, &msg, sizeof(message), airport_num, 0) == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        if(msg.data.flight.ready_for_termination==true) break;

        
        flight_detail detail;

        detail.flight = msg.data.flight;

        detail.airport.airport_num = airport_num;
        detail.airport.num_runways = num_runways;
        detail.airport.load_cap = load_cap;

        pthread_t flight_id;
        int status = pthread_create(&flight_id, NULL, flight_runner, &detail);
        flight_threads[thread_count++] = flight_id;
        if (status != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
        
    }
    
    destroyFCFSSemaphore(&fcfs_sem);
    return 0;
}
