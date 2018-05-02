// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include <functional>
#include "TPGameDemoGameMode.h"
#include "TPGameDemoGameState.h"

void ARoomBuilder::BuildRoom_Implementation(const TArray<int>& doorPositionsOnWalls){}

void ARoomBuilder::DestroyRoom_Implementation(){}

void AWallBuilder::BuildSouthWall_Implementation(){}

void AWallBuilder::BuildWestWall_Implementation(){}

void AWallBuilder::DestroySouthWall_Implementation(){}

void AWallBuilder::DestroyWestWall_Implementation(){}

void AWallBuilder::SpawnSouthDoor_Implementation(){}

void AWallBuilder::SpawnWestDoor_Implementation(){}

void AWallBuilder::DestroySouthDoor_Implementation(){}

void AWallBuilder::DestroyWestDoor_Implementation(){}

ATPGameDemoGameState::ATPGameDemoGameState(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

ATPGameDemoGameState::~ATPGameDemoGameState()
{
    for(int i = 0; i < RoomBuilders.Num(); ++i)
    {
        TArray<ARoomBuilder*>& builderRow = RoomBuilders[i];
        for (int c = 0; c < builderRow.Num(); ++c)
        {
            builderRow[c] = nullptr;
        }
    }
    RoomBuilders.Empty();

    for(int i = 0; i < WallBuilders.Num(); ++i)
    {
        TArray<AWallBuilder*>& builderRow = WallBuilders[i];
        for (int c = 0; c < builderRow.Num(); ++c)
        {
            builderRow[c] = nullptr;
        }
    }
    WallBuilders.Empty();
}

void ATPGameDemoGameState::InitialiseArrays()
{
    for (int x = 0; x < NumGridsXY; ++x)
    {
        TArray<WallStateCouple> wallsRow;
        TArray<RoomState> roomsRow;
        TArray<ARoomBuilder*> roomBuilderRow;
        TArray<AWallBuilder*> wallBuilderRow;
        for (int y = 0; y < NumGridsXY; ++y)
        {
            wallsRow.Add(WallStateCouple());
            roomsRow.Add(RoomState());
            roomBuilderRow.Add(nullptr);
            wallBuilderRow.Add(nullptr);
        }
        // Add one final wall couple (where the west wall will be the east wall of the final room, and the south wall will be ignored).
        wallsRow.Add(WallStateCouple());
        wallBuilderRow.Add(nullptr);
        RoomStates.Add(roomsRow);
        WallStates.Add(wallsRow);
        RoomBuilders.Add(roomBuilderRow);
        WallBuilders.Add(wallBuilderRow);
    }
    // Add the last row of wall couples (where the south wall will be the north wall of the final room, and the west wall will be ignored).
    TArray<WallStateCouple> wallsRow;
    TArray<AWallBuilder*> wallBuilderRow;
    for (int y = 0; y < NumGridsXY + 1; ++y)
    {
        wallsRow.Add(WallStateCouple());
        wallBuilderRow.Add(nullptr);
    }
    WallStates.Add(wallsRow);
    WallBuilders.Add(wallBuilderRow);
}

void ATPGameDemoGameState::Tick( float DeltaTime )
{
    for (auto wallPosition : WallsToUpdate)
    {
        auto roomCoords = wallPosition.WallCoupleCoords;
        auto wallType = wallPosition.WallType;
        auto wallState = GetWallState(roomCoords, wallType);
        auto neighbourCoords = GetRoomCoords(GetNeighbouringRoomIndices(roomCoords, wallType));
        bool roomExists = DoesRoomExist(roomCoords);
        bool neighbourExists = DoesRoomExist(neighbourCoords); 

        if (!(roomExists || neighbourExists))
        {
            DisableWallState(roomCoords, wallType);
            continue;
        }

        bool roomTrained = IsRoomTrained(roomCoords);
        bool neighbourTrained = IsRoomTrained(neighbourCoords);

        EnableWallState(roomCoords, wallType);
        if (roomTrained && neighbourTrained)
            DisableDoorState(roomCoords, wallType);
        else
            EnableDoorState(roomCoords, wallType);
    }
    WallsToUpdate.Empty();
}

//============================================================================
// Acessors
//============================================================================

bool ATPGameDemoGameState::DoesRoomExist(FIntPoint roomCoords) const
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomStates[roomIndices.X][roomIndices.Y].RoomExists();
}

