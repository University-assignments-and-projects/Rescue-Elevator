# Rescue-Elevator
Rescue Elevator Game 

EL-2003 COAL LAB — GROUP G12
ELEVATOR RESCUE SIMULATOR — GUI + FILE HANDLING
================================================
Team: Munesh Kumar, Parshant Kumar, Anesh Kumar
Section: BCS-3J | Fall 2026

WHAT IS INCLUDED?
-----------------
ElevatorRescue_G12.sln       Open this in Visual Studio
ElevatorRescue_G12.vcxproj   Visual Studio project configuration
ElevatorRescue_G12.vcxproj.filters  File layout
GameCore.asm                 x86 MASM game logic (movement, rescue, scan, shop, timers, score)
GameState.h                  Shared C++/Assembly game state structure
Main.cpp                     Windows GUI and filing functions
README.txt                   These instructions

BUILD AND RUN (WINDOWS)
-----------------------
1. Right-click the ZIP and select Extract All. Keep all files in one folder.
2. Double-click ElevatorRescue_G12.sln (Visual Studio, NOT VS Code).
3. Make sure 'Desktop development with C++' and MASM build tools are installed.
4. At the top select 'Debug' and 'x86' (sometimes shown as Win32).
   DO NOT use x64; GameCore.asm is 32-bit x86 Assembly.
5. Press Ctrl+Shift+B (Build Solution).
6. Press Ctrl+F5 (Start Without Debugging).
7. Enter a name, click Start Game and use the GUI buttons.

If you see error MSB8020 or a missing v145 toolset:
  Right-click project > Properties > General > Platform Toolset
  and choose your installed toolset (for example v143), then rebuild.
If 'masm.props' cannot be found, install MSVC C++ build tools / MASM.

HOW TO PLAY
-----------
Start Game       Enter name and begin a mission
Up / Down        Move elevator one floor (1 energy each)
Rescue / Unload  Pick up passengers or unload at floor 0
Scan             See waiting people above and below (2 energy)
Buy Energy       Only floor 0: +20 energy for 100 credits
Buy Seats        Only floor 0: +2 capacity for 250 credits
Save Game        Saves current game to SaveGame.txt
Load Game        Reads state back from SaveGame.txt
View Results     Reads Results.csv (completed rounds)
New Round        Restart a new round
End Round        Finish and record the round

GUI controls are buttons. Keyboard shortcuts W/S/A/X/D may work when focus
is on the main window. Buttons always work.

FILE HANDLING
-------------
The program writes two readable text files in its executable folder:
  Debug\SaveGame.txt or Release\SaveGame.txt
  Debug\Results.csv  or Release\Results.csv

SaveGame.txt: Contains operator name, game status, current floor, energy,
credits, passengers, arrays for floor people/types/timers, etc.
Load Game: Reloads and validates that same state.
Results.csv: Adds date, operator, rescued/lost and score when a round ends.
View Results: Reads back the recent records.
Open these files in Notepad; Results.csv also opens in Excel.

IMPORTANT
---------
- This is a newly structured GUI version that uses MASM for the game engine
  and Win32 C++ for the GUI/file handling, as the instructor permitted.
- It is NOT the exact same .asm-only console executable from before.
- This source package was prepared without a Windows Visual Studio build
  available in the creation environment. Compile it on your own laptop,
  test Save/Load and Results, and fix any machine-specific setup errors.
- Your instructor requires disclosure of AI-generated code and expects
  each group member to understand what they present during the viva.

QUICK SMOKE TEST
----------------
1. Start. Look for 10 floors; lobby is floor 0.
2. Click Up -> now floor 1.
3. Click Rescue -> pick up 2 passengers.
4. Click Down -> back to lobby.
5. Click Rescue -> unloaded: rescued count becomes 2.
6. Click Save Game. Move Up again. Click Load Game -> returns to saved floor.
7. Click End Round. Click View Results -> the round is recorded.

This tests the GUI, Assembly integration, and both file read/write features.
