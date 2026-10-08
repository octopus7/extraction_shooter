#include "CanopyRimSubsystem.h"
#include "CanopyRimViewExtension.h"

UCanopyRimSubsystem::UCanopyRimSubsystem()
{
    for (FVector4f& Style : Styles) Style = FVector4f(0,0,0,0);
}

void UCanopyRimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (FApp::CanEverRender()) Extension = FSceneViewExtensions::NewExtension<FCanopyRimViewExtension>(GetWorld());
}

void UCanopyRimSubsystem::Deinitialize()
{
    Extension.Reset();
    Super::Deinitialize();
}

bool UCanopyRimSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE || Type == EWorldType::Editor || Type == EWorldType::EditorPreview;
}

FIntPoint UCanopyRimSubsystem::MaskExtent(FIntPoint Size)
{
    return FIntPoint(FMath::Max(1, FMath::DivideAndRoundUp(Size.X, 4)), FMath::Max(1, FMath::DivideAndRoundUp(Size.Y, 4)));
}

uint8 UCanopyRimSubsystem::UpdateTree(UObject* Owner, FLinearColor Color, float Strength, float Width, float Brightness)
{
    if (!IsValid(Owner) || Strength <= 0) { RemoveTree(Owner); return 0; }
    int32 Slot = 0, Free = 0;
    for (int32 Id = FirstId; Id <= LastId; ++Id)
    {
        if (!Owners[Id].IsValid())
        {
            Owners[Id].Reset(); Styles[Id] = Styles[Id + 256] = FVector4f(0,0,0,0);
            if (!Free) Free = Id;
        }
        else if (Owners[Id].Get() == Owner) Slot = Id;
    }
    if (!Slot) Slot = Free;
    // Do not reuse a live tree's ID if the finite stencil namespace is exhausted.
    if (!Slot) return 0;
    Owners[Slot] = Owner;
    Styles[Slot] = FVector4f(FMath::Max(0.f,Color.R)*Brightness, FMath::Max(0.f,Color.G)*Brightness,
        FMath::Max(0.f,Color.B)*Brightness, FMath::Clamp(Strength,0.f,1.f));
    Styles[Slot + 256] = FVector4f(FMath::Clamp(Width,1.f,128.f),0,0,0);
    Publish();
    return uint8(Slot);
}

void UCanopyRimSubsystem::RemoveTree(UObject* Owner)
{
    bool Changed = false;
    for (int32 Id = FirstId; Id <= LastId; ++Id)
        if (!Owners[Id].IsValid() || Owners[Id].Get() == Owner)
        {
            Changed |= Styles[Id].W != 0;
            Owners[Id].Reset(); Styles[Id] = Styles[Id + 256] = FVector4f(0,0,0,0);
        }
    if (Changed) Publish();
}

void UCanopyRimSubsystem::Publish()
{
    if (Extension) Extension->SetStyles(TArray<FVector4f>(Styles, UE_ARRAY_COUNT(Styles)));
}
