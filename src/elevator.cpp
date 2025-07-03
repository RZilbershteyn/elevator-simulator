#include "elevator.h"

using namespace std;


Person::Person()
{
    currentFloor = rand_dis(gen);
    desiredFloor = rand_dis(gen);

    while (desiredFloor == currentFloor)
    {
        desiredFloor = rand_dis(gen);
    }
}

Person::Person(int currFloor)
{
    assert(0 <= currFloor && currFloor < BUILDING_HEIGHT);

    currentFloor = currFloor;
    desiredFloor = rand_dis(gen);

    while (desiredFloor == currentFloor)
    {
        desiredFloor = rand_dis(gen);
    }
}

Person::Person(int currFloor, int desiredFloor)
{
    assert(0 <= currFloor && currFloor < BUILDING_HEIGHT);
    assert(0 <= desiredFloor && desiredFloor < BUILDING_HEIGHT);
    
    currentFloor = currFloor;
    desiredFloor = desiredFloor;
}

int Person::getCurrentFloor() const
{
    return currentFloor;
}

int Person::getDesiredFloor() const
{
    return desiredFloor;
}

Elevator::Elevator()
{
    currentFloor = 0;
    progress = 0;
    targetFloor = -1;
    isDoorsOpen = false;

    for (int i = 0; i < BUILDING_HEIGHT; i++) {
        callButtons[i] = false;
        orderedFloors[i] = false;
    }

    currentState = States::Resting;
    memoryState = currentState;
}

void Elevator::displayState(const vector<queue<Person>> &floors) const
{
    for (int i = BUILDING_HEIGHT - 1; i >= 0; i--) {
        cout << i << ": ";

        for (int j = 0; j < 10 - floors[i].size(); j++) {
            cout << '-';
        }

        for (int j = 10 - floors[i].size(); j < 10; j++) {
            cout << '*';
        }

        cout << " |";

        if (currentFloor == i) {
            cout << " <-";
        }

        cout << endl;
    }
    cout << endl;
    cout << "State: ";
    switch (currentState)
    {
    case Resting:
        cout << "Resting";
        break;
    case GoingUp:
        cout << "GoingUp";
        break;
    case GoingDown:
        cout << "GoingDown";
        break;
    case PendingDoorsToOpen:
        cout << "PendingDoorsToOpen";
        break;
    case PendingDoorsToClose:
        cout << "PendingDoorsToClose";
        break;
    case LoadingPassengers:
        cout << "LoadingPassengers";
        break;
    case DroppignPassengers:
        cout << "DroppignPassengers";
        break;
    default:
        break;
    }
    cout << '\n';
    cout << "target " << (targetFloor != -1 ? to_string(targetFloor) : "") << '\n';
    cout << "cabin: " << cabin.size() << '\n';
    cout << '\n';
    cout << "Passengers: ";
    for (auto passeng : cabin) {
        cout << passeng.getDesiredFloor() << ' ';
    }
    cout << '\n' << endl;
}

