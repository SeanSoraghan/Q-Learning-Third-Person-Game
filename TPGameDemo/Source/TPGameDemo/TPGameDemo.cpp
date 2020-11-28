// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, TPGameDemo, "TPGameDemo" );

const FString LevelBuilderHelpers::LevelsDir() { return FPaths::/*GameDir*/ProjectDir() + "Content/Levels/"; }

EDirectionType DirectionHelpers::GetOppositeDirection(EDirectionType direction)
{
    return (EDirectionType)(((int)direction + 2) % (int)EDirectionType::NumDirectionTypes);
}

FString DirectionHelpers::GetDisplayString(EDirectionType direction)
{
    switch (direction)
    {
    case EDirectionType::North: return "North";
    case EDirectionType::East: return "East";
    case EDirectionType::South: return "South";
    case EDirectionType::West: return "West";
    default: return "None";
    }
}

FIntPoint LevelBuilderHelpers::GetTargetPointForAction(FIntPoint startingPoint, EDirectionType actionType, int numSpaces /*= 1*/)
{
    FIntPoint endPoint = startingPoint;

    switch(actionType)
    {
        case EDirectionType::North:
        {
            endPoint.X += numSpaces;
            break;
        }
        case EDirectionType::East:
        {
            endPoint.Y += numSpaces;
            break;
        }
        case EDirectionType::South:
        {
            endPoint.X -= numSpaces;
            break;
        }
        case EDirectionType::West:
        {
            endPoint.Y -= numSpaces;
            break;
        }
    }

    return endPoint;
}

bool LevelBuilderHelpers::GridPositionIsValid(FIntPoint position, int sizeX, int sizeY)
{
    return !(position.X < 0 || position.Y < 0 || position.X > sizeX - 1 || position.Y > sizeY -1);
}

/*
Takes in a text file and fills an array with FDirectionSets.
File should be a square grid format with FDirectionSets separated by spaces.
If invertX is true, the lines in the text file will be entered from bottom to top
into the 2D int array (rather than top to bottom).
*/
void LevelBuilderHelpers::FillArrayFromTextFile (FString fileName, TArray<TArray<FDirectionSet>>& arrayRef, bool invertX /*= false*/)
{

    #if ON_SCREEN_DEBUGGING
    if ( ! FPlatformFileManager::Get().GetPlatformFile().FileExists (*fileName))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Cant Find file: | %s |"), *fileName));
        return;
    }
    #endif

    arrayRef.Empty();
    TArray<FString> LevelPolicyStringArray;
    FFileHelper::LoadANSITextFileToStrings(*(fileName), NULL, LevelPolicyStringArray);
    const int numRows = LevelPolicyStringArray.Num();
    for (int row = (invertX ? numRows - 1 : 0); (invertX ? row >= 0 : row < numRows); (invertX ? row-- : row++))
    {
        
        TArray<FString> behaviourStrings; // the optimal direction set at each position
        LevelPolicyStringArray[row].ParseIntoArray (behaviourStrings, *Delimiters::DirectionSetStringDelimiter, 1);
        if (behaviourStrings.Num() > 0)
        {
            arrayRef.Add (TArray<FDirectionSet>());
            TArray<FDirectionSet>& behaviourMapRow = arrayRef[arrayRef.Num() - 1];
            behaviourMapRow.Empty();
            for (int col = 0; col < behaviourStrings.Num(); col++)
            {
                TArray<FString> actionStrings; // the actions to choose from
                behaviourStrings[col].ParseIntoArray(actionStrings, *Delimiters::ActionDelimiter, 1);
                behaviourMapRow.Add(FDirectionSet());
                ensure(actionStrings.Num() <= (int)EDirectionType::NumDirectionTypes && actionStrings.Num() > 0);
                for (int action = 0; action < actionStrings.Num(); ++action)
                {
                    behaviourMapRow[col].EnableDirection((EDirectionType)(FCString::Atoi(*actionStrings[action])));
                }
                ensure(behaviourMapRow[col].IsValid());
            }
        }
    }
}


