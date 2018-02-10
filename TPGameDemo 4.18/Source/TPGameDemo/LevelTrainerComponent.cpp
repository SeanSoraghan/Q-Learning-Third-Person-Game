// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.cpp"
#include "LevelBuilderComponent.h"
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
            TrainerComponent.TrainNextGoalPosition(10, 10);
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

static FThreadSafeCounter ThreadCounter;

ULevelTrainerComponent::ULevelTrainerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    TrainerRunnable = MakeShareable(new LevelTrainerRunnable(*this));
    FString ThreadName(FString::Printf(TEXT("LevelTrainerThread%i"), ThreadCounter.Increment()));
    TrainerThread = MakeShareable(FRunnableThread::Create(TrainerRunnable.Get(), *ThreadName, 0,
                                  EThreadPriority::TPri_BelowNormal));
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
    if (TrainerRunnable.IsValid())
        TrainerRunnable->StartTraining();
}

void ULevelTrainerComponent::PauseTraining()
{
    if (TrainerRunnable.IsValid())
        TrainerRunnable->PauseTraining();
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
    FString CurrentLevelPath = LevelBuilderConstants::LevelsDir + CurrentLevelName + ".txt";
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

                if (x == 0 || LevelStructure[x - 1][y] == 1)
                    state.SetActionTarget(EActionType::North, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::North, FIntPoint(x - 1, y));

                if (y == sizeY - 1 || LevelStructure[x][y + 1] == 1)
                    state.SetActionTarget(EActionType::East, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::East, FIntPoint(x, y + 1));

                if (x == sizeX - 1 || LevelStructure[x + 1][y] == 1)
                    state.SetActionTarget(EActionType::South, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::South, FIntPoint(x + 1, y));

                if (y == 0 || LevelStructure[x][y - 1] == 1)
                    state.SetActionTarget(EActionType::West, FIntPoint(x,y));
                else
                    state.SetActionTarget(EActionType::West, FIntPoint(x, y - 1));
            }
        }
    }
}

void ULevelTrainerComponent::TrainNextGoalPosition(int numSimulationsPerStartingPosition, int maxNumActionsPerSimulation)
{
    if (GetState(CurrentGoalPosition).IsStateValid())
    {
        GetState(CurrentGoalPosition).SetIsGoal(true);
        for(int x = 0; x < Environment.Num(); ++x)
        {
            for(int y = 0; y < Environment[0].Num(); ++y)
            {
                for (int s = 0; s < numSimulationsPerStartingPosition; ++s)
                {
                    if (GetState(FIntPoint(x,y)).IsStateValid())
                        SimulateRun(FIntPoint(x,y), maxNumActionsPerSimulation);
                }
            }
        }
        FString CurrentPositionString = CurrentGoalPosition.X + "_" + CurrentGoalPosition.Y;
        FString CurrentPositionDir = LevelBuilderConstants::LevelsDir + CurrentLevelName + "/" + CurrentPositionString;
        //Create string and save to text file.
        GetState(CurrentGoalPosition).SetIsGoal(false);
    }
    IncrementGoalPosition();
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

void ULevelTrainerComponent::IncrementGoalPosition()
{
    if (CurrentGoalPosition.Y == Environment[0].Num() - 1)
    {
        if (CurrentGoalPosition.X == Environment.Num() - 1)
        {
            LevelTrained = true;
            return;
        }
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




