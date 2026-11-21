#pragma once

// All fields are 32-bit integers, matching DWORD fields in GameCore.asm.
// This is the shared data structure between MASM and the C++ interface.
struct GameState {
    int roundNum;
    int currentFloor;
    int energy;
    int credits;
    int capacity;
    int onboard;
    int rescued;
    int lost;
    int score;
    int status;        // 0=playing 1=success 2=no energy 3=too many lost 4=ended
    int eventCode;     // Most recent action message
    int eventValue;
    int above;
    int below;
    int floorPeople[10];
    int floorKind[10]; // 0=normal, 1=urgent, 2=energy pack, 3=empty
    int floorTime[10];
};

static_assert(sizeof(GameState) == 176, "C++/MASM structure layout mismatch");

// Implemented entirely in x86 MASM. Compile the project for Win32/x86.
extern "C" {
    void __cdecl GameInit(GameState* game);
    void __cdecl GameCommand(GameState* game, int command);
}

enum GameCommandCode {
    CMD_UP = 1, CMD_DOWN = 2, CMD_ACTION = 3, CMD_SCAN = 4,
    CMD_BUY_ENERGY = 5, CMD_BUY_CAPACITY = 6, CMD_END = 7
};
