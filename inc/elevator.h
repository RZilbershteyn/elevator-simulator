#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <iostream>
#include <list>      // For passengers in the cabin.
#include <queue>     // For passengers queue.
#include <random>    // For random number generator

using namespace std;


#define BUILDING_HEIGHT 10
#define ELEVATOR_CAPACITY 4

// Uniform number distribution
extern mt19937 gen;
extern uniform_int_distribution<> rand_dis;

/**
 * @class Person
 * @brief The user of the elevator.
 */
class Person
{
public:
    Person();
    Person(int currFloor);
    Person(int currFloor, int desiredFloor);
    ~Person(){};

    int getCurrentFloor() const;
    int getDesiredFloor() const;

private:
    int currentFloor;
    int desiredFloor;
};


/**
 * @class Elevator
 * @brief Simulator of an elevator implemented in the singleton design.
 */
class Elevator
{
public:
    static Elevator& getInstance() {
        // If the instance doesn't exists, create it.
        if (!instance) {
            instance = new Elevator();
        }
        
        return *instance;
    }

    // Delete the copy constructor and assignment operator.
    Elevator(const Elevator&) = delete;
    Elevator& operator=(const Elevator&) = delete;


    /// Display the current state of the simulation.
    void displayState(const vector<queue<Person>> &floors) const;

    /// do 1 time step
    void proceed(vector<queue<Person>> &floors);

    void call(int floor);
    void order(int floor);
    void OpenTheDoors();
    void CloseTheDoors();
    
private:
    Elevator();
    ~Elevator(){};

    static Elevator* instance;

    enum States {
        Resting,              // 0 no passengers, no calls
        GoingUp,              // 1
        GoingDown,            // 2
        PendingDoorsToOpen,   // 3
        PendingDoorsToClose,  // 4
        LoadingPassengers,    // 5
        DroppignPassengers    // 6
    } currentState, memoryState;

    int currentFloor;
    int progress;
    int targetFloor;
    bool isDoorsOpen;
    list<Person> cabin;
    
    bool callButtons[BUILDING_HEIGHT];
    bool orderedFloors[BUILDING_HEIGHT];

    bool _getCall();
    void _updateDirection();
};

#endif