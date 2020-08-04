// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "Engine/World.h"
#include "TextParserComponent.h"
#include "LevelTrainerComponent.h"

//====================================================================================================
// LevelTrainerRunnable
//====================================================================================================

LevelTrainerRunnable::LevelTrainerRunnable(ULevelTrainerComponent& trainerComponent) 
    : TrainerComponent(trainerComponent)
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
            TrainerComponent.TrainNextGoalPosition(NUM_TRAINING_SIMULATIONS, MAX_NUM_MOVEMENTS_PER_SIMULATION);
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
// ULevelTrainerComponent
//====================================================================================================

static FThreadSafeCounter ThreadCounter;

ULevelTrainerComponent::ULevelTrainerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda([this](UWorld* world, bool, bool)
    {
        if (IsValid(this))
        {
            //if (GetWorld() == world)
            //{
                if (TrainerRunnable.IsValid())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Exiting training thread."));
                    TrainerRunnable->Exit();
                }
                FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
            //}
        }
    });
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
        while(TrainerRunnable.IsValid() && TrainerRunnable->IsTraining){}            
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
    //This is called from the room builder BP in OnBuildRoom, after the walls have been spawned (BuildGeneratedRoom). The game state should hold the qvalues, so that we can build on them if the room structure changes.
    CurrentLevelName = levelName;
    FString CurrentLevelPath = LevelBuilderHelpers::LevelsDir() + CurrentLevelName + ".txt";
    TArray<TArray<int>> LevelStructure;
    LevelBuilderHelpers::FillArrayFromTextFile (CurrentLevelPath, LevelStructure);
    const int sizeX = LevelStructure.Num();
    const int sizeY = LevelStructure[0].Num();
    MaxTrainingPosition.Set(sizeX * sizeY - 1.0f);
    Environment = GetNavigationEnvironmentForRoom(LevelStructure, RoomCoords);
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(GetWorld()->GetGameState());
    if (gameState != nullptr)
    {
        NavSets = gameState->GetRoomNavSets(RoomCoords);
        gameState->UpdateRoomNavEnvironment(RoomCoords, Environment);
    }
}

void ULevelTrainerComponent::TrainNextGoalPosition(int numSimulationsPerStartingPosition, int maxNumActionsPerSimulation)
{
    ClearEnvironment();
    float maxGoalDistance = sqrt(pow((Environment.Num() - 1), 2.0f) + pow((Environment[0].Num() - 1), 2.0f));
    if (GetPosState(Environment, CurrentGoalPosition).IsStateValid())
    {
        GetPosState(Environment, CurrentGoalPosition).SetIsGoal(true);
        for(int x = 0; x < Environment.Num(); ++x)
        {
            for(int y = 0; y < Environment[0].Num(); ++y)
            {
                if (GetPosState(Environment, FIntPoint(x, y)).IsStateValid() && FIntPoint(x, y) != CurrentGoalPosition)
                {
                    bool deltaQConverged = false;
                    int s = 0;
                    float distanceFromGoal = sqrt(pow((CurrentGoalPosition.X - x), 2.0f) + pow((CurrentGoalPosition.Y - y), 2.0f));
                    float normedDistanceFromGoal = (distanceFromGoal - 1.0f) / (maxGoalDistance - 1.0f);
                    int actionsTakenConvergenceThreshold = (int)(normedDistanceFromGoal * (CONVERGENCE_NUM_ACTIONS_MAX - CONVERGENCE_NUM_ACTIONS_MIN)) + CONVERGENCE_NUM_ACTIONS_MIN;
                    while (/*!deltaQConverged &&*/ s < numSimulationsPerStartingPosition)
                    {
                        float averageDeltaQ = 0.0f;
                        int numActionsTaken = 0;
                        SimulateRun(FIntPoint(x, y), maxNumActionsPerSimulation, averageDeltaQ, numActionsTaken);
                        deltaQConverged = numActionsTaken >= actionsTakenConvergenceThreshold && averageDeltaQ <= DELTA_Q_CONVERGENCE_THRESHOLD;
                        ++s;
                    }
                }
            }
        }
        FString CurrentPositionString = FString::FromInt(CurrentGoalPosition.X) + FString("_") + FString::FromInt(CurrentGoalPosition.Y);
        //Create string and save to text file.
        FString CurrentPositionFileName = LevelBuilderHelpers::LevelsDir() + CurrentLevelName + "/" + CurrentPositionString + ".txt";
        TArray<TArray<FDirectionSet>> envArray = GetBehaviourMap();
        //LevelBuilderHelpers::PrintArray(envArray);
        UWorld* world = GetWorld();
        if (IsValid(world))
        {
            ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(world->GetGameState());
            if (gameState != nullptr)
            {
                gameState->SetRoomNavigationSet(RoomCoords, CurrentGoalPosition, GetRoomNavSet(NavSets, CurrentGoalPosition));
            }
        }
#pragma message("move this to save function in game state.")
        LevelBuilderHelpers::WriteArrayToTextFile(envArray, CurrentPositionFileName);
        GetPosState(Environment, CurrentGoalPosition).SetIsGoal(false);
    }
    IncrementGoalPosition();
}