bool ATPGameDemoGameState::IsRoomTrained(FIntPoint roomCoords) const
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomStates[roomIndices.X][roomIndices.Y].RoomStatus == RoomState::Status::Trained;
}

bool ATPGameDemoGameState::DoesWallExist(FIntPoint roomCoords, EDirectionType wallType)
{
    auto wallState = GetWallState(roomCoords, wallType);
    return wallState.bWallExists;
}

bool ATPGameDemoGameState::DoesDoorExist(FIntPoint roomCoords, EDirectionType wallType)
{
    auto wallState = GetWallState(roomCoords, wallType);
    if (!wallState.bWallExists && wallState.bDoorExists)
        ensure(false); // Invalid state!
    return wallState.bWallExists && wallState.bDoorExists;
}

float ATPGameDemoGameState::GetRoomHealth(FIntPoint roomCoords) const
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomStates[roomIndices.X][roomIndices.Y].RoomHealth;
}

int ATPGameDemoGameState::GetDoorPositionOnWall(FIntPoint roomCoords, EDirectionType wallType)
{
    WallState& wallState = GetWallState(roomCoords, wallType);
    return wallState.DoorPosition;
}

EQuadrantType ATPGameDemoGameState::GetQuadrantTypeForRoomCoords(FIntPoint roomCoords) const
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    int x = roomIndices.X;
    int y = roomIndices.Y;
    if(x >= NumGridsXY / 2)
    {
        if (y >= NumGridsXY / 2)
        {
            return EQuadrantType::NorthEast;
        }
        return EQuadrantType::NorthWest;
    }
    else if (y < NumGridsXY / 2)
    {
        return EQuadrantType::SouthWest;
    }
    return EQuadrantType::SouthEast;
}

TArray<WallState*> ATPGameDemoGameState::GetWallStatesForRoom(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    TArray<WallState*> wallStates;
    wallStates.Empty();
    auto northNeighbourIndices = GetNeighbouringRoomIndices(roomCoords, EDirectionType::North);
    auto eastNeighbourIndices = GetNeighbouringRoomIndices(roomCoords, EDirectionType::East);
    wallStates.Add(&WallStates[northNeighbourIndices.X][northNeighbourIndices.Y].SouthWall);
    wallStates.Add(&WallStates[eastNeighbourIndices.X][eastNeighbourIndices.Y].WestWall);
    wallStates.Add(&WallStates[roomIndices.X][roomIndices.Y].SouthWall);
    wallStates.Add(&WallStates[roomIndices.X][roomIndices.Y].WestWall);
    return wallStates;
}

FIntPoint ATPGameDemoGameState::GetNeighbouringRoomIndices(FIntPoint roomCoords, EDirectionType neighbourPosition) const
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));

    switch(neighbourPosition)
    {
        case EDirectionType::North: return FIntPoint(roomIndices.X + 1, roomIndices.Y);
        case EDirectionType::East:  return FIntPoint(roomIndices.X, roomIndices.Y + 1);
        case EDirectionType::South: return FIntPoint(roomIndices.X - 1, roomIndices.Y);
        case EDirectionType::West:  return FIntPoint(roomIndices.X, roomIndices.Y - 1);
        default: ensure(false); return FIntPoint(-1, -1);
    }
}

TArray<bool> ATPGameDemoGameState::GetNeighbouringRoomStates(FIntPoint roomCoords) const
{
    TArray<bool> neighbouringRoomStates{false, false, false, false};
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    
    for (int p = 0; p < (int)EDirectionType::NumDirectionTypes; ++p)
    {
        FIntPoint neighbour = GetNeighbouringRoomIndices(roomCoords, (EDirectionType)p);
        if (RoomXYIndicesValid(neighbour) && RoomStates[neighbour.X][neighbour.Y].RoomExists())
            neighbouringRoomStates[p] = true;
    }
    return neighbouringRoomStates;
}

