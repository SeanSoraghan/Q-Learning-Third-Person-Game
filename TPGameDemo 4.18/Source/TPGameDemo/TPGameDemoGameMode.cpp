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


