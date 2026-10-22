#pragma once

#include "CoreMinimal.h"
#include "PixelWorldProfile.h"

struct FPixelWorldImage
{
    int32 Width = 0;
    int32 Height = 0;
    TArray<FColor> Pixels;
};

struct FPixelWorldPlacement
{
    int32 RuleIndex = 0;
    int32 VegetationIndex = 0;
    FTransform Transform;
};

struct FPixelWorldPlan
{
    int32 VerticesX = 0;
    int32 VerticesY = 0;
    FVector LandscapeOrigin = FVector::ZeroVector;
    FVector LandscapeScale = FVector::OneVector;
    TArray<uint16> Heights;
    TArray<TArray<uint8>> LayerWeights;
    TArray<int32> RulePixelCounts;
    TArray<FPixelWorldPlacement> Placements;
    int32 RejectedPlacements = 0;
};

namespace PixelWorld
{
    FText Text(const TCHAR* Key);
    bool ReadColorPng(const FString& Path, FPixelWorldImage& Out, FText& Error);
    bool ReadHeightPng(const FString& Path, int32 Width, int32 Height, TArray<uint16>& Out, FText& Error);
    bool BuildPlan(const UPixelWorldProfile& Profile, FPixelWorldPlan& Out, FText& Error);
    bool Scatter(const UPixelWorldProfile& Profile, const FPixelWorldImage& Image, const TArray<TArray<int32>>& PixelsByRule, FPixelWorldPlan& Out, FText& Error);
    FVector PixelCenter(int32 X, int32 Y, int32 Width, int32 Height, const UPixelWorldProfile& Profile);
    FText Describe(const UPixelWorldProfile& Profile, const FPixelWorldPlan& Plan);
}
