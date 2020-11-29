// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.h"
#include "TPGameDemoGameState.h"
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

TArray<FWallSegmentDescriptor> ULevelBuilderComponent::GenerateLevel(float normedDensity, float normedComplexity, FIntPoint roomCoords)
{
    int sideLength = 3;
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        sideLength = gameState->NumGridUnitsX; 
    }

    return GenerateLevelOfSize(sideLength, normedDensity, normedComplexity, roomCoords);
}

TArray<FWallSegmentDescriptor> ULevelBuilderComponent::GenerateLevelOfSize(int sideLength, float normedDensity, float normedComplexity, FIntPoint roomCoords)
{
    ensure(normedDensity >= 0.0f && normedDensity <= 1.0f && normedComplexity >= 0.0f && normedComplexity <= 1.0f);
    
    TArray<FWallSegmentDescriptor> wallSegments;

    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        LevelStructure.Empty();
        TArray<int> ExistingDoorPositions;
        gameState->GetDoorPositionsNESW(roomCoords, ExistingDoorPositions);
        // Generate random doors on perimiter, unless doors already exist.
        int existingDoorNorth = ExistingDoorPositions[(int)EDirectionType::North];
        int existingDoorEast  = ExistingDoorPositions[(int)EDirectionType::East];
        int existingDoorSouth = ExistingDoorPositions[(int)EDirectionType::South];
        int existingDoorWest  = ExistingDoorPositions[(int)EDirectionType::West];

        ensure(existingDoorNorth > 0 && existingDoorNorth < sideLength - 1);
        ensure(existingDoorEast > 0 && existingDoorEast < sideLength - 1);
        ensure(existingDoorSouth > 0 && existingDoorSouth < sideLength - 1);
        ensure(existingDoorWest > 0 && existingDoorWest < sideLength - 1);

        FIntPoint northDoor = FIntPoint(sideLength - 1, existingDoorNorth);
        FIntPoint eastDoor  = FIntPoint(existingDoorEast, sideLength - 1);
        FIntPoint southDoor = FIntPoint(0, existingDoorSouth);
        FIntPoint westDoor  = FIntPoint(existingDoorWest, 0);
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

        wallSegments = GenerateInnerStructure(sideLength, normedDensity, normedComplexity);

        ensure(sideLength == gameState->NumGridUnitsX);
        InnerRoomBitmask innerLevelBitmask = LevelBuilderHelpers::ArrayToBitmask(LevelStructure);
        gameState->SetRoomInnerStructure(roomCoords, innerLevelBitmask);

        /*UE_LOG(LogTemp, Warning, TEXT("Generated Level:"));
        LevelBuilderHelpers::PrintArray(LevelStructure);*/
    }

    return wallSegments;
}

