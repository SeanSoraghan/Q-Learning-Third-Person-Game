// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.h"
#include "TPGameDemoGameMode.h"
#include "LevelBuilderComponent.h"



ULevelBuilderComponent::ULevelBuilderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    LevelsDirFound = FPlatformFileManager::Get().GetPlatformFile().DirectoryExists (*LevelBuilderHelpers::LevelsDir());
}

FIntPoint ULevelBuilderComponent::GetRandomEvenCell()
{
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();

    int xPos = FMath::RandRange(0, sizeX / 2) * 2;

    int yPos = FMath::RandRange(0, sizeY / 2) * 2;

    return FIntPoint(xPos,yPos);
}

void ULevelBuilderComponent::GenerateLevel(int sideLength, float normedDensity, float normedComplexity, FString levelName)
{
    ensure(normedDensity >= 0.0f && normedDensity <= 1.0f && normedComplexity >= 0.0f && normedComplexity <= 1.0f);
    const int complexity = int(normedComplexity * (10 * (sideLength)));
    const int density = int(normedDensity * (FMath::Pow ((sideLength / 2.0f), 2.0f)));
    LevelStructure.Empty();
    // Generate random doors on perimiter.
    FIntPoint northDoor = FIntPoint(0, FMath::RandRange(1, sideLength - 2));
    FIntPoint eastDoor  = FIntPoint(FMath::RandRange(1, sideLength - 2), sideLength - 1);
    FIntPoint southDoor = FIntPoint(sideLength - 1, FMath::RandRange(1, sideLength - 2));
    FIntPoint westDoor  = FIntPoint(FMath::RandRange(1, sideLength - 2), 0);
    for (int x = 0; x < sideLength; ++x)
    {
        TArray<int> row;
        for (int y = 0; y < sideLength; ++y)
        {
            // inside
            int cellState = (int)ECellState::Open;
            // doors
            FIntPoint p(x,y);
            if (p == northDoor || p == eastDoor || p == southDoor || p == westDoor)
            {
                cellState = (int)ECellState::Door;
            } // perimiter walls
            else if (x == 0 || x == sideLength - 1 || y == 0 || y == sideLength - 1)
            {
                cellState = (int)ECellState::Closed;
            }
            row.Add(cellState);
        }
        LevelStructure.Add(row);
    }

    for (int island = 0; island < density; ++island)
    {
        FIntPoint currentIslandPoint = GetRandomEvenCell();
        for (int islandSection = 0; islandSection < complexity; ++islandSection)
        {
            EActionType direction = (EActionType)FMath::RandRange(0, (int)EActionType::NumActionTypes - 1);
            FIntPoint islandSectionEndTarget = LevelBuilderHelpers::GetTargetPointForAction(currentIslandPoint, direction, 2);
            if (LevelBuilderHelpers::GridPositionIsValid(islandSectionEndTarget, sideLength, sideLength) &&
                LevelStructure[islandSectionEndTarget.X][islandSectionEndTarget.Y] == (int)ECellState::Open)
            {
                FIntPoint islandSectionMiddle = LevelBuilderHelpers::GetTargetPointForAction(currentIslandPoint, direction, 1);
                if (!IsCellTouchingDoorCell(islandSectionEndTarget) && !IsCellTouchingDoorCell(islandSectionMiddle))
                {
                    LevelStructure[islandSectionEndTarget.X][islandSectionEndTarget.Y] = (int)ECellState::Closed;
                    LevelStructure[islandSectionMiddle.X][islandSectionMiddle.Y] = (int)ECellState::Closed;
                    currentIslandPoint = islandSectionEndTarget;
                }
            }
        }
    }

    if (LevelsDirFound && !levelName.IsEmpty())
    {
        CurrentLevelPath = LevelBuilderHelpers::LevelsDir() + levelName + ".txt";
        LevelBuilderHelpers::WriteArrayToTextFile(LevelStructure, CurrentLevelPath);
    }
}

bool ULevelBuilderComponent::IsCellTouchingDoorCell(FIntPoint cellPosition)
{
    const int sizeX = LevelStructure.Num();
    ensure(LevelStructure.Num() > 0);
    const int sizeY = LevelStructure[0].Num();
    for (int action = 0; action < (int)EActionType::NumActionTypes; ++action)
    {
        FIntPoint actionTarget = LevelBuilderHelpers::GetTargetPointForAction(cellPosition, (EActionType)action);
        if (LevelBuilderHelpers::GridPositionIsValid(actionTarget, sizeX, sizeY) && 
            LevelStructure[actionTarget.X][actionTarget.Y] == (int)ECellState::Door)
            return true;
    }
    return false;
}

void ULevelBuilderComponent::LoadLevel (FString levelName)
{
    if (LevelsDirFound)
    {
        CurrentLevelPath = LevelBuilderHelpers::LevelsDir() + levelName + ".txt";

      #if ON_SCREEN_DEBUGGING
        if ( ! FPlatformFileManager::Get().GetPlatformFile().FileExists (*CurrentLevelPath))
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Cant Find file: | %s |"), *CurrentLevelPath));
      #endif

        LevelBuilderHelpers::FillArrayFromTextFile (CurrentLevelPath, LevelStructure);
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
    return LevelStructure[x][y] != (int)ECellState::Open;
}

ECellState ULevelBuilderComponent::GetCellState (int x, int y)
{
    return (ECellState)LevelStructure[x][y];
}

EActionType ULevelBuilderComponent::GetWallTypeForDoorPosition(int x, int y)
{
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();
    if (x == 0 && y > 0 && y < sizeY - 1)
        return EActionType::North;
    if (y == sizeY - 1 && x > 0 && x < sizeY - 1)
        return EActionType::East;
    if (x == sizeX - 1 && y > 0 && y < sizeY - 1)
        return EActionType::South;
    if (y == 0 && x > 0 && x < sizeX - 1)
        return EActionType::West;
    return EActionType::NumActionTypes;
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

    float positionX = ((x - gameMode->NumGridUnitsX / 2) /*+ 0.5f*/) * gameMode->GridUnitLengthXCM;
    float positionY = ((y - gameMode->NumGridUnitsY / 2) /*+ 0.5f*/) * gameMode->GridUnitLengthYCM;
    positionX += RoomOffsetX * GetNumGridUnitsX() * gameMode->GridUnitLengthXCM - RoomOffsetX * gameMode->GridUnitLengthXCM;
    positionY += RoomOffsetY * GetNumGridUnitsY() * gameMode->GridUnitLengthYCM - RoomOffsetY * gameMode->GridUnitLengthYCM;
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

