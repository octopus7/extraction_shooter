#include "TunaSweeperEnemyAIDebugTool.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "UObject/Package.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"

#define LOCTEXT_NAMESPACE "TunaSweeperEnemyAIDebugTool"

namespace TunaSweeperEnemyAIDebug
{
	const FName TabName(TEXT("TunaSweeperEnemyAIDebug"));
	const FName DistanceColumnName(TEXT("Distance"));
	const FName EnemyColumnName(TEXT("Enemy"));
	const FName StateColumnName(TEXT("State"));
	const FName CombatColumnName(TEXT("Combat"));
	const FName SightColumnName(TEXT("Sight"));
	const FName TimeColumnName(TEXT("Time"));
	const FName ReasonColumnName(TEXT("Reason"));
	const FString RaidMapPackageName(TEXT("/Game/RaidMap"));
	const TCHAR* SkirtPhysicsDebugConsoleVariables[] = {
		TEXT("p.Chaos.DebugDraw.Enabled"),
		TEXT("p.RigidBodyNode.DebugDraw"),
		TEXT("p.Chaos.ImmPhys.DebugDrawShapes"),
		TEXT("p.Chaos.ImmPhys.DebugDrawJoints")
	};

	constexpr double RefreshIntervalSeconds = 0.25;

	enum class EPanelState : uint8
	{
		RaidLevelOnly,
		WaitingForPlay,
		WaitingForPlayer,
		NoEnemies,
		Ready
	};

	struct FEnemyRow
	{
		TWeakObjectPtr<ATunaSweeperEnemyCharacter> Enemy;
		TWeakObjectPtr<APlayerController> PlayerController;
		FString EnemyName;
		double DistanceSquared2D = 0.0;
		FTunaSweeperEnemyCombatDebugSnapshot Snapshot;
	};