TArray<int> ATPGameDemoGameState::GetDoorPositionsForExistingNeighbours(FIntPoint roomCoords)
{
    TArray<bool> neighbourStates = GetNeighbouringRoomStates(roomCoords);
    TArray<int> doorPositions = {0,0,0,0};
    for (EDirectionType p = EDirectionType::North; p < EDirectionType::NumDirectionTypes; p = (EDirectionType)((int)p + 1))
    {
        if (neighbourStates[(int)p])
        {
            EDirectionType direction = (EDirectionType)p;
            WallState& wallState = GetWallState(roomCoords, direction);
            doorPositions[(int)p] = wallState.DoorPosition;
        }
    }
    return doorPositions;
}

AActor* ATPGameDemoGameState::GetRoomBuilder(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomBuilders[roomIndices.X][roomIndices.Y];
}

AWallBuilder* ATPGameDemoGameState::GetWallBuilder(FIntPoint roomCoords, EDirectionType direction)
{
    FIntPoint wallRoomIndices = GetRoomXYIndices(roomCoords);
    ensure(WallXYIndicesValid(wallRoomIndices));
    if (direction == EDirectionType::South || direction == EDirectionType::West)
        return WallBuilders[wallRoomIndices.X][wallRoomIndices.Y];
    FIntPoint neighbourRoom = GetNeighbouringRoomIndices(roomCoords, direction);
    return WallBuilders[neighbourRoom.X][neighbourRoom.Y];
}

//============================================================================
// Modifiers
//============================================================================

void ATPGameDemoGameState::SetRoomHealth(FIntPoint roomCoords, float health)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomStates[roomIndices.X][roomIndices.Y].RoomHealth = health;
    if (RoomStates[roomIndices.X][roomIndices.Y].RoomHealth <= 0.0f && 
        RoomStates[roomIndices.X][roomIndices.Y].RoomExists())
        DisableRoomState(roomCoords);
}

void ATPGameDemoGameState::UpdateRoomHealth(FIntPoint roomCoords, float healthDelta)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomStates[roomIndices.X][roomIndices.Y].RoomHealth += healthDelta;
    if (RoomStates[roomIndices.X][roomIndices.Y].RoomHealth <= 0.0f && 
        RoomStates[roomIndices.X][roomIndices.Y].RoomExists())
        DisableRoomState(roomCoords);
}

void ATPGameDemoGameState::SetRoomBuilder(FIntPoint roomCoords, ARoomBuilder* roomBuilderActor)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomBuilders[roomIndices.X][roomIndices.Y] = roomBuilderActor;
}

void ATPGameDemoGameState::SetWallBuilder(FIntPoint roomCoords, AWallBuilder* builder)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(WallXYIndicesValid(roomIndices));
    WallBuilders[roomIndices.X][roomIndices.Y] = builder;
}

void ATPGameDemoGameState::EnableRoomState(FIntPoint roomCoords)
{
    // initialize random door positions for walls that haven't yet generated their door positions....
    auto wallStates = GetWallStatesForRoom(roomCoords);

    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();
    if (gameMode != nullptr)
    {
        for(int p = 0; p < (int)EDirectionType::NumDirectionTypes; ++p)
        {
            auto wallState = wallStates[p];
            if(!wallState->HasDoor())
            {
                EDirectionType direction = (EDirectionType)p;
                int maxDoorPosition = (direction == EDirectionType::North || direction == EDirectionType::South) ? gameMode->NumGridUnitsY - 2
                                                                                                                 : gameMode->NumGridUnitsX - 2;
                wallState->GenerateRandomDoorPosition(maxDoorPosition);
            }
        }
    }

    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    if (!DoesRoomExist(roomCoords))
    {
        RoomStates[roomIndices.X][roomIndices.Y].InitializeRoom();

        RoomBuilders[roomIndices.X][roomIndices.Y]->BuildRoom({wallStates[(int)EDirectionType::North]->DoorPosition,
                                                               wallStates[(int)EDirectionType::East]->DoorPosition,
                                                               wallStates[(int)EDirectionType::South]->DoorPosition,
                                                               wallStates[(int)EDirectionType::West]->DoorPosition});
    
        FlagWallsForUpdate(roomCoords);
    }
}