TArray<FWallSegmentDescriptor> ULevelBuilderComponent::GenerateInnerStructure(int sideLength, float normedDensity, float normedComplexity)
{
    TArray<FWallSegmentDescriptor> wallSegments;
    wallSegments.Empty();

    const int complexity = int(normedComplexity * (10 * (sideLength)));
    const int density = int(normedDensity * (FMath::Pow ((sideLength / 2.0f), 2.0f)));

    // Used for creating the wall descriptors.
    int gridMaxX = LevelStructure.Num() - 1;
    int gridMaxY = LevelStructure[0].Num() - 1;

    FWallSegmentDescriptor currentWallSegment = {FIntPoint(0,0), FIntPoint(0,0), EDirectionType::North};
    int totalSegmentsStarted = 0;

    for (int island = 0; island < density; ++island)
    {
        bool wallSegmentInitialized = false;
        int numWallSegmentsStarted = 0;

        FIntPoint currentIslandPoint = GetRandomEvenCell();
        if(LevelStructure[currentIslandPoint.X][currentIslandPoint.Y] == (int)ECellState::Open && 
           !IsCellTouchingDoorCell(currentIslandPoint))
        {
            for (int islandSection = 0; islandSection < complexity; ++islandSection)
            {
                EDirectionType direction = (EDirectionType)FMath::RandRange(0, (int)EDirectionType::NumDirectionTypes - 1);
                FIntPoint islandSectionEndTarget = LevelBuilderHelpers::GetTargetPointForAction(currentIslandPoint, direction, 2);
                if (LevelBuilderHelpers::GridPositionIsValid(islandSectionEndTarget, sideLength, sideLength) &&
                    LevelStructure[islandSectionEndTarget.X][islandSectionEndTarget.Y] == (int)ECellState::Open)
                {
                    FIntPoint islandSectionMiddle = LevelBuilderHelpers::GetTargetPointForAction(currentIslandPoint, direction, 1);
                    if (!IsCellTouchingDoorCell(islandSectionEndTarget) && !IsCellTouchingDoorCell(islandSectionMiddle))
                    {
                        if (!wallSegmentInitialized)
                        {
                            currentWallSegment = {currentIslandPoint, islandSectionEndTarget, direction};
                            wallSegmentInitialized = true;
                            ++numWallSegmentsStarted;
                        }
                        else if (direction == currentWallSegment.Direction)
                        {
                            currentWallSegment.End = islandSectionEndTarget;
                        }
                        else
                        {
                            wallSegments.Add(currentWallSegment);
                            currentWallSegment = {islandSectionMiddle, islandSectionEndTarget, direction};
                            ++numWallSegmentsStarted;
                        }
                        LevelStructure[islandSectionEndTarget.X][islandSectionEndTarget.Y] = (int)ECellState::Closed;
                        LevelStructure[islandSectionMiddle.X][islandSectionMiddle.Y] = (int)ECellState::Closed;
                        LevelStructure[currentIslandPoint.X][currentIslandPoint.Y] = (int)ECellState::Closed;
                        currentIslandPoint = islandSectionEndTarget;
                    }
                }
            }
            totalSegmentsStarted += numWallSegmentsStarted;
            if (wallSegments.Num() != totalSegmentsStarted)
            {
                ensure(totalSegmentsStarted == wallSegments.Num() + 1);
                wallSegments.Add(currentWallSegment);
            }
        }
    }
    
    return wallSegments;
}

TArray<FWallSegmentDescriptor> ULevelBuilderComponent::RegenerateInnerStructure(float normedDensity, float normedComplexity, FIntPoint roomCoords)
{
    int sideLength = 3;
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        sideLength = gameState->NumGridUnitsX; 
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

    TArray<FWallSegmentDescriptor> wallSegments = GenerateInnerStructure(sideLength, normedDensity, normedComplexity);

    if (gameState != nullptr)
    {
        ensure(sideLength == gameState->NumGridUnitsX);
        InnerRoomBitmask innerLevelBitmask = LevelBuilderHelpers::ArrayToBitmask(LevelStructure);
        gameState->SetRoomInnerStructure(roomCoords, innerLevelBitmask);
    }

    return wallSegments;
}

bool ULevelBuilderComponent::IsCellTouchingDoorCell(FIntPoint cellPosition)
{
    const int sizeX = LevelStructure.Num();
    ensure(LevelStructure.Num() > 0);
    const int sizeY = LevelStructure[0].Num();
    for (int action = 0; action < (int)EDirectionType::NumDirectionTypes; ++action)
    {
        FIntPoint actionTarget = LevelBuilderHelpers::GetTargetPointForAction(cellPosition, (EDirectionType)action);
        if (LevelBuilderHelpers::GridPositionIsValid(actionTarget, sizeX, sizeY) && 
            LevelStructure[actionTarget.X][actionTarget.Y] == (int)ECellState::Door)
            return true;
    }
    return false;
}

