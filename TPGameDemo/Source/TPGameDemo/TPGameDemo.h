// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <set>
#include "Engine.h"
#include "TPGameDemo.generated.h"

#define ON_SCREEN_DEBUGGING 0

UENUM(BlueprintType)
enum class EDirectionType : uint8
{
    North UMETA (DisplayName = "North"),
    East  UMETA (DisplayName = "East"),
    South UMETA (DisplayName = "South"),
    West  UMETA (DisplayName = "West"),
    NumDirectionTypes UMETA(DisplayName = "None")
};

namespace Delimiters
{
    const FString DirectionSetStringDelimiter = " ";
    const FString ActionDelimiter = "-";
};

USTRUCT(BlueprintType)
struct FDirectionSet
{
    GENERATED_USTRUCT_BODY()

    // For easy construction
    enum class EDirectionFlag : uint8
    {
        NorthF = 1 << 0,
        EastF = 1 << 1,
        SouthF = 1 << 2,
        WestF = 1 << 3,
        NumDirectionFlags = 1 << 4
    };

    FDirectionSet()
    {
        DirectionSelectionSet.empty();
    }

    FDirectionSet(uint8 directionsMask) : DirectionsMask(directionsMask) 
    {
        DirectionSelectionSet.empty();
        for (int d = 0; d < (int)EDirectionType::NumDirectionTypes; ++d)
            if (CheckDirection((EDirectionType)d))
                DirectionSelectionSet.insert((EDirectionType)d);
    }

    EDirectionType ChooseDirection() 
    {
        if (!IsValid())
            return EDirectionType::NumDirectionTypes;

        float r = rand() / (float)RAND_MAX;
        int choice = r == 1.0f ? DirectionSelectionSet.size() - 1 : (int)(r * DirectionSelectionSet.size());
        std::set<EDirectionType>::iterator it = DirectionSelectionSet.begin();
        advance(it, choice);
        ensure((int)*it >= (int)EDirectionType::North && (int)*it < (int)EDirectionType::NumDirectionTypes);
        return *it;
    }

    bool IsValid() { return DirectionsMask & ((uint8)pow(2, (int)EDirectionType::NumDirectionTypes) - 1); }

    bool CheckDirection(EDirectionType direction) { return DirectionsMask & (1 << (int)direction); }
    
    void Clear()
    {
        DirectionsMask = 0;
        DirectionSelectionSet.empty();
    }

    void EnableDirection(EDirectionType direction) 
    { 
        DirectionsMask |= (1 << (int)direction);
        DirectionSelectionSet.insert(direction);
    }
    void DisableDirection(EDirectionType direction) 
    { 
        DirectionsMask &= (~(1 << (int)direction));
        DirectionSelectionSet.erase(direction);
    }

    FString ToString(bool padSpaces = false) 
    {
        FString str = "";
        if (CheckDirection(EDirectionType::North))
            str += FString::FromInt((int)EDirectionType::North);
        else if (padSpaces)
            str += " ";
        for (int d = (int)EDirectionType::East; d < (int)EDirectionType::NumDirectionTypes; ++d)
        {
            if (CheckDirection((EDirectionType)d))
                str += Delimiters::ActionDelimiter + FString::FromInt(d);
            else if (padSpaces)
                str += Delimiters::ActionDelimiter + " ";
        }
        return str;
    }

    uint8 DirectionsMask = 0;
    std::set<EDirectionType> DirectionSelectionSet;
};

UENUM(BlueprintType)
enum class ECellState : uint8
{
    Open UMETA (DisplayName = "Open"),
    Closed  UMETA (DisplayName = "Closed"),
    Door UMETA (DisplayName = "Door"),
    NumStates
};

USTRUCT(Blueprintable)
struct FRoomPositionPair
{
    GENERATED_USTRUCT_BODY()
    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Rooms And Positions")
    FIntPoint RoomCoords;
    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Rooms And Positions")
    FIntPoint PositionInRoom;
};

namespace DirectionHelpers
{
    EDirectionType GetOppositeDirection(EDirectionType direction);
    FString GetDisplayString(EDirectionType direction);
};

namespace LevelBuilderHelpers
{
    const FString LevelsDir();

    FIntPoint GetTargetPointForAction(FIntPoint startingPoint, EDirectionType actionType, int numSpaces = 1);

    bool GridPositionIsValid(FIntPoint position, int sizeX, int sizeY);

    /*
    Takes in a text file and fills an array with FDirectionSets.
    File should be a square grid format with FDirectionSets separated by spaces.
    If invertX is true, the lines in the text file will be entered from bottom to top
    into the 2D int array (rather than top to bottom).
    */
    void FillArrayFromTextFile (FString fileName, TArray<TArray<FDirectionSet>>& arrayRef, bool invertX = false);

    /*
    Writes a 2D FDirectionSet array to a text file. File will have a square grid format with FDirectionSets 
    separated by spaces. If invertX is true, the lines in the text file will be entered 
    from bottom to top (rather than top to bottom).
    */
    void WriteArrayToTextFile (TArray<TArray<FDirectionSet>>& arrayRef, FString fileName, bool invertX = false);

    /*
    Takes in a text file and fills an array with ints.
    File should be a square grid format with ints separated by spaces.
    If invertX is true, the lines in the text file will be entered from bottom to top
    into the 2D int array (rather than top to bottom).
    */
    void FillArrayFromTextFile(FString fileName, TArray<TArray<int>>& arrayRef, bool invertX = false);

    /*
    Writes a 2D int array to a text file. File will have a square grid format with ints
    separated by spaces. If invertX is true, the lines in the text file will be entered
    from bottom to top (rather than top to bottom).
    */
    void WriteArrayToTextFile(TArray<TArray<int>>& arrayRef, FString fileName, bool invertX = false);

    void PrintArray(TArray<TArray<FDirectionSet>>& arrayRef);
    void PrintArray(TArray<TArray<int>>& arrayRef);
};