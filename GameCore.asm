; ------------------------------------------------------------
; EL-2003 COAL Lab - Group G12, BCS-3J
; Elevator Rescue Simulator - Assembly GAME ENGINE
; Group: Munesh Kumar, Parshant Kumar, Anesh Kumar
;
; Windows GUI and text files are in Main.cpp.
; ALL game rules below are implemented in x86 MASM.
; Build platform: Win32/x86 (not x64).
; ------------------------------------------------------------
.386
.model flat, C
option casemap:none

GAME_STATE STRUCT
    roundNum        DWORD ?
    currentFloor    DWORD ?
    energy          DWORD ?
    credits         DWORD ?
    capacity        DWORD ?
    onboard         DWORD ?
    rescued         DWORD ?
    lost            DWORD ?
    score           DWORD ?
    status          DWORD ?
    eventCode       DWORD ?
    eventValue      DWORD ?
    above           DWORD ?
    below           DWORD ?
    floorPeople     DWORD 10 DUP(?)
    floorKind       DWORD 10 DUP(?)
    floorTime       DWORD 10 DUP(?)
GAME_STATE ENDS

.code

; GameInit(GameState* p)
; Reset everything and create a fixed, repeatable building.
GameInit PROC USES eax ecx edi, p:PTR GAME_STATE
    mov edi, p
    mov ecx, SIZEOF GAME_STATE / 4
    xor eax, eax
    cld
    rep stosd

    mov edi, p
    mov [edi].GAME_STATE.roundNum, 1
    mov [edi].GAME_STATE.energy, 55
    mov [edi].GAME_STATE.credits, 150
    mov [edi].GAME_STATE.capacity, 4

    ; Floor 1: 2 normal people, time 20
    mov [edi].GAME_STATE.floorPeople[4], 2
    mov [edi].GAME_STATE.floorKind[4], 0
    mov [edi].GAME_STATE.floorTime[4], 20
    ; Floor 2: 1 urgent person, time 30
    mov [edi].GAME_STATE.floorPeople[8], 1
    mov [edi].GAME_STATE.floorKind[8], 1
    mov [edi].GAME_STATE.floorTime[8], 30
    ; Floor 3: 3 normal people, time 45
    mov [edi].GAME_STATE.floorPeople[12], 3
    mov [edi].GAME_STATE.floorTime[12], 45
    ; Floor 4: energy pack
    mov [edi].GAME_STATE.floorKind[16], 2
    ; Floor 5: 2 normal people, time 65
    mov [edi].GAME_STATE.floorPeople[20], 2
    mov [edi].GAME_STATE.floorTime[20], 65
    ; Floor 6: urgent person, time 80
    mov [edi].GAME_STATE.floorPeople[24], 1
    mov [edi].GAME_STATE.floorKind[24], 1
    mov [edi].GAME_STATE.floorTime[24], 80
    ; Floor 7: energy pack
    mov [edi].GAME_STATE.floorKind[28], 2
    ; Floor 8: 2 normal people, time 105
    mov [edi].GAME_STATE.floorPeople[32], 2
    mov [edi].GAME_STATE.floorTime[32], 105
    ; Floor 9: 1 normal person, time 125
    mov [edi].GAME_STATE.floorPeople[36], 1
    mov [edi].GAME_STATE.floorTime[36], 125
    ret
GameInit ENDP

; GameCommand(GameState* p, int command)
; Commands: 1 up, 2 down, 3 rescue/unload, 4 scan,
;           5 buy energy, 6 buy capacity, 7 end round.
GameCommand PROC USES ebx ecx edx esi edi, p:PTR GAME_STATE, command:DWORD
    mov esi, p
    cmp [esi].GAME_STATE.status, 0
    jne CommandDone
    mov [esi].GAME_STATE.eventCode, 0
    mov [esi].GAME_STATE.eventValue, 0

    mov eax, command
    cmp eax, 1
    je MoveUp
    cmp eax, 2
    je MoveDown
    cmp eax, 3
    je RescueAction
    cmp eax, 4
    je ScanAction
    cmp eax, 5
    je BuyEnergy
    cmp eax, 6
    je BuyCapacity
    cmp eax, 7
    je EndRound
    mov [esi].GAME_STATE.eventCode, 14
    jmp CommandDone

