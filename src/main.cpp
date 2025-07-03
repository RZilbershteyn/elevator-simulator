#include <windows.h>

#include <iostream>
#include <string>
#include <chrono>
// #include <random>   // For random number generator
#include <cstdlib>  // For system()
#include <iomanip>  // For std::setw and std::setprecision>

#include "elevator.h"
#include <queue>

#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

HWND hMainWindow;

HWND hComboAction, hComboFloors;
#define IDC_COMBOACTION 2001
#define IDC_COMBOFLOORS 2002

HWND hSubmitButton;
#define IDC_SUBMITBTN   3000

WNDPROC OriginalEditProc;

#define PERSONAPEARCHANCE 10  // % chance of apearance of a new person.


// Window procedure declaration
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

// Create the elevator object.
Elevator* Elevator::instance = nullptr;
Elevator& elevator = Elevator::getInstance();

// Uniform distribution of random numbers
random_device rd;
mt19937 gen = mt19937{rd()};  // Standard mersenne_twister_engine seeded with rd()
uniform_int_distribution<> rand_dis = uniform_int_distribution<>{0, BUILDING_HEIGHT - 1};
uniform_int_distribution<> rand_dis_appear_chance = uniform_int_distribution<>{0, 100};


void appearPerson(vector<queue<Person>> &floors)
{
    if (rand_dis_appear_chance(gen) < PERSONAPEARCHANCE) {
        Person person(rand_dis(gen), rand_dis(gen));  // Create a passenger
        floors[person.getCurrentFloor()].push(person);        // Add him to the floor
        elevator.call(person.getCurrentFloor());          // Call the elevator
    }
}

void addPerson(vector<queue<Person>> &floors, int floor)
{
    Person person(floor);
    floors[person.getCurrentFloor()].push(person);
    elevator.call(person.getCurrentFloor());
}


size_t currTime = 0;
vector<queue<Person>> floors(BUILDING_HEIGHT);
queue<pair<int, int>> userActionQueue;
mutex userActionQueueMutex, endgameMutex;
condition_variable endgameCV;

bool gameRunFlag = true;
bool gameEndedFlag = false;

// Main game loop (runs in a separate thread)
void GameLoop()
{
    while (gameRunFlag)
    {
        {  // Process user actions
            lock_guard<mutex> lock(userActionQueueMutex);

            while (!userActionQueue.empty())
            {
                switch (userActionQueue.front().first)
                {
                    case 0: elevator.call(userActionQueue.front().second); break;
                    case 1: elevator.order(userActionQueue.front().second); break;
                    case 2: addPerson(floors, userActionQueue.front().second); break;
                    default: break;
                }
                userActionQueue.pop();
            }
        }

        appearPerson(floors);  // check if someone wants to use the elevator

        system("cls");                 // Clear the console.
        cout << "Time: " << currTime << '\n';  // Display elevator's local time.
        elevator.displayState(floors);         // Display elevator's state.
        this_thread::sleep_for(chrono::seconds(1));  // Simulate the process of execution of the commands by the elevator.

        elevator.proceed(floors);
        currTime++;
    }

    {
        lock_guard<mutex> lock(endgameMutex);
        gameEndedFlag = true;
        endgameCV.notify_all();  // notify about
    }
}

// Main Window Procedure (should only handle UI events)
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:  // Fixed typo
        {
            if (LOWORD(wParam) == IDC_SUBMITBTN)
            {
                int userChosenAction = 0;
                int userChosenFloor = 0;

                int idx = SendMessage(hComboAction, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR)
                    userChosenAction = idx;
                else
                    break;

                idx = SendMessage(hComboFloors, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR)
                    userChosenFloor = idx;
                else
                    break;

                {
                    lock_guard<mutex> lock(userActionQueueMutex);
                    userActionQueue.push(make_pair(userChosenAction, userChosenFloor));
                }
            }
            break;
        }

        case WM_DESTROY:
            gameRunFlag = false;  // send a pseudosignal to stop the game

            // wait until game's end is confirmed
            {
                unique_lock<mutex> lock(endgameMutex);
                endgameCV.wait(lock, []{ return gameEndedFlag;});
            }

            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void EnableConsole() {
    AllocConsole();

    FILE* fpOut;
    FILE* fpErr;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    freopen_s(&fpErr, "CONOUT$", "w", stderr);
}

// Application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    // Enable console as a display for a simulation.
    EnableConsole();

    // Register main window class
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, MainWndProc, 0, 0,
                      hInstance, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1),
                      NULL, L"MainWindowClass", NULL };
    RegisterClassExW(&wc);

    // =============================================================================================================
    // Create main window
    hMainWindow = CreateWindowW(L"MainWindowClass", L"Configuration Window", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, hInstance, NULL);

    // Create checkboxes for language selection.
    int yPos = 20, yStep = 35;
    int xPos = 20;

    // Create comboboxes for words' sphere and specifications choice.
    hComboAction = CreateWindowW(L"COMBOBOX", L"Action", WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS,
                                xPos, yPos, 90, 150, hMainWindow, (HMENU) IDC_COMBOACTION, hInstance, NULL);
    SendMessage(hComboAction, CB_ADDSTRING, 0, (LPARAM) "call");
    SendMessage(hComboAction, CB_ADDSTRING, 0, (LPARAM) "order");
    SendMessage(hComboAction, CB_ADDSTRING, 0, (LPARAM) "add passenger");

    yPos += yStep;
    hComboFloors = CreateWindowW(L"COMBOBOX", L"Floor", WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS,
                                xPos, yPos, 90, 150, hMainWindow, (HMENU) IDC_COMBOFLOORS, hInstance, NULL);
    for (int i = 0; i < BUILDING_HEIGHT; i++) {
        SendMessage(hComboFloors, CB_ADDSTRING, 0, (LPARAM) to_wstring(i).c_str());
    }

    yPos += yStep;
    hSubmitButton = CreateWindowW(L"BUTTON", L"Submit", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                              xPos, yPos, 100, 30, hMainWindow, (HMENU) IDC_SUBMITBTN, hInstance, NULL);


    // =============================================================================================================

    ShowWindow(hMainWindow, iCmdShow);
    UpdateWindow(hMainWindow);

    std::thread gameThread(GameLoop);
    gameThread.detach();  // Before destroying main window, i.e. main thread terminates, main thread sends signal to break the game loop, and waits until gameThread is terminated.

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
