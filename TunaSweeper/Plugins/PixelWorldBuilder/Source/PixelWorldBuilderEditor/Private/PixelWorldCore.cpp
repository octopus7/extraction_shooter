#include "PixelWorldCore.h"
#include "Engine/StaticMesh.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"

namespace
{
    uint32 ColorId(uint8 R,uint8 G,uint8 B) { return (uint32(R)<<16)|(uint32(G)<<8)|B; }
    FFormatNamedArguments ColorArgs(const FColor& C)
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("R"),int32(C.R)); Args.Add(TEXT("G"),int32(C.G)); Args.Add(TEXT("B"),int32(C.B));
        return Args;
    }
    bool ValidateProfile(const UPixelWorldProfile& P, FText& Error)
    {
        if(P.Rules.Num()<1 || P.Rules.Num()>64) { Error=PixelWorld::Text(TEXT("Error.Rules")); return false; }
        if(!FMath::IsFinite(P.MetersPerPixel) || P.MetersPerPixel<0.01f || P.MetersPerPixel>1000.f || P.Origin.ContainsNaN() ||
            P.Origin.GetAbsMax()>1.e9 || P.HeightSmoothingPasses<0 || P.HeightSmoothingPasses>8 ||
            !FMath::IsFinite(P.HeightmapMinimumMeters) || !FMath::IsFinite(P.HeightmapMaximumMeters) ||
            P.HeightmapMinimumMeters>P.HeightmapMaximumMeters || FMath::Abs(P.HeightmapMinimumMeters)>10000 || FMath::Abs(P.HeightmapMaximumMeters)>10000)
        { Error=PixelWorld::Text(TEXT("Error.Numeric")); return false; }
        TSet<uint32> Colors;
        TSet<FName> Layers;
        bool bAnyLayer=false;
        for(const auto& R:P.Rules) bAnyLayer|=R.LandscapeLayer!=nullptr;
        const auto MaterialLayers=P.LandscapeMaterial ? ALandscapeProxy::GetLayersFromMaterial(P.LandscapeMaterial) : TArray<FName>();
        for(const auto& R:P.Rules)
        {
            const uint32 Id=ColorId(R.Red,R.Green,R.Blue);
            if(Colors.Contains(Id))
            { Error=FText::Format(PixelWorld::Text(TEXT("Error.Duplicate")),ColorArgs(FColor(R.Red,R.Green,R.Blue))); return false; }
            Colors.Add(Id);
            if(!FMath::IsFinite(R.HeightMeters) || FMath::Abs(R.HeightMeters)>10000.f)
            { Error=PixelWorld::Text(TEXT("Error.Numeric")); return false; }
            if(bAnyLayer)
            {
                if(!IsValid(R.LandscapeLayer) || R.LandscapeLayer->GetLayerName().IsNone() || Layers.Contains(R.LandscapeLayer->GetLayerName()) || !MaterialLayers.Contains(R.LandscapeLayer->GetLayerName()))
                { Error=PixelWorld::Text(TEXT("Error.Layers")); return false; }
                Layers.Add(R.LandscapeLayer->GetLayerName());
            }
            for(const auto& V:R.Vegetation)
            {
                if((V.Mesh!=nullptr)==(V.ActorClass!=nullptr) || (V.Mesh && (!IsValid(V.Mesh) || V.Mesh->GetNumLODs()==0)) ||
                    (V.ActorClass && (V.ActorClass->HasAnyClassFlags(CLASS_Abstract|CLASS_Deprecated|CLASS_NewerVersionExists) ||
                    V.ActorClass->IsChildOf(ALandscapeProxy::StaticClass()) || V.ActorClass->GetDefaultObject<AActor>()->IsEditorOnly())))
                { Error=PixelWorld::Text(TEXT("Error.Vegetation")); return false; }
                if(!FMath::IsFinite(V.DensityPer100SquareMeters) || V.DensityPer100SquareMeters<0 ||
                    !FMath::IsFinite(V.MinimumSpacingMeters) || V.MinimumSpacingMeters<0 || V.MinimumSpacingMeters>1.e6f ||
                    !FMath::IsFinite(V.MinimumScale) || !FMath::IsFinite(V.MaximumScale) || V.MinimumScale<=0 || V.MaximumScale<V.MinimumScale || V.MaximumScale>1000 ||
                    !FMath::IsFinite(V.MaximumSlopeDegrees) || V.MaximumSlopeDegrees<0 || V.MaximumSlopeDegrees>90 || !FMath::IsFinite(V.HeightOffsetCm))
                { Error=PixelWorld::Text(TEXT("Error.Numeric")); return false; }
            }
        }
        return true;
    }
}