MoveUp:
    mov eax, [esi].GAME_STATE.currentFloor
    cmp eax, 9
    jge EdgeOfBuilding
    inc eax
    mov [esi].GAME_STATE.currentFloor, eax
    mov [esi].GAME_STATE.eventCode, 1
    mov [esi].GAME_STATE.eventValue, eax
    dec [esi].GAME_STATE.energy
    jmp AfterCommand

MoveDown:
    mov eax, [esi].GAME_STATE.currentFloor
    cmp eax, 0
    jle EdgeOfBuilding
    dec eax
    mov [esi].GAME_STATE.currentFloor, eax
    mov [esi].GAME_STATE.eventCode, 1
    mov [esi].GAME_STATE.eventValue, eax
    dec [esi].GAME_STATE.energy
    jmp AfterCommand

EdgeOfBuilding:
    mov [esi].GAME_STATE.eventCode, 2
    dec [esi].GAME_STATE.energy
    jmp AfterCommand

RescueAction:
    cmp [esi].GAME_STATE.currentFloor, 0
    jne RescueOnFloor
    ; At floor 0: unload everybody on the elevator.
    mov eax, [esi].GAME_STATE.onboard
    cmp eax, 0
    jle NothingHere
    add [esi].GAME_STATE.rescued, eax
    mov [esi].GAME_STATE.eventValue, eax
    mov ebx, 50
    mul ebx
    add [esi].GAME_STATE.credits, eax
    mov [esi].GAME_STATE.onboard, 0
    mov [esi].GAME_STATE.eventCode, 4
    jmp AfterCommand

RescueOnFloor:
    mov ebx, [esi].GAME_STATE.currentFloor
    shl ebx, 2
    cmp [esi].GAME_STATE.floorKind[ebx], 2
    jne CheckWaiting
    ; Collect energy pack, only once.
    add [esi].GAME_STATE.energy, 15
    mov [esi].GAME_STATE.floorKind[ebx], 3
    mov [esi].GAME_STATE.eventCode, 5
    mov [esi].GAME_STATE.eventValue, 15
    jmp AfterCommand

CheckWaiting:
    mov ecx, [esi].GAME_STATE.floorPeople[ebx]
    cmp ecx, 0
    jle NothingHere
    mov edx, [esi].GAME_STATE.capacity
    sub edx, [esi].GAME_STATE.onboard
    cmp edx, 0
    jle LiftFull
    ; Load min(waiting people, free seats).
    cmp ecx, edx
    jle LoadPassengers
    mov ecx, edx
LoadPassengers:
    add [esi].GAME_STATE.onboard, ecx
    sub [esi].GAME_STATE.floorPeople[ebx], ecx
    mov [esi].GAME_STATE.eventCode, 3
    mov [esi].GAME_STATE.eventValue, ecx
    dec [esi].GAME_STATE.energy
    cmp [esi].GAME_STATE.floorKind[ebx], 1
    jne CheckEmptyFloor
    ; Bonus credits for picking up urgent people.
    mov eax, ecx
    imul eax, 50
    add [esi].GAME_STATE.credits, eax
CheckEmptyFloor:
    cmp [esi].GAME_STATE.floorPeople[ebx], 0
    jne AfterCommand
    mov [esi].GAME_STATE.floorKind[ebx], 3
    mov [esi].GAME_STATE.floorTime[ebx], 0
    jmp AfterCommand

LiftFull:
    mov [esi].GAME_STATE.eventCode, 7
    jmp AfterCommand
NothingHere:
    mov [esi].GAME_STATE.eventCode, 6
    jmp AfterCommand

ScanAction:
    cmp [esi].GAME_STATE.energy, 2
    jl NotEnoughEnergy
    sub [esi].GAME_STATE.energy, 2
    mov [esi].GAME_STATE.above, 0
    mov [esi].GAME_STATE.below, 0
    mov ecx, 1
ScanLoop:
    cmp ecx, 10
    jge ScanComplete
    mov ebx, ecx
    shl ebx, 2
    cmp ecx, [esi].GAME_STATE.currentFloor
    jg CountAbove
    jl CountBelow
    jmp ScanNext
CountAbove:
    mov eax, [esi].GAME_STATE.floorPeople[ebx]
    add [esi].GAME_STATE.above, eax
    jmp ScanNext
CountBelow:
    mov eax, [esi].GAME_STATE.floorPeople[ebx]
    add [esi].GAME_STATE.below, eax
ScanNext:
    inc ecx
    jmp ScanLoop
