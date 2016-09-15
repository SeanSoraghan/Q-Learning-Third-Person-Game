// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.cpp"
#include "TPGameDemoGameMode.h"
#include "LevelBuilderComponent.h"


ULevelBuilderComponent::ULevelBuilderComponent()
{
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
    LevelsDir      = FPaths::GameDir();
    LevelsDir     += "Content/Levels/";
    LevelsDirFound = FPlatformFileManager::Get().GetPlatformFile().DirectoryExists (*LevelsDir);
}

void ULevelBuilderComponent::LoadLevel (FString levelName)
{
    if (LevelsDirFound)
    {
        CurrentLevelPath = LevelsDir + levelName + ".txt";

      #if ON_SCREEN_DEBUGGING
        if ( ! FPlatformFileManager::Get().GetPlatformFile().FileExists (*CurrentLevelPath))
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Cant Find file: | %s |"), *CurrentLevelPath));
      #endif

        FillArrayFromTextFile (CurrentLevelPath, LevelStructure);
        ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

        if (gameMode != nullptr)
        {
            gameMode->NumGridUnitsX = LevelStructure.Num();
            if (LevelStructure.Num() > 0)
                gameMode->NumGridUnitsY = LevelStructure[0].Num();
        }
    }
}

int ULevelBuilderComponent::GetNumGridUnitsX() { return LevelStructure.Num(); }
int ULevelBuilderComponent::GetNumGridUnitsY() 
{
    if (LevelStructure.Num() > 0)
        return LevelStructure[0].Num();
    return 0;
}

bool ULevelBuilderComponent::IsGridPositionBlocked (int x, int y) 
{
    if (x > LevelStructure.Num() - 1 || x < 0 || y > LevelStructure[0].Num() - 1 || y < 0)
        return false;
    return LevelStructure[x][y] == 1;
}

FVector2D ULevelBuilderComponent::GetGridCentrePosition (int x, int y)
{
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

    if (gameMode == nullptr)
        return FVector2D (0.0f, 0.0f);

    float positionX = ((x - gameMode->NumGridUnitsX / 2) + 0.5f) * gameMode->GridUnitLengthXCM;
    float positionY = ((y - gameMode->NumGridUnitsY / 2) + 0.5f) * gameMode->GridUnitLengthYCM;
    return FVector2D (positionX, positionY);
}

void ULevelBuilderComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULevelBuilderComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}