void ATPGameDemoGameState::DisableRoomState(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    if (DoesRoomExist(roomCoords))
    {
        RoomStates[roomIndices.X][roomIndices.Y].DisableRoom();
        RoomBuilders[roomIndices.X][roomIndices.Y]->DestroyRoom();
        FlagWallsForUpdate(roomCoords);
    }
}

void ATPGameDemoGameState::SetRoomTrained(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    if (DoesRoomExist(roomCoords))
    {
        RoomStates[roomIndices.X][roomIndices.Y].SetRoomTrained(true);
        FlagWallsForUpdate(roomCoords);
    }
}

void ATPGameDemoGameState::EnableWallState(FIntPoint roomCoords, EDirectionType wallType)
{
    if (!DoesWallExist(roomCoords, wallType))
    {
        GetWallState(roomCoords, wallType).InitializeWall();
        auto wallBuilder = GetWallBuilder(roomCoords, wallType);
        if (wallBuilder != nullptr)
        {
            if (wallType == EDirectionType::North || wallType == EDirectionType::South)
                wallBuilder->BuildSouthWall();
            else
                wallBuilder->BuildWestWall();
        }
    }
}

void ATPGameDemoGameState::DisableWallState(FIntPoint roomCoords, EDirectionType wallType)
{
    if (DoesWallExist(roomCoords, wallType))
    {
        GetWallState(roomCoords, wallType).DisableWall();
        auto wallBuilder = GetWallBuilder(roomCoords, wallType);
        if (wallBuilder != nullptr)
        {
            if (wallType == EDirectionType::North || wallType == EDirectionType::South)
                wallBuilder->DestroySouthWall();
            else
                wallBuilder->DestroyWestWall();
        }
    }
}

void ATPGameDemoGameState::EnableDoorState(FIntPoint roomCoords, EDirectionType wallType)
{
    if (!DoesDoorExist(roomCoords, wallType))
    {
        GetWallState(roomCoords, wallType).InitializeDoor();
        auto wallBuilder = GetWallBuilder(roomCoords, wallType);
        if (wallBuilder != nullptr)
        {
            if (wallType == EDirectionType::North || wallType == EDirectionType::South)
                wallBuilder->SpawnSouthDoor();
            else
                wallBuilder->SpawnWestDoor();
        }
    }
}

void ATPGameDemoGameState::DisableDoorState(FIntPoint roomCoords, EDirectionType wallType)
{
    if (DoesDoorExist(roomCoords, wallType))
    {
        GetWallState(roomCoords, wallType).DisableDoor();
        auto wallBuilder = GetWallBuilder(roomCoords, wallType);
        if (wallBuilder != nullptr)
        {
            if (wallType == EDirectionType::North || wallType == EDirectionType::South)
                wallBuilder->DestroySouthDoor();
            else
                wallBuilder->DestroyWestDoor();
        }
    }
}

void ATPGameDemoGameState::DestroyDoorInRoom(FIntPoint roomCoords, EDirectionType doorWallDirection)
{
    if (auto wallBuilder = GetWallBuilder(roomCoords, doorWallDirection))
    {
        if (doorWallDirection == EDirectionType::North || doorWallDirection == EDirectionType::South)
            wallBuilder->DestroySouthDoor();
        else
            wallBuilder->DestroyWestDoor();
    }
}

void ATPGameDemoGameState::DestroyNeighbouringDoors(FIntPoint roomCoords, TArray<bool> positionsToDestroy)
{
    for (int p = 0; p < (int)EDirectionType::NumDirectionTypes; ++p)
    {
        if (p < positionsToDestroy.Num() && positionsToDestroy[p])
        {
            EDirectionType direction = (EDirectionType)p;
            DestroyDoorInRoom(roomCoords, direction);
        }
    }
}

