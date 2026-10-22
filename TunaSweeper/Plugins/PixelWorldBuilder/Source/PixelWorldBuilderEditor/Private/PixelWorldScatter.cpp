#include "PixelWorldCore.h"
#include "Math/RandomStream.h"
#include "Math/RotationMatrix.h"

namespace
{
    void Surface(const FPixelWorldPlan& P,FVector& Position,FVector& Normal)
    {
        const double GX=FMath::Clamp((Position.X-P.LandscapeOrigin.X)/P.LandscapeScale.X,0.0,double(P.VerticesX-1));
        const double GY=FMath::Clamp((Position.Y-P.LandscapeOrigin.Y)/P.LandscapeScale.Y,0.0,double(P.VerticesY-1));
        const int32 X=FMath::Min(FMath::FloorToInt(GX),P.VerticesX-2),Y=FMath::Min(FMath::FloorToInt(GY),P.VerticesY-2);
        const double U=GX-X,V=GY-Y;
        auto H=[&](int32 IX,int32 IY) { return (int32(P.Heights[IY*P.VerticesX+IX])-32768)*P.LandscapeScale.Z/128.0; };
        const double H00=H(X,Y),H10=H(X+1,Y),H01=H(X,Y+1),H11=H(X+1,Y+1);
        // Match Landscape's i00-i11 diagonal, rather than interpolating a bilinear patch.
        double DX,DY;
        if(U>=V) { DX=H10-H00; DY=H11-H10; }
        else { DX=H11-H01; DY=H01-H00; }
        Position.Z=P.LandscapeOrigin.Z+H00+U*DX+V*DY;
        Normal=FVector(-DX/P.LandscapeScale.X,-DY/P.LandscapeScale.Y,1).GetSafeNormal();
    }
}

bool PixelWorld::Scatter(const UPixelWorldProfile& P,const FPixelWorldImage& Image,const TArray<TArray<int32>>& PixelsByRule,FPixelWorldPlan& Out,FText& Error)
{
    double MeshBudget=0,ActorBudget=0;
    for(int32 R=0;R<P.Rules.Num();++R)
    for(const auto& V:P.Rules[R].Vegetation)
    {
        const double Target=FMath::RoundToDouble(PixelsByRule[R].Num()*double(P.MetersPerPixel)*P.MetersPerPixel*V.DensityPer100SquareMeters/100.0);
        (V.ActorClass ? ActorBudget : MeshBudget)+=Target;
        if(!FMath::IsFinite(Target) || MeshBudget>50000 || ActorBudget>2000)
        { Error=Text(TEXT("Error.Budget")); return false; }
    }
    for(int32 R=0;R<P.Rules.Num();++R)
    {
        const auto& Rule=P.Rules[R]; const auto& Pixels=PixelsByRule[R];
        if(Pixels.IsEmpty()) continue;
        for(int32 E=0;E<Rule.Vegetation.Num();++E)
        {
            const auto& V=Rule.Vegetation[E];
            const int32 Target=FMath::RoundToInt(Pixels.Num()*double(P.MetersPerPixel)*P.MetersPerPixel*V.DensityPer100SquareMeters/100.0);
            const uint32 Color=(uint32(Rule.Red)<<16)|(uint32(Rule.Green)<<8)|Rule.Blue;
            FRandomStream Random(int32(uint32(P.Seed)^Color^(uint32(E+1)*0x9e3779b9u)));
            const double Spacing=V.MinimumSpacingMeters*100.0;
            const double CellSize=FMath::Max(Spacing,1.0);
            TMap<FIntPoint,TArray<FVector2D>> Occupied;
            int32 Accepted=0;
            for(int32 Attempt=0;Attempt<Target*12+32 && Accepted<Target;++Attempt)
            {
                const int32 Pixel=Pixels[Random.RandRange(0,Pixels.Num()-1)];
                FVector Position=PixelCenter(Pixel%Image.Width,Pixel/Image.Width,Image.Width,Image.Height,P);
                Position.X+=(Random.FRand()-0.5)*P.MetersPerPixel*100.0;
                Position.Y+=(Random.FRand()-0.5)*P.MetersPerPixel*100.0;
                FVector Normal; Surface(Out,Position,Normal);
                if(Normal.Z+1.e-8<FMath::Cos(FMath::DegreesToRadians(double(V.MaximumSlopeDegrees)))) continue;
                const FIntPoint Cell(FMath::FloorToInt((Position.X-P.Origin.X)/CellSize),FMath::FloorToInt((Position.Y-P.Origin.Y)/CellSize));
                bool bClear=true;
                if(Spacing>0)
                    for(int32 DX=-1;DX<=1 && bClear;++DX)
                    for(int32 DY=-1;DY<=1 && bClear;++DY)
                        if(const auto* Neighbors=Occupied.Find(Cell+FIntPoint(DX,DY)))
                            for(const auto& Other:*Neighbors)
                                if(FVector2D::DistSquared(FVector2D(Position.X,Position.Y),Other)<Spacing*Spacing) { bClear=false; break; }
                if(!bClear) continue;
                Occupied.FindOrAdd(Cell).Add(FVector2D(Position.X,Position.Y));
                Position.Z+=V.HeightOffsetCm;
                const FQuat Yaw(FVector::UpVector,FMath::DegreesToRadians(Random.FRandRange(0.f,360.f)));
                const FQuat Rotation=V.bAlignToSurface ? FRotationMatrix::MakeFromZ(Normal).ToQuat()*Yaw : Yaw;
                auto& Placement=Out.Placements.AddDefaulted_GetRef();
                Placement.RuleIndex=R; Placement.VegetationIndex=E;
                Placement.Transform=FTransform(Rotation,Position,FVector(Random.FRandRange(V.MinimumScale,V.MaximumScale)));
                ++Accepted;
            }
            Out.RejectedPlacements+=Target-Accepted;
        }
    }
    return true;
}
