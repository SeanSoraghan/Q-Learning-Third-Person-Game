// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.h"
#include "LevelTrainerComponent.h"

//====================================================================================================
// LevelTrainerRunnable
//====================================================================================================

LevelTrainerRunnable::LevelTrainerRunnable(ULevelTrainerComponent& trainerComponent) : TrainerComponent(trainerComponent)
{
    WaitEvent = FPlatformProcess::GetSynchEventFromPool(true);
}

LevelTrainerRunnable::~LevelTrainerRunnable()
{
    FPlatformProcess::ReturnSynchEventToPool(WaitEvent);
    WaitEvent = nullptr;
}


void LevelTrainerRunnable::Wake()
{
    WaitEvent->Trigger();
}

/* FRunnable interface */
bool LevelTrainerRunnable::Init()
{
    return true;
}

uint32 LevelTrainerRunnable::Run()
{
    IsTraining = true;
    ensure(!IsInGameThread());
    while (!ThreadShouldExit)
    {
        if (ShouldTrain)
        {
            TrainerComponent.TrainNextGoalPosition(300, 300); //int numSimulationsPerStartingPosition, int maxNumActionsPerSimulation
            if (TrainerComponent.LevelTrained)
                ThreadShouldExit = true;
        }
        IsTraining = false;
    }
    IsTraining = false;
    return 0;
}

void LevelTrainerRunnable::StartTraining()
{
    ShouldTrain = true;
}

void LevelTrainerRunnable::PauseTraining()
{
    ShouldTrain = false;
}

void LevelTrainerRunnable::Stop()
{
    ThreadShouldExit = true;
}

void LevelTrainerRunnable::Exit()
{
    ThreadShouldExit = true;
}

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

const float GridState::GetOptimalQValueAndActions(TArray<EDirectionType>* ActionsArrayToSet) const 
{
    const bool updateArray = ActionsArrayToSet != nullptr;
    if (updateArray)
        ActionsArrayToSet->Empty();
    ensure(ActionQValues.Num() == (int)EDirectionType::NumDirectionTypes);
    float optimalQValue = ActionQValues[0];
    if (updateArray)
        ActionsArrayToSet->Add((EDirectionType)0);
    for (int i = 1; i < ActionQValues.Num(); ++i)
    {
        float currentV = ActionQValues[i];
        if (currentV >= optimalQValue)
        {
            if (currentV > optimalQValue)
            {
                if (updateArray)
                    ActionsArrayToSet->Empty();
                optimalQValue = currentV;
            }
            if (updateArray)
                ActionsArrayToSet->Add((EDirectionType)i);
        }
    }
    return optimalQValue;
} 

FIntPoint GridState::GetActionTarget(EDirectionType actionType) const
{
    return ActionTargets[(int)actionType];
}

void GridState::ResetQValues()
{
    for (int actionType = 0; actionType < (int)EDirectionType::NumDirectionTypes; ++actionType)
        ActionQValues[actionType] = 0.0f;
}

void GridState::UpdateQValue(EDirectionType actionType, float deltaQ) 
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
    else
        ActionRewards = {GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost,
                         GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost};
}

void GridState::SetValid(bool valid)
{
    IsValid = valid;
}

bool GridState::IsStateValid()
{
    return IsValid;
}

void GridState::SetActionTarget(EDirectionType actionType, FIntPoint position)
{
    ensure(ActionTargets.Num() == (int)EDirectionType::NumDirectionTypes);
    ActionTargets[(int)actionType] = position;
}

//====================================================================================================
// ULevelTrainerComponent
//====================================================================================================

static FThreadSafeCounter ThreadCounter;

ULevelTrainerComponent::ULevelTrainerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULevelTrainerComponent::BeginDestroy()
{
    if (TrainerRunnable.IsValid())
    {
        TrainerRunnable->Exit();
        TrainerRunnable->Wake();
    }
    if (TrainerThread.IsValid())
    {
        if (!TrainerThread->Kill(true))
        {
            UE_LOG(LogTemp, Error, TEXT("Trainer Thread Failed to Exit!"));
        }
    }
    if (TrainerRunnable.IsValid())
    {
        while (TrainerRunnable->IsTraining){}
    }
    Super::BeginDestroy();
}

void ULevelTrainerComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
    if (LevelTrained)
    {
        OnLevelTrained.Broadcast();
        LevelTrained = false;
    }
}

void ULevelTrainerComponent::StartTraining()
{
    if(TrainerRunnable.IsValid())
        TrainerRunnable->Stop();
    if (TrainerThread.IsValid())
    {
        TrainerThread->WaitForCompletion();
        TrainerThread.Reset();
    }
    if(TrainerRunnable.IsValid())
        TrainerRunnable.Reset();
    InitTrainerThread();
    TrainerRunnable->StartTraining();
}

void ULevelTrainerComponent::PauseTraining()
{
    if (TrainerRunnable.IsValid())
        TrainerRunnable->PauseTraining();
}

void ULevelTrainerComponent::InitTrainerThread()
{
    TrainerRunnable = MakeShareable(new LevelTrainerRunnable(*this));
    FString ThreadName(FString::Printf(TEXT("LevelTrainerThread%i"), ThreadCounter.Increment()));
    TrainerThread = MakeShareable(FRunnableThread::Create(TrainerRunnable.Get(), *ThreadName, 0,
                                  EThreadPriority::TPri_BelowNormal));
}

void ULevelTrainerComponent::RegisterLevelTrainedCallback(const FOnLevelTrained& Callback)
{
    OnLevelTrained.AddLambda([Callback]()
    {
        Callback.ExecuteIfBound();
    });
}

void ULevelTrainerComponent::UpdateEnvironmentForLevel(FString levelName)
{
    CurrentLevelName = levelName;
    FString CurrentLevelPath = LevelBuilderHelpers::LevelsDir() + CurrentLevelName + ".txt";
    TArray<TArray<int>> LevelStructure;
    LevelBuilderHelpers::FillArrayFromTextFile (CurrentLevelPath, LevelStructure);
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
            state.SetValid(LevelStructure[x][y] == (int)ECellState::Open);
            if (state.IsStateValid())
            {
                for (int a = 0; a < (int)EDirectionType::NumDirectionTypes; ++a)
                {
                    EDirectionType actionType = EDirectionType(a);
                    FIntPoint targetPoint = LevelBuilderHelpers::GetTargetPointForAction(FIntPoint(x,y), actionType);
                    
                    const bool targetValid = LevelBuilderHelpers::GridPositionIsValid(targetPoint, sizeX, sizeY) &&
                                             LevelStructure[targetPoint.X][targetPoint.Y] == (int)ECellState::Open;

                    state.SetActionTarget(actionType, targetValid ? targetPoint : FIntPoint(x,y));
                }
            }
        }
    }
}

void ULevelTrainerComponent::TrainNextGoalPosition(int numSimulationsPerStartingPosition, int maxNumActionsPerSimulation)
{
    ClearEnvironment();
    if (GetState(CurrentGoalPosition).IsStateValid())
    {
        GetState(CurrentGoalPosition).SetIsGoal(true);
        for(int x = 0; x < Environment.Num(); ++x)
        {
            for(int y = 0; y < Environment[0].Num(); ++y)
            {
                for (int s = 0; s < numSimulationsPerStartingPosition; ++s)
                {
                    if (GetState(FIntPoint(x,y)).IsStateValid() && FIntPoint(x,y) != CurrentGoalPosition)
                    {
                        SimulateRun(FIntPoint(x,y), maxNumActionsPerSimulation);
                    }
                }
            }
        }
        FString CurrentPositionString = FString::FromInt(CurrentGoalPosition.X) + FString("_") + FString::FromInt(CurrentGoalPosition.Y);
        //Create string and save to text file.
        FString CurrentPositionFileName = LevelBuilderHelpers::LevelsDir() + CurrentLevelName + "/" + CurrentPositionString + ".txt";
        TArray<TArray<int>> envArray = GetEnvironmentIntArray();
        //LevelBuilderHelpers::PrintArray(envArray);
        LevelBuilderHelpers::WriteArrayToTextFile(envArray, CurrentPositionFileName);
        GetState(CurrentGoalPosition).SetIsGoal(false);
    }
    IncrementGoalPosition();
}

void ULevelTrainerComponent::ResetGoalPosition()
{
    CurrentGoalPosition = FIntPoint(0,0);
}

