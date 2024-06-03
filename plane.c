#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>

#define MAX_PASSENGERS 10
#define MAX_LUGGAGE_WEIGHT 25
#define MAX_BODY_WEIGHT 100
#define MIN_BODY_WEIGHT 10
#define NUM_CREW_MEMBERS 7
#define AVERAGE_CREW_WEIGHT 75
#define MSG_KEY 12345 // Unique key for message queue

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


sem_t mutex; // Semaphore for synchronization

void createPassengerProcesses(int num_passengers, int pipefd[][2]);

int main() {
    flight_info plane;
    plane.total_weight = 0;
    plane.for_departure=true;
    
    sem_init(&mutex, 0, 1); // Initialize semaphore

    key_t key=ftok("plane.c",'A');
    if (key == -1){
        
        printf("error in creating unique key\n");
        exit(1);
    }
    
    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }
    
    message init;
    init.mtype=13;
    init.data.flight.for_init=true;
    
    if (msgsnd(msgid, &init, sizeof(message), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
    
    
    
    // Ask user to enter unique plane ID
    printf("Enter Plane ID: ");
    scanf("%ld", &plane.plane_id);
    // Ask user to enter type of plane (passenger or cargo)
    printf("Enter Type of Plane (1 for Passenger, 0 for Cargo): ");
    scanf("%d", &plane.is_passenger_plane);

    // Print plane ID and type
    if (plane.is_passenger_plane) 
    {
        printf("Enter Number of Occupied Seats (1-10): ");
        scanf("%d", &plane.num_occupied_seats);
        
        if (plane.num_occupied_seats < 1 || plane.num_occupied_seats > 10) 
        {
            fprintf(stderr, "Invalid number of occupied seats. Must be between 1 and 10.\n");
            exit(EXIT_FAILURE);
        }
        
        int pipefd[MAX_PASSENGERS][2]; // Array to store pipe file descriptors

        // Create pipes for each passenger
        for (int i = 0; i < plane.num_occupied_seats; i++) 
        {
            if (pipe(pipefd[i]) == -1) {
                perror("Pipe creation failed");
                exit(EXIT_FAILURE);
            }
        }
		
        // Create passenger processes
        createPassengerProcesses(plane.num_occupied_seats, pipefd);

        // Close pipe file descriptors in parent process
        for (int i = 0; i < plane.num_occupied_seats; i++) 
        {
            
            close(pipefd[i][1]); // Close write end
        }

        // Calculate total weight of passengers and luggage
        int passenger_weight = 0;
        int luggage_weight = 0;

        // Calculate total weight of passengers and luggage
        for (int i = 0; i < plane.num_occupied_seats; i++) 
        {
            int weights[2]; // Array to store body weight and luggage weight
            read(pipefd[i][0], weights, sizeof(weights));
            passenger_weight += weights[0];
            luggage_weight += weights[1];
        }

        // Calculate total weight including crew members
        plane.total_weight = passenger_weight + luggage_weight + (NUM_CREW_MEMBERS * AVERAGE_CREW_WEIGHT);
        for (int i = 0; i < plane.num_occupied_seats; i++) 
        {
            close(pipefd[i][0]); // Close read end
        }
        
    } 
    
    
    else {
    	printf("Enter Number of Cargo Items : ");
        scanf("%d", &plane.num_cargo_items);
        
        if (plane.num_cargo_items < 1 || plane.num_cargo_items > 100) 
        {
            fprintf(stderr, "Invalid number of cargo items. Must be between 1 and 100.\n");
            exit(EXIT_FAILURE);
        }
        
    	printf("Enter Average Weight of Cargo Items : ");
        scanf("%d", &plane.total_weight);
        if (plane.total_weight < 1 || plane.total_weight > 100) {
            fprintf(stderr, "Invalid average weight of cargo items. Must be between 1 and 100.\n");
            exit(EXIT_FAILURE);
        }
        
        plane.total_weight = (plane.num_cargo_items * plane.total_weight) + (2 * AVERAGE_CREW_WEIGHT);
    }

    // Ask user to enter airport number for departure
    printf("Enter Airport Number for Departure (1-10): ");
    scanf("%ld", &plane.departure_airport);
    if (plane.departure_airport < 1 || plane.departure_airport > 10) {
        fprintf(stderr, "Invalid airport number for departure. Must be between 1 and 10.\n");
        exit(EXIT_FAILURE);
    }

    // Ask user to enter airport number for arrival
    printf("Enter Airport Number for Arrival (1-10): ");
    scanf("%ld", &plane.arrival_airport);
    if (plane.arrival_airport < 1 || plane.arrival_airport > 10 || plane.arrival_airport == plane.departure_airport) 
    {
        fprintf(stderr, "Invalid airport number for arrival. Must be between 1 and 10 and not equal to departure airport.\n");
        exit(EXIT_FAILURE);
    }

    // Send message containing plane details to air traffic controller
    

    message m;
    m.mtype = 10+plane.plane_id; // Message type
    m.data.flight = plane; // Plane details
    m.data.flight.for_departure=true;
    m.data.flight.ready_for_termination=false;
    m.data.flight.for_init=false;
	
    if (msgsnd(msgid, &m, sizeof(message), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
    // Receive message from atc indicating completion of flight or shutdown of atc
    
    if (msgrcv(msgid, (void *)&m, sizeof(message),plane.plane_id+30, 0) == -1) {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
    }
    if(m.data.flight.ready_for_termination){
    	printf("ATC is Shut Down\n");
        if (sem_destroy(&mutex) == -1) {
        perror("Semaphore destruction failed");
        exit(EXIT_FAILURE);
        }
    	return 0;
    }
    

    
    // Display message indicating successful journey
    printf("Plane %ld has successfully traveled from Airport %ld to Airport %ld!\n",
           m.data.flight.plane_id, m.data.flight.departure_airport, m.data.flight.arrival_airport);
    if (sem_destroy(&mutex) == -1) {
        perror("Semaphore destruction failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// Function to create passenger processes
void createPassengerProcesses(int num_passengers, int pipefd[][2]) {
    pid_t pid;
    int passenger_weight, luggage_weight;
    int weights[2]; // Array to store body weight and luggage weight
    
    for (int i = 0; i < num_passengers; i++) {
        sem_wait(&mutex); // Wait for access
        
        pid = fork();
        if (pid < 0) {
            // Error occurred
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            close(pipefd[i][0]);
            // Prompt user to enter body weight
            printf("Enter Weight of Your Luggage: ");
            scanf("%d", &weights[1]);
            if (weights[1] < 0 || weights[1] > MAX_LUGGAGE_WEIGHT) {
                fprintf(stderr, "Invalid luggage weight. Must be between 0 and %d.\n", MAX_LUGGAGE_WEIGHT);
                exit(EXIT_FAILURE);
            }
            printf("Enter Your Body Weight: ");
            scanf("%d", &weights[0]);
            if (weights[0] < MIN_BODY_WEIGHT || weights[0] > MAX_BODY_WEIGHT) {
                fprintf(stderr, "Invalid body weight. Must be between %d and %d.\n", MIN_BODY_WEIGHT, MAX_BODY_WEIGHT);
                exit(EXIT_FAILURE);
            }
            // Prompt user to enter luggage weight
            

            // Write body weight and luggage weight to pipe
            write(pipefd[i][1], weights, sizeof(weights));
            close(pipefd[i][1]);
            exit(EXIT_SUCCESS);
        }
        
        sem_post(&mutex); // Release access
        wait(NULL);
    }
}