/*
Writes a 2D int array to a text file. File will have a square grid format with numbers 
separated by spaces. If invertX is true, the lines in the text file will be entered 
from bottom to top (rather than top to bottom).
*/
void LevelBuilderHelpers::WriteArrayToTextFile (TArray<TArray<FDirectionSet>>& arrayRef, FString fileName, bool invertX /*= false*/)
{
    const int numX = arrayRef.Num();
    const int numY = arrayRef[0].Num();
    FString text = FString("");
    for (int x = (invertX ? numX - 1 : 0); (invertX ? x >= 0 : x < numX); (invertX ? x-- : x++))       
    {
        FString row = "";
        for (int y = 0; y < numY; ++y)
        {
            row += arrayRef[x][y].ToString();
            if (y < numY - 1)
                row += Delimiters::DirectionSetStringDelimiter;
            else if (x < numX - 1)
                row += FString("\n");
        }
        text += row;
    }
    FFileHelper::SaveStringToFile(text, * fileName);
}

void LevelBuilderHelpers::FillArrayFromTextFile(FString fileName, TArray<TArray<int>>& arrayRef, bool invertX)
{
#if ON_SCREEN_DEBUGGING
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*fileName))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Cant Find file: | %s |"), *fileName));
        return;
    }
#endif

    arrayRef.Empty();
    TArray<FString> LevelPolicyStringArray;
    FFileHelper::LoadANSITextFileToStrings(*(fileName), NULL, LevelPolicyStringArray);
    const int numRows = LevelPolicyStringArray.Num();
    for (int row = (invertX ? numRows - 1 : 0); (invertX ? row >= 0 : row < numRows); (invertX ? row-- : row++))
    {

        TArray<FString> actionStrings;
        LevelPolicyStringArray[row].ParseIntoArray(actionStrings, *Delimiters::DirectionSetStringDelimiter, 1);
        if (actionStrings.Num() > 0)
        {
            arrayRef.Add(TArray<int>());
            arrayRef[arrayRef.Num() - 1].Empty();
            for (int action = 0; action < actionStrings.Num(); action++)
            {
                arrayRef[arrayRef.Num() - 1].Add(FCString::Atoi(*actionStrings[action]));
            }
        }
    }
}

void LevelBuilderHelpers::WriteArrayToTextFile(TArray<TArray<int>>& arrayRef, FString fileName, bool invertX)
{
    const int numX = arrayRef.Num();
    const int numY = arrayRef[0].Num();
    FString text = FString("");
    for (int x = (invertX ? numX - 1 : 0); (invertX ? x >= 0 : x < numX); (invertX ? x-- : x++))
    {
        FString row = "";
        for (int y = 0; y < numY; ++y)
        {
            row += FString::FromInt(arrayRef[x][y]);
            if (y < numY - 1)
                row += Delimiters::DirectionSetStringDelimiter;
            else if (x < numX - 1)
                row += FString("\n");
        }
        text += row;
    }
    FFileHelper::SaveStringToFile(text, *fileName);
}

void LevelBuilderHelpers::PrintArray(TArray<TArray<FDirectionSet>>& arrayRef)
{
    UE_LOG(LogTemp, Warning, TEXT("Level Array--------------"));
    for (int row = arrayRef.Num() - 1; row >= 0; --row)
    {
        FString s = "";

        for (int col = 0; col < arrayRef[row].Num(); col++)
            s += FString("|") + " " + arrayRef[row][col].ToString(true) + " ";

        UE_LOG(LogTemp, Warning, TEXT("%s"), *s);
        //GEngine->AddOnScreenDebugMessage(-1, 1000.f, FColor::Red, s);
    }
}

void LevelBuilderHelpers::PrintArray(TArray<TArray<int>>& arrayRef)
{
    UE_LOG(LogTemp, Warning, TEXT("Level Array--------------"));
    for (int row = arrayRef.Num() - 1; row >= 0; --row)
    {
        FString s = "";

        for (int col = 0; col < arrayRef[row].Num(); col++)
            s += FString("|") +
            (arrayRef[row][col] == -1 ? FString("") : FString(" ")) +
            FString::FromInt(arrayRef[row][col]);

        UE_LOG(LogTemp, Warning, TEXT("%s"), *s);
        //GEngine->AddOnScreenDebugMessage(-1, 1000.f, FColor::Red, s);
    }
}