void ULevelBuilderComponent::LoadLevel (FIntPoint roomCoords)
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(GetWorld()->GetGameState());
    if (gameState != nullptr)
    {
        //This is called from the room builder BP in OnBuildRoom, after the walls have been spawned (BuildGeneratedRoom). The game state should hold the qvalues, so that we can build on them if the room structure changes.
        TArray<TArray<int>> LevelStructure;
        int sideLength = gameState->NumGridUnitsX;
        LevelStructure.SetNumZeroed(sideLength);
        for (int x = 0; x < sideLength; ++x)
            LevelStructure[x].SetNumZeroed(sideLength);
        InnerRoomBitmask roomBitmask = gameState->GetRoomInnerStructure(roomCoords);
        LevelBuilderHelpers::BitMaskToArray(roomBitmask, LevelStructure);
        TArray<int> neswDoorPositions;
        gameState->GetDoorPositionsNESW(roomCoords, neswDoorPositions);
        for (int s = 0; s < sideLength; ++s)
        {
            LevelStructure[sideLength - 1][s] = (int)ECellState::Closed;
            LevelStructure[s][sideLength - 1] = (int)ECellState::Closed;
            LevelStructure[0][s] = (int)ECellState::Closed;
            LevelStructure[s][0] = (int)ECellState::Closed;
        }
        LevelStructure[sideLength - 1][neswDoorPositions[(int)EDirectionType::North]] = (int)ECellState::Door;
        LevelStructure[neswDoorPositions[(int)EDirectionType::East]][sideLength - 1] = (int)ECellState::Door;
        LevelStructure[0][neswDoorPositions[(int)EDirectionType::South]] = (int)ECellState::Door;
        LevelStructure[neswDoorPositions[(int)EDirectionType::West]][0] = (int)ECellState::Door;
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

EDirectionType ULevelBuilderComponent::GetWallTypeForDoorPosition(int x, int y)
{
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();
    if (x == 0 && y > 0 && y < sizeY - 1)
        return EDirectionType::South;
    if (y == sizeY - 1 && x > 0 && x < sizeY - 1)
        return EDirectionType::East;
    if (x == sizeX - 1 && y > 0 && y < sizeY - 1)
        return EDirectionType::North;
    if (y == 0 && x > 0 && x < sizeX - 1)
        return EDirectionType::West;
    return EDirectionType::NumDirectionTypes;
}

EDirectionType ULevelBuilderComponent::GetWallTypeForBlockPosition(int x, int y)
{
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();
    if (x == 0 && y >= 0 && y < sizeY)
        return EDirectionType::South;
    if (y == sizeY - 1 && x >= 0 && x < sizeY)
        return EDirectionType::East;
    if (x == sizeX - 1 && y >= 0 && y < sizeY)
        return EDirectionType::North;
    if (y == 0 && x >= 0 && x < sizeX)
        return EDirectionType::West;
    return EDirectionType::NumDirectionTypes;
}

FVector2D ULevelBuilderComponent::GetClosestEmptyCell (int x, int y)
{
    int xPos = x;
    int yPos = y;
    const int totalCells = GetNumGridUnitsX() * GetNumGridUnitsY();
    int numCellsVisited = 0;
    EDirectionType actionType = EDirectionType::North;
    int numActionsToTake = 1;
    int numActionsTaken = 0;
    while(numCellsVisited < totalCells)
    {
        if (xPos < GetNumGridUnitsX() && yPos < GetNumGridUnitsY() && !IsGridPositionBlocked(xPos, yPos))
            return FVector2D(xPos, yPos);
        switch (actionType)
        {
            case EDirectionType::North: 
                ++xPos;
                break;
            case EDirectionType::East: 
                ++yPos;
                break;
            case EDirectionType::South: 
                --xPos;
                break;
            case EDirectionType::West: 
                --yPos;
                break;
            default: break;
        }
        ++numActionsTaken;
        if (numActionsTaken == numActionsToTake)
        {
            // change action to the next clockwise action (north goes to east, east to south etc.)
            actionType = (EDirectionType)(((int)actionType + 1) % (int)EDirectionType::NumDirectionTypes);
            if (actionType == EDirectionType::North || actionType == EDirectionType::South)
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
    return ATPGameDemoGameState::GetCellWorldPosition(this, x, y, RoomOffsetX, RoomOffsetY, getCentre);
}

FVector2D ULevelBuilderComponent::FindMostCentralEmptyCell()
{
    return GetClosestEmptyCell(GetNumGridUnitsX() / 2, GetNumGridUnitsY() / 2);
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

void ULevelBuilderComponent::UpdateInnerWallCellActorCounts(FIntPoint roomCoords, bool Increment)
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        for (int row = 1; row < LevelStructure.Num() - 2; ++row)
        {
            for (int col = 1; col < LevelStructure[0].Num() - 2; ++col)
            {
                if (LevelStructure[row][col] == (int)ECellState::Closed)
                {
                    if (Increment)
                    {
                        gameState->ActorEnteredTilePosition(roomCoords, FIntPoint(row, col));
                    }
                    else
                    {
                        gameState->ActorExitedTilePosition(roomCoords, FIntPoint(row, col));
                    }
                }
            }
        }
    }
}

