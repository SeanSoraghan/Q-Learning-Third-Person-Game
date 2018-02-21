// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TPGameDemoGameMode.h"

void ATPGameDemoGameMode::SetGridUnitLengthXCM (int x)
{
    GridUnitLengthXCM = x;
    OnMazeDimensionsChanged.Broadcast();
}

void ATPGameDemoGameMode::SetGridUnitLengthYCM (int y)
{
    GridUnitLengthYCM = y;
    OnMazeDimensionsChanged.Broadcast();
}

void ATPGameDemoGameMode::SetNumGridUnitsX (int numUnitsX)
{
    NumGridUnitsX = numUnitsX;
    OnMazeDimensionsChanged.Broadcast();
}

void ATPGameDemoGameMode::SetNumGridUnitsY (int numUnitsY)
{
    NumGridUnitsY = numUnitsY;
    OnMazeDimensionsChanged.Broadcast();
}

FVector2D ATPGameDemoGameMode::GetCellWorldPosition (UWorld* world, int x, int y, int RoomOffsetX, int RoomOffsetY, bool getCentre /* = true */)
{
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) world->GetAuthGameMode();

    if (gameMode == nullptr)
        return FVector2D (0.0f, 0.0f);

    float centreOffset = getCentre ? 0.5f : 0.0f;
    float positionX = ((x - gameMode->NumGridUnitsX / 2) + centreOffset) * gameMode->GridUnitLengthXCM;
    float positionY = ((y - gameMode->NumGridUnitsY / 2) + centreOffset) * gameMode->GridUnitLengthYCM;
    positionX += RoomOffsetX * gameMode->NumGridUnitsX * gameMode->GridUnitLengthXCM - RoomOffsetX * gameMode->GridUnitLengthXCM;
    positionY += RoomOffsetY * gameMode->NumGridUnitsY * gameMode->GridUnitLengthYCM - RoomOffsetY * gameMode->GridUnitLengthYCM;
    return FVector2D (positionX, positionY);
}