TArray<TArray<int>> ULevelTrainerComponent::GetEnvironmentIntArray()
{
    TArray<TArray<int>> outArray;
    for (int x = 0; x < Environment.Num(); ++x)
    {
        outArray.Add(TArray<int>());
        for (int y = 0; y < Environment[0].Num(); ++y)
        {
            if (!GetState(FIntPoint(x,y)).IsStateValid())
            {
                outArray[x].Add(-1);
            }
            else
            {
                TArray<EDirectionType> optimalActions;
                GetState(FIntPoint(x,y)).GetOptimalQValueAndActions(&optimalActions);
                ensure(optimalActions.Num() > 0);
                EDirectionType actionToTake = optimalActions[0];
                if (optimalActions.Num() > 1)
                {
                    int actionIndex = FMath::RandRange(0, optimalActions.Num() - 1);
                    actionToTake = optimalActions[actionIndex];
                }
                outArray[x].Add((int)actionToTake);
            }
        }
    }
    return outArray;
}

void ULevelTrainerComponent::ClearEnvironment()
{
    for(int x = 0; x < Environment.Num(); ++x)
    {
        for(int y = 0; y < Environment[0].Num(); ++y)
        {
            GetState(FIntPoint(x,y)).ResetQValues();
            // Move in from edges if on an edge.
            if (x == 0)
            {
                GetState(FIntPoint(x,y)).UpdateQValue(EDirectionType::North, 100.0f);
            }
            else if (y == 0)
            {
                GetState(FIntPoint(x,y)).UpdateQValue(EDirectionType::East, 100.0f);
            }
            else if (x == Environment.Num() - 1)
            {
                GetState(FIntPoint(x,y)).UpdateQValue(EDirectionType::South, 100.0f);
            }
            else if (y == Environment[0].Num() - 1)
            {
                GetState(FIntPoint(x,y)).UpdateQValue(EDirectionType::West, 100.0f);
            }
        }
    }
}

void ULevelTrainerComponent::SimulateRun(FIntPoint startingStatePosition, int maxNumActions)
{
    int numActionsTaken = 0;
    bool goalReached = false;
    if (GetState(startingStatePosition).IsGoalState())
            goalReached = true;
    FIntPoint currentPosition = startingStatePosition;
    while (numActionsTaken < maxNumActions && !goalReached)
    {
        TArray<EDirectionType> optimalActions;
        GridState& currentState = GetState(currentPosition);
        currentState.GetOptimalQValueAndActions(&optimalActions);
        ensure(optimalActions.Num() > 0);
        EDirectionType actionToTake = optimalActions[0];
        if (optimalActions.Num() > 1)
        {
            int actionIndex = FMath::RandRange(0, optimalActions.Num() - 1);
            actionToTake = optimalActions[actionIndex];
        }
        const float maxNextReward = GetState(currentState.GetActionTarget(actionToTake)).GetOptimalQValueAndActions(nullptr);
        const float currentQValue = currentState.GetQValues()[(int)actionToTake];
        const float discountedNextReward = GridTrainingConstants::DiscountFactor * maxNextReward;
        const float immediateReward = currentState.GetRewards()[(int)actionToTake];
        const float deltaQ = GridTrainingConstants::LearningRate * (immediateReward + discountedNextReward - currentQValue);
        currentState.UpdateQValue(actionToTake, deltaQ);
        currentPosition = currentState.GetActionTarget(actionToTake);
        ++numActionsTaken;
        if (GetState(currentPosition).IsGoalState())
            goalReached = true;
    }
}

void ULevelTrainerComponent::IncrementGoalPosition()
{
    if (CurrentGoalPosition.Y == Environment[0].Num() - 1)
    {
        if (CurrentGoalPosition.X == Environment.Num() - 1)
        {
            LevelTrained = true;
            return;
        }
        CurrentGoalPosition.Y = 0;
        ++CurrentGoalPosition.X;
        return;
    }
    ++CurrentGoalPosition.Y;
}

GridState& ULevelTrainerComponent::GetState(FIntPoint statePosition)
{
    ensure(Environment.Num() > statePosition.X && statePosition.X >= 0 &&
           Environment[1].Num() > statePosition.Y && statePosition.Y >= 0);
    return Environment[statePosition.X][statePosition.Y];
}




