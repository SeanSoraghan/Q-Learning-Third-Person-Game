// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include <functional>
#include "TPGameDemoGameMode.h"
#include "TPGameDemoGameState.h"

ATPGameDemoGameState::~ATPGameDemoGameState()
{
    for(int i = 0; i < RoomBuilders.Num(); ++i)
    {
        TArray<AActor*>& builderRow = RoomBuilders[i];
        for (int c = 0; c < builderRow.Num(); ++c)
        {
            builderRow[c] = nullptr;
        }
    }
    RoomBuilders.Empty();
}

EWallPosition ATPGameDemoGameState::GetWallPositionForActionType(EActionType actionType)
{
    switch (actionType)
    {
        case EActionType::North: return EWallPosition::North;
        case EActionType::East: return EWallPosition::East;
        case EActionType::South: return EWallPosition::South;
        case EActionType::West: return EWallPosition::West;
        default: ensure(false); return EWallPosition::NumWallPositions;
    }
}

void ATPGameDemoGameState::InitialiseArrays()
{
    for (int x = 0; x < NumGridsXY; ++x)
    {
        TArray<RoomState> statesRow;
        TArray<AActor*> builderRow;
        for (int y = 0; y < NumGridsXY; ++y)
        {
            builderRow.Add(nullptr);
            statesRow.Add(RoomState());
        }
        RoomStates.Add(statesRow);
        RoomBuilders.Add(builderRow);
    }
}

void ATPGameDemoGameState::DestroyDoorInRoom(FIntPoint roomCoords, EWallPosition doorWallPosition)
{
    FIntPoint roomIndicesXY = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndicesXY));
    RoomStates[roomIndicesXY.X][roomIndicesXY.Y].DestroyDoor(doorWallPosition);
}

FIntPoint ATPGameDemoGameState::GetNeighbouringRoomIndices(FIntPoint roomCoords, EWallPosition neighbourPosition)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));

    switch(neighbourPosition)
    {
        case EWallPosition::North: return FIntPoint(roomIndices.X + 1, roomIndices.Y);
        case EWallPosition::East:  return FIntPoint(roomIndices.X, roomIndices.Y + 1);
        case EWallPosition::South: return FIntPoint(roomIndices.X - 1, roomIndices.Y);
        case EWallPosition::West:  return FIntPoint(roomIndices.X, roomIndices.Y - 1);
        default: ensure(false); return FIntPoint(-1, -1);
    }
}

TArray<bool> ATPGameDemoGameState::GetNeighbouringRoomStates(FIntPoint roomCoords)
{
    TArray<bool> neighbouringRoomStates{false, false, false, false};
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    
    for (int p = 0; p < (int)EWallPosition::NumWallPositions; ++p)
    {
        FIntPoint neighbour = GetNeighbouringRoomIndices(roomCoords, (EWallPosition)p);
        if (RoomXYIndicesValid(neighbour) && RoomStates[neighbour.X][neighbour.Y].bRoomExists)
            neighbouringRoomStates[p] = true;
    }
    return neighbouringRoomStates;
}

EWallPosition ATPGameDemoGameState::GetWallPositionInNeighbouringRoom(EWallPosition wallPosition)
{
    switch(wallPosition)
    {
        case EWallPosition::North: return EWallPosition::South;
        case EWallPosition::East: return EWallPosition::West;
        case EWallPosition::South: return EWallPosition::North;
        case EWallPosition::West: return EWallPosition::East;
        default: ensure(false); return EWallPosition::NumWallPositions;
    }
    ensure(false);
    return EWallPosition::NumWallPositions;
}

void ATPGameDemoGameState::DestroyNeighbouringDoors(FIntPoint roomCoords, TArray<bool> positionsToDestroy)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));

    for (int p = 0; p < (int)EWallPosition::NumWallPositions; ++p)
    {
        if (p < positionsToDestroy.Num() && positionsToDestroy[p])
        {
            FIntPoint neighbourRoom = GetNeighbouringRoomIndices(roomCoords, EWallPosition(p));
            if(RoomXYIndicesValid(neighbourRoom))
            {
                const EWallPosition positionInNeighbouringRoom = GetWallPositionInNeighbouringRoom((EWallPosition)p);//(p + 2) % EWallPosition::NumWallPositions
                RoomStates[neighbourRoom.X][neighbourRoom.Y].DestroyDoor(positionInNeighbouringRoom);
            }
        }
    }
}

bool ATPGameDemoGameState::DoesRoomExist(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomStates[roomIndices.X][roomIndices.Y].bRoomExists;
}

