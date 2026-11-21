// EL-2003 COAL Lab, Fall 2026 - BCS-3J - Group G12
// Windows GUI and filing for the MASM game engine in GameCore.asm.
// Group: Munesh Kumar, Parshant Kumar, Anesh Kumar
// Build as a 32-bit (Win32 / x86) Windows Desktop Application.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <climits>
#include "GameState.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace {
    constexpr int W = 1100;
    constexpr int H = 790;
    constexpr int ID_NAME = 100;
    constexpr int ID_START = 101;
    constexpr int ID_UP = 102;
    constexpr int ID_DOWN = 103;
    constexpr int ID_RESCUE = 104;
    constexpr int ID_SCAN = 105;
    constexpr int ID_ENERGY = 106;
    constexpr int ID_CAPACITY = 107;
    constexpr int ID_SAVE = 108;
    constexpr int ID_LOAD = 109;
    constexpr int ID_HISTORY = 110;
    constexpr int ID_NEW = 111;
    constexpr int ID_END = 112;
    constexpr int ID_QUIT = 113;

    HWND windowHandle = nullptr;
    HWND nameEdit = nullptr;
    HWND gameButtons[7] = {};
    HFONT titleFont = nullptr;
    HFONT normalFont = nullptr;
    HFONT smallFont = nullptr;
    HFONT monoFont = nullptr;

    GameState game{};
    std::string operatorName = "Operator";
    std::string notification = "Enter your operator name, then press Start Game.";
    bool hasStarted = false;
    bool recordedResult = false;

    std::string executableFolder() {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string file(path);
        const std::string::size_type slash = file.find_last_of("\\/");
        return slash == std::string::npos ? std::string() : file.substr(0, slash + 1);
    }
    std::string savePath() { return executableFolder() + "SaveGame.txt"; }
    std::string historyPath() { return executableFolder() + "Results.csv"; }

    void showMessage(const std::string& message, const char* caption = "Elevator Rescue Simulator",
                     UINT flags = MB_OK | MB_ICONINFORMATION) {
        MessageBoxA(windowHandle, message.c_str(), caption, flags);
    }

    std::string getOperatorName() {
        char buffer[64]{};
        GetWindowTextA(nameEdit, buffer, static_cast<int>(sizeof(buffer)));
        std::string value(buffer);
        // Avoid line breaks in the text save format or the CSV file.
        value.erase(std::remove_if(value.begin(), value.end(), [](char c) {
            return c == '\r' || c == '\n' || c == '"';
        }), value.end());
        if (value.empty()) value = "Operator";
        if (value.size() > 31) value.resize(31);
        return value;
    }

    std::string statusText() {
        switch (game.status) {
        case 1: return "MISSION COMPLETE: All waiting people have been handled.";
        case 2: return "MISSION FAILED: The elevator has no energy.";
        case 3: return "MISSION FAILED: Too many passengers were lost.";
        case 4: return "Round ended by the operator.";
        default: return "Mission in progress";
        }
    }

    std::string actionText() {
        switch (game.eventCode) {
        case 1: return "Elevator moved to floor " + std::to_string(game.eventValue) + ".";
        case 2: return "The elevator is at the building limit. Energy -1.";
        case 3: return "Picked up " + std::to_string(game.eventValue) + " passenger(s).";
        case 4: return "Delivered " + std::to_string(game.eventValue) + " passenger(s) to the lobby.";
        case 5: return "Energy pack collected: +15 energy.";
        case 6: return "Nothing to pick up or unload on this floor.";
        case 7: return "Elevator full. Return to floor 0 to unload.";
        case 8: return "Scan: above " + std::to_string(game.above) +
                ", below " + std::to_string(game.below) + " passenger(s).";
        case 9: return "Purchased 20 energy for 100 credits.";
        case 10: return "Purchased 2 extra seats for 250 credits.";
        case 11: return "Not enough resources. Required: " + std::to_string(game.eventValue) + ".";
        case 12: return "Shop is only available at the lobby (floor 0).";
        case 13: return "The current round was ended.";
        default: return "Select a command to operate the elevator.";
        }
    }

    void updateButtons() {
        for (HWND button : gameButtons) if (button)
            EnableWindow(button, hasStarted && game.status == 0);
        InvalidateRect(windowHandle, nullptr, FALSE);
    }

    void startNew(bool resetRounds) {
        const int nextRound = (!resetRounds && hasStarted) ? game.roundNum + 1 : 1;
        operatorName = getOperatorName();
        GameInit(&game); // Assembly procedure
        game.roundNum = nextRound;
        hasStarted = true;
        recordedResult = false;
        notification = "New mission started. Use Up / Down and Rescue / Unload.";
        updateButtons();
    }

    bool appendResult() {
        // Creates the history CSV if it does not exist, then appends completed rounds.
        std::ifstream test(historyPath());
        const bool missingOrEmpty = !test || test.peek() == std::ifstream::traits_type::eof();
        test.close();
        std::ofstream file(historyPath(), std::ios::app);
        if (!file) return false;
        if (missingOrEmpty) file << "Date,Operator,Round,Rescued,Lost,Score,Status\n";
        SYSTEMTIME time{};
        GetLocalTime(&time);
        file << std::setfill('0') << time.wYear << '-'
             << std::setw(2) << time.wMonth << '-' << std::setw(2) << time.wDay << ' '
             << std::setw(2) << time.wHour << ':' << std::setw(2) << time.wMinute
             << std::setfill(' ') << ','
             << '"' << operatorName << '"' << ','
             << game.roundNum << ',' << game.rescued << ',' << game.lost << ','
             << game.score << ',' << '"' << statusText() << '"' << '\n';
        return static_cast<bool>(file);
    }

    void runCommand(int command) {
        if (!hasStarted || game.status != 0) return;
        GameCommand(&game, command); // Assembly procedure
        notification = actionText();
        if (game.status != 0 && !recordedResult) {
            if (!appendResult()) notification += " (Could not write Results.csv)";
            recordedResult = true;
        }
        updateButtons();
    }

    bool saveGame() {
        if (!hasStarted) {
            showMessage("Press Start Game before saving.");
            return false;
        }
        std::ofstream file(savePath(), std::ios::trunc);
        if (!file) {
            showMessage("Cannot create SaveGame.txt. Try extracting the project into a writable folder.",
                        "Save error", MB_OK | MB_ICONERROR);
            return false;
        }
        file << "ERS_G12_V1\n" << operatorName << '\n';
        file << game.roundNum << ' ' << game.currentFloor << ' ' << game.energy << ' '
             << game.credits << ' ' << game.capacity << ' ' << game.onboard << ' '
             << game.rescued << ' ' << game.lost << ' ' << game.score << ' '
             << game.status << ' ' << game.eventCode << ' ' << game.eventValue << ' '
             << game.above << ' ' << game.below << '\n';
        for (int i = 0; i < 10; ++i) file << game.floorPeople[i] << ' ';
        file << '\n';
        for (int i = 0; i < 10; ++i) file << game.floorKind[i] << ' ';
        file << '\n';
        for (int i = 0; i < 10; ++i) file << game.floorTime[i] << ' ';
        file << '\n';
        if (!file) {
            showMessage("An error occurred while writing the save file.", "Save error", MB_OK | MB_ICONERROR);
            return false;
        }
        notification = "Game saved to SaveGame.txt (beside the program EXE).";
        updateButtons();
        return true;
    }

    bool loadGame() {
        std::ifstream file(savePath());
        if (!file) {
            showMessage("No SaveGame.txt found yet. Start the game, then click Save Game.",
                        "Load game");
            return false;
        }
        std::string header, name;
        std::getline(file, header);
        std::getline(file, name);
        if (header != "ERS_G12_V1" || name.size() > 31 || name.empty()) {
            showMessage("Unknown or damaged game save file.", "Load error", MB_OK | MB_ICONERROR);
            return false;
        }
        GameState loaded{};
        if (!(file >> loaded.roundNum >> loaded.currentFloor >> loaded.energy
                   >> loaded.credits >> loaded.capacity >> loaded.onboard
                   >> loaded.rescued >> loaded.lost >> loaded.score
                   >> loaded.status >> loaded.eventCode >> loaded.eventValue
                   >> loaded.above >> loaded.below)) {
            showMessage("Game save header is incomplete.", "Load error", MB_OK | MB_ICONERROR);
            return false;
        }
        for (int& value : loaded.floorPeople) if (!(file >> value)) return showMessage("Damaged floor data.", "Load error", MB_OK | MB_ICONERROR), false;
        for (int& value : loaded.floorKind) if (!(file >> value)) return showMessage("Damaged floor types.", "Load error", MB_OK | MB_ICONERROR), false;
        for (int& value : loaded.floorTime) if (!(file >> value)) return showMessage("Damaged floor timers.", "Load error", MB_OK | MB_ICONERROR), false;

        auto valid = [](int value, int low, int high) { return value >= low && value <= high; };
        bool okay = valid(loaded.roundNum, 1, 100000) && valid(loaded.currentFloor, 0, 9) &&
            valid(loaded.energy, 0, 100000) && valid(loaded.credits, 0, 100000000) &&
            valid(loaded.capacity, 1, 100) && valid(loaded.onboard, 0, loaded.capacity) &&
            valid(loaded.rescued, 0, 10000) && valid(loaded.lost, 0, 10000) &&
            valid(loaded.score, 0, 100000000) && valid(loaded.status, 0, 4);
        for (int i = 0; i < 10; ++i) {
            okay = okay && valid(loaded.floorPeople[i], 0, 100) &&
                valid(loaded.floorKind[i], 0, 3) && valid(loaded.floorTime[i], 0, 100000);
        }
        if (!okay) {
            showMessage("The game save contains invalid values.", "Load error", MB_OK | MB_ICONERROR);
            return false;
        }
        game = loaded;
        operatorName = name;
        SetWindowTextA(nameEdit, operatorName.c_str());
        hasStarted = true;
        recordedResult = game.status != 0;
        notification = "Saved mission loaded from SaveGame.txt.";
        updateButtons();
        return true;
    }

    void showHistory() {
        std::ifstream file(historyPath());
        if (!file) {
            showMessage("No completed rounds yet. Finish a round to create Results.csv.", "Saved results");
            return;
        }
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(file, line)) {
            if (!line.empty()) lines.push_back(line);
        }
        if (lines.empty()) {
            showMessage("Results.csv is empty.", "Saved results");
            return;
        }
        std::ostringstream out;
        out << "Saved game results (latest records):\n\n";
        const size_t from = lines.size() > 11 ? lines.size() - 11 : 0;
        for (size_t i = from; i < lines.size(); ++i) out << lines[i] << '\n';
        out << "\nFull history: Results.csv beside the program EXE.";
        showMessage(out.str(), "Results history");
    }

    void drawText(HDC dc, const std::string& s, int x, int y, COLORREF color,
                  HFONT font, int width = 520, int height = 42) {
        RECT rect{x, y, x + width, y + height};
        SetTextColor(dc, color);
        SetBkMode(dc, TRANSPARENT);
        HFONT old = static_cast<HFONT>(SelectObject(dc, font));
        DrawTextA(dc, s.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, old);
    }

    void coloredRect(HDC dc, RECT r, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(dc, &r, brush);
        DeleteObject(brush);
    }

    void drawGame(HDC dc) {
        RECT background{0, 0, W, H};
        coloredRect(dc, background, RGB(241, 245, 250));
        RECT header{0, 0, W, 68};
        coloredRect(dc, header, RGB(23, 46, 77));
        drawText(dc, "ELEVATOR RESCUE SIMULATOR", 25, 18, RGB(255,255,255), titleFont, 610, 40);
        drawText(dc, "COAL LAB   |   GROUP G12   |   MASM + WINDOWS GUI + FILES",
                 620, 25, RGB(207, 226, 248), smallFont, 460, 30);

        drawText(dc, "Operator name:", 572, 88, RGB(27,49,69), normalFont, 140);
        drawText(dc, "BUILDING  /  FLOOR STATUS", 27, 105, RGB(27,49,69), normalFont);

        RECT buildingBox{22, 142, 540, 630};
        coloredRect(dc, buildingBox, RGB(255,255,255));
        if (hasStarted) {
            for (int floor = 9; floor >= 0; --floor) {
                const int rowIndex = 9 - floor;
                const int y = 149 + rowIndex * 46;
                RECT row{32, y, 529, y + 42};
                const bool elevatorHere = (game.currentFloor == floor);
                coloredRect(dc, row, elevatorHere ? RGB(221,239,249) :
                            (rowIndex % 2 ? RGB(248,250,252) : RGB(241,246,250)));
                drawText(dc, "F" + std::to_string(floor), 42, y + 9,
                         RGB(31,48,70), monoFont, 50);
                RECT shaft{91, y + 5, 153, y + 37};
                coloredRect(dc, shaft, elevatorHere ? RGB(39,124,180) : RGB(221,229,237));
                drawText(dc, elevatorHere ? "[ E ]" : "[   ]", 97, y + 9,
                         elevatorHere ? RGB(255,255,255) : RGB(98,112,127), monoFont, 57);
                std::string detail;
                COLORREF detailColor = RGB(47,68,89);
                if (floor == 0) detail = "LOBBY  /  DROP-OFF  /  SHOP";
                else if (game.floorKind[floor] == 2) detail = "ENERGY PACK  (+15)";
                else if (game.floorPeople[floor] > 0) {
                    detail = "People: " + std::to_string(game.floorPeople[floor]);
                    if (game.floorKind[floor] == 1) {
                        detail += "  URGENT";
                        detailColor = RGB(184, 49, 49);
                    }
                    detail += "   Time: " + std::to_string(game.floorTime[floor]);
                } else detail = "Clear";
                drawText(dc, detail, 164, y + 10, detailColor, monoFont, 358);
            }
        } else {
            drawText(dc, "Enter your name and click Start Game.", 60, 230,
                     RGB(65,85,107), normalFont, 415);
        }

        RECT statsBox{561, 142, 1080, 290};
        coloredRect(dc, statsBox, RGB(255,255,255));
        drawText(dc, "GAME STATUS", 575, 154, RGB(23,46,77), normalFont);
        if (hasStarted) {
            drawText(dc, "Round: " + std::to_string(game.roundNum) + "     Floor: " +
                     std::to_string(game.currentFloor), 575, 186, RGB(42,65,90), monoFont);
            drawText(dc, "Energy: " + std::to_string(game.energy) + "     Credits: " +
                     std::to_string(game.credits), 575, 213, RGB(42,65,90), monoFont);
            drawText(dc, "Aboard: " + std::to_string(game.onboard) + "/" +
                     std::to_string(game.capacity) + "   Rescued: " +
                     std::to_string(game.rescued) + "   Lost: " + std::to_string(game.lost),
                     575, 240, RGB(42,65,90), monoFont);
            drawText(dc, "Score: " + std::to_string(game.score), 575, 264,
                     RGB(20,113,75), monoFont);
        } else drawText(dc, "Waiting to start...", 575, 201, RGB(85,96,112), normalFont);

        drawText(dc, "CONTROLS", 571, 307, RGB(23,46,77), normalFont);
        drawText(dc, "Move, rescue, scan and buy supplies using buttons.", 572, 341,
                 RGB(80,99,118), smallFont);
        drawText(dc, "FILE HANDLING", 571, 478, RGB(23,46,77), normalFont);
        drawText(dc, "Save Game, Load Game, View Results", 572, 508, RGB(80,99,118), smallFont);

        RECT noticeBox{22, 648, 1080, 718};
        coloredRect(dc, noticeBox, RGB(228,238,249));
        drawText(dc, hasStarted ? statusText() : "Ready to start", 34, 661,
                 hasStarted && game.status != 0 ? RGB(150,48,48) : RGB(17,73,115),
                 normalFont, 1025);
        drawText(dc, notification, 34, 688, RGB(44,66,88), smallFont, 1010);
        drawText(dc, "Data files are created next to ElevatorRescue_G12.exe (Debug\\ or Release\\ folder).",
                 25, 731, RGB(84,105,123), smallFont, 1020);
    }

    HWND makeButton(HWND parent, int id, const char* label, int x, int y, int width = 156) {
        HWND button = CreateWindowExA(0, "BUTTON", label,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            x, y, width, 36, parent,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandleA(nullptr), nullptr);
        SendMessageA(button, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
        return button;
    }

    LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            windowHandle = wnd;
            titleFont = CreateFontA(27, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    0, 0, 0, DEFAULT_PITCH, "Segoe UI");
            normalFont = CreateFontA(19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     0, 0, 0, DEFAULT_PITCH, "Segoe UI");
            smallFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    0, 0, 0, DEFAULT_PITCH, "Segoe UI");
            monoFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   0, 0, 0, DEFAULT_PITCH, "Consolas");
            nameEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "Parshant",
                WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP,
                707, 83, 195, 30, wnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_NAME)),
                GetModuleHandleA(nullptr), nullptr);
            SendMessageA(nameEdit, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
            SendMessageA(nameEdit, EM_LIMITTEXT, 31, 0);
            makeButton(wnd, ID_START, "Start Game", 916, 81, 150);

            gameButtons[0] = makeButton(wnd, ID_UP, "Up [W]", 570, 365);
            gameButtons[1] = makeButton(wnd, ID_DOWN, "Down [S]", 740, 365);
            gameButtons[2] = makeButton(wnd, ID_RESCUE, "Rescue [A]", 910, 365);
            gameButtons[3] = makeButton(wnd, ID_SCAN, "Scan [X]", 570, 408);
            gameButtons[4] = makeButton(wnd, ID_ENERGY, "Buy Energy", 740, 408);
            gameButtons[5] = makeButton(wnd, ID_CAPACITY, "Buy Seats", 910, 408);
            gameButtons[6] = makeButton(wnd, ID_END, "End Round", 910, 551);

            makeButton(wnd, ID_SAVE, "Save Game", 570, 551);
            makeButton(wnd, ID_LOAD, "Load Game", 740, 551);
            makeButton(wnd, ID_HISTORY, "View Results", 570, 592);
            makeButton(wnd, ID_NEW, "New Round", 740, 592);
            makeButton(wnd, ID_QUIT, "Exit", 910, 592);
            updateButtons();
            return 0;
        }
        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            switch (id) {
            case ID_START: startNew(true); break;
            case ID_NEW: startNew(false); break;
            case ID_UP: runCommand(CMD_UP); break;
            case ID_DOWN: runCommand(CMD_DOWN); break;
            case ID_RESCUE: runCommand(CMD_ACTION); break;
            case ID_SCAN:
            {
                if (!hasStarted || game.status != 0)
                    break;

                runCommand(CMD_SCAN);

                if (game.eventCode == 8)
                {
                    std::string result =
                        "Passengers above: " + std::to_string(game.above) +
                        "\nPassengers below: " + std::to_string(game.below) +
                        "\nEnergy remaining: " + std::to_string(game.energy);

                    MessageBoxA(wnd, result.c_str(),
                        "Building Scan Results",
                        MB_OK | MB_ICONINFORMATION);
                }
                else if (game.eventCode == 11)
                {
                    MessageBoxA(wnd, "Not enough energy to scan!",
                        "Scan Failed",
                        MB_OK | MB_ICONWARNING);
                }

                break;
            }
            case ID_ENERGY: runCommand(CMD_BUY_ENERGY); break;
            case ID_CAPACITY: runCommand(CMD_BUY_CAPACITY); break;
            case ID_END: runCommand(CMD_END); break;
            case ID_SAVE: saveGame(); break;
            case ID_LOAD: loadGame(); break;
            case ID_HISTORY: showHistory(); break;
            case ID_QUIT: DestroyWindow(wnd); break;
            default: break;
            }
            return 0;
        }
        case WM_KEYDOWN: {
            if (GetFocus() == nameEdit) break;
            switch (wParam) {
            case 'W': runCommand(CMD_UP); return 0;
            case 'S': runCommand(CMD_DOWN); return 0;
            case 'A': runCommand(CMD_ACTION); return 0;
            case 'X': runCommand(CMD_SCAN); return 0;
            case 'D': runCommand(CMD_END); return 0;
            default: break;
            }
            break;
        }
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(wnd, &ps);
            drawGame(dc);
            EndPaint(wnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            DeleteObject(titleFont);
            DeleteObject(normalFont);
            DeleteObject(smallFont);
            DeleteObject(monoFont);
            PostQuitMessage(0);
            return 0;
        default: break;
        }
        return DefWindowProcA(wnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = instance;
    wc.lpszClassName = "ElevatorRescueG12Window";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    if (!RegisterClassExA(&wc)) return 1;

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT dimensions{0, 0, W, H};
    AdjustWindowRect(&dimensions, style, FALSE);
    HWND wnd = CreateWindowExA(0, wc.lpszClassName,
        "Elevator Rescue Simulator | COAL G12", style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        dimensions.right - dimensions.left, dimensions.bottom - dimensions.top,
        nullptr, nullptr, instance, nullptr);
    if (!wnd) return 2;
    ShowWindow(wnd, showCommand);
    UpdateWindow(wnd);
    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return static_cast<int>(message.wParam);
}
