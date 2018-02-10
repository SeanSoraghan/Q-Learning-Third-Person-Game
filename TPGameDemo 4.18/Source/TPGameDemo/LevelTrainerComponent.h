// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "Engine/EngineBaseTypes.h"
//#include "Engine/EngineTypes.h"
#include "Runnable.h"
#include "MazeActor.h"
#include "Components/ActorComponent.h"
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

private:
    FThreadSafeBool LevelTrained = false;
    FString CurrentLevelName = FString("");
    /** A non-0 value indicates that UE is exiting. */
    FThreadSafeCounter AppExitingCounter = 0;
    /** Thread on which the WAAPI connection is monitored. */
    TSharedPtr<FRunnableThread> TrainerThread;
    /** The connection to WAAPI is monitored by this connection handler.
    *  It tries to reconnect when connection is lost, and continuously polls WAAPI for the connection status when WAAPI is connected.
    *  This behaviour can be disabled in AkSettings using the AutoConnectToWaapi boolean option.
    */
    TSharedPtr<LevelTrainerRunnable> TrainerRunnable;
    FCriticalSection ClientSection;

	TArray<TArray<GridState>> Environment;
    void TrainNextGoalPosition(int numSimulationsPerStartingPosition, int maxNumActionsPerSimulation);
    void SimulateRun(FIntPoint startingStatePosition, int maxNumActions);
    void IncrementGoalPosition();
    GridState& GetState(FIntPoint statePosition);
    FIntPoint CurrentGoalPosition {0,0};

    LevelTrainedEvent OnLevelTrained;
};