//====================================================================================================
// ActionQValuesAndRewards
//====================================================================================================

const TArray<float> ActionQValuesAndRewards::GetQValues() const
{
    return ActionQValues;
}

const TArray<float> ActionQValuesAndRewards::GetRewards() const
{
    return ActionRewards;
}

const float ActionQValuesAndRewards::GetOptimalQValueAndActions(FDirectionSet& Actions) const
{
    ensure(ActionQValues.Num() == (int)EDirectionType::NumDirectionTypes);
    float optimalQValue = ActionQValues[0] + ActionRewardTrackers[0].GetAverage();
    Actions.EnableDirection((EDirectionType)0);
    for (int i = 1; i < ActionQValues.Num(); ++i)
    {
        float currentV = ActionQValues[i] + ActionRewardTrackers[i].GetAverage();
        if (currentV >= optimalQValue)
        {
            if (currentV > optimalQValue)
            {
                Actions.Clear();
                optimalQValue = currentV;
            }
            Actions.EnableDirection((EDirectionType)i);
        }
    }
    return optimalQValue;
}

const float ActionQValuesAndRewards::GetOptimalQValueAndActions_Valid(FDirectionSet& ValidActions) const
{
    ensure(ActionQValues.Num() == (int)EDirectionType::NumDirectionTypes);
    EDirectionType direction = EDirectionType::North;
    FDirectionSet optimalActions;
    optimalActions.Clear();
    while (!ValidActions.CheckDirection(direction))
        direction = (EDirectionType)((int)direction + 1);
    float optimalQValue = ActionQValues[(int)direction] + ActionRewardTrackers[(int)direction].GetAverage();
    optimalActions.EnableDirection(direction);
    
    for (int i = (int)direction + 1; i < ActionQValues.Num(); ++i)
    {
        if (ValidActions.CheckDirection((EDirectionType)i))
        {
            float currentV = ActionQValues[i] + ActionRewardTrackers[i].GetAverage();
            if (currentV >= optimalQValue)
            {
                if (currentV > optimalQValue)
                {
                    optimalActions.Clear();
                    optimalQValue = currentV;
                }
                optimalActions.EnableDirection((EDirectionType)i);
            }
        }
    }
    ValidActions = optimalActions;
    return optimalQValue;
}

FRoomPositionPair ActionTargets::GetActionTarget(EDirectionType actionType) const
{
    return Targets[(int)actionType];
}

void ActionQValuesAndRewards::ResetQValues()
{
    for (int actionType = 0; actionType < (int)EDirectionType::NumDirectionTypes; ++actionType)
        ActionQValues[actionType] = 0.0f;
}

void ActionQValuesAndRewards::UpdateQValue(EDirectionType actionType, float learningRate, float deltaQ)
{
    float currentQValue = ActionQValues[(int)actionType];
    ActionQValues[(int)actionType] = (1.0f - learningRate) * currentQValue + deltaQ;
}

bool ActionTargets::IsGoalState() const
{
    return IsGoal;
}

void ActionTargets::SetIsGoal(bool isGoal)
{
    IsGoal = isGoal;
}

void ActionTargets::SetValid(bool valid)
{
    IsValid = valid;
}

bool ActionTargets::IsStateValid() const
{
    return IsValid;
}

void ActionTargets::SetActionTarget(EDirectionType actionType, FRoomPositionPair roomAndPosition)
{
    ensure(Targets.Num() == (int)EDirectionType::NumDirectionTypes);
    Targets[(int)actionType] = roomAndPosition;
}

//====================================================================================================
// RoomState
//====================================================================================================
void RoomState::DisableRoom()
{
    RoomStatus = RoomState::Status::Dead;
}