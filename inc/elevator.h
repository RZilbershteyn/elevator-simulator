#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <iostream>
#include <string>
#include <list>      // For passengers in the cabin.
#include <queue>     // For passengers queue.
#include <random>    // For random number generator
#include <assert.h>

// using namespace std;


inline constexpr int BUILDING_HEIGHT = 10;
inline constexpr int ELEVATOR_CAPACITY = 4;


// Uniform number distribution
extern std::mt19937 gen;
extern std::uniform_int_distribution<> rand_dis;

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
        static Elevator instance;        
        return instance;
    }

    // Delete the copy constructor and assignment operator.
    Elevator(const Elevator&) = delete;
    Elevator& operator=(const Elevator&) = delete;


    /// Display the current state of the simulation.
    void displayState(const std::vector<std::queue<Person>> &floors) const;

    /// do 1 time step
    void proceed(std::vector<std::queue<Person>> &floors);

    void call(int floor);
    void order(int floor);
    void OpenTheDoors();
    void CloseTheDoors();
    
private:
    Elevator();
    ~Elevator(){};

    enum States {
        Resting,  // no passengers, no calls
        GoingUp,
        GoingDown,
        PendingDoorsToOpen,
        PendingDoorsToClose,
        LoadingPassengers,
        DroppignPassengers
    } currentState, memoryState;

    int currentFloor;
    int progress;
    int targetFloor;
    bool isDoorsOpen;
    std::list<Person> cabin;
    
    bool callButtons[BUILDING_HEIGHT];
    bool orderedFloors[BUILDING_HEIGHT];

    bool _getCall();
    void _updateDirection();
};

#endif  // ELEVATOR_H