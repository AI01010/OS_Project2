#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <random>
#include <chrono>
#include <fstream>

using namespace std;

// Constants
const int NUM_TELLERS = 3;
const int NUM_CUSTOMERS = 5;

// Shared resources
sem_t doorSemaphore; // Semaphore for the door (max 2 customers)
sem_t safeSemaphore; // Semaphore for the safe (max 2 tellers)
sem_t managerSemaphore; // Semaphore for manager interaction
pthread_mutex_t mutex; // Mutex for critical sections

// Queues for teller lines
queue<int> tellerLines[NUM_TELLERS];
bool tellerAvailable[NUM_TELLERS] = {true, true, true}; // Track teller availability

// Log file
ofstream logFile("logfile.txt");

// Log message formatting function
void logMessage(string threadType, int threadId, string message)
{
    string formattedMessage = threadType + " [" + to_string(threadId) + "]: " + message;
    cout << formattedMessage << endl;
    logFile << formattedMessage << endl;
}

// Customer thread function
void customerThread(int customerId)
{
    // Randomly decide transaction type
    bool isDeposit = (rand() % 2 == 0);
    int transactionAmount = rand() % 100 + 1; // Random amount between 1 and 100

    // Wait before entering the bank
    this_thread::sleep_for(chrono::milliseconds(rand() % 100));

    // Enter the bank (wait for door semaphore)
    sem_wait(&doorSemaphore);
    logMessage("Customer", customerId, "enters the bank");

    // Find an available teller or join a line
    int assignedTeller = -1;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < NUM_TELLERS; i++)
    {
        if (tellerAvailable[i])
        {
            tellerAvailable[i] = false;
            assignedTeller = i;
            break;
        }
    }
    if (assignedTeller == -1)
    {
        // No teller available, join the shortest line
        int shortestLine = 0;
        for (int i = 1; i < NUM_TELLERS; i++)
        {
            if (tellerLines[i].size() < tellerLines[shortestLine].size())
            {
                shortestLine = i;
            }
        }
        tellerLines[shortestLine].push(customerId);
        logMessage("Customer", customerId, "joins line for Teller " + to_string(shortestLine));
    }
    pthread_mutex_unlock(&mutex);

    if (assignedTeller != -1)
    {
        // Approve teller
        logMessage("Customer", customerId, "approves Teller " + to_string(assignedTeller));

        // Provide transaction info
        string transactionType = isDeposit ? "Deposit" : "Withdrawal";
        logMessage("Customer", customerId, "provides transaction info: " + transactionType + " of $" + to_string(transactionAmount));

        // Wait for teller to complete the transaction
        pthread_mutex_lock(&mutex);
        tellerLines[assignedTeller].push(customerId);
        pthread_mutex_unlock(&mutex);
    }

    // Leave the bank
    logMessage("Customer", customerId, "leaves the bank");
    sem_post(&doorSemaphore);
}

// Teller thread function
void tellerThread(int tellerId)
{
    while (true)
    {
        pthread_mutex_lock(&mutex);
        if (!tellerLines[tellerId].empty())
        {
            int customerId = tellerLines[tellerId].front();
            tellerLines[tellerId].pop();
            tellerAvailable[tellerId] = false;
            pthread_mutex_unlock(&mutex);

            // Greet the customer
            logMessage("Teller", tellerId, "greets Customer " + to_string(customerId));

            // Simulate transaction processing
            logMessage("Teller", tellerId, "begins serving Customer " + to_string(customerId));
            this_thread::sleep_for(chrono::milliseconds(rand() % 41 + 10)); // Sleep 10-50ms

            // Complete transaction
            logMessage("Teller", tellerId, "completes transaction for Customer " + to_string(customerId));

            pthread_mutex_lock(&mutex);
            tellerAvailable[tellerId] = true;
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            pthread_mutex_unlock(&mutex);
            this_thread::sleep_for(chrono::milliseconds(10)); // Avoid busy waiting
        }
    }
}

int main()
{
    srand(time(0)); // Seed for random number generation

    // Initialize semaphores and mutex
    sem_init(&doorSemaphore, 0, 2); // Max 2 customers in the bank
    sem_init(&safeSemaphore, 0, 2); // Max 2 tellers in the safe
    sem_init(&managerSemaphore, 0, 1); // Only 1 teller can interact with the manager
    pthread_mutex_init(&mutex, NULL);

    // Create teller threads
    vector<thread> tellers;
    for (int i = 0; i < NUM_TELLERS; i++)
    {
        tellers.emplace_back(tellerThread, i);
    }

    // Create customer threads
    vector<thread> customers;
    for (int i = 0; i < NUM_CUSTOMERS; i++)
    {
        customers.emplace_back(customerThread, i);
    }

    // Join customer threads
    for (auto &customer : customers)
    {
        customer.join();
    }

    // Detach teller threads
    for (auto &teller : tellers)
    {
        teller.detach();
    }

    // Clean up resources
    sem_destroy(&doorSemaphore);
    sem_destroy(&safeSemaphore);
    sem_destroy(&managerSemaphore);
    pthread_mutex_destroy(&mutex);
    logFile.close();

    return 0;
}