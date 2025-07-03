#include <windows.h>
#include "elevator.h"

#include <iostream>
#include <string>
#include <vector>
#include <queue>    // For passengers queue and user actions queue
#include <chrono>   // For chrono::seconds()
#include <random>   // For random number generator
#include <cstdlib>  // For system()

#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;


inline constexpr float PERSONAPEARCHANCE = 10.0;  // % a new person apearance chance.


HWND hMainWindow;

HWND hComboAction, hComboFloors;
#define IDC_COMBOACTION 2001
#define IDC_COMBOFLOORS 2002

HWND hSubmitButton;
#define IDC_SUBMITBTN   3000

WNDPROC OriginalEditProc;

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

size_t currTime = 0;
vector<queue<Person>> floors(BUILDING_HEIGHT);
queue<pair<int, int>> userActionQueue;  // Queue of pairs of actionIdx and floorIdx
mutex userActionQueueMutex, endgameMutex;
condition_variable endgameCV;

bool gameRunFlag = true;
bool gameEndedFlag = false;


/**
 * @brief On every call creates a person on a random floor from the given floors with a chance of PERSONAPEARCHANCE %.
 * @param floors
 */
void appearPerson(vector<queue<Person>> &floors)
{
    if (rand_dis_appear_chance(gen) < PERSONAPEARCHANCE) {
        Person person(rand_dis(gen), rand_dis(gen));  // Create a passenger
        floors[person.getCurrentFloor()].push(person);        // Add him to the floor
        elevator.call(person.getCurrentFloor());          // Call the elevator
    }
}


/**
 * @brief Creates a person on the floorIdx-th floor of the floors.
 * @param floors
 * @param floorIdx Index of the floor a person to be created. If floorIdx exceeds [0, BUILDING_HEIGHT) range, does nothing.
 */
void addPerson(vector<queue<Person>> &floors, int floorIdx)
{
    if (0 <= floorIdx && floorIdx < BUILDING_HEIGHT) {
        Person person(floorIdx);
        floors[person.getCurrentFloor()].push(person);
        elevator.call(person.getCurrentFloor());
    }
}


/**
 * @brief Main game loop (runs in a separate thread)
 */
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

    // notify about the termination of secondary (simulation) thread
    {
        lock_guard<mutex> lock(endgameMutex);
        gameEndedFlag = true;
        endgameCV.notify_all();
    }
}

/**
 * @brief Main Window Procedure (should only handle UI events)
 * @param hwnd 
 * @param msg 
 * @param wParam 
 * @param lParam 
 * @return 
 */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
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

/**
 * @brief Application entry point
 * @param hInstance 
 * @param hPrevInstance 
 * @param szCmdLine 
 * @param iCmdShow 
 * @return 
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    // Validate predefined constants
    assert(BUILDING_HEIGHT > 0);
    assert(ELEVATOR_CAPACITY > 0);
    assert(0 <= PERSONAPEARCHANCE && PERSONAPEARCHANCE <= 100);


    // Enable console as a display for a simulation.
    EnableConsole();

    // Register main window class
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, MainWndProc, 0, 0,
                      hInstance, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW+1),
                      NULL, L"MainWindowClass", NULL };
    RegisterClassExW(&wc);

    // ============================================= Create main window =============================================
    hMainWindow = CreateWindowW(L"MainWindowClass", L"Configuration Window", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, NULL, NULL, hInstance, NULL);

    // Create checkboxes for language selection.
    int yPos = 20, yStep = 35;
    int xPos = 20;

    // Create comboboxe for type of the action choice.
    hComboAction = CreateWindowW(L"COMBOBOX", L"Action", WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS,
                                xPos, yPos, 90, 150, hMainWindow, (HMENU) IDC_COMBOACTION, hInstance, NULL);
    SendMessage(hComboAction, CB_ADDSTRING, 0, (LPARAM) "call");           // Push the elevator call button on the chosen floor.
    SendMessage(hComboAction, CB_ADDSTRING, 0, (LPARAM) "order");          // Push the desired floor button in the elevator's cabin.
    SendMessage(hComboAction, CB_ADDSTRING, 0, (LPARAM) "add passenger");  // Add a passenger on the chosen dloor.


    // Create comboboxe for floor choice.
    yPos += yStep;
    hComboFloors = CreateWindowW(L"COMBOBOX", L"Floor", WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | CBS_HASSTRINGS,
                                xPos, yPos, 90, 150, hMainWindow, (HMENU) IDC_COMBOFLOORS, hInstance, NULL);
    for (int i = 0; i < BUILDING_HEIGHT; i++) {
        SendMessage(hComboFloors, CB_ADDSTRING, 0, (LPARAM) to_wstring(i).c_str());
    }


    // Create submit button for the chosen action submition.
    yPos += yStep;
    hSubmitButton = CreateWindowW(L"BUTTON", L"Submit", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                              xPos, yPos, 100, 30, hMainWindow, (HMENU) IDC_SUBMITBTN, hInstance, NULL);


    // ==============================================================================================================

    ShowWindow(hMainWindow, iCmdShow);
    UpdateWindow(hMainWindow);

    std::thread gameThread(GameLoop);  // Create secondary thread to run simulation, while the main thread processes user inputs.
    gameThread.detach();  // Before destroying main window, i.e. main thread terminates, main thread sends signal to break the game loop, and waits until gameThread is terminated.

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}