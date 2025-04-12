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

// Constants
#define NUM_TELLERS 3
#define NUM_CUSTOMERS 5

// Shared resources
sem_t bank_open;
sem_t safe_access;      // Semaphore for the safe (max 2 tellers)
sem_t manager_access;   // Semaphore for manager interaction (max 1 teller)
sem_t door_access;     // Semaphore for the door (max 2 customers)
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
int customers_served = 0;

// // Teller and customer states
// int tellerAvailable[NUM_TELLERS] = {1, 1, 1}; // Track teller availability
// int customersRemaining = NUM_CUSTOMERS;      // Track remaining customers

// Function to log messages
void logMessage(const char *threadType, int threadId, const char *message) 
{
    printf("%s [%d]: %s\n", threadType, threadId, message);
}

// Customer thread function
void *customer(void *arg) 
{
    int customer_id = *(int *)arg;
    usleep(rand() % 100000); // wait before entering

    sem_wait(&door_access);
    printf("Customer %d: Entering bank\n", customer_id);
    logMessage("Customer", customer_id, "has entered the bank");
    sem_post(&door_access);

    pthread_exit(NULL);
}

// Teller thread function
void *teller(void *arg) 
{
    int teller_id = *(int *)arg;
    printf("Teller %d: Ready to serve!\n", teller_id);
    logMessage("Teller", teller_id, "is ready to serve customers");

    if (teller_id == NUM_TELLERS - 1) 
    {
        sem_post(&bank_open); // Last teller signals that the bank is open
    }
    
    while (1) 
    {
        pthread_mutex_lock(&queue_mutex);
        if (customers_served >= NUM_CUSTOMERS) 
        {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        customers_served++;
        pthread_mutex_unlock(&queue_mutex);

        printf("Teller %d: Waiting for customer\n", teller_id);
        logMessage("Teller", teller_id, "is waiting for a customer");
        usleep(rand() % 1000);

        int transaction = rand() % 2; // 0: Deposit, 1: Withdraw
        printf("Teller %d: Processing transaction\n", teller_id);
        logMessage("Teller", teller_id, "is processing a transaction");

        if (transaction == 1) 
        {
            sem_wait(&manager_access);
            printf("Teller %d: Requesting withdrawal permission\n", teller_id);
            logMessage("Teller", teller_id, "is requesting withdrawal permission from the manager");
            usleep((rand() % 25 + 5) * 1000);
            printf("Teller %d: Permission granted\n", teller_id);
            logMessage("Teller", teller_id, "has received permission from the manager");
            sem_post(&manager_access);
        }

        sem_wait(&safe_access);
        printf("Teller %d: Accessing safe\n", teller_id);
        logMessage("Teller", teller_id, "is accessing the safe");
        usleep((rand() % 40 + 10) * 1000);
        printf("Teller %d: Transaction complete\n", teller_id);
        logMessage("Teller", teller_id, "has completed the transaction");
        sem_post(&safe_access);
    }

    // logMessage("Teller", tellerId, "is leaving for the day");
    pthread_exit(NULL);
}

int main() 
{
    srand(time(NULL)); // Seed for random number generation

    // Initialize semaphores and mutex
    sem_init(&safe_access, 0, 2);            // Max 2 tellers in the safe
    sem_init(&manager_access, 0, 1);        // Only 1 teller can interact with the manager
    sem_init(&door_access, 0, 2);        // Max 2 customers in the door
    sem_init(&bank_open, 0, 0);           

    pthread_t tellers[NUM_TELLERS], customers[NUM_CUSTOMERS];
    int teller_ids[NUM_TELLERS], customer_ids[NUM_CUSTOMERS];

    
    // Create teller threads
    for (int i = 0; i < NUM_TELLERS; i++) 
    {
        teller_ids[i] = i;
        pthread_create(&tellers[i], NULL, teller, &teller_ids[i]);
    }
    
    sem_wait(&bank_open); // Wait for bank to open

    // Create customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) 
    {
        customer_ids[i] = i;
        pthread_create(&customers[i], NULL, customer, &customer_ids[i]);
    }

    // Wait for all customer threads to finish
    for (int i = 0; i < NUM_CUSTOMERS; i++) 
    {
        pthread_join(customers[i], NULL);
    }

    // Wait for all teller threads to finish
    for (int i = 0; i < NUM_TELLERS; i++) 
    {
        pthread_join(tellers[i], NULL);
    }

    // Clean up resources
    sem_destroy(&door_access);
    sem_destroy(&safe_access);
    sem_destroy(&manager_access);
    sem_destroy(&bank_open);
    pthread_mutex_destroy(&queue_mutex);
    
    printf("Bank has closed.\n");
    logMessage("Bank", 0, "is closed for the day");
    return 0;
}