// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TextParserComponent.h"

//const FString LevelBuilderHelpers::LevelsDir() { return FPaths::/*GameDir*/ProjectDir() + "Content/Levels/"; }
//
//FIntPoint LevelBuilderHelpers::GetTargetPointForAction(FIntPoint startingPoint, EActionType actionType, int numSpaces /*= 1*/)
//{
//    FIntPoint endPoint = startingPoint;
//
//    switch(actionType)
//    {
//        case EActionType::North:
//        {
//            endPoint.X -= numSpaces;
//            break;
//        }
//        case EActionType::East:
//        {
//            endPoint.Y += numSpaces;
//            break;
//        }
//        case EActionType::South:
//        {
//            endPoint.X += numSpaces;
//            break;
//        }
//        case EActionType::West:
//        {
//            endPoint.Y -= numSpaces;
//            break;
//        }
//    }
//
//    return endPoint;
//}
//
//bool LevelBuilderHelpers::GridPositionIsValid(FIntPoint position, int sizeX, int sizeY)
//{
//    return !(position.X < 0 || position.Y < 0 || position.X > sizeX - 1 || position.Y > sizeY -1);
//}
//
//const FString LevelBuilderHelpers::ActionStringDelimiter() {return FString(" ");}
//
///*
//Takes in a text file and fills an array with numbers.
//File should be a square grid format with numbers separated by spaces.
//If invertX is true, the lines in the text file will be entered from bottom to top
//into the 2D int array (rather than top to bottom).
//*/
//void LevelBuilderHelpers::FillArrayFromTextFile (FString fileName, TArray<TArray<int>>& arrayRef, bool invertX /*= false*/)
//{
//
//    #if ON_SCREEN_DEBUGGING
//    if ( ! FPlatformFileManager::Get().GetPlatformFile().FileExists (*fileName))
//    {
//        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Cant Find file: | %s |"), *fileName));
//        return;
//    }
//    #endif
//
//    arrayRef.Empty();
//    TArray<FString> LevelPolicyStringArray;
//    FFileHelper::LoadANSITextFileToStrings(*(fileName), NULL, LevelPolicyStringArray);
//    const int numRows = LevelPolicyStringArray.Num();
//    for (int row = (invertX ? numRows - 1 : 0); (invertX ? row >= 0 : row < numRows); (invertX ? row-- : row++))
//    {
//        
//        TArray<FString> actionStrings;
//        LevelPolicyStringArray[row].ParseIntoArray (actionStrings, *ActionStringDelimiter(), 1);
//        if (actionStrings.Num() > 0)
//        {
//            arrayRef.Add (TArray<int>());
//            arrayRef[arrayRef.Num() - 1].Empty();
//            for (int action = 0; action < actionStrings.Num(); action++)
//            {
//                arrayRef[arrayRef.Num() - 1].Add (FCString::Atoi(*actionStrings[action]));
//            }
//        }
//    }
//}
//
//
///*
//Writes a 2D int array to a text file. File will have a square grid format with numbers 
//separated by spaces. If invertX is true, the lines in the text file will be entered 
//from bottom to top (rather than top to bottom).
//*/
//void LevelBuilderHelpers::WriteArrayToTextFile (TArray<TArray<int>>& arrayRef, FString fileName, bool invertX /*= false*/)
//{
//    const int numX = arrayRef.Num();
//    const int numY = arrayRef[0].Num();
//    FString text = FString("");
//    for (int x = (invertX ? numX - 1 : 0); (invertX ? x >= 0 : x < numX); (invertX ? x-- : x++))       
//    {
//        FString row = "";
//        for (int y = 0; y < numY; ++y)
//        {
//            row += FString::FromInt(arrayRef[x][y]);
//            if (y < numY - 1)
//                row += FString(" ");
//            else if (x < numX - 1)
//                row += FString("\n");
//        }
//        text += row;
//    }
//    FFileHelper::SaveStringToFile(text, * fileName);
//}