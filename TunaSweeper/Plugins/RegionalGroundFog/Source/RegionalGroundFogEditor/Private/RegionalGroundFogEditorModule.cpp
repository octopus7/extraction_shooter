#include "RegionalGroundFogActor.h"

#include "ComponentVisualizer.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "Modules/ModuleManager.h"
#include "PrimitiveDrawInterface.h"
#include "SceneManagement.h"

#define LOCTEXT_NAMESPACE "RegionalGroundFogEditor"

namespace RegionalGroundFogEditor
{
	class FRegionalGroundFogVisualizer final : public FComponentVisualizer
	{
	public:
		virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override
		{
			const URegionalGroundFogVisualizationComponent* VisualizationComponent = Cast<URegionalGroundFogVisualizationComponent>(Component);
			const ARegionalGroundFogActor* FogActor = VisualizationComponent ? Cast<ARegionalGroundFogActor>(VisualizationComponent->GetOwner()) : nullptr;
			if (!FogActor)
			{
				return;
			}

			const FTransform ActorTransform = FogActor->GetActorTransform();
			for (const FRegionalGroundFogNode& Node : FogActor->GetFogNodes())
			{
				const FVector Center = ActorTransform.TransformPosition(Node.LocalCenter);
				DrawWireSphere(PDI, Center, FColor(65, 216, 235), Node.CoreRadius, 20, SDPG_World, 1.5f);
				DrawWireSphere(PDI, Center, FColor(56, 116, 255), Node.OuterRadius, 28, SDPG_World, 2.0f);
				PDI->DrawPoint(Center, FColor::White, 10.0f, SDPG_World);
			}
		}
	};
}

class FRegionalGroundFogEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (GUnrealEd)
		{
			Visualizer = MakeShared<RegionalGroundFogEditor::FRegionalGroundFogVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(URegionalGroundFogVisualizationComponent::StaticClass()->GetFName(), Visualizer);
		}
	}

	virtual void ShutdownModule() override
	{
		if (GUnrealEd && Visualizer.IsValid())
		{
			GUnrealEd->UnregisterComponentVisualizer(URegionalGroundFogVisualizationComponent::StaticClass()->GetFName());
		}
		Visualizer.Reset();
	}

private:
	TSharedPtr<FComponentVisualizer> Visualizer;
};

IMPLEMENT_MODULE(FRegionalGroundFogEditorModule, RegionalGroundFogEditor)

#undef LOCTEXT_NAMESPACE