void ULevelTrainerComponent::ResetGoalPosition()
{
    CurrentGoalPosition = FIntPoint(0,0);
}

BehaviourMap ULevelTrainerComponent::GetBehaviourMap()
{
    BehaviourMap outArray;
    outArray.Reserve(Environment.Num());
    for (int x = 0; x < Environment.Num(); ++x)
    {
        outArray.Add(TArray<FDirectionSet>());
        for (int y = 0; y < Environment[0].Num(); ++y)
        {
            outArray[x].Add(FDirectionSet());
            FDirectionSet& directionSet = outArray[x][y];
            directionSet.Clear();
            if (GetPosState(Environment, FIntPoint(x, y)).IsStateValid())
            {
                GetNavStateForGoalPosition(NavSets, CurrentGoalPosition, FIntPoint(x, y)).GetOptimalQValueAndActions(directionSet);
                ensure(directionSet.IsValid());
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
            NavigationState& navState = GetNavStateForGoalPosition(NavSets, CurrentGoalPosition, FIntPoint(x, y));
            navState.ResetQValues();
            // Move in from edges if on an edge.
            /*if (x == 0)
            {
                navState.UpdateQValue(EDirectionType::North, 100.0f);
            }
            else if (y == 0)
            {
                navState.UpdateQValue(EDirectionType::East, 100.0f);
            }
            else if (x == Environment.Num() - 1)
            {
                navState.UpdateQValue(EDirectionType::South, 100.0f);
            }
            else if (y == Environment[0].Num() - 1)
            {
                navState.UpdateQValue(EDirectionType::West, 100.0f);
            }*/
        }
    }
}

void ULevelTrainerComponent::SimulateRun(FIntPoint startingStatePosition, int maxNumActions, float& averageDeltaQ, int& numActionsTaken)
{
    numActionsTaken = 0;
    averageDeltaQ = 0.0f;
    bool goalReached = false;
    if (GetPosState(Environment, startingStatePosition).IsGoalState())
        goalReached = true;
    FIntPoint currentPosition = startingStatePosition;
    while (numActionsTaken < maxNumActions && !goalReached)
    {
        FDirectionSet optimalActions;
        NavigationState& currentNavState = GetNavStateForGoalPosition(NavSets, CurrentGoalPosition, currentPosition);
        NavPositionState& currentPosState = GetPosState(Environment, currentPosition);
        currentNavState.GetOptimalQValueAndActions(optimalActions);
        ensure(optimalActions.IsValid());
        EDirectionType actionToTake = optimalActions.ChooseDirection();
        FDirectionSet dummyNextActions;
        FRoomPositionPair actionTarget = currentPosState.GetActionTarget(actionToTake);
        const float maxNextReward = GetNavStateForGoalPosition(NavSets, CurrentGoalPosition, actionTarget.PositionInRoom).GetOptimalQValueAndActions(dummyNextActions);
        const float currentQValue = currentNavState.GetQValues()[(int)actionToTake];
        const float discountedNextReward = GridTrainingConstants::SimDiscountFactor * maxNextReward;
        const float immediateReward = currentNavState.GetRewards()[(int)actionToTake];
        const float deltaQ = GridTrainingConstants::SimLearningRate * (immediateReward + discountedNextReward - currentQValue);
        averageDeltaQ += deltaQ;
        currentNavState.UpdateQValue(actionToTake, GridTrainingConstants::SimLearningRate, deltaQ);
#pragma message("Careful here! This assumes the room never changes, during training. If we come back and train a room again after the doors have been unlocked, the actionTarget's RoomCoords may be different here!")
        currentPosition = actionTarget.PositionInRoom;
        ++numActionsTaken;
        if (GetPosState(Environment, currentPosition).IsGoalState())
            goalReached = true;
    }
    averageDeltaQ /= (float)numActionsTaken;
}

void ULevelTrainerComponent::IncrementGoalPosition()
{
    if (CurrentGoalPosition.Y == Environment[0].Num() - 1)
    {
        if (CurrentGoalPosition.X == Environment.Num() - 1)
        {
            LevelTrained = true;
            //return;
        }
        else
        {
            CurrentGoalPosition.Y = 0;
            ++CurrentGoalPosition.X;
        }//return;
    }
    else
    {
        ++CurrentGoalPosition.Y;
    }
    TrainingPosition.Set(CurrentGoalPosition.Y + CurrentGoalPosition.X * Environment[0].Num());
}

float ULevelTrainerComponent::GetTrainingProgress()
{
    float trainingPosition = (float) TrainingPosition.GetValue();
    ensure(MaxTrainingPosition.GetValue() != 0);
    //UE_LOG(LogTemp, Warning, TEXT("X: %d | Y: %d || Current: %d || Max: %d"),CurrentGoalPosition.X, CurrentGoalPosition.Y, TrainingPosition.GetValue(), MaxTrainingPosition.GetValue());
    return trainingPosition / MaxTrainingPosition.GetValue();
}