void Elevator::proceed(vector<queue<Person>> &floors)
{
    switch (currentState)
    {
    case States::Resting:
        if (_getCall()) {
            if (targetFloor == currentFloor) {
                currentState = States::PendingDoorsToOpen;
            } else {
                currentState = targetFloor > currentFloor ? States::GoingUp : States::GoingDown;
            }
        }

        break;

    case States::GoingUp:
        progress++;

        if (progress == 2) {
            progress = 0;

            currentFloor++;

            if (currentFloor == targetFloor || (!floors[currentFloor].empty() && cabin.size() < ELEVATOR_CAPACITY) || callButtons[currentFloor] || orderedFloors[currentFloor]) {
                memoryState = currentState;
                currentState = States::PendingDoorsToOpen;
            }
        }

        break;

    case States::GoingDown:
        progress++;

        if (progress == 2) {
            progress = 0;

            currentFloor--;

            if (currentFloor == targetFloor || (!floors[currentFloor].empty() && cabin.size() < ELEVATOR_CAPACITY) || callButtons[currentFloor] || orderedFloors[currentFloor]) {
                memoryState = currentState;
                currentState = States::PendingDoorsToOpen;
            }
        }

        break;

    case States::PendingDoorsToOpen:
        OpenTheDoors();
        // callButtons[currentFloor] = false;
        orderedFloors[currentFloor] = false;
        currentState = cabin.empty() ? States::LoadingPassengers : States::DroppignPassengers;
        break;

    case States::PendingDoorsToClose:
        CloseTheDoors();
        callButtons[currentFloor] = false;
        _updateDirection();
        
        if (!floors[currentFloor].empty()) {
            callButtons[currentFloor] = true;
        }
        
        break;

    case States::DroppignPassengers:
        // Priority 1: if reached target floor, passengers are coming out 1 by 1
        for (list<Person>::iterator passenger = cabin.begin(); passenger != cabin.end(); passenger++) {
            if (passenger->getDesiredFloor() == currentFloor) {
                cabin.erase(passenger);
                // break;
                return;
            }
        }

        currentState = States::LoadingPassengers;

        break;

    case States::LoadingPassengers:
        // Priority 2: Passengers coming in 1 by 1
        if (!floors[currentFloor].empty() && cabin.size() < ELEVATOR_CAPACITY) {
            cabin.push_back(floors[currentFloor].front());  // get into the cabin
            order(floors[currentFloor].front().getDesiredFloor());  // set the target floor
            
            floors[currentFloor].pop();  // remove from the current floor's queue

        } else {
            currentState = States::PendingDoorsToClose;
        }

        break;

    default:
        break;
    }
}

bool Elevator::_getCall()
{
    int dist = BUILDING_HEIGHT + 1;
    targetFloor = -1;

    for (int i = 0; i < BUILDING_HEIGHT; i++) {
        if (callButtons[i]) {
            if (abs(currentFloor - i) < dist) {
                targetFloor = i;
                dist = abs(currentFloor - i);
            }
        }
    }

    if (targetFloor != -1) {
        return true;
    }

    return false;
}

void Elevator::_updateDirection()
{
    targetFloor = -1;

    if (cabin.empty()) {
        int dist = -1, currDist = BUILDING_HEIGHT + 1;

        for (int i = 0; i < BUILDING_HEIGHT; i++) {
            dist = abs(currentFloor - i);
            if (callButtons[i] && currDist > dist) {
                targetFloor = i;
                currDist = dist;
            }
        }

        if (targetFloor == -1) {
            currentState = States::Resting;
        } else if (targetFloor == currentFloor) {
            currentState = States::PendingDoorsToOpen;
        } else {
            currentState = targetFloor > currentFloor ? States::GoingUp : States::GoingDown;
        }

    } else if (memoryState == States::GoingUp) {
        for (int i = currentFloor + 1; i < BUILDING_HEIGHT; i++) {
            if (orderedFloors[i]) {
                targetFloor = i;
                currentState = States::GoingUp;
                return;
            }
        }

        for (int i = currentFloor - 1; i >= 0; i--) {
            if (orderedFloors[i]) {
                targetFloor = i;
                currentState = States::GoingDown;
                return;
            }
        }

    } else if (memoryState == States::GoingDown) {
        for (int i = currentFloor - 1; i >= 0; i--) {
            if (orderedFloors[i]) {
                targetFloor = i;
                currentState = States::GoingDown;
                return;
            }
        }

        for (int i = currentFloor + 1; i < BUILDING_HEIGHT; i++) {
            if (orderedFloors[i]) {
                targetFloor = i;
                currentState = States::GoingUp;
                return;
            }
        }
    }

    // } else {
    //     int dist = 0, currDist = BUILDING_HEIGHT + 1;

    //     for (int i = 0; i < BUILDING_HEIGHT; i++) {
    //         dist = abs(currentFloor - i);
    //         if (orderedFloors[i] && currDist > dist) {
    //             targetFloor = i;
    //             currDist = dist;
    //         }
    //     }

    //     currentState = targetFloor > currentFloor ? States::GoingUp : States::GoingDown;
    // }
}

void Elevator::call(int floor)
{
    callButtons[floor] = true;
}

void Elevator::order(int floor)
{
    if (!cabin.empty()) {
        orderedFloors[floor] = true;
    }
}

void Elevator::OpenTheDoors()
{
    isDoorsOpen = true;
}

void Elevator::CloseTheDoors()
{
    isDoorsOpen = false;
}