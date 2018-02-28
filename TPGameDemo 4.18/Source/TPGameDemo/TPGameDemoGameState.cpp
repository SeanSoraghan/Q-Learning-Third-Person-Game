// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include <functional>
#include "TPGameDemoGameMode.h"
#include "TPGameDemoGameState.h"


void ATPGameDemoGameState::BeginPlay()
{
    InitialiseRoomStates();
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

void ATPGameDemoGameState::InitialiseRoomStates()
{
    for (int x = 0; x < NumGridsXY; ++x)
    {
        TArray<RoomState> statesRow;
        for (int y = 0; y < NumGridsXY; ++y)
        {
            statesRow.Add(RoomState());
        }
        RoomStates.Add(statesRow);
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

void ATPGameDemoGameState::DestroyNeighbouringDoors(FIntPoint roomCoords, TArray<bool> positionsToDestroy)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));

    std::function<EWallPosition(EWallPosition wallPosition)> GetWallPositionInNeighbouringRoom = [](EWallPosition wallPosition)
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
        return EWallPosition::North;
    };

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

void ATPGameDemoGameState::EnableRoomState(FIntPoint roomCoords, TArray<AActor*> doors)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomStates[roomIndices.X][roomIndices.Y].InitializeRoom(doors);
}

void ATPGameDemoGameState::DisableRoomState(FIntPoint roomCoords)
{
    FIntPoint roomIndices = GetRoomXYIndices(roomCoords);
    ensure(RoomXYIndicesValid(roomIndices));
    RoomStates[roomIndices.X][roomIndices.Y].TerminateRoom();
}

FIntPoint ATPGameDemoGameState::GetRoomXYIndices(FIntPoint roomCoords)
{
    const int x = roomCoords.X + NumGridsXY / 2;
    const int y = roomCoords.Y + NumGridsXY / 2;
    FIntPoint roomIndices = FIntPoint(x,y);
    return roomIndices;
}

bool ATPGameDemoGameState::RoomXYIndicesValid(FIntPoint roomIndices)
{
    const int x = roomIndices.X;
    const int y = roomIndices.Y;
    return x >= 0 && x < RoomStates.Num() && y >= 0 && y < RoomStates[0].Num();
}