ScanComplete:
    mov [esi].GAME_STATE.eventCode, 8
    jmp AfterCommand
NotEnoughEnergy:
    mov [esi].GAME_STATE.eventCode, 11
    mov [esi].GAME_STATE.eventValue, 2
    jmp AfterCommand

BuyEnergy:
    cmp [esi].GAME_STATE.currentFloor, 0
    jne ShopNotAtLobby
    cmp [esi].GAME_STATE.credits, 100
    jl CannotAffordEnergy
    sub [esi].GAME_STATE.credits, 100
    add [esi].GAME_STATE.energy, 20
    mov [esi].GAME_STATE.eventCode, 9
    mov [esi].GAME_STATE.eventValue, 20
    jmp AfterCommand
CannotAffordEnergy:
    mov [esi].GAME_STATE.eventCode, 11
    mov [esi].GAME_STATE.eventValue, 100
    jmp AfterCommand

BuyCapacity:
    cmp [esi].GAME_STATE.currentFloor, 0
    jne ShopNotAtLobby
    cmp [esi].GAME_STATE.credits, 250
    jl CannotAffordCapacity
    sub [esi].GAME_STATE.credits, 250
    add [esi].GAME_STATE.capacity, 2
    mov [esi].GAME_STATE.eventCode, 10
    mov [esi].GAME_STATE.eventValue, 2
    jmp AfterCommand
CannotAffordCapacity:
    mov [esi].GAME_STATE.eventCode, 11
    mov [esi].GAME_STATE.eventValue, 250
    jmp AfterCommand
ShopNotAtLobby:
    mov [esi].GAME_STATE.eventCode, 12
    jmp AfterCommand

EndRound:
    mov [esi].GAME_STATE.status, 4
    mov [esi].GAME_STATE.eventCode, 13
    jmp RecalculateScore

AfterCommand:
    ; Every command uses one turn: reduce all active rescue timers.
    mov ecx, 1
TickLoop:
    cmp ecx, 10
    jge DetermineStatus
    mov ebx, ecx
    shl ebx, 2
    cmp [esi].GAME_STATE.floorPeople[ebx], 0
    jle NextTimer
    cmp [esi].GAME_STATE.floorTime[ebx], 0
    jle LosePeople
    dec [esi].GAME_STATE.floorTime[ebx]
    cmp [esi].GAME_STATE.floorTime[ebx], 0
    jg NextTimer
LosePeople:
    mov eax, [esi].GAME_STATE.floorPeople[ebx]
    add [esi].GAME_STATE.lost, eax
    mov [esi].GAME_STATE.floorPeople[ebx], 0
    mov [esi].GAME_STATE.floorKind[ebx], 3
    mov [esi].GAME_STATE.floorTime[ebx], 0
NextTimer:
    inc ecx
    jmp TickLoop

DetermineStatus:
    cmp [esi].GAME_STATE.lost, 8
    jge TooManyLost
    ; Clear only when all waiting people have been handled
    ; AND no one is left aboard the elevator.
    cmp [esi].GAME_STATE.onboard, 0
    jne CheckPower
    xor edx, edx
    mov ecx, 1
CheckFloor:
    cmp ecx, 10
    jge CheckClear
    mov ebx, ecx
    shl ebx, 2
    add edx, [esi].GAME_STATE.floorPeople[ebx]
    inc ecx
    jmp CheckFloor
CheckClear:
    cmp edx, 0
    je AllCleared
CheckPower:
    cmp [esi].GAME_STATE.energy, 0
    jle OutOfEnergy
    jmp RecalculateScore
TooManyLost:
    mov [esi].GAME_STATE.status, 3
    jmp RecalculateScore
AllCleared:
    mov [esi].GAME_STATE.status, 1
    jmp RecalculateScore
OutOfEnergy:
    mov [esi].GAME_STATE.status, 2

RecalculateScore:
    ; Score = rescued * 100 + credits - lost * 50 (minimum 0).
    mov eax, [esi].GAME_STATE.rescued  
    imul eax, 100
    add eax, [esi].GAME_STATE.credits
    mov ecx, [esi].GAME_STATE.lost
    imul ecx, 50
    sub eax, ecx
    jge ScoreNonnegative
    xor eax, eax
ScoreNonnegative:
    mov [esi].GAME_STATE.score, eax
CommandDone:
    ret
GameCommand ENDP

END
