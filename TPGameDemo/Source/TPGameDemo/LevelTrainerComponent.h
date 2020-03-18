// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "Engine/EngineBaseTypes.h"
//#include "Engine/EngineTypes.h"
#include "Runnable.h"
//#include "MazeActor.h"
#include "Components/ActorComponent.h"
#include "TPGameDemoGameState.h"
#include "LevelTrainerComponent.generated.h"

class ULevelTrainerComponent;

//====================================================================================================
// LevelTrainerRunnable
//====================================================================================================

class LevelTrainerRunnable : public FRunnable
{
public:
    LevelTrainerRunnable(ULevelTrainerComponent& trainerComponent);
    ~LevelTrainerRunnable();
    // FRunnable interface.
    virtual bool   Init() override;
    virtual uint32 Run()  override;
    virtual void   Stop() override;
    virtual void   Exit() override;

    void Wake();

    void StartTraining();
    void PauseTraining();
    FThreadSafeBool IsTraining = false;
private:
    ULevelTrainerComponent& TrainerComponent;
    FThreadSafeBool ShouldTrain = false;
	FThreadSafeBool ThreadShouldExit = false;
    FEvent* WaitEvent;
    FCriticalSection TrainSection;
};

namespace GridTrainingConstants
{
    static const float GoalReward = 1.0f;
    static const float MovementCost = -0.04f;
    static const float LearningRate = 0.5f;
    static const float DiscountFactor = 0.9f;
};

//====================================================================================================
// Grid State
//====================================================================================================

class GridState
{
public:
    const TArray<float> GetQValues() const;
    const TArray<float> GetRewards() const;

    const float GetOptimalQValueAndActions(FDirectionSet& Actions) const;

    void UpdateQValue(EDirectionType actionType, float deltaQ);
    void ResetQValues();

    void SetIsGoal(bool isGoal);
    bool IsGoalState() const;

    FIntPoint GetActionTarget(EDirectionType actionType) const;

    void SetValid(bool valid);
    bool IsStateValid();

    void SetActionTarget(EDirectionType actionType, FIntPoint position);
private:
    bool IsGoal = false;
    bool IsValid = true;
    TArray<float> ActionQValues {0.0f, 0.0f, 0.0f, 0.0f};
    TArray<float> ActionRewards {GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost, 
                                 GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost};
    TArray<FIntPoint> ActionTargets {{0,0}, {0,0}, {0,0}, {0,0}};
};

//====================================================================================================
// ULevelTrainerComponent
//====================================================================================================
DECLARE_EVENT(ULevelTrainerComponent, LevelTrainedEvent);
DECLARE_DYNAMIC_DELEGATE(FOnLevelTrained);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPGAMEDEMO_API ULevelTrainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
    friend class LevelTrainerRunnable;

	// Sets default values for this component's properties
	ULevelTrainerComponent();
    void BeginDestroy() override;
    void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

    UFUNCTION(BlueprintCallable, Category = "Level Training")
    void UpdateEnvironmentForLevel(FString levelName);
    
    UFUNCTION(BlueprintCallable, Category = "Level Training")
    void RegisterLevelTrainedCallback(const FOnLevelTrained& Callback);

    UFUNCTION(BlueprintCallable, Category = "Level Training")
    void StartTraining();

    UFUNCTION(BlueprintCallable, Category = "Level Training")
    void PauseTraining();

    UFUNCTION(BlueprintCallable, Category = "Level Training")
    void ResetGoalPosition();

    UFUNCTION(BlueprintCallable, Category = "Level Training")
    float GetTrainingProgress();

    /* These properties need to be set from blueprint around the time of level setup. */
    UPROPERTY(BlueprintReadWrite, Category = "Level Trainer Room Position")
    FIntPoint RoomCoords = FIntPoint(0,0);

private:
    FThreadSafeBool LevelTrained = false;
    FString CurrentLevelName = FString("");
    TSharedPtr<FRunnableThread> TrainerThread;

    TSharedPtr<LevelTrainerRunnable> TrainerRunnable;
    FCriticalSection ClientSection;

    BehaviourMap GetBehaviourMap();
    void ClearEnvironment();
	TArray<TArray<GridState>> Environment;
    void InitTrainerThread();
    void TrainNextGoalPosition(int numSimulationsPerStartingPosition, int maxNumActionsPerSimulation);
    // Simulate a run through the while keepting track of the average deltaQ and the num actions taken. (these will be used to measure convergence)
    void SimulateRun(FIntPoint startingStatePosition, int maxNumActions, float& averageDeltaQ, int& numActionsTaken);
    void IncrementGoalPosition();
    FThreadSafeCounter TrainingPosition = 0;
    FThreadSafeCounter MaxTrainingPosition = 0;
    GridState& GetState(FIntPoint statePosition);
    FIntPoint CurrentGoalPosition {0,0};

    LevelTrainedEvent OnLevelTrained;
    //FThreadSafeCounter NumUnfinishedTasks = 0;
};


