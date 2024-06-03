#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <errno.h>
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

int main(){
	key_t key=ftok("plane.c",'A');
	if (key == -1){
        printf("error in creating unique key\n");
        exit(1);
    }
    int msg_id;
    bool running_state=true;
    int plane_count=0;
	message m;
    

    // Create a message queue
    if ((msg_id = msgget(key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    int no_of_airports;
    printf("Enter the number of airports to be handled/managed:");
    scanf("%d",&no_of_airports);
    int active_airports[no_of_airports+1];
    for(int i=0;i<=no_of_airports;i++){
    	active_airports[i]=0;
    }
 
    FILE *file;
    char mess[100]; // Buffer to store the message

    // Open the file in append mode
    file = fopen("AirTrafficController.txt", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }
    char planeID[20];
    int departureAirport, arrivalAirport;
    bool check_for_closing=false;
    m.data.flight.ready_for_termination=false;
    
    while (1) {
    // msgrcv to receive message
    	
		if (msgrcv(msg_id,&m, sizeof(message),100, IPC_NOWAIT) == -1) {
		        if (errno != ENOMSG) {
		            // No message of type i, continue to the next type
		            perror("msgrcv");
		            exit(EXIT_FAILURE);
		        } 
		 }
		 else{
		 	
		 	running_state=false;
		 	check_for_closing=true;
		 	
		 }
	    for(long i=11;i<=30;i++){
	   		
		    if (msgrcv(msg_id,&m, sizeof(message),i, IPC_NOWAIT) == -1) {
		        if (errno == ENOMSG) {
		            // No message of type i, continue to the next type
		            continue;
		        } else {
		            // Other error occurred
		            perror("msgrcv");
		            exit(EXIT_FAILURE);
		        }
		    } else {
		        // Message received successfully
		        
		        if(m.mtype<=20)
			{ 
				if (m.data.flight.for_init)
				{
					plane_count++;
					
				}
				else
					{
					//msg from plane
					    
					    if (check_for_closing)
					    {
					    	m.data.flight.ready_for_termination=true;
					    	m.mtype=m.data.flight.plane_id+30;
					    	if(msgsnd(msg_id, &m, sizeof(message), 0) == -1){
								printf("error in sending message");
								exit(1);
								}
							plane_count--;
							
					    }
					    else {
					    
							active_airports[m.data.flight.departure_airport]++;
							
							if(m.data.flight.for_departure==true) m.mtype=m.data.flight.departure_airport;
								else m.mtype=m.data.flight.arrival_airport;
								
								
								if(msgsnd(msg_id, &m, sizeof(message), 0) == -1){
									printf("error in sending message");
									exit(1);
								}
							
					    }
				    
				} 
		        }
		        else if(m.mtype<=30) {
		          
		           	if(m.data.flight.for_departure==true)
                    {   
                     	m.mtype=m.data.flight.arrival_airport;
                        m.data.flight.for_departure=false;
                        active_airports[m.data.flight.departure_airport]--;
                        active_airports[m.data.flight.arrival_airport]++;
                        sprintf(mess, "Plane %ld has departed from Airport %ld and will land at Airport %ld.\n", m.data.flight.plane_id, m.data.flight.departure_airport, m.data.flight.arrival_airport);
                        fprintf(file, "%s", mess);
                        fflush(file);
                        if(msgsnd(msg_id, &m,sizeof(message), 0) == -1){
                        printf("error in sending message");
                        exit(1);
                        }
                        
                     }
                     else {
                     	m.mtype=m.data.flight.plane_id+30;
                        if(msgsnd(msg_id, &m, sizeof(message), 0) == -1){
		                    printf("error in sending message");
		                    exit(1);
                     	}
                     	active_airports[m.data.flight.arrival_airport]--;
                     	
                     	plane_count--;
				    }
				}
			}
		}
		if (plane_count==0 && check_for_closing)
			break;
		
	}
	
	m.data.flight.ready_for_termination=true;
	for(int i=1;i<=no_of_airports;i++){
		m.mtype=i;
		if(msgsnd(msg_id, &m, sizeof(message), 0) == -1){
			printf("error in sending message");
			exit(1);
        }
	}
	for(int i=0;i<30;i++) i++;
	if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(EXIT_FAILURE);
    }
	
	return 0;
}


        
        
          
            



