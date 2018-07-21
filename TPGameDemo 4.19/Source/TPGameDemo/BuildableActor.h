// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MazeActor.h"
#include "BuildableActor.generated.h"

/**
 * 
 */
UCLASS()
class TPGAMEDEMO_API ABuildableActor : public AMazeActor
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadWrite, Category = "Buildable Items")
        bool IsBuilt = false;

private:
	
};
