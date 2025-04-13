/*******************************************************************************
*  CS4348 Operating Systems
*  Project 2  - Banker Simulation
*  Ashraful Islam
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// Constants
#define NUM_TELLERS 3
#define NUM_CUSTOMERS 5 //50
#define MAX_CUSTOMERS_INSIDE 2
#define MAX_SAFE_OCCUPANCY 2


// Shared resources
sem_t door_sem;     // Semaphore for the door (max 2 customers)
sem_t safe_sem;    // Semaphore for the safe (max 2 tellers)    
sem_t manager_sem;  // Semaphore for manager interaction (max 1 teller)
sem_t customers_ready;  // Semaphore for customers ready to be served
sem_t customer_done[NUM_CUSTOMERS]; // Semaphores for each customer
sem_t transaction_asked[NUM_CUSTOMERS];  // Synchronization for asking transaction


// mutexes for synchronization
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER; // printing
pthread_mutex_t line_mutex = PTHREAD_MUTEX_INITIALIZER; // line access
pthread_mutex_t served_mutex = PTHREAD_MUTEX_INITIALIZER; // served customers
// int customers_served = 0;

// // Teller and customer states
// int tellerAvailable[NUM_TELLERS] = {1, 1, 1}; // Track teller availability
// int customersRemaining = NUM_CUSTOMERS;      // Track remaining customers

// shared data
int line[NUM_CUSTOMERS];
int front = 0;
int rear = 0;
int customers_served = 0;
int transaction_type[NUM_CUSTOMERS];  // 0 = deposit, 1 = withdrawal
int teller_assigned[NUM_CUSTOMERS];  // Tracks teller for each customer

// functions to log messages
void log_action(const char *actor_type, int actor_id, const char *related_type, int related_id, const char *msg) 
{
    pthread_mutex_lock(&print_mutex);
    if (related_type == NULL) 
    {
        printf("%s %d []: %s\n", actor_type, actor_id, msg);
    } 
    else 
    {
        printf("%s %d [%s %d]: %s\n", actor_type, actor_id, related_type, related_id, msg);
    }
    pthread_mutex_unlock(&print_mutex);
}

void log_teller(int tid, const char *msg) 
{
    log_action("Teller", tid, NULL, 0, msg);
}

void log_customer(int cid, const char *msg) 
{
    log_action("Customer", cid, NULL, 0, msg);
}

void log_teller_with_customer(int tid, int cid, const char *msg) 
{
    log_action("Teller", tid, "Customer", cid, msg);
}

void log_customer_with_teller(int cid, int tid, const char *msg) 
{
    log_action("Customer", cid, "Teller", tid, msg);
}


// Customer thread function
void *customer(void *arg) 
{
    int cid = *((int *)arg);
    transaction_type[cid] = rand() % 2; // random transaction type (0 = deposit, 1 = withdrawal)

    // Log the transaction type the customer wants
    if (transaction_type[cid] == 0)
    {
        log_customer(cid, "wants to perform a deposit transaction");
    }
    else
    {
        log_customer(cid, "wants to perform a withdrawal transaction");
    }
    usleep(rand() % 101 * 1000); // Random arrival time (0-100ms)

    log_customer(cid, "going to bank");
    sem_wait(&door_sem);

    //line access
    pthread_mutex_lock(&line_mutex);
    line[rear] = cid;
    rear = (rear + 1) % NUM_CUSTOMERS;
    pthread_mutex_unlock(&line_mutex);

    // Log customer entering the bank and getting in line
    log_customer(cid, "entering bank");
    log_customer(cid, "getting in line");
    log_customer(cid, "selecting a teller");

    // Wait for teller to assign transaction
    sem_post(&customers_ready);

    // Wait for the teller to ask for transaction
    sem_wait(&transaction_asked[cid]);

    int tid = teller_assigned[cid];  // Retrieve assigned teller
    log_customer_with_teller(cid, tid, "selects teller");
    log_customer_with_teller(cid, tid, "introduces itself");

    // Log the customer asking for transaction
    if (transaction_type[cid] == 0)
    {
        log_customer_with_teller(cid, tid, "asks for deposit transaction");
    }
    else
    {
        log_customer_with_teller(cid, tid, "asks for withdrawal transaction");
    }
    
    // Wait for teller to finish transaction
    sem_wait(&customer_done[cid]);
    log_customer(cid, "leaves the bank");
    sem_post(&door_sem);

    return NULL;
}

// Teller thread function
void *teller(void *arg) 
{
    int tid = *((int *)arg);
    log_teller(tid, "ready to serve");
    // logMessage("Teller", teller_id, "is ready to serve customers");

    // if (teller_id == NUM_TELLERS - 1) 
    // {
    //     sem_post(&bank_open); // Last teller signals that the bank is open
    // }
    
    while (1) 
    {
        log_teller(tid, "waiting for a customer");
        sem_wait(&customers_ready);
        
        pthread_mutex_lock(&line_mutex);
        if (front == rear) 
        {
            pthread_mutex_unlock(&line_mutex);
            break;  // No more customers
        }
        int cid = line[front];
        front = (front + 1) % NUM_CUSTOMERS;

        teller_assigned[cid] = tid;  // Assign current teller to the customer
        log_teller_with_customer(tid, cid, "serving a customer");
        log_teller_with_customer(tid, cid, "asks for transaction");

        pthread_mutex_unlock(&line_mutex);

        // Let customer to proceed with the transaction
        sem_post(&transaction_asked[cid]); 

        // Log transaction type
        if (transaction_type[cid] == 0) 
        {
            log_teller_with_customer(tid, cid, "handling deposit transaction");
        } 
        else 
        {
            log_teller_with_customer(tid, cid, "handling withdrawal transaction");
        }

        usleep((rand() % 26 + 5) * 1000); // Simulate 5-30ms work

        if (transaction_type[cid] == 1) // widthdrawal 
        {
            log_teller_with_customer(tid, cid, "going to the manager");
            log_teller_with_customer(tid, cid, "getting manager's permission");
            sem_wait(&manager_sem);
            usleep((rand() % 26 + 5) * 1000); // Simulate 5-30ms work
            log_teller_with_customer(tid, cid, "got manager's permission");
            sem_post(&manager_sem);
        }

        // teller goes to the safe
        log_teller_with_customer(tid, cid, "going to safe");
        sem_wait(&safe_sem);
        log_teller_with_customer(tid, cid, "enter safe");
        usleep((rand() % 41 + 10) * 1000);
        log_teller_with_customer(tid, cid, "leaving safe");
        sem_post(&safe_sem);

        // Log transaction type
        if (transaction_type[cid] == 0) 
        {
            log_teller_with_customer(tid, cid, "finishes deposit transaction");
        } 
        else 
        {
            log_teller_with_customer(tid, cid, "finishes withdrawal transaction");
        }

        sem_post(&customer_done[cid]);

        pthread_mutex_lock(&served_mutex);
        if (customers_served >= NUM_CUSTOMERS) 
        {
            pthread_mutex_unlock(&served_mutex);
            break;
        }
        customers_served++;
        pthread_mutex_unlock(&served_mutex);
    }

    log_teller(tid, "leaving for the day");
    // logMessage("Teller", tellerId, "is leaving for the day");
    // pthread_exit(NULL);
    return NULL;
}

int main() 
{
    srand(time(NULL)); // Seed for random number generation

    sem_init(&door_sem, 0, MAX_DOOR_ENTRY); // Semaphore for door (max 2 customers)
    sem_init(&safe_sem, 0, MAX_SAFE_OCCUPANCY); // Semaphore for safe (max 2 tellers)
    sem_init(&manager_sem, 0, 1);           // Semaphore for manager (max 1 teller)
    sem_init(&customers_ready, 0, 0);   // Semaphore for customers ready to be served

    for (int i = 0; i < NUM_CUSTOMERS; i++) // Initialize customer semaphores
    {
        sem_init(&customer_done[i], 0, 0);
        sem_init(&transaction_asked[i], 0, 0);
    }

    // tellers and customers thread and id lists
    pthread_t tellers[NUM_TELLERS];
    pthread_t customers[NUM_CUSTOMERS];
    int teller_ids[NUM_TELLERS];
    int customer_ids[NUM_CUSTOMERS];

    // Create teller threads
    for (int i = 0; i < NUM_TELLERS; i++) 
    {
        teller_ids[i] = i;
        pthread_create(&tellers[i], NULL, teller, &teller_ids[i]);
    }

    // Create customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) 
    {
        customer_ids[i] = i;
        usleep(5000);
        pthread_create(&customers[i], NULL, customer, &customer_ids[i]);
    }

    // Wait for customer threads to finish
    for (int i = 0; i < NUM_CUSTOMERS; i++) 
    {
        pthread_join(customers[i], NULL);
    }

    // Wake up remaining tellers
    for (int i = 0; i < NUM_TELLERS; i++) 
    {
        sem_post(&customers_ready); 
    }

    // Wait for teller threads to finish
    for (int i = 0; i < NUM_TELLERS; i++) 
    {
        pthread_join(tellers[i], NULL);
    }

    // Cleanup semaphores
    sem_destroy(&door_sem);
    sem_destroy(&safe_sem);
    sem_destroy(&manager_sem);
    sem_destroy(&customers_ready);
    for (int i = 0; i < NUM_CUSTOMERS; i++) // destroy customer semaphores and transaction semaphores
    {
        sem_destroy(&customer_done[i]);
        sem_destroy(&transaction_asked[i]);
    }

    printf("Bank has closed.\n");
    // logMessage("Bank", 0, "is closed for the day");
    return 0;
}