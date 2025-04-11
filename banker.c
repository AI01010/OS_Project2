#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Constants
#define NUM_TELLERS 3
#define NUM_CUSTOMERS 5
#define MAX_DOOR_CAPACITY 2

// Shared resources
sem_t doorSemaphore;       // Semaphore for the door (max 2 customers)
sem_t safeSemaphore;       // Semaphore for the safe (max 2 tellers)
sem_t managerSemaphore;    // Semaphore for manager interaction
pthread_mutex_t mutex;     // Mutex for critical sections

// Teller and customer states
int tellerAvailable[NUM_TELLERS] = {1, 1, 1}; // Track teller availability
int customersRemaining = NUM_CUSTOMERS;      // Track remaining customers

// Function to log messages
void logMessage(const char *threadType, int threadId, const char *message) 
{
    printf("%s [%d]: %s\n", threadType, threadId, message);
}

// Customer thread function
void *customerThread(void *arg) 
{
    int customerId = *(int *)arg;
    free(arg);

    // Randomly decide transaction type
    int isDeposit = rand() % 2;
    int transactionAmount = rand() % 100 + 1; // Random amount between 1 and 100

    // Wait before entering the bank
    usleep(rand() % 100000); // Sleep for 0-100ms

    // Enter the bank (wait for door semaphore)
    sem_wait(&doorSemaphore);
    logMessage("Customer", customerId, "enters the bank");

    // Find an available teller
    int assignedTeller = -1;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < NUM_TELLERS; i++) 
    {
        if (tellerAvailable[i]) 
        {
            tellerAvailable[i] = 0; // Mark teller as busy
            assignedTeller = i;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    if (assignedTeller != -1) 
    {
        // Approach and greet the teller
        logMessage("Customer", customerId, "approaches Teller");
        logMessage("Teller", assignedTeller, "greets Customer");

        // Teller asks for transaction info
        logMessage("Teller", assignedTeller, "asks for transaction info");
        if (isDeposit) 
        {
            logMessage("Customer", customerId, "requests Deposit");
            logMessage("Teller", assignedTeller, "completes Deposit transaction");
        } 
        else 
        {   
            logMessage("Customer", customerId, "requests Withdrawal");

            // Teller approaches manager for permission
            sem_wait(&managerSemaphore);
            logMessage("Teller", assignedTeller, "asks Manager for permission");
            usleep(rand() % 100000); // Simulate manager interaction
            logMessage("Manager", 0, "grants permission");
            sem_post(&managerSemaphore);

            // Teller accesses the safe
            sem_wait(&safeSemaphore);
            logMessage("Teller", assignedTeller, "enters the safe");
            usleep(rand() % 100000); // Simulate safe access
            logMessage("Teller", assignedTeller, "exits the safe");
            sem_post(&safeSemaphore);

            logMessage("Teller", assignedTeller, "completes Withdrawal transaction");
        }

        // Customer leaves the teller
        logMessage("Customer", customerId, "leaves Teller");
        pthread_mutex_lock(&mutex);
        tellerAvailable[assignedTeller] = 1; // Mark teller as available
        pthread_mutex_unlock(&mutex);
    }

    // Exit the bank
    logMessage("Customer", customerId, "exits the bank");
    sem_post(&doorSemaphore);

    // Decrement remaining customers
    pthread_mutex_lock(&mutex);
    customersRemaining--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

// Teller thread function
void *tellerThread(void *arg) 
{
    int tellerId = *(int *)arg;
    free(arg);

    logMessage("Teller", tellerId, "is ready to serve");

    while (1) 
    {
        pthread_mutex_lock(&mutex);
        if (customersRemaining <= 0) 
        {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);

        usleep(10000); // Simulate waiting for customers
    }

    logMessage("Teller", tellerId, "is leaving for the day");
    return NULL;
}

int main() 
{
    srand(time(NULL)); // Seed for random number generation

    // Initialize semaphores and mutex
    sem_init(&doorSemaphore, 0, MAX_DOOR_CAPACITY); // Max 2 customers in the bank
    sem_init(&safeSemaphore, 0, 2);                // Max 2 tellers in the safe
    sem_init(&managerSemaphore, 0, 1);             // Only 1 teller can interact with the manager
    pthread_mutex_init(&mutex, NULL);

    // Create teller threads
    pthread_t tellers[NUM_TELLERS];
    for (int i = 0; i < NUM_TELLERS; i++) 
    {
        int *tellerId = malloc(sizeof(int));
        *tellerId = i;
        pthread_create(&tellers[i], NULL, tellerThread, tellerId);
    }

    // Create customer threads
    pthread_t customers[NUM_CUSTOMERS];
    for (int i = 0; i < NUM_CUSTOMERS; i++) 
    {
        int *customerId = malloc(sizeof(int));
        *customerId = i;
        pthread_create(&customers[i], NULL, customerThread, customerId);
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
    sem_destroy(&doorSemaphore);
    sem_destroy(&safeSemaphore);
    sem_destroy(&managerSemaphore);
    pthread_mutex_destroy(&mutex);

    logMessage("Bank", 0, "is closed for the day");
    return 0;
}