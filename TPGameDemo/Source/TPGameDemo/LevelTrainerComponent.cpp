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
            if (TrainerRunnable.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("Exiting training thread."));
                TrainerRunnable->Exit();
                TrainerRunnable->Wake();
                while (TrainerRunnable->IsTraining) {}
            }
            FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
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
    TrainerRunnable = nullptr;
    FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
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

void ULevelTrainerComponent::UpdateEnvironmentForLevel()
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
        InnerRoomBitmask roomBitmask = gameState->GetRoomInnerStructure(RoomCoords);
        LevelBuilderHelpers::BitMaskToArray(roomBitmask, LevelStructure);
        TArray<int> neswDoorPositions;
        gameState->GetDoorPositionsNESW(RoomCoords, neswDoorPositions);
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


        const int sizeX = LevelStructure.Num();
        const int sizeY = LevelStructure[0].Num();
        MaxTrainingPosition.Set(sizeX * sizeY - 1.0f);
    
        gameState->UpdateRoomNavEnvironmentForStructure(RoomCoords, LevelStructure);

        /*UE_LOG(LogTemp, Warning, TEXT("Loaded Level:"));
        LevelBuilderHelpers::PrintArray(LevelStructure);*/
    }
}

void ULevelTrainerComponent::TrainNextGoalPosition(int numSimulationsPerStartingPosition, int maxNumActionsPerSimulation)
{
    ClearEnvironment();
    float maxGoalDistance = sqrt(pow((GetNavEnvironment().Num() - 1), 2.0f) + pow((GetNavEnvironment()[0].Num() - 1), 2.0f));
    if (Get_ActionTargets(GetNavEnvironment(), CurrentGoalPosition).IsStateValid())
    {
        ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(GetWorld()->GetGameState());
        if (gameState != nullptr)
        {
            gameState->SetPositionIsGoal(RoomCoords, CurrentGoalPosition, true);
        }
        for(int x = 0; x < GetNavEnvironment().Num(); ++x)
        {
            for(int y = 0; y < GetNavEnvironment()[0].Num(); ++y)
            {
                if (Get_ActionTargets(GetNavEnvironment(), FIntPoint(x, y)).IsStateValid() && FIntPoint(x, y) != CurrentGoalPosition)
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
        UWorld* world = GetWorld();
        if (IsValid(world))
        {
            ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(world->GetGameState());
            if (gameState != nullptr)
            {
                gameState->SetRoomQValuesRewardsSet(RoomCoords, CurrentGoalPosition, Get_QValuesRewardsSet_For_GoalPosition(GetNavSets(), CurrentGoalPosition));
            }
        }
        if (gameState != nullptr)
        {
            gameState->SetPositionIsGoal(RoomCoords, CurrentGoalPosition, false);
        }
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
    outArray.Reserve(GetNavEnvironment().Num());
    for (int x = 0; x < GetNavEnvironment().Num(); ++x)
    {
        outArray.Add(TArray<FDirectionSet>());
        for (int y = 0; y < GetNavEnvironment()[0].Num(); ++y)
        {
            outArray[x].Add(FDirectionSet());
            FDirectionSet& directionSet = outArray[x][y];
            directionSet.Clear();
            if (Get_ActionTargets(GetNavEnvironment(), FIntPoint(x, y)).IsStateValid())
            {
                Get_ActionQValuesAndRewards_FromRoom(GetNavSets(), CurrentGoalPosition, FIntPoint(x, y)).GetOptimalQValueAndActions(directionSet);
                ensure(directionSet.IsValid());
            }
        }
    }
    return outArray;
}

void ULevelTrainerComponent::ClearEnvironment()
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(GetWorld()->GetGameState());
    ensure(gameState != nullptr);
    return gameState->ClearQValuesAndRewards(RoomCoords, CurrentGoalPosition);
}

const RoomTargetsQValuesRewardsSets& ULevelTrainerComponent::GetNavSets() const
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(GetWorld()->GetGameState());
    ensure(gameState != nullptr);
    return gameState->GetRoomQValuesRewardsSets(RoomCoords);
}

const NavigationEnvironment& ULevelTrainerComponent::GetNavEnvironment() const
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(GetWorld()->GetGameState());
    ensure(gameState != nullptr);
    return gameState->GetNavEnvironment(RoomCoords);
}