	class SScreenBearingIndicator final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SScreenBearingIndicator)
		{
		}
			SLATE_ARGUMENT(TWeakObjectPtr<ATunaSweeperEnemyCharacter>, Enemy)
			SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, PlayerController)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Enemy = InArgs._Enemy;
			PlayerController = InArgs._PlayerController;
			ForceVolatile(true);
			SetToolTipText(LOCTEXT("ScreenBearingTooltip", "현재 플레이 화면 기준 적 방향"));
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const override
		{
			FVector2f ScreenDirection;
			if (!ResolveScreenDirection(ScreenDirection))
			{
				return LayerId;
			}

			const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
			const float Radius = FMath::Max(
				0.0f,
				FMath::Min(static_cast<float>(LocalSize.X), static_cast<float>(LocalSize.Y)) * 0.5f - 1.0f);
			if (Radius <= KINDA_SMALL_NUMBER)
			{
				return LayerId;
			}

			const FVector2f Center(
				static_cast<float>(LocalSize.X) * 0.5f,
				static_cast<float>(LocalSize.Y) * 0.5f);
			const FVector2f Perpendicular(-ScreenDirection.Y, ScreenDirection.X);
			const FVector2f Tip = Center + ScreenDirection * Radius;
			const FVector2f RearCenter = Center - ScreenDirection * Radius * 0.55f;
			const FVector2f RearLeft = RearCenter + Perpendicular * Radius * 0.43f;
			const FVector2f RearRight = RearCenter - Perpendicular * Radius * 0.43f;

			const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled)
				? ESlateDrawEffect::None
				: ESlateDrawEffect::DisabledEffect;
			const FLinearColor WidgetTint = InWidgetStyle.GetColorAndOpacityTint();
			const FLinearColor TipColor =
				FLinearColor(0.72f, 0.95f, 1.0f, 1.0f) * WidgetTint;
			const FLinearColor RearColor =
				FLinearColor(0.10f, 0.48f, 0.90f, 0.90f) * WidgetTint;

			TArray<FSlateVertex> Vertices;
			Vertices.Reserve(3);
			auto AddVertex = [&AllottedGeometry, &Vertices](const FVector2f& LocalPoint, const FLinearColor& Color)
			{
				Vertices.AddZeroed();
				FSlateVertex& Vertex = Vertices.Last();
				Vertex.Position = FVector2f(AllottedGeometry.LocalToAbsolute(
					FVector2D(LocalPoint.X, LocalPoint.Y)));
				Vertex.Color = Color.ToFColor(false);
			};
			AddVertex(Tip, TipColor);
			AddVertex(RearLeft, RearColor);
			AddVertex(RearRight, RearColor);

			const FSlateBrush* WhiteBrush = FAppStyle::GetBrush(TEXT("WhiteBrush"));
			if (WhiteBrush)
			{
				const TArray<SlateIndex> Indices = { 0, 1, 2 };
				FSlateDrawElement::MakeCustomVerts(
					OutDrawElements,
					LayerId,
					WhiteBrush->GetRenderingResource(),
					Vertices,
					Indices,
					nullptr,
					0,
					0,
					DrawEffects);
			}

			TArray<FVector2f> OutlinePoints = { Tip, RearLeft, RearRight, Tip };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(OutlinePoints),
				DrawEffects,
				FLinearColor(0.72f, 0.95f, 1.0f, 0.95f) * WidgetTint,
				true,
				1.0f);

			return LayerId + 2;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(16.0f, 16.0f);
		}

	private:
		bool ResolveScreenDirection(FVector2f& OutScreenDirection) const
		{
			APlayerController* LocalPlayerController = PlayerController.Get();
			ATunaSweeperEnemyCharacter* LocalEnemy = Enemy.Get();
			APawn* PlayerPawn = LocalPlayerController ? LocalPlayerController->GetPawn() : nullptr;
			if (!LocalPlayerController || !LocalEnemy || !PlayerPawn)
			{
				return false;
			}

			const FVector PlayerLocation = PlayerPawn->GetActorLocation();
			FVector WorldDirection = LocalEnemy->GetActorLocation() - PlayerLocation;
			WorldDirection.Z = 0.0f;
			if (!WorldDirection.Normalize())
			{
				return false;
			}

			constexpr float DirectionProbeDistance = 1000.0f;
			FVector2D PlayerScreenPosition;
			FVector2D DirectionProbeScreenPosition;
			if (LocalPlayerController->ProjectWorldLocationToScreen(
					PlayerLocation,
					PlayerScreenPosition,
					true) &&
				LocalPlayerController->ProjectWorldLocationToScreen(
					PlayerLocation + WorldDirection * DirectionProbeDistance,
					DirectionProbeScreenPosition,
					true))
			{
				FVector2D ProjectedDirection = DirectionProbeScreenPosition - PlayerScreenPosition;
				if (ProjectedDirection.Normalize())
				{
					OutScreenDirection = FVector2f(ProjectedDirection);
					return true;
				}
			}

			FVector ViewLocation;
			FRotator ViewRotation;
			LocalPlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
			const float ViewYawRadians = FMath::DegreesToRadians(ViewRotation.Yaw);
			const FVector ViewForward(
				FMath::Cos(ViewYawRadians),
				FMath::Sin(ViewYawRadians),
				0.0f);
			const FVector ViewRight(-ViewForward.Y, ViewForward.X, 0.0f);
			FVector2D FallbackDirection(
				FVector::DotProduct(WorldDirection, ViewRight),
				-FVector::DotProduct(WorldDirection, ViewForward));
			if (!FallbackDirection.Normalize())
			{
				return false;
			}

			OutScreenDirection = FVector2f(FallbackDirection);
			return true;
		}

		TWeakObjectPtr<ATunaSweeperEnemyCharacter> Enemy;
		TWeakObjectPtr<APlayerController> PlayerController;
	};

	class SEnemyRow final : public SMultiColumnTableRow<TSharedPtr<FEnemyRow>>
	{
	public:
		SLATE_BEGIN_ARGS(SEnemyRow)
		{
		}
			SLATE_ARGUMENT(TSharedPtr<FEnemyRow>, Item)
		SLATE_END_ARGS()

		void Construct(
			const FArguments& InArgs,
			const TSharedRef<STableViewBase>& InOwnerTableView)
		{
			Item = InArgs._Item;
			SMultiColumnTableRow<TSharedPtr<FEnemyRow>>::Construct(
				FSuperRowType::FArguments().Padding(FMargin(4.0f, 2.0f)),
				InOwnerTableView);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
		{
			FText Text = FText::GetEmpty();
			FSlateColor Color = FSlateColor::UseForeground();

			if (!Item.IsValid())
			{
				return SNew(STextBlock).Text(Text);
			}

			if (ColumnName == DistanceColumnName)
			{
				const double DistanceMeters = FMath::Sqrt(Item->DistanceSquared2D) / 100.0;
				Text = FText::FromString(FString::Printf(TEXT("%.1f m"), DistanceMeters));
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						SNew(SScreenBearingIndicator)
						.Enemy(Item->Enemy)
						.PlayerController(Item->PlayerController)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(Text)
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.ToolTipText(Text)
					];
			}
			else if (ColumnName == EnemyColumnName)
			{
				Text = FText::FromString(Item->EnemyName);
			}
			else if (ColumnName == StateColumnName)
			{
				Text = FText::FromString(Item->Snapshot.StateLabel);
				Color = Item->Snapshot.bIsCombatEngaged
					? FSlateColor(FLinearColor(1.0f, 0.55f, 0.20f))
					: FSlateColor(FLinearColor(0.65f, 0.85f, 1.0f));
			}
			else if (ColumnName == CombatColumnName)
			{
				Text = Item->Snapshot.bIsCombatEngaged
					? LOCTEXT("CombatYes", "전투")
					: LOCTEXT("CombatNo", "비전투");
				Color = Item->Snapshot.bIsCombatEngaged
					? FSlateColor(FLinearColor(1.0f, 0.36f, 0.18f))
					: FSlateColor(FLinearColor(0.55f, 0.75f, 0.62f));
			}
			else if (ColumnName == SightColumnName)
			{
				Text = Item->Snapshot.bHasDirectTargetSight
					? LOCTEXT("SightYes", "있음")
					: LOCTEXT("SightNo", "없음");
				Color = Item->Snapshot.bHasDirectTargetSight
					? FSlateColor(FLinearColor(1.0f, 0.86f, 0.28f))
					: FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f));
			}
			else if (ColumnName == TimeColumnName)
			{
				Text = Item->Snapshot.MaxStateSeconds > 0.0f
					? FText::FromString(FString::Printf(
						TEXT("%.1f / %.1fs"),
						Item->Snapshot.RemainingStateSeconds,
						Item->Snapshot.MaxStateSeconds))
					: FText::FromString(TEXT("-"));
			}
			else if (ColumnName == ReasonColumnName)
			{
				Text = Item->Snapshot.RecentEntryReason.IsEmpty()
					? FText::FromString(TEXT("-"))
					: FText::FromString(Item->Snapshot.RecentEntryReason);
			}

			return SNew(STextBlock)
				.Text(Text)
				.ColorAndOpacity(Color)
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				.ToolTipText(Text);
		}

	private:
		TSharedPtr<FEnemyRow> Item;
	};

	class SEnemyAIDebugPanel final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SEnemyAIDebugPanel)
		{
		}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 8.0f, 8.0f, 4.0f)
				[
					SNew(STextBlock)
					.Text(this, &SEnemyAIDebugPanel::GetSummaryText)
					.Font(FAppStyle::GetFontStyle(TEXT("BoldFont")))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(8.0f, 4.0f, 8.0f, 8.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SAssignNew(ListView, SListView<TSharedPtr<FEnemyRow>>)
						.ListItemsSource(&Rows)
						.OnGenerateRow(this, &SEnemyAIDebugPanel::GenerateRow)
						.OnMouseButtonDoubleClick(this, &SEnemyAIDebugPanel::HandleRowDoubleClicked)
						.SelectionMode(ESelectionMode::Single)
						.Visibility(this, &SEnemyAIDebugPanel::GetListVisibility)
						.HeaderRow
						(
							SNew(SHeaderRow)
							+ SHeaderRow::Column(DistanceColumnName)
							.DefaultLabel(LOCTEXT("DistanceColumn", "거리"))
							.FixedWidth(108.0f)
							+ SHeaderRow::Column(EnemyColumnName)
							.DefaultLabel(LOCTEXT("EnemyColumn", "적"))
							.FillWidth(0.18f)
							+ SHeaderRow::Column(StateColumnName)
							.DefaultLabel(LOCTEXT("StateColumn", "상태"))
							.FillWidth(0.16f)
							+ SHeaderRow::Column(CombatColumnName)
							.DefaultLabel(LOCTEXT("CombatColumn", "교전"))
							.FixedWidth(68.0f)
							+ SHeaderRow::Column(SightColumnName)
							.DefaultLabel(LOCTEXT("SightColumn", "직접 시야"))
							.FixedWidth(78.0f)
							+ SHeaderRow::Column(TimeColumnName)
							.DefaultLabel(LOCTEXT("TimeColumn", "남은 시간"))
							.FixedWidth(104.0f)
							+ SHeaderRow::Column(ReasonColumnName)
							.DefaultLabel(LOCTEXT("ReasonColumn", "최근 진입 사유"))
							.FillWidth(0.28f)
						)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
						.Padding(24.0f)
						.Visibility(this, &SEnemyAIDebugPanel::GetEmptyStateVisibility)
						[
							SNew(STextBlock)
							.Text(this, &SEnemyAIDebugPanel::GetEmptyStateText)
							.Justification(ETextJustify::Center)
							.AutoWrapText(true)
						]
					]
				]
			];

			RefreshRows();
		}

		virtual void Tick(
			const FGeometry& AllottedGeometry,
			const double InCurrentTime,
			const float InDeltaTime) override
		{
			SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
			SecondsUntilRefresh -= InDeltaTime;
			if (SecondsUntilRefresh <= 0.0)
			{
				RefreshRows();
			}
		}

	private:
		static bool IsRaidWorld(const UWorld* World)
		{
			return World &&
				UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()).Equals(
					RaidMapPackageName,
					ESearchCase::CaseSensitive);
		}

		static APlayerController* FindLocalPlayerController(UWorld* World)
		{
			if (!World)
			{
				return nullptr;
			}

			for (FConstPlayerControllerIterator ControllerIt = World->GetPlayerControllerIterator();
				ControllerIt;
				++ControllerIt)
			{
				APlayerController* PlayerController = ControllerIt->Get();
				if (PlayerController && PlayerController->IsLocalController() && PlayerController->GetPawn())
				{
					return PlayerController;
				}
			}

			return nullptr;
		}

		static APawn* FindLocalPlayerPawn(UWorld* World)
		{
			APlayerController* PlayerController = FindLocalPlayerController(World);
			return PlayerController ? PlayerController->GetPawn() : nullptr;
		}

		static UWorld* ResolvePlayWorld()
		{
			if (!GEngine)
			{
				return nullptr;
			}

			UWorld* FallbackPlayWorld = nullptr;
			for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
			{
				if (WorldContext.WorldType != EWorldType::PIE || WorldContext.RunAsDedicated)
				{
					continue;
				}

				UWorld* World = WorldContext.World();
				if (!World)
				{
					continue;
				}

				if (!FallbackPlayWorld)
				{
					FallbackPlayWorld = World;
				}
				if (FindLocalPlayerPawn(World))
				{
					return World;
				}
			}

			return FallbackPlayWorld;
		}

		void RefreshRows()
		{
			SecondsUntilRefresh = RefreshIntervalSeconds;
			Rows.Reset();

			UWorld* PlayWorld = ResolvePlayWorld();
			if (!PlayWorld)
			{
				UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
				PanelState = IsRaidWorld(EditorWorld)
					? EPanelState::WaitingForPlay
					: EPanelState::RaidLevelOnly;
				RequestListRefresh();
				return;
			}

			if (!IsRaidWorld(PlayWorld))
			{
				PanelState = EPanelState::RaidLevelOnly;
				RequestListRefresh();
				return;
			}

			APlayerController* PlayerController = FindLocalPlayerController(PlayWorld);
			APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
			if (!PlayerPawn)
			{
				PanelState = EPanelState::WaitingForPlayer;
				RequestListRefresh();
				return;
			}

			const FVector PlayerLocation = PlayerPawn->GetActorLocation();
			for (TActorIterator<ATunaSweeperEnemyCharacter> EnemyIt(PlayWorld); EnemyIt; ++EnemyIt)
			{
				ATunaSweeperEnemyCharacter* Enemy = *EnemyIt;
				ATunaSweeperEnemyAIController* EnemyController = Enemy
					? Cast<ATunaSweeperEnemyAIController>(Enemy->GetController())
					: nullptr;
				FTunaSweeperEnemyCombatDebugSnapshot Snapshot;
				if (!EnemyController || !EnemyController->GetCombatDebugSnapshot(Snapshot))
				{
					continue;
				}

				TSharedPtr<FEnemyRow> Row = MakeShared<FEnemyRow>();
				Row->Enemy = Enemy;
				Row->PlayerController = PlayerController;
				Row->EnemyName = Enemy->GetActorNameOrLabel();
				Row->DistanceSquared2D = FVector::DistSquared2D(PlayerLocation, Enemy->GetActorLocation());
				Row->Snapshot = MoveTemp(Snapshot);
				Rows.Add(MoveTemp(Row));
			}

			Rows.Sort([](const TSharedPtr<FEnemyRow>& Left, const TSharedPtr<FEnemyRow>& Right)
			{
				if (!Left.IsValid() || !Right.IsValid())
				{
					return Left.IsValid();
				}
				if (Left->DistanceSquared2D != Right->DistanceSquared2D)
				{
					return Left->DistanceSquared2D < Right->DistanceSquared2D;
				}
				return Left->EnemyName < Right->EnemyName;
			});

			PanelState = Rows.IsEmpty() ? EPanelState::NoEnemies : EPanelState::Ready;
			RequestListRefresh();
		}

		void RequestListRefresh() const
		{
			if (ListView.IsValid())
			{
				ListView->RequestListRefresh();
			}
		}

		TSharedRef<ITableRow> GenerateRow(
			TSharedPtr<FEnemyRow> Item,
			const TSharedRef<STableViewBase>& OwnerTable) const
		{
			return SNew(SEnemyRow, OwnerTable)
				.Item(MoveTemp(Item));
		}

		void HandleRowDoubleClicked(TSharedPtr<FEnemyRow> Item) const
		{
			ATunaSweeperEnemyCharacter* Enemy = Item.IsValid() ? Item->Enemy.Get() : nullptr;
			if (!GEditor || !Enemy)
			{
				return;
			}

			GEditor->SelectNone(false, true, false);
			GEditor->SelectActor(Enemy, true, true, true);
			GEditor->MoveViewportCamerasToActor(*Enemy, false);
		}

		FText GetSummaryText() const
		{
			if (PanelState != EPanelState::Ready)
			{
				return LOCTEXT("SummaryIdle", "Enemy AI Monitor · RaidMap PIE/SIE 전용");
			}

			return FText::Format(
				LOCTEXT("SummaryReady", "Enemy AI Monitor · 가까운 순 · 0.25초 갱신 · {0}명"),
				FText::AsNumber(Rows.Num()));
		}

		FText GetEmptyStateText() const
		{
			switch (PanelState)
			{
			case EPanelState::WaitingForPlay:
				return LOCTEXT("WaitingForPlay", "RaidMap에서 플레이 또는 시뮬레이트를 시작하세요");
			case EPanelState::WaitingForPlayer:
				return LOCTEXT("WaitingForPlayer", "플레이어를 기다리는 중입니다");
			case EPanelState::NoEnemies:
				return LOCTEXT("NoEnemies", "표시할 적 AI가 없습니다");
			case EPanelState::RaidLevelOnly:
			default:
				return LOCTEXT("RaidLevelOnly", "전투 레벨에서만 동작합니다");
			}
		}

		EVisibility GetListVisibility() const
		{
			return PanelState == EPanelState::Ready
				? EVisibility::Visible
				: EVisibility::Collapsed;
		}

		EVisibility GetEmptyStateVisibility() const
		{
			return PanelState == EPanelState::Ready
				? EVisibility::Collapsed
				: EVisibility::Visible;
		}

		EPanelState PanelState = EPanelState::RaidLevelOnly;
		double SecondsUntilRefresh = 0.0;
		TArray<TSharedPtr<FEnemyRow>> Rows;
		TSharedPtr<SListView<TSharedPtr<FEnemyRow>>> ListView;
	};
}

