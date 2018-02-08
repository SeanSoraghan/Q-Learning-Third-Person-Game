// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.cpp"
#include "LevelBuilderComponent.h"
#include "LevelTrainerComponent.h"

//====================================================================================================
// GridState
//====================================================================================================
const TArray<float> GridState::GetQValues() const 
{
    return ActionQValues;
}

const TArray<float> GridState::GetRewards() const 
{
    return ActionRewards;
}

const float GridState::GetOptimalQValueAndActions(TArray<EActionType>* ActionsArrayToSet) const 
{
    const bool updateArray = ActionsArrayToSet != nullptr;
    if (updateArray)
        ActionsArrayToSet->Empty();
    ensure(ActionQValues.Num() == (int)EActionType::NumActionTypes);
    float optimalQValue = ActionQValues[0];
    if (updateArray)
        ActionsArrayToSet->Add((EActionType)0);
    for (int i = 1; i < ActionQValues.Num(); ++i)
    {
        float currentV = ActionQValues[i];
        if (currentV <= optimalQValue)
        {
            if (currentV < optimalQValue)
            {
                if (updateArray)
                    ActionsArrayToSet->Empty();
                optimalQValue = currentV;
            }
            if (updateArray)
                ActionsArrayToSet->Add((EActionType)i);
        }
    }
    return optimalQValue;
} 

FIntPoint GridState::GetActionTarget(EActionType actionType) const
{
    return ActionTargets[(int)actionType];
}

void GridState::UpdateQValue(EActionType actionType, float deltaQ) 
{
    ActionQValues[(int)actionType] += deltaQ;
}

bool GridState::IsGoalState() const
{
    return IsGoal;
}

void GridState::SetIsGoal(bool isGoal)
{
    IsGoal = isGoal;
    if (IsGoal)
        ActionRewards = {GridTrainingConstants::GoalReward, GridTrainingConstants::GoalReward,
                         GridTrainingConstants::GoalReward, GridTrainingConstants::GoalReward};
}

void GridState::SetValid(bool valid)
{
    IsValid = valid;
}

bool GridState::IsStateValid()
{
    return IsValid;
}

void GridState::SetActionTarget(EActionType actionType, FIntPoint position)
{
    ensure(ActionTargets.Num() == (int)EActionType::NumActionTypes);
    ActionTargets[(int)actionType] = position;
}

//====================================================================================================
// ULevelTrainerComponent
//====================================================================================================

ULevelTrainerComponent::ULevelTrainerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULevelTrainerComponent::UpdateEnvironmentForLevel(FString levelName)
{
    FString CurrentLevelPath = LevelBuilderConstants::LevelsDir + levelName + ".txt";
    TArray<TArray<int>> LevelStructure;
    FillArrayFromTextFile (CurrentLevelPath, LevelStructure);
    Environment.Empty();
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();
    for (int x = 0; x < sizeX; ++x)
    {
        Environment.Add(TArray<GridState>());
        TArray<GridState>& row = Environment[x];
        for (int y = 0; y < sizeY; ++y)
        {
            row.Add(GridState());
            GridState& state = row[y];
            state.SetValid(LevelStructure[x][y] != 1);
            if (state.IsStateValid())
            {
                // What is the correct way to map actions to states here ... what is the correct x, y indexing ...?

                if (y == sizeY - 1 || LevelStructure[x + 1][y] == 1)
                    state.SetActionTarget(EActionType::North, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::North, FIntPoint(x + 1, y));

                if (x == sizeX - 1 || LevelStructure[x][y + 1] == 1)
                    state.SetActionTarget(EActionType::East, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::East, FIntPoint(x, y + 1));

                if (y == 0 || LevelStructure[x - 1][y] == 1)
                    state.SetActionTarget(EActionType::South, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::South, FIntPoint(x + 1, y));

                if (y == sizeY - 1 || LevelStructure[x + 1][y] == 1)
                    state.SetActionTarget(EActionType::West, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::West, FIntPoint(x + 1, y));
            }
        }
    }
}

void ULevelTrainerComponent::Train()
{

}

void ULevelTrainerComponent::SimulateRun(FIntPoint startingStatePosition, int maxNumActions)
{
    int numActionsTaken = 0;
    bool goalReached = false;
    GridState& currentState = GetState(startingStatePosition);
    while (numActionsTaken < maxNumActions && !goalReached)
    {
        TArray<EActionType> optimalActions;
        currentState.GetOptimalQValueAndActions(&optimalActions);
        ensure(optimalActions.Num() > 0);
        EActionType actionToTake = optimalActions[0];
        if (optimalActions.Num() > 1)
        {
            int actionIndex = FMath::FloorToInt((FMath::Rand() / (float)RAND_MAX) * optimalActions.Num());
            while (actionIndex == optimalActions.Num())
                actionIndex = FMath::FloorToInt((FMath::Rand() / (float)RAND_MAX) * optimalActions.Num());
            actionToTake = optimalActions[actionIndex];
        }
        const float maxNextReward = GetState(currentState.GetActionTarget(actionToTake)).GetOptimalQValueAndActions(nullptr);
        const float currentQValue = currentState.GetQValues()[(int)actionToTake];
        const float discountedNextReward = GridTrainingConstants::DiscountFactor * maxNextReward;
        const float immediateReward = currentState.GetRewards()[(int)actionToTake];
        const float deltaQ = GridTrainingConstants::LearningRate * (immediateReward + discountedNextReward - currentQValue);
        currentState.UpdateQValue(actionToTake, deltaQ);
        currentState = GetState(currentState.GetActionTarget(actionToTake));
        ++numActionsTaken;
        if (currentState.IsGoalState())
            goalReached = true;
    }
}

GridState& ULevelTrainerComponent::GetState(FIntPoint statePosition)
{
    return Environment[statePosition.X][statePosition.Y];
}