void ATPGameDemoGameState::EnableRoomState(FIntPoint roomCoords, TArray<AActor*> doors, TArray<int> doorPositionsOnWalls)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomStates[roomIndices.X][roomIndices.Y].InitializeRoom(doors, doorPositionsOnWalls);
}

void ATPGameDemoGameState::DisableRoomState(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    TArray<bool> neighbouringRoomStates = GetNeighbouringRoomStates(roomCoords);
    RoomStates[roomIndices.X][roomIndices.Y].DisableRoom(neighbouringRoomStates);
    for (int p = 0; p < (int)EWallPosition::NumWallPositions; ++p)
    {
        if (neighbouringRoomStates[p])
        {
            FIntPoint neighbourIndices = GetNeighbouringRoomIndices(roomCoords, (EWallPosition)p);
            FIntPoint neighbourCoords = GetRoomCoords(neighbourIndices);
            EWallPosition wallPositionInNeighbour = GetWallPositionInNeighbouringRoom((EWallPosition)p);
            int doorPositionOnWall = RoomStates[roomIndices.X][roomIndices.Y].GetDoorPositionOnWall((EWallPosition)p);
            OnSpawnDoor.Broadcast(neighbourCoords, wallPositionInNeighbour, doorPositionOnWall, roomCoords);
        }
    }
}

FIntPoint ATPGameDemoGameState::GetRoomXYIndices(FIntPoint roomCoords)
{
    const int x = roomCoords.X + NumGridsXY / 2;
    const int y = roomCoords.Y + NumGridsXY / 2;
    FIntPoint roomIndices = FIntPoint(x,y);
    return roomIndices;
}

FIntPoint ATPGameDemoGameState::GetRoomCoords(FIntPoint roomIndices)
{
    const int x = roomIndices.X - NumGridsXY / 2;
    const int y = roomIndices.Y - NumGridsXY / 2;
    FIntPoint roomCoords = FIntPoint(x,y);
    return roomCoords;
}

bool ATPGameDemoGameState::RoomXYIndicesValid(FIntPoint roomIndices)
{
    const int x = roomIndices.X;
    const int y = roomIndices.Y;
    return x >= 0 && x < RoomStates.Num() && y >= 0 && y < RoomStates[0].Num();
}

AActor* ATPGameDemoGameState::GetRoomBuilder(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomBuilders[roomIndices.X][roomIndices.Y];
}

void ATPGameDemoGameState::SetRoomBuilder(FIntPoint roomCoords, AActor* roomBuilderActor)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomBuilders[roomIndices.X][roomIndices.Y] = roomBuilderActor;
}

void ATPGameDemoGameState::UpdateRoomHealth(FIntPoint roomCoords, float healthDelta)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomStates[roomIndices.X][roomIndices.Y].RoomHealth += healthDelta;
    if (RoomStates[roomIndices.X][roomIndices.Y].RoomHealth <= 0.0f && 
        RoomStates[roomIndices.X][roomIndices.Y].bRoomExists)
        KillRoom(roomCoords);
}

void ATPGameDemoGameState::SetRoomHealth(FIntPoint roomCoords, float health)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomStates[roomIndices.X][roomIndices.Y].RoomHealth = health;
    if (RoomStates[roomIndices.X][roomIndices.Y].RoomHealth <= 0.0f && 
        RoomStates[roomIndices.X][roomIndices.Y].bRoomExists)
        KillRoom(roomCoords);
}

float ATPGameDemoGameState::GetRoomHealth(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomStates[roomIndices.X][roomIndices.Y].RoomHealth;
}

void ATPGameDemoGameState::KillRoom(FIntPoint roomCoords)
{
    DisableRoomState(roomCoords);
    OnRoomDied.Broadcast(roomCoords);
}

int ATPGameDemoGameState::GetDoorPositionOnWall(FIntPoint roomCoords, EWallPosition wallType)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    return RoomStates[roomIndices.X][roomIndices.Y].GetDoorPositionOnWall(wallType);
}

TArray<int> ATPGameDemoGameState::GetDoorPositionsForExistingNeighbours(FIntPoint roomCoords)
{
    TArray<bool> neighbourStates = GetNeighbouringRoomStates(roomCoords);
    TArray<int> doorPositions = {0,0,0,0};
    for (EWallPosition p = EWallPosition::North; p < EWallPosition::NumWallPositions; p = (EWallPosition)((int)p + 1))
    {
        if (neighbourStates[(int)p])
        {
            doorPositions[(int)p] = GetDoorPositionOnWall(roomCoords, p);
        }
    }
    return doorPositions;
}
