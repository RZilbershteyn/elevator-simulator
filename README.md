# elevator-simulator
Simulating the elevator. Implemented using OOP C++, WinAPI and thread library.

- Height of the building, i.e. number of floors in the building, may be set via BUILDING_HEIGHT const expression in elevator.h.
- Elevator's capacity, i.e. maximum number of people that can be in the elevator's cabin at the same time, may be set via ELEVATOR_CAPACITY const expression in elevator.h.
- Chance of passenger to appear on some floor is set via PERSONAPEARCHANCE const expression in main.cpp.

To compile the program use:
g++ -mwindows src\main.cpp src\elevator.cpp -Iinc -o elevator.exe