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

void ULevelBuilderComponent::GenerateLevel(float normedDensity, float normedComplexity, FString levelName, TArray<int> ExistingDoorPositions)
{
    int sideLength = 3;
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();
    if (gameMode != nullptr)
    {
        sideLength = gameMode->NumGridUnitsX; 
    }

    GenerateLevelOfSize(sideLength, normedDensity, normedComplexity, levelName, ExistingDoorPositions);
}

void ULevelBuilderComponent::GenerateLevelOfSize(int sideLength, float normedDensity, float normedComplexity, FString levelName, TArray<int> ExistingDoorPositions)
{
    ensure(normedDensity >= 0.0f && normedDensity <= 1.0f && normedComplexity >= 0.0f && normedComplexity <= 1.0f);
    
    LevelStructure.Empty();
    // Generate random doors on perimiter, unless doors already exist.
    int existingDoorNorth = ExistingDoorPositions[(int)EActionType::North];
    int existingDoorEast  = ExistingDoorPositions[(int)EActionType::East];
    int existingDoorSouth = ExistingDoorPositions[(int)EActionType::South];
    int existingDoorWest  = ExistingDoorPositions[(int)EActionType::West];

    int doorPositionNorth = (existingDoorNorth > 0 && existingDoorNorth < sideLength - 1) ? existingDoorNorth 
                                                                                        : FMath::RandRange(1, sideLength - 2);
    int doorPositionEast = (existingDoorEast > 0 && existingDoorEast < sideLength - 1) ? existingDoorEast 
                                                                                     : FMath::RandRange(1, sideLength - 2);
    int doorPositionSouth = (existingDoorSouth > 0 && existingDoorSouth < sideLength - 1) ? existingDoorSouth 
                                                                                        : FMath::RandRange(1, sideLength - 2);
    int doorPositionWest = (existingDoorWest > 0 && existingDoorWest < sideLength - 1) ? existingDoorWest 
                                                                                       : FMath::RandRange(1, sideLength - 2);
    FIntPoint northDoor = FIntPoint(sideLength - 1, doorPositionNorth);
    FIntPoint eastDoor  = FIntPoint(doorPositionEast, sideLength - 1);
    FIntPoint southDoor = FIntPoint(0, doorPositionSouth);
    FIntPoint westDoor  = FIntPoint(doorPositionWest, 0);
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

    GenerateInnerStructure(sideLength, normedDensity, normedComplexity);

    UE_LOG(LogTemp, Warning, TEXT("Generated Level:"));
    //LevelBuilderHelpers::PrintArray(LevelStructure);

    if (LevelsDirFound && !levelName.IsEmpty())
    {
        CurrentLevelPath = LevelBuilderHelpers::LevelsDir() + levelName + ".txt";
        LevelBuilderHelpers::WriteArrayToTextFile(LevelStructure, CurrentLevelPath);
    }
}

void ULevelBuilderComponent::GenerateInnerStructure(int sideLength, float normedDensity, float normedComplexity)
{
    const int complexity = int(normedComplexity * (10 * (sideLength)));
    const int density = int(normedDensity * (FMath::Pow ((sideLength / 2.0f), 2.0f)));

    for (int island = 0; island < density; ++island)
    {
        FIntPoint currentIslandPoint = GetRandomEvenCell();
        if(LevelStructure[currentIslandPoint.X][currentIslandPoint.Y] == (int)ECellState::Open && 
           !IsCellTouchingDoorCell(currentIslandPoint))
        {
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
                        LevelStructure[currentIslandPoint.X][currentIslandPoint.Y] = (int)ECellState::Closed;
                        currentIslandPoint = islandSectionEndTarget;
                    }
                }
            }
        }
    }
}

void ULevelBuilderComponent::RegenerateInnerStructure(float normedDensity, float normedComplexity, FString levelName)
{
    int sideLength = 3;
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();
    if (gameMode != nullptr)
    {
        sideLength = gameMode->NumGridUnitsX; 
    }

    //Clear existing inner structure
    for (int x = 0; x < sideLength - 1; ++x)
    {
        TArray<int> row;
        for (int y = 0; y < sideLength - 1; ++y)
        {
            row.Add((int)ECellState::Open);
        }
        LevelStructure.Add(row);
    }

    GenerateInnerStructure(sideLength, normedDensity, normedComplexity);

    UE_LOG(LogTemp, Warning, TEXT("Regenerated Level:"));
    //LevelBuilderHelpers::PrintArray(LevelStructure);

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
        LevelStructure.Empty();
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
    if (x >= 0 && y >= 0 && x < LevelStructure.Num() && y < LevelStructure.Num())
        return (ECellState)LevelStructure[x][y];
    return (ECellState)0;
}

EActionType ULevelBuilderComponent::GetWallTypeForDoorPosition(int x, int y)
{
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();
    if (x == 0 && y > 0 && y < sizeY - 1)
        return EActionType::South;
    if (y == sizeY - 1 && x > 0 && x < sizeY - 1)
        return EActionType::East;
    if (x == sizeX - 1 && y > 0 && y < sizeY - 1)
        return EActionType::North;
    if (y == 0 && x > 0 && x < sizeX - 1)
        return EActionType::West;
    return EActionType::NumActionTypes;
}

EActionType ULevelBuilderComponent::GetWallTypeForBlockPosition(int x, int y)
{
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();
    if (x == 0 && y >= 0 && y < sizeY)
        return EActionType::South;
    if (y == sizeY - 1 && x >= 0 && x < sizeY)
        return EActionType::East;
    if (x == sizeX - 1 && y >= 0 && y < sizeY)
        return EActionType::North;
    if (y == 0 && x >= 0 && x < sizeX)
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
                ++xPos;
                break;
            case EActionType::East: 
                ++yPos;
                break;
            case EActionType::South: 
                --xPos;
                break;
            case EActionType::West: 
                --yPos;
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

FVector2D ULevelBuilderComponent::GetCellWorldPosition (int x, int y, int RoomOffsetX, int RoomOffsetY, bool getCentre /* = true */)
{
    //ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

    //if (gameMode == nullptr)
    //    return FVector2D (0.0f, 0.0f);

    //return gameMode->GetCellWorldPosition(x, y, RoomOffsetX, RoomOffsetY, getCentre);
    return ATPGameDemoGameMode::GetCellWorldPosition(this, x, y, RoomOffsetX, RoomOffsetY, getCentre);
}

FVector2D ULevelBuilderComponent::FindMostCentralSpawnPosition(int RoomOffsetX, int RoomOffsetY)
{
    FVector2D cellPosition = GetClosestEmptyCell(GetNumGridUnitsX() / 2, GetNumGridUnitsY() / 2);
    return GetCellWorldPosition ((int) cellPosition.X, (int) cellPosition.Y, RoomOffsetX, RoomOffsetY);
}

void ULevelBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULevelBuilderComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}