FText PixelWorld::Text(const TCHAR* Key)
{
    return FText::FromStringTable(TEXT("PixelWorldBuilder.Editor"), Key);
}

FVector PixelWorld::PixelCenter(int32 X,int32 Y,int32 Width,int32 Height,const UPixelWorldProfile& P)
{
    const double Step=P.MetersPerPixel*100.0;
    return P.Origin+FVector((Height*0.5-Y-0.5)*Step,(X+0.5-Width*0.5)*Step,0);
}

bool PixelWorld::BuildPlan(const UPixelWorldProfile& P, FPixelWorldPlan& Out, FText& Error)
{
    Out={}; Error=FText::GetEmpty();
    if(!ValidateProfile(P,Error)) return false;
    FPixelWorldImage Image;
    if(!ReadColorPng(P.ColorPng.FilePath,Image,Error)) return false;
    const int32 ComponentsX=FMath::DivideAndRoundUp(Image.Height,63);
    const int32 ComponentsY=FMath::DivideAndRoundUp(Image.Width,63);
    if(ComponentsX*ComponentsY>256) { Error=Text(TEXT("Error.ImageSize")); return false; }

    TMap<uint32,int32> Rules;
    for(int32 I=0;I<P.Rules.Num();++I) Rules.Add(ColorId(P.Rules[I].Red,P.Rules[I].Green,P.Rules[I].Blue),I);
    TArray<int32> PixelRules; PixelRules.SetNumUninitialized(Image.Pixels.Num());
    TArray<TArray<int32>> PixelsByRule; PixelsByRule.SetNum(P.Rules.Num());
    int32 FirstUnknown=INDEX_NONE,UnknownCount=0;
    for(int32 I=0;I<Image.Pixels.Num();++I)
    {
        const FColor C=Image.Pixels[I];
        if(C.A!=255)
        {
            FFormatNamedArguments A; A.Add(TEXT("X"),I%Image.Width); A.Add(TEXT("Y"),I/Image.Width);
            Error=FText::Format(Text(TEXT("Error.Alpha")),A); return false;
        }
        const int32* Rule=Rules.Find(ColorId(C.R,C.G,C.B));
        if(!Rule) { if(FirstUnknown==INDEX_NONE) FirstUnknown=I; ++UnknownCount; continue; }
        PixelRules[I]=*Rule; PixelsByRule[*Rule].Add(I);
    }
    if(UnknownCount)
    {
        auto A=ColorArgs(Image.Pixels[FirstUnknown]);
        A.Add(TEXT("X"),FirstUnknown%Image.Width); A.Add(TEXT("Y"),FirstUnknown/Image.Width); A.Add(TEXT("Count"),UnknownCount);
        Error=FText::Format(Text(TEXT("Error.Unknown")),A); return false;
    }
    for(const auto& Pixels:PixelsByRule) Out.RulePixelCounts.Add(Pixels.Num());
    TArray<uint16> InputHeights;
    if(!P.HeightPng.FilePath.IsEmpty() && !ReadHeightPng(P.HeightPng.FilePath,Image.Width,Image.Height,InputHeights,Error)) return false;
    Out.VerticesX=ComponentsX*63+1; Out.VerticesY=ComponentsY*63+1;
    const double SizeX=Image.Height*P.MetersPerPixel*100.0, SizeY=Image.Width*P.MetersPerPixel*100.0;
    Out.LandscapeOrigin=P.Origin-FVector(SizeX*0.5,SizeY*0.5,0);
    Out.LandscapeScale=FVector(SizeX/(Out.VerticesX-1),SizeY/(Out.VerticesY-1),100);
    const int32 Count=Out.VerticesX*Out.VerticesY;
    TArray<double> Elevations; Elevations.SetNumUninitialized(Count);
    Out.LayerWeights.SetNum(P.Rules.Num());
    if(P.Rules[0].LandscapeLayer)
        for(auto& Weights:Out.LayerWeights) Weights.SetNumZeroed(Count);
    for(int32 Y=0;Y<Out.VerticesY;++Y)
    for(int32 X=0;X<Out.VerticesX;++X)
    {
        const int32 Column=FMath::Min(Image.Width-1,int32(double(Y)*Image.Width/(Out.VerticesY-1)));
        const int32 Row=FMath::Clamp(int32((1.0-double(X)/(Out.VerticesX-1))*Image.Height),0,Image.Height-1);
        const int32 Pixel=Row*Image.Width+Column, Vertex=Y*Out.VerticesX+X, Rule=PixelRules[Pixel];
        Elevations[Vertex]=100.0*(InputHeights.IsEmpty() ? P.Rules[Rule].HeightMeters :
            FMath::Lerp(double(P.HeightmapMinimumMeters),double(P.HeightmapMaximumMeters),InputHeights[Pixel]/65535.0));
        if(!Out.LayerWeights[Rule].IsEmpty()) Out.LayerWeights[Rule][Vertex]=255;
    }
    // Smooth elevation only; categorical mask and original RGBs remain untouched.
    for(int32 Pass=0;Pass<P.HeightSmoothingPasses;++Pass)
    {
        TArray<double> Next; Next.SetNumUninitialized(Count);
        for(int32 Y=0;Y<Out.VerticesY;++Y)
        for(int32 X=0;X<Out.VerticesX;++X)
        {
            const int32 I=Y*Out.VerticesX+X;
            Next[I]=(Elevations[I]+Elevations[Y*Out.VerticesX+FMath::Max(0,X-1)]+Elevations[Y*Out.VerticesX+FMath::Min(Out.VerticesX-1,X+1)]+
                Elevations[FMath::Max(0,Y-1)*Out.VerticesX+X]+Elevations[FMath::Min(Out.VerticesY-1,Y+1)*Out.VerticesX+X])/5.0;
        }
        Elevations=MoveTemp(Next);
    }
    double MaxAbs=0;
    for(double Height:Elevations) MaxAbs=FMath::Max(MaxAbs,FMath::Abs(Height));
    Out.LandscapeScale.Z=FMath::Max(100.0,MaxAbs/255.0);
    Out.Heights.Reserve(Count);
    for(double Height:Elevations) Out.Heights.Add(uint16(FMath::Clamp(FMath::RoundToInt(32768.0+Height*128.0/Out.LandscapeScale.Z),0,65535)));
    return Scatter(P,Image,PixelsByRule,Out,Error);
}

FText PixelWorld::Describe(const UPixelWorldProfile& P,const FPixelWorldPlan& Plan)
{
    FFormatNamedArguments A;
    A.Add(TEXT("X"),Plan.VerticesX); A.Add(TEXT("Y"),Plan.VerticesY);
    A.Add(TEXT("North"),(Plan.VerticesX-1)*Plan.LandscapeScale.X/100.0);
    A.Add(TEXT("East"),(Plan.VerticesY-1)*Plan.LandscapeScale.Y/100.0);
    A.Add(TEXT("Plants"),Plan.Placements.Num()); A.Add(TEXT("Rejected"),Plan.RejectedPlacements);
    TArray<FText> Lines; Lines.Add(FText::Format(Text(TEXT("Tool.Summary")),A));
    for(int32 I=0;I<P.Rules.Num();++I)
    {
        auto R=ColorArgs(FColor(P.Rules[I].Red,P.Rules[I].Green,P.Rules[I].Blue));
        R.Add(TEXT("Count"),Plan.RulePixelCounts[I]);
        Lines.Add(FText::Format(Text(TEXT("Tool.RuleSummary")),R));
    }
    return FText::Join(FText::FromString(TEXT("\n")),Lines);
}
