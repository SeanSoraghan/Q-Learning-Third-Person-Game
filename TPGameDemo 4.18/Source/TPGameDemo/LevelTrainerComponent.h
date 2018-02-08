// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MazeActor.h"
#include "Components/ActorComponent.h"
#include "LevelTrainerComponent.generated.h"

namespace GridTrainingConstants
{
    static const float GoalReward = 1.0f;
    static const float MovementCost = -0.04f;
    static const float LearningRate = 0.5f;
    static const float DiscountFactor = 0.9f;
};

class GridState
{
public:
    const TArray<float> GetQValues() const;
    const TArray<float> GetRewards() const;

    const float GetOptimalQValueAndActions(TArray<EActionType>* ActionsArrayToSet) const;

    void UpdateQValue(EActionType actionType, float deltaQ);

    void SetIsGoal(bool isGoal);
    bool IsGoalState() const;

    FIntPoint GetActionTarget(EActionType actionType) const;

    void SetValid(bool valid);
    bool IsStateValid();

    void SetActionTarget(EActionType actionType, FIntPoint position);
private:
    bool IsGoal = false;
    bool IsValid = true;
    TArray<float> ActionQValues {0.0f, 0.0f, 0.0f, 0.0f};
    TArray<float> ActionRewards {GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost, 
                                 GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost};
    TArray<FIntPoint> ActionTargets {{0,0}, {0,0}, {0,0}, {0,0}};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPGAMEDEMO_API ULevelTrainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULevelTrainerComponent();

    void Train();
    void UpdateEnvironmentForLevel(FString levelName);
private:	
	TArray<TArray<GridState>> Environment;
    void SimulateRun(FIntPoint startingStatePosition, int maxNumActions);
    GridState& GetState(FIntPoint statePosition);
    FIntPoint CurrentGoalPosition {0,0};
};