void FTunaSweeperEnemyAIDebugTool::Startup()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TunaSweeperEnemyAIDebug::TabName,
		FOnSpawnTab::CreateRaw(this, &FTunaSweeperEnemyAIDebugTool::SpawnToolTab))
		.SetDisplayName(LOCTEXT("TabTitle", "Enemy AI Monitor"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Inspect RaidMap enemy AI states ordered by distance during PIE or SIE."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperEnemyAIDebugTool::RegisterMenus));
}

void FTunaSweeperEnemyAIDebugTool::Shutdown()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

	if (TSharedPtr<SDockTab> LiveTab = FGlobalTabmanager::Get()->FindExistingLiveTab(
		TunaSweeperEnemyAIDebug::TabName))
	{
		LiveTab->RequestCloseTab();
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TunaSweeperEnemyAIDebug::TabName);
}

void FTunaSweeperEnemyAIDebugTool::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
	FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
	if (!MainSection.FindEntry(TEXT("TunaSweeper")))
	{
		FToolMenuEntry& TunaSweeperEntry = MainSection.AddSubMenu(
			TEXT("TunaSweeper"),
			LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
			LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
			FNewToolMenuChoice());
		TunaSweeperEntry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
	}

	UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
		TEXT("LevelEditor.MainMenu.TunaSweeper"),
		NAME_None,
		EMultiBoxType::Menu,
		false);
	FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
		TEXT("Debug"),
		LOCTEXT("DebugMenuSection", "Debug"));
	Section.AddMenuEntry(
		TEXT("OpenTunaSweeperEnemyAIDebug"),
		LOCTEXT("MenuEntry", "Enemy AI Monitor"),
		LOCTEXT("MenuEntryTooltip", "Open the RaidMap PIE/SIE enemy AI monitor."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperEnemyAIDebugTool::OpenToolWindow)));
	Section.AddMenuEntry(
		TEXT("ToggleTunaSweeperSkirtPhysicsDebugDraw"),
		LOCTEXT("SkirtPhysicsDebugMenuEntry", "Skirt Physics Debug Draw"),
		LOCTEXT(
			"SkirtPhysicsDebugMenuEntryTooltip",
			"Toggle the Chaos, Rigid Body AnimNode, Immediate Physics shape, and joint debug drawing used to inspect Luna's skirt during PIE or SIE."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTunaSweeperEnemyAIDebugTool::ToggleSkirtPhysicsDebugDraw),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FTunaSweeperEnemyAIDebugTool::IsSkirtPhysicsDebugDrawEnabled)),
		EUserInterfaceActionType::ToggleButton);
}