void ULevelTrainerComponent::SimulateRun(FIntPoint startingStatePosition, int maxNumActions, float& averageDeltaQ, int& numActionsTaken)
{
    numActionsTaken = 0;
    averageDeltaQ = 0.0f;
    bool goalReached = false;
    if (Get_ActionTargets(GetNavEnvironment(), startingStatePosition).IsGoalState())
        goalReached = true;
    FIntPoint currentPosition = startingStatePosition;
    while (numActionsTaken < maxNumActions && !goalReached)
    {
        FDirectionSet optimalActions;
        const ActionQValuesAndRewards& qValuesRewards = Get_ActionQValuesAndRewards_FromRoom(GetNavSets(), CurrentGoalPosition, currentPosition);
        const ActionTargets& targets = Get_ActionTargets(GetNavEnvironment(), currentPosition);
        qValuesRewards.GetOptimalQValueAndActions(optimalActions);
        ensure(optimalActions.IsValid());
        EDirectionType actionToTake = optimalActions.ChooseDirection();
        FDirectionSet dummyNextActions;
        FRoomPositionPair actionTarget = targets.GetActionTarget(actionToTake);
        const float maxNextReward = Get_ActionQValuesAndRewards_FromRoom(GetNavSets(), CurrentGoalPosition, actionTarget.PositionInRoom).GetOptimalQValueAndActions(dummyNextActions);
        const float currentQValue = qValuesRewards.GetQValues()[(int)actionToTake];
        const float discountedNextReward = GridTrainingConstants::SimDiscountFactor * maxNextReward;
        const float immediateReward = qValuesRewards.GetRewards()[(int)actionToTake];
        const float deltaQ = GridTrainingConstants::SimLearningRate * (immediateReward + discountedNextReward - currentQValue);
        averageDeltaQ += deltaQ;
        ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(GetWorld()->GetGameState());
        if (gameState != nullptr)
        {
            gameState->UpdateQValue({ RoomCoords, currentPosition }, CurrentGoalPosition, actionToTake, GridTrainingConstants::SimLearningRate, deltaQ);
        }
#pragma message("Careful here! This assumes the room never changes, during training. If we come back and train a room again after the doors have been unlocked, the actionTarget's RoomCoords may be different here!")
        currentPosition = actionTarget.PositionInRoom;
        ++numActionsTaken;
        if (Get_ActionTargets(GetNavEnvironment(), currentPosition).IsGoalState())
            goalReached = true;
    }
    averageDeltaQ /= (float)numActionsTaken;
}

void ULevelTrainerComponent::IncrementGoalPosition()
{
    if (CurrentGoalPosition.Y == GetNavEnvironment()[0].Num() - 1)
    {
        if (CurrentGoalPosition.X == GetNavEnvironment().Num() - 1)
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
    TrainingPosition.Set(CurrentGoalPosition.Y + CurrentGoalPosition.X * GetNavEnvironment()[0].Num());
}

float ULevelTrainerComponent::GetTrainingProgress()
{
    float trainingPosition = (float) TrainingPosition.GetValue();
    ensure(MaxTrainingPosition.GetValue() != 0);
    //UE_LOG(LogTemp, Warning, TEXT("X: %d | Y: %d || Current: %d || Max: %d"),CurrentGoalPosition.X, CurrentGoalPosition.Y, TrainingPosition.GetValue(), MaxTrainingPosition.GetValue());
    return trainingPosition / MaxTrainingPosition.GetValue();
}




