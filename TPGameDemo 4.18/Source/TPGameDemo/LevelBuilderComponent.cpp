// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.cpp"
#include "TPGameDemoGameMode.h"
#include "LevelBuilderComponent.h"


ULevelBuilderComponent::ULevelBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    LevelsDir      = FPaths::/*GameDir*/ProjectDir();
    LevelsDir     += "Content/Levels/";
    LevelsDirFound = FPlatformFileManager::Get().GetPlatformFile().DirectoryExists (*LevelsDir);
}

void ULevelBuilderComponent::LoadLevel (FString levelName)
{
    if (LevelsDirFound)
    {
        CurrentLevelPath = LevelsDir + levelName + ".txt";

      #if ON_SCREEN_DEBUGGING
        if ( ! FPlatformFileManager::Get().GetPlatformFile().FileExists (*CurrentLevelPath))
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Cant Find file: | %s |"), *CurrentLevelPath));
      #endif

        FillArrayFromTextFile (CurrentLevelPath, LevelStructure);
        ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

        if (gameMode != nullptr)
        {
            gameMode->NumGridUnitsX = LevelStructure.Num();
            if (LevelStructure.Num() > 0)
                gameMode->NumGridUnitsY = LevelStructure[0].Num();
        }
    }
}

int ULevelBuilderComponent::GetNumGridUnitsX() { return LevelStructure.Num(); }
int ULevelBuilderComponent::GetNumGridUnitsY() 
{
    if (LevelStructure.Num() > 0)
        return LevelStructure[0].Num();
    return 0;
}

bool ULevelBuilderComponent::IsGridPositionBlocked (int x, int y) 
{
    if (x > LevelStructure.Num() - 1 || x < 0 || y > LevelStructure[0].Num() - 1 || y < 0)
        return false;
    return LevelStructure[x][y] == 1;
}

FVector2D ULevelBuilderComponent::GetClosestEmptyCell (int x, int y)
{
    int xPos = x;
    int yPos = y;
    const int totalCells = GetNumGridUnitsX() * GetNumGridUnitsY();
    int numCellsVisited = 0;
    EActionType actionType = EActionType::North;
    int numActionsToTake = 1;
    int numActionsTaken = 0;
    while(numCellsVisited < totalCells)
    {
        if (xPos < GetNumGridUnitsX() && yPos < GetNumGridUnitsY() && !IsGridPositionBlocked(xPos, yPos))
            return FVector2D(xPos, yPos);
        switch (actionType)
        {
            case EActionType::North: 
                ++yPos;
                break;
            case EActionType::East: 
                ++xPos;
                break;
            case EActionType::South: 
                --yPos;
                break;
            case EActionType::West: 
                --xPos;
                break;
            default: break;
        }
        ++numActionsTaken;
        if (numActionsTaken == numActionsToTake)
        {
            // change action to the next clockwise action (north goes to east, east to south etc.)
            actionType = (EActionType)(((int)actionType + 1) % (int)EActionType::NumActionTypes);
            if (actionType == EActionType::North || actionType == EActionType::South)
                ++numActionsToTake;
            numActionsTaken = 0;
        }
        ++numCellsVisited;
    }
    UE_LOG(LogTemp, Warning, TEXT("No closest empty grid cell found for %i | %i"), x, y);
    return FVector2D::ZeroVector;
}

FVector2D ULevelBuilderComponent::GetCellCentreWorldPosition (int x, int y, int RoomOffsetX, int RoomOffsetY)
{
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

    if (gameMode == nullptr)
        return FVector2D (0.0f, 0.0f);

    float positionX = ((x - gameMode->NumGridUnitsX / 2) + 0.5f) * gameMode->GridUnitLengthXCM;
    float positionY = ((y - gameMode->NumGridUnitsY / 2) + 0.5f) * gameMode->GridUnitLengthYCM;
    positionX += RoomOffsetX * GetNumGridUnitsX() * gameMode->GridUnitLengthXCM;
    positionX += RoomOffsetY * GetNumGridUnitsY() * gameMode->GridUnitLengthYCM;
    return FVector2D (positionX, positionY);
}

FVector2D ULevelBuilderComponent::FindMostCentralSpawnPosition(int RoomOffsetX, int RoomOffsetY)
{
    const int xOffset = RoomOffsetX * GetNumGridUnitsX();
    const int yOffset = RoomOffsetY * GetNumGridUnitsY();
    FVector2D cellPosition = GetClosestEmptyCell(GetNumGridUnitsX() / 2 + xOffset, GetNumGridUnitsY() / 2 + yOffset);
    return GetCellCentreWorldPosition ((int) cellPosition.X, (int) cellPosition.Y, RoomOffsetX, RoomOffsetY);
}

void ULevelBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULevelBuilderComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}