void FTunaSweeperEnemyAIDebugTool::OpenToolWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TunaSweeperEnemyAIDebug::TabName);
}

void FTunaSweeperEnemyAIDebugTool::ToggleSkirtPhysicsDebugDraw()
{
	const int32 TargetValue = IsSkirtPhysicsDebugDrawEnabled() ? 0 : 1;
	for (const TCHAR* ConsoleVariableName : TunaSweeperEnemyAIDebug::SkirtPhysicsDebugConsoleVariables)
	{
		if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(ConsoleVariableName))
		{
			ConsoleVariable->Set(TargetValue, ECVF_SetByConsole);
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Luna skirt physics debug draw %s."),
		TargetValue != 0 ? TEXT("enabled") : TEXT("disabled"));
}

bool FTunaSweeperEnemyAIDebugTool::IsSkirtPhysicsDebugDrawEnabled() const
{
	for (const TCHAR* ConsoleVariableName : TunaSweeperEnemyAIDebug::SkirtPhysicsDebugConsoleVariables)
	{
		const IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(ConsoleVariableName);
		if (!ConsoleVariable || ConsoleVariable->GetInt() == 0)
		{
			return false;
		}
	}

	return true;
}

TSharedRef<SDockTab> FTunaSweeperEnemyAIDebugTool::SpawnToolTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(TunaSweeperEnemyAIDebug::SEnemyAIDebugPanel)
		];
}

#undef LOCTEXT_NAMESPACE
