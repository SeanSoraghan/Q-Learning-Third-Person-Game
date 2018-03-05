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

FVector2D ATPGameDemoGameMode::GetCellWorldPosition(UObject* worldContextObject, int x, int y, int RoomOffsetX, int RoomOffsetY, bool getCentre /* = true */)
{
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) worldContextObject->GetWorld()->GetAuthGameMode();

    if (gameMode == nullptr)
        return FVector2D (0.0f, 0.0f);

    return gameMode->GetGridCellWorldPosition(x, y, RoomOffsetX, RoomOffsetY, getCentre);
}

FVector2D ATPGameDemoGameMode::GetGridCellWorldPosition (int x, int y, int RoomOffsetX, int RoomOffsetY, bool getCentre /* = true */)
{
    float centreOffset = getCentre ? 0.5f : 0.0f;
    float positionX = ((x - NumGridUnitsX / 2) + centreOffset) * GridUnitLengthXCM;
    float positionY = ((y - NumGridUnitsY / 2) + centreOffset) * GridUnitLengthYCM;
    positionX += RoomOffsetX * NumGridUnitsX * GridUnitLengthXCM - RoomOffsetX * GridUnitLengthXCM;
    positionY += RoomOffsetY * NumGridUnitsY * GridUnitLengthYCM - RoomOffsetY * GridUnitLengthYCM;
    return FVector2D (positionX, positionY);
}