void ATPGameDemoGameState::DoorOpened(FIntPoint roomCoords, EDirectionType wallDirection)
{
    if (!DoesRoomExist(roomCoords))
    {
        EnableRoomState(roomCoords);
    }
    FIntPoint neighbourRoomCoords = GetRoomCoords(GetNeighbouringRoomIndices(roomCoords, wallDirection));
    if (!DoesRoomExist(neighbourRoomCoords))
    {
        EnableRoomState(neighbourRoomCoords);
    }
}

//============================================================================
//============================================================================

FIntPoint ATPGameDemoGameState::GetRoomXYIndices(FIntPoint roomCoords) const
{
    const int x = roomCoords.X + NumGridsXY / 2;
    const int y = roomCoords.Y + NumGridsXY / 2;
    FIntPoint roomIndices = FIntPoint(x,y);
    return roomIndices;
}

FIntPoint ATPGameDemoGameState::GetRoomCoords(FIntPoint roomIndices) const
{
    const int x = roomIndices.X - NumGridsXY / 2;
    const int y = roomIndices.Y - NumGridsXY / 2;
    FIntPoint roomCoords = FIntPoint(x,y);
    return roomCoords;
}

bool ATPGameDemoGameState::RoomXYIndicesValid(FIntPoint roomIndices) const
{
    const int x = roomIndices.X;
    const int y = roomIndices.Y;
    return x >= 0 && x < RoomStates.Num() && y >= 0 && y < RoomStates[0].Num();
}

bool ATPGameDemoGameState::WallXYIndicesValid(FIntPoint wallRoomCoords) const
{
    const int x = wallRoomCoords.X;
    const int y = wallRoomCoords.Y;
    return x >= 0 && x < WallStates.Num() && y >= 0 && y < WallStates[0].Num();
}

WallState& ATPGameDemoGameState::GetWallState(FIntPoint roomCoords, EDirectionType direction)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(WallXYIndicesValid(roomIndices));
    switch(direction)
    {
        case EDirectionType::North: 
        {
            FIntPoint neighbourRoom = GetNeighbouringRoomIndices(roomCoords, direction);
            return WallStates[neighbourRoom.X][neighbourRoom.Y].SouthWall;
        }
        case EDirectionType::East:
        {
            FIntPoint neighbourRoom = GetNeighbouringRoomIndices(roomCoords, direction);
            return WallStates[neighbourRoom.X][neighbourRoom.Y].WestWall;
        }
        case EDirectionType::South:
        {
            return WallStates[roomIndices.X][roomIndices.Y].SouthWall;
        }
        case EDirectionType::West: 
            return WallStates[roomIndices.X][roomIndices.Y].WestWall;
        default: ensure(false); return WallStates[0][0].SouthWall;
    }
}

bool ATPGameDemoGameState::IsWallInUpdateList(FIntPoint coords, EDirectionType wallType)
{
    for(auto wallDescriptor : WallsToUpdate)
        if(wallDescriptor.WallCoupleCoords == coords && wallDescriptor.WallType == wallType)
            return true;
    return false;
}

void ATPGameDemoGameState::FlagWallsForUpdate(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(WallXYIndicesValid(roomIndices));
    TArray<WallState*> wallStates;
    wallStates.Empty();
    auto northNeighbourCoords = GetRoomCoords(GetNeighbouringRoomIndices(roomCoords, EDirectionType::North));
    auto eastNeighbourCoords = GetRoomCoords(GetNeighbouringRoomIndices(roomCoords, EDirectionType::East));

    if(!IsWallInUpdateList(roomCoords, EDirectionType::South))
        WallsToUpdate.Add({roomCoords, EDirectionType::South});
    if(!IsWallInUpdateList(roomCoords, EDirectionType::West))
        WallsToUpdate.Add({roomCoords, EDirectionType::West});
    if(!IsWallInUpdateList(northNeighbourCoords, EDirectionType::South))
        WallsToUpdate.Add({northNeighbourCoords, EDirectionType::South});
    if(!IsWallInUpdateList(eastNeighbourCoords, EDirectionType::West))
        WallsToUpdate.Add({eastNeighbourCoords, EDirectionType::West});
}

