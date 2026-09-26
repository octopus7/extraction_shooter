#if WITH_DEV_AUTOMATION_TESTS

#include "AssetCompilingManager.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/PanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"
#include "UI/TunaSweeperResearchWidgets.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

namespace TunaSweeperResearchIconTests
{
	struct FWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};

	struct FExpectedIcon
	{
		const TCHAR* NodeId;
		const TCHAR* AssetName;
	};

	// Hand checked against the nine supplied icon illustrations. Repeated tiers share an illustration.
	constexpr FExpectedIcon ExpectedIcons[] = {
		{TEXT("vitality_1"), TEXT("T_Research_Vitality_White")},
		{TEXT("nutrition_1"), TEXT("T_Research_Nutrition_White")},
		{TEXT("hydration_1"), TEXT("T_Research_Hydration_White")},
		{TEXT("vitality_2"), TEXT("T_Research_Vitality_White")},
		{TEXT("stamina_1"), TEXT("T_Research_Stamina_White")},
		{TEXT("carry_1"), TEXT("T_Research_Carry_White")},
		{TEXT("nutrition_2"), TEXT("T_Research_Nutrition_White")},
		{TEXT("hydration_2"), TEXT("T_Research_Hydration_White")},
		{TEXT("vitality_3"), TEXT("T_Research_Vitality_White")},
		{TEXT("stamina_2"), TEXT("T_Research_Stamina_White")},
		{TEXT("carry_2"), TEXT("T_Research_Carry_White")},
		{TEXT("survival_mastery"), TEXT("T_Research_SurvivalMastery_White")},
		{TEXT("ultimate_conditioning"), TEXT("T_Research_UltimateConditioning_White")},
		{TEXT("weapon_burn_1"), TEXT("T_Research_WeaponBurn_White")},
		{TEXT("ammo_burn_1"), TEXT("T_Research_AmmoBurn_White")},
		{TEXT("weapon_burn_2"), TEXT("T_Research_WeaponBurn_White")},
		{TEXT("ammo_burn_2"), TEXT("T_Research_AmmoBurn_White")},
	};

	FString ObjectPath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("/Game/UI/Research/Icons/%s.%s"), AssetName, AssetName);
	}

	bool WriteIconFixture(const FString& OriginalJson, FName NodeId,
		const FString& IconPath, const FString& Destination)
	{
		TArray<TSharedPtr<FJsonValue>> FixtureRecords;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(OriginalJson), FixtureRecords)) return false;
		bool bChanged = false;
		for (const TSharedPtr<FJsonValue>& Record : FixtureRecords)
		{
			const TSharedPtr<FJsonObject> Object = Record.IsValid() ? Record->AsObject() : nullptr;
			FString Id;
			if (Object.IsValid() && Object->TryGetStringField(TEXT("node_id"), Id) && FName(*Id) == NodeId)
			{
				Object->SetStringField(TEXT("icon"), IconPath);
				bChanged = true;
				break;
			}
		}
		if (!bChanged) return false;
		FString FixtureJson;
		if (!FJsonSerializer::Serialize(FixtureRecords, TJsonWriterFactory<>::Create(&FixtureJson))) return false;
		return FFileHelper::SaveStringToFile(FixtureJson, *Destination);
	}

	void DescribeHierarchy(FAutomationTestBase& Test, const UWidgetTree& Tree)
	{
		TArray<UWidget*> Widgets;
		Tree.GetAllWidgets(Widgets);
		for (const UWidget* Widget : Widgets)
		{
			const UPanelWidget* Parent = Widget->GetParent();
			FString Detail = FString::Printf(TEXT("ResearchNode designer: %s (%s), parent=%s, slot=%s"),
				*Widget->GetName(), *Widget->GetClass()->GetName(),
				Parent ? *Parent->GetName() : TEXT("<root>"),
				Widget->Slot ? *Widget->Slot->GetClass()->GetName() : TEXT("<none>"));
			if (const USizeBox* SizeBox = Cast<USizeBox>(Widget))
			{
				Detail += FString::Printf(TEXT(", widthOverride=%s %.1f, heightOverride=%s %.1f"),
					SizeBox->IsWidthOverride() ? TEXT("yes") : TEXT("no"), SizeBox->GetWidthOverride(),
					SizeBox->IsHeightOverride() ? TEXT("yes") : TEXT("no"), SizeBox->GetHeightOverride());
			}
			Test.AddInfo(Detail);
		}
	}

	bool SaveCapture(FAutomationTestBase& Test, FWidgetRenderer& Renderer,
		const TSharedRef<SWidget>& Slate, FVector2D Size, const FString& Filename)
	{
		TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, Size));
		if (Target.Get())
		{
			FlushRenderingCommands();
			Renderer.DrawWidget(Target.Get(), Slate, Size, 0.f);
			FlushRenderingCommands();
			Renderer.DrawWidget(Target.Get(), Slate, Size, 0.f);
		}
		FlushRenderingCommands();
		FImage Pixels;
		if (!Test.TestTrue(*FString::Printf(TEXT("%s render is readable"), *Filename),
			Target.Get() && FImageUtils::GetRenderTargetImage(Target.Get(), Pixels))) return false;
		Pixels.GammaSpace = EGammaSpace::Linear;
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Screenshots/ResearchIcons");
		IFileManager::Get().MakeDirectory(*Directory, true);
		return Test.TestTrue(*FString::Printf(TEXT("%s capture saves"), *Filename),
			FImageUtils::SaveImageByExtension(*(Directory / Filename), Pixels));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperResearchAuthoredIconLayoutTest,
	"TunaSweeper.Research.AuthoredIconLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchAuthoredIconLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/UI/WBP_ResearchNode.WBP_ResearchNode"));
	if (!TestNotNull(TEXT("Editable research node Widget Blueprint loads"), Blueprint)) return false;
	const UWidgetTree* Tree = Blueprint->WidgetTree.Get();
	if (!TestNotNull(TEXT("Editable research node designer hierarchy is serialized"), Tree)) return false;
	TunaSweeperResearchIconTests::DescribeHierarchy(*this, *Tree);
	for (const TCHAR* WidgetName : {TEXT("NodeHeader"), TEXT("IconSizeBox"), TEXT("IconImage")})
	{
		const FGuid* Guid = Blueprint->WidgetVariableNameToGuidMap.Find(FName(WidgetName));
		TestTrue(FString::Printf(TEXT("Editable %s has a serialized widget variable GUID"), WidgetName),
			Guid && Guid->IsValid());
	}
	const UWidgetBlueprintGeneratedClass* GeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(Blueprint->GeneratedClass.Get());
	if (!TestNotNull(TEXT("Authored research node Blueprint loads"), GeneratedClass)) return false;
	const UWidgetTree* RuntimeTemplate = GeneratedClass->GetWidgetTreeArchetype();
	if (!TestNotNull(TEXT("Compiled research node hierarchy loads"), RuntimeTemplate)) return false;
	TestNotNull(TEXT("IconImage is present in the compiled template"), RuntimeTemplate->FindWidget(TEXT("IconImage")));

	const UImage* Icon = Cast<UImage>(Tree->FindWidget(TEXT("IconImage")));
	const UTextBlock* Name = Cast<UTextBlock>(Tree->FindWidget(TEXT("NameText")));
	if (!TestNotNull(TEXT("IconImage is an authored UImage"), Icon) ||
		!TestNotNull(TEXT("NameText remains an authored UTextBlock"), Name)) return false;
	const UHorizontalBox* Row = Cast<UHorizontalBox>(Name->GetParent());
	if (!TestNotNull(TEXT("NameText is in an editable horizontal row"), Row)) return false;
	const USizeBox* IconSizeBox = Cast<USizeBox>(Icon->GetParent());
	const UWidget* IconRowChild = IconSizeBox ? static_cast<const UWidget*>(IconSizeBox) : static_cast<const UWidget*>(Icon);
	TestTrue(TEXT("IconImage shares the name row"), IconRowChild->GetParent() == Row);
	TestTrue(TEXT("Icon appears to the left of NameText"), Row->GetChildIndex(IconRowChild) < Row->GetChildIndex(Name));
	if (IconSizeBox)
	{
		TestTrue(TEXT("Icon width has a designer override"), IconSizeBox->IsWidthOverride());
		TestTrue(TEXT("Icon height has a designer override"), IconSizeBox->IsHeightOverride());
		TestEqual(TEXT("Icon designer width is 48 px"), IconSizeBox->GetWidthOverride(), 48.f);
		TestEqual(TEXT("Icon designer height is 48 px"), IconSizeBox->GetHeightOverride(), 48.f);
	}
	else
	{
		TestEqual(TEXT("Icon brush designer width is 48 px"), Icon->GetBrush().ImageSize.X, 48.f);
		TestEqual(TEXT("Icon brush designer height is 48 px"), Icon->GetBrush().ImageSize.Y, 48.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperResearchIconPresentationTest,
	"TunaSweeper.Research.IconDataAndPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchIconPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString JsonText;
	const FString DataPath = FPaths::ProjectContentDir() / TEXT("Data/StatResearchNodes.json");
	if (!TestTrue(TEXT("Research JSON loads"), FFileHelper::LoadFileToString(JsonText, *DataPath))) return false;
	TArray<TSharedPtr<FJsonValue>> Records;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!TestTrue(TEXT("Research JSON parses"), FJsonSerializer::Deserialize(Reader, Records))) return false;
	TestEqual(TEXT("Research JSON has exactly 17 nodes"), Records.Num(), 17);
	TMap<FName, FString> AuthoredPaths;
	for (const TSharedPtr<FJsonValue>& Record : Records)
	{
		const TSharedPtr<FJsonObject> Object = Record.IsValid() ? Record->AsObject() : nullptr;
		if (!TestTrue(TEXT("Research JSON row is an object"), Object.IsValid())) return false;
		FString Id, IconPath;
		if (!TestTrue(TEXT("Research JSON row has an ID and icon"),
			Object->TryGetStringField(TEXT("node_id"), Id) && Object->TryGetStringField(TEXT("icon"), IconPath))) return false;
		AuthoredPaths.Add(FName(*Id), IconPath);
	}
	TestEqual(TEXT("Research IDs are unique"), AuthoredPaths.Num(), 17);
	for (const TunaSweeperResearchIconTests::FExpectedIcon& Expected : TunaSweeperResearchIconTests::ExpectedIcons)
	{
		const FString* Path = AuthoredPaths.Find(FName(Expected.NodeId));
		if (Path && !Path->IsEmpty()) LoadObject<UTexture2D>(nullptr, **Path);
	}
	// Newly imported textures can report a 32 px placeholder until their async compilation completes.
	FAssetCompilingManager::Get().FinishAllCompilation();
	TSet<FString> UniquePaths;
	for (const TunaSweeperResearchIconTests::FExpectedIcon& Expected : TunaSweeperResearchIconTests::ExpectedIcons)
	{
		const FString* Path = AuthoredPaths.Find(FName(Expected.NodeId));
		if (!TestNotNull(FString::Printf(TEXT("JSON icon for %s"), Expected.NodeId), Path)) continue;
		const FString ExpectedPath = TunaSweeperResearchIconTests::ObjectPath(Expected.AssetName);
		TestEqual(FString::Printf(TEXT("%s uses its intended illustration"), Expected.NodeId), *Path, ExpectedPath);
		UniquePaths.Add(*Path);
		if (!TestFalse(FString::Printf(TEXT("%s icon path is not blank"), Expected.NodeId), Path->IsEmpty())) continue;
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, **Path);
		if (!TestNotNull(FString::Printf(TEXT("%s icon asset loads"), Expected.NodeId), Texture)) continue;
		TestTrue(TEXT("Research icon source image is available"), Texture->Source.IsValid());
		TestTrue(TEXT("Research icon source width is 256 px"), Texture->Source.GetSizeX() == 256);
		TestTrue(TEXT("Research icon source height is 256 px"), Texture->Source.GetSizeY() == 256);
		TestEqual(TEXT("Research icon width"), Texture->GetSizeX(), 256);
		TestEqual(TEXT("Research icon height"), Texture->GetSizeY(), 256);
		TestTrue(TEXT("Research icon uses UI texture group"), Texture->LODGroup == TEXTUREGROUP_UI);
		TestTrue(TEXT("Research icon has no generated mips"), Texture->MipGenSettings == TMGS_NoMipmaps);
		TestTrue(TEXT("Research icon retains transparent pixels"), Texture->HasAlphaChannel());
	}
	TestEqual(TEXT("Nine distinct illustrations cover the research tree"), UniquePaths.Num(), 9);

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	TunaSweeperResearchIconTests::FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	Instance->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Instance->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		TunaSweeperResearchIconTests::FWorldContextAccess::Attach(Instance.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	World->InitializeActorsForPlay(FURL());
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	Player->SetControllerId(0);
	Controller->SetPlayer(Player);
	UTunaSweeperResearchSubsystem* Research = Instance->GetSubsystem<UTunaSweeperResearchSubsystem>();
	if (!TestNotNull(TEXT("Real research subsystem initializes"), Research)) return false;
	TArray<FTunaSweeperResearchNodeView> Views;
	if (!TestTrue(TEXT("Real research node views load"), Research->GetAllNodeViews(Views))) return false;
	TestEqual(TEXT("Real subsystem exposes all 17 nodes"), Views.Num(), 17);
	UClass* Class = LoadClass<UTunaSweeperResearchNodeWidget>(nullptr,
		TEXT("/Game/UI/WBP_ResearchNode.WBP_ResearchNode_C"));
	if (!TestNotNull(TEXT("Authored node class loads for presentation"), Class)) return false;
	TStrongObjectPtr<UTunaSweeperResearchNodeWidget> Widget(CreateWidget<UTunaSweeperResearchNodeWidget>(Controller, Class));
	if (!TestNotNull(TEXT("Real research node widget instantiates"), Widget.Get())) return false;
	TSharedRef<SWidget> NodeSlate = Widget->TakeWidget();
	bool bPresentationReady = true;
	for (const TCHAR* TextName : {TEXT("NameText"), TEXT("RequirementText"), TEXT("RemainingTimeText"), TEXT("ActionText")})
	{
		UTextBlock* Text = Cast<UTextBlock>(Widget->GetWidgetFromName(FName(TextName)));
		if (TestNotNull(FString::Printf(TEXT("Node %s text exists"), TextName), Text))
		{
			bPresentationReady &= TestTrue(FString::Printf(TEXT("Node %s uses a valid runtime font"), TextName),
				Text->GetFont().HasValidFont());
		}
		else bPresentationReady = false;
	}
	UClass* TreeClass = LoadClass<UTunaSweeperResearchTreeWidget>(nullptr,
		TEXT("/Game/UI/WBP_ResearchTree.WBP_ResearchTree_C"));
	if (!TestNotNull(TEXT("Authored whole research tree loads"), TreeClass)) return false;
	TStrongObjectPtr<UTunaSweeperResearchTreeWidget> TreeWidget(CreateWidget<UTunaSweeperResearchTreeWidget>(Controller, TreeClass));
	if (!TestNotNull(TEXT("Whole research tree instantiates"), TreeWidget.Get())) return false;
	TSharedRef<SWidget> TreeSlate = TreeWidget->TakeWidget();
	UTextBlock* StatusText = Cast<UTextBlock>(TreeWidget->GetWidgetFromName(TEXT("ResearchStatusText")));
	if (!TestNotNull(TEXT("Research status text exists"), StatusText)) return false;
	bPresentationReady &= TestTrue(TEXT("Research status uses a valid runtime font"), StatusText->GetFont().HasValidFont());
	UImage* Icon = Cast<UImage>(Widget->GetWidgetFromName(TEXT("IconImage")));
	if (!TestNotNull(TEXT("Real node exposes IconImage"), Icon)) return false;

	for (const FTunaSweeperResearchNodeView& View : Views)
	{
		Widget->NodeId = View.NodeId;
		Widget->RefreshFromSubsystem();
		const FString* Path = AuthoredPaths.Find(View.NodeId);
		if (!TestNotNull(FString::Printf(TEXT("Icon mapping exists for %s"), *View.NodeId.ToString()), Path)) continue;
		UTexture2D* ExpectedTexture = LoadObject<UTexture2D>(nullptr, **Path);
		bPresentationReady &= TestTrue(FString::Printf(TEXT("%s displays its JSON texture"), *View.NodeId.ToString()),
			Icon->GetBrush().GetResourceObject() == ExpectedTexture && ExpectedTexture != nullptr);
		bPresentationReady &= TestTrue(FString::Printf(TEXT("%s icon is visible"), *View.NodeId.ToString()), Icon->IsVisible());
		const float ExpectedAlpha = View.State == ETunaSweeperResearchNodeState::Locked ? 0.35f : 1.0f;
		bPresentationReady &= TestTrue(FString::Printf(TEXT("%s icon uses the state opacity"), *View.NodeId.ToString()),
			FMath::IsNearlyEqual(Icon->GetColorAndOpacity().A, ExpectedAlpha, 0.01f));
	}
	if (bPresentationReady)
	{
		FWidgetRenderer Renderer(false);
		TStrongObjectPtr<UTextureRenderTarget2D> GeometryTarget(Renderer.DrawWidget(NodeSlate, FVector2D(480, 180)));
		if (!TestNotNull(TEXT("Node layout render target exists"), GeometryTarget.Get())) return false;
		UTextBlock* NameText = CastChecked<UTextBlock>(Widget->GetWidgetFromName(TEXT("NameText")));
		UTextBlock* RequirementText = CastChecked<UTextBlock>(Widget->GetWidgetFromName(TEXT("RequirementText")));
		struct FLanguageCase { ETunaSweeperItemTextLanguage Language; const TCHAR* Suffix; };
		const FLanguageCase Languages[] = {
			{ETunaSweeperItemTextLanguage::Korean, TEXT("Korean")},
			{ETunaSweeperItemTextLanguage::English, TEXT("English")},
			{ETunaSweeperItemTextLanguage::Japanese, TEXT("Japanese")},
		};
		for (const FLanguageCase& Language : Languages)
		{
			Instance->SetCurrentTextLanguage(Language.Language, false);
			for (const FTunaSweeperResearchNodeView& View : Views)
			{
				Widget->NodeId = View.NodeId;
				Widget->RefreshFromSubsystem();
				Widget->ForceLayoutPrepass();
				Renderer.DrawWidget(GeometryTarget.Get(), NodeSlate, FVector2D(480, 180), 0.f);
				const FGeometry NameGeometry = NameText->GetCachedGeometry();
				const FGeometry RequirementGeometry = RequirementText->GetCachedGeometry();
				const FGeometry IconGeometry = Icon->GetCachedGeometry();
				NameText->ForceLayoutPrepass();
				const FString NodeContext = FString::Printf(TEXT("%s %s"), Language.Suffix, *View.NodeId.ToString());
				TestTrue(NodeContext + TEXT(" name fits its rendered allocation"),
					NameText->GetDesiredSize().Y <= NameGeometry.GetLocalSize().Y + 1.f);
				TestTrue(NodeContext + TEXT(" name does not overlap requirements"),
					NameGeometry.LocalToAbsolute(FVector2D(0.f, NameText->GetDesiredSize().Y)).Y <=
					RequirementGeometry.GetAbsolutePosition().Y + 1.f);
				TestTrue(NodeContext + TEXT(" icon renders at 48 px"),
					FMath::IsNearlyEqual(IconGeometry.GetAbsoluteSize().X, 48.f, 1.f) &&
					FMath::IsNearlyEqual(IconGeometry.GetAbsoluteSize().Y, 48.f, 1.f));
				TestTrue(NodeContext + TEXT(" icon is left of the name"),
					IconGeometry.GetAbsolutePosition().X + IconGeometry.GetAbsoluteSize().X <=
					NameGeometry.GetAbsolutePosition().X + 1.f);
			}
			UScrollBox* Scroll = Cast<UScrollBox>(TreeWidget->GetWidgetFromName(TEXT("ResearchScrollBox")));
			if (TestNotNull(TEXT("Whole research tree exposes its authored scroll box"), Scroll))
			{
				Scroll->SetScrollOffset(0.f);
				TreeWidget->ForceLayoutPrepass();
				TunaSweeperResearchIconTests::SaveCapture(*this, Renderer, TreeSlate, FVector2D(1280, 720),
					FString::Printf(TEXT("Tree_Top_%s_1280x720.png"), Language.Suffix));
				Scroll->SetScrollOffset(100000.f);
				TreeWidget->ForceLayoutPrepass();
				TunaSweeperResearchIconTests::SaveCapture(*this, Renderer, TreeSlate, FVector2D(1280, 720),
					FString::Printf(TEXT("Tree_Bottom_%s_1280x720.png"), Language.Suffix));
			}
		}
		for (const TCHAR* CaptureNodeId : {TEXT("vitality_1"), TEXT("nutrition_1")})
		{
			Widget->NodeId = FName(CaptureNodeId);
			Widget->RefreshFromSubsystem();
			Widget->ForceLayoutPrepass();
			TunaSweeperResearchIconTests::SaveCapture(*this, Renderer, NodeSlate, FVector2D(480, 180),
				FString::Printf(TEXT("Node_%s.png"), CaptureNodeId));
		}
	}
	Widget->NodeId = FName(TEXT("nonexistent_research_node"));
	Widget->RefreshFromSubsystem();
	TestNull(TEXT("An invalid node clears the previous icon"), Icon->GetBrush().GetResourceObject());
	TestFalse(TEXT("An invalid node does not leave an icon visible"), Icon->IsVisible());

	const FString FixtureDirectory = FPaths::ProjectSavedDir() / TEXT("Automation");
	IFileManager::Get().MakeDirectory(*FixtureDirectory, true);
	const FString FixturePath = FixtureDirectory / FString::Printf(TEXT("ResearchIconFixture_%s.json"),
		*FGuid::NewGuid().ToString(EGuidFormats::Digits));
	ON_SCOPE_EXIT
	{
		Research->LoadResearchDataFromFile(DataPath);
		IFileManager::Get().Delete(*FixturePath, false, true);
	};
	const FName FixtureNodeId(TEXT("vitality_1"));
	UTexture2D* ValidTexture = LoadObject<UTexture2D>(nullptr,
		*TunaSweeperResearchIconTests::ObjectPath(TEXT("T_Research_Vitality_White")));
	Widget->NodeId = FixtureNodeId;
	Widget->RefreshFromSubsystem();
	TestTrue(TEXT("Fixture starts from a visible valid icon"),
		Icon->IsVisible() && Icon->GetBrush().GetResourceObject() == ValidTexture && ValidTexture != nullptr);
	if (!TestTrue(TEXT("Empty-icon fixture writes"), TunaSweeperResearchIconTests::WriteIconFixture(
		JsonText, FixtureNodeId, FString(), FixturePath))) return false;
	if (!TestTrue(TEXT("Real subsystem accepts the empty-icon fixture"), Research->LoadResearchDataFromFile(FixturePath))) return false;
	Widget->RefreshFromSubsystem();
	TestNull(TEXT("Empty icon clears the formerly valid brush"), Icon->GetBrush().GetResourceObject());
	TestFalse(TEXT("Empty icon hides the image"), Icon->IsVisible());
	Widget->RefreshFromSubsystem();
	TestNull(TEXT("Repeated empty-icon refresh keeps the brush clear"), Icon->GetBrush().GetResourceObject());

	if (!TestTrue(TEXT("Wrong-type icon fixture writes"), TunaSweeperResearchIconTests::WriteIconFixture(
		JsonText, FixtureNodeId, TEXT("/Game/UI/WBP_ResearchNode.WBP_ResearchNode"), FixturePath))) return false;
	if (!TestTrue(TEXT("Real subsystem accepts the wrong-type icon fixture"), Research->LoadResearchDataFromFile(FixturePath))) return false;
	Widget->RefreshFromSubsystem();
	TestNull(TEXT("Wrong-type asset does not become an icon brush"), Icon->GetBrush().GetResourceObject());
	TestFalse(TEXT("Wrong-type asset hides the image"), Icon->IsVisible());
	Widget->RefreshFromSubsystem();
	TestNull(TEXT("Repeated wrong-type refresh keeps the brush clear"), Icon->GetBrush().GetResourceObject());

	Widget->RemoveFromParent();
	Widget->ReleaseSlateResources(true);
	TreeWidget->RemoveFromParent();
	TreeWidget->ReleaseSlateResources(true);
	return true;
}

#endif
