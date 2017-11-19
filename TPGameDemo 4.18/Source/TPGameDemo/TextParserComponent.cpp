// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.h"

static FString ActionStringDelimiter = " ";

/*
Takes in a text file and fills an array with numbers.
File should be a square grid format with numbers separated by spaces.
If invertX is true, the lines in the text file will be entered from bottom to top
into the 2D int array (rather than top to bottom).
*/
static void FillArrayFromTextFile (FString fileName, TArray<TArray<int>>& arrayRef, bool invertX = false)
{

  #if ON_SCREEN_DEBUGGING
    if ( ! FPlatformFileManager::Get().GetPlatformFile().FileExists (*fileName))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Cant Find file: | %s |"), *fileName));
        return;
    }
  #endif

    arrayRef.Empty();
    TArray<FString> LevelPolicyStringArray;
    FFileHelper::LoadANSITextFileToStrings(*(fileName), NULL, LevelPolicyStringArray);
    const int numRows = LevelPolicyStringArray.Num();
    for (int row = (invertX ? numRows - 1 : 0); (invertX ? row >= 0 : row < numRows); (invertX ? row-- : row++))
    {
        
        TArray<FString> actionStrings;
        LevelPolicyStringArray[row].ParseIntoArray (actionStrings, *ActionStringDelimiter, 1);
        if (actionStrings.Num() > 0)
        {
            arrayRef.Add (TArray<int>());
            arrayRef[arrayRef.Num() - 1].Empty();
            for (int action = 0; action < actionStrings.Num(); action++)
            {
                arrayRef[arrayRef.Num() - 1].Add (FCString::Atoi(*actionStrings[action]));
            }
        }
    }
}