#include "SRTSMapGeneratorWindow.h"
#include "URTSMapForgeEditorSubsystem.h"
#include "Core/URTSGenerationSettings.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "FRTSMapForgeEdMode.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "RTSMapForge"

void SRTSMapGeneratorWindow::Construct(const FArguments& InArgs)
{
    InitializeSettings();

    // Populate overlay mode options
    OverlayOptions.Empty();
    for (int32 i = 0; i <= static_cast<int32>(ERTSDebugOverlayMode::ChokePoints); ++i)
    {
        ERTSDebugOverlayMode Mode = static_cast<ERTSDebugOverlayMode>(i);
        OverlayOptions.Add(MakeShared<FString>(OverlayModeToString(Mode)));
    }
    CurrentOverlayOption = OverlayOptions[static_cast<int32>(ERTSDebugOverlayMode::Heightmap)];

    PreviewBrush.DrawAs = ESlateBrushDrawType::Image;
    PreviewBrush.Tiling = ESlateBrushTileType::NoTile;
    PreviewBrush.Mirror = ESlateBrushMirrorType::NoMirror;
    PreviewBrush.ImageSize = PreviewDesiredSize;

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(8.0f)
        [
            SNew(SVerticalBox)

            // === TITLE ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WindowTitle", "RTS MapForge — Battlefield Generator"))
                .Font(FAppStyle::GetFontStyle("HeadingSmall"))
            ]

            // === PRESET & SEED ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Text(LOCTEXT("SeedLabel", "Seed: "))
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                [
                    SNew(SEditableTextBox)
                    .Text_Lambda([this]() -> FText {
                        return Settings.IsValid() ? FText::FromString(FString::Printf(TEXT("%lld"), Settings->Seed)) : FText::GetEmpty();
                    })
                    .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
                        if (Settings.IsValid()) Settings->Seed = FCString::Atoi64(*NewText.ToString());
                    })
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("Randomize", "🎲"))
                    .OnClicked_Lambda([this]() -> FReply { OnRandomizeSeedClicked(); return FReply::Handled(); })
                ]
            ]

            // === MAP SIZE ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Text(LOCTEXT("SizeLabel", "Size W/H: "))
                ]
                + SHorizontalBox::Slot().FillWidth(0.5f)
                [
                    SNew(SNumericEntryBox<int32>)
                    .MinValue(16).MaxValue(1024).MaxSliderValue(512)
                    .Value_Lambda([this]() -> TOptional<int32> {
                        return Settings.IsValid() ? TOptional<int32>(Settings->GridWidth) : TOptional<int32>();
                    })
                    .OnValueCommitted_Lambda([this](int32 NewVal, ETextCommit::Type) {
                        if (Settings.IsValid()) { Settings->GridWidth = NewVal; Settings->GridHeight = NewVal; }
                    })
                ]
            ]

            // === PLAYERS & SYMMETRY ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(0.5f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(STextBlock).Text(LOCTEXT("PlayersLabel", "Players: "))
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [
                        SNew(SNumericEntryBox<int32>)
                        .MinValue(2).MaxValue(12)
                        .Value_Lambda([this]() -> TOptional<int32> {
                            return Settings.IsValid() ? TOptional<int32>(Settings->NumPlayers) : TOptional<int32>();
                        })
                        .OnValueCommitted_Lambda([this](int32 NewVal, ETextCommit::Type) {
                            if (Settings.IsValid()) Settings->NumPlayers = NewVal;
                        })
                    ]
                ]
                + SHorizontalBox::Slot().FillWidth(0.5f).Padding(8, 0, 0, 0)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(STextBlock).Text(LOCTEXT("SymmetryLabel", "Symmetry: "))
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [
                        SNew(SNumericEntryBox<float>)
                        .MinValue(0.0f).MaxValue(1.0f)
                        .Value_Lambda([this]() -> TOptional<float> {
                            return Settings.IsValid() ? TOptional<float>(Settings->SymmetryStrength) : TOptional<float>();
                        })
                        .OnValueCommitted_Lambda([this](float NewVal, ETextCommit::Type) {
                            if (Settings.IsValid()) Settings->SymmetryStrength = NewVal;
                        })
                    ]
                ]
            ]

            // === GENERATE BUTTON ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .Text(LOCTEXT("Generate", "GENERATE MAP"))
                .OnClicked_Lambda([this]() -> FReply { OnGenerateClicked(); return FReply::Handled(); })
            ]

            // === OVERLAY MODE ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Text(LOCTEXT("OverlayLabel", "Overlay: "))
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&OverlayOptions)
                    .OnSelectionChanged(this, &SRTSMapGeneratorWindow::OnOverlaySelectionChanged)
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text(this, &SRTSMapGeneratorWindow::GetCurrentOverlayText)
                    ]
                    .OnGenerateWidget_Lambda([this](TSharedPtr<FString> Opt) -> TSharedRef<SWidget> {
                        return SNew(STextBlock).Text(FText::FromString(*Opt.Get()));
                    })
                ]
            ]

            // === VIEWPORT OVERLAY TOGGLE ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SNew(SCheckBox)
                .IsChecked(this, &SRTSMapGeneratorWindow::GetViewportOverlayState)
                .OnCheckStateChanged(this, &SRTSMapGeneratorWindow::OnViewportOverlayToggled)
                .Content()
                [
                    SNew(STextBlock).Text(LOCTEXT("ViewportOverlay", "Show Viewport Overlay"))
                ]
            ]

            // === MINIMAP PREVIEW ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .WidthOverride_Lambda([this]() -> FOptionalSize { return FOptionalSize(PreviewDesiredSize.X); })
                    .HeightOverride_Lambda([this]() -> FOptionalSize { return FOptionalSize(PreviewDesiredSize.Y); })
                    [
                        SNew(SImage)
                        .Image_Lambda([this]() -> const FSlateBrush* {
                            if (Subsystem.IsValid() && Subsystem->GetPreviewTexture())
                            {
                                PreviewBrush.SetResourceObject(Subsystem->GetPreviewTexture());
                                return &PreviewBrush;
                            }
                            return FAppStyle::GetBrush("WhiteBrush");
                        })
                    ]
                ]
            ]

            // === SCORE & VALIDATION ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText {
                    return FText::FromString(ScoreText);
                })
                .ColorAndOpacity_Lambda([this]() -> FSlateColor {
                    return Subsystem.IsValid() && Subsystem->GetLastValidation().bPassed ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Yellow);
                })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText {
                    return FText::FromString(ValidationText);
                })
                .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
            ]

            // === EXPORT ===
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ExportJSON", "Export JSON"))
                    .OnClicked_Lambda([this]() -> FReply {
                        if (Subsystem.IsValid()) Subsystem->ExportMetadataToJSON();
                        return FReply::Handled();
                    })
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("CycleOverlay", "Cycle Overlay"))
                    .OnClicked_Lambda([this]() -> FReply {
                        if (Subsystem.IsValid())
                        {
                            Subsystem->CycleOverlayMode();
                            CurrentOverlayOption = MakeShared<FString>(OverlayModeToString(Subsystem->GetOverlayMode()));
                        }
                        return FReply::Handled();
                    })
                ]
            ]
        ]
    ];

    RefreshReadouts();
}

void SRTSMapGeneratorWindow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    if (Subsystem.IsValid())
    {
        bool bHasGrid = Subsystem->HasValidGrid();
        if (bHasGrid != bLastHadGrid)
        {
            RefreshReadouts();
            bLastHadGrid = bHasGrid;
        }
    }
}

void SRTSMapGeneratorWindow::InitializeSettings()
{
    if (GEditor)
    {
        Subsystem = GEditor->GetEditorSubsystem<URTSMapForgeEditorSubsystem>();
    }

    Settings = NewObject<URTSGenerationSettings>(GetTransientPackage());
    Settings->GridWidth = 256;
    Settings->GridHeight = 256;
    Settings->bRandomSeed = true;  // Default to random seed; pipeline resolves once
    Settings->Seed = 0;
    Settings->NumPlayers = 2;
    Settings->SymmetryStrength = 1.0f;
    Settings->NumExpansions = 3;
}

void SRTSMapGeneratorWindow::RefreshReadouts()
{
    if (!Subsystem.IsValid())
    {
        ScoreText = TEXT("Subsystem not ready");
        ValidationText = TEXT("");
        return;
    }

    const FRTSValidationResult& Val = Subsystem->GetLastValidation();
    const FRTSMapMetadata& Meta = Subsystem->GetLastMetadata();

    if (!Subsystem->HasValidGrid())
    {
        ScoreText = TEXT("No map generated yet.");
        ValidationText = TEXT("Click GENERATE MAP to start.");
        return;
    }

    ScoreText = FString::Printf(TEXT("Score: %.1f | Bases: %d | Expansions: %d | Chokes: %d"),
        Val.OverallScore, Meta.Bases.Num(), Meta.Expansions.Num(), Meta.Chokes.Num());

    if (Val.bPassed)
    {
        ValidationText = FString::Printf(TEXT("Validation: PASS (%d issue(s))"), Val.Issues.Num());
    }
    else
    {
        ValidationText = FString::Printf(TEXT("Validation: FAIL — %d issue(s)"), Val.Issues.Num());
        for (const auto& Issue : Val.Issues)
        {
            ValidationText += FString::Printf(TEXT("\n• [%s] %s"),
                Issue.Severity == ERTSValidationSeverity::Critical ? TEXT("CRITICAL") : TEXT("WARN"),
                *Issue.Reason);
        }
    }

    if (Meta.GridWidth > 0 && Meta.GridHeight > 0)
    {
        float Aspect = static_cast<float>(Meta.GridHeight) / static_cast<float>(Meta.GridWidth);
        PreviewDesiredSize = FVector2D(256.0f, 256.0f * Aspect);
        PreviewBrush.ImageSize = PreviewDesiredSize;
    }
}

void SRTSMapGeneratorWindow::OnGenerateClicked()
{
    if (Subsystem.IsValid() && Settings.IsValid())
    {
        Subsystem->GenerateMap(Settings);
        
        // After generation, sync the displayed seed to what was actually resolved.
        // The pipeline resolved it deterministically; reflect that in the UI.
        const FRTSMapMetadata& Meta = Subsystem->GetLastMetadata();
        if (Meta.Seed != 0)
        {
            Settings->Seed = Meta.Seed;
            Settings->bRandomSeed = false;
        }
        
        RefreshReadouts();
    }
}

void SRTSMapGeneratorWindow::OnRandomizeSeedClicked()
{
    if (Settings.IsValid())
    {
        // CRITICAL: Do NOT call Settings->ResolveSeed() here.
        // ResolveSeed() must be called EXACTLY ONCE inside the pipeline.
        // 
        // Instead, we set bRandomSeed=true and leave Seed=0 (or any value).
        // The pipeline will resolve the actual random seed once during generation,
        // and then we sync the resolved value back to the UI via OnGenerateClicked().
        //
        // This prevents:
        //   - Pre-resolving a seed that doesn't match the pipeline's resolution
        //   - Double-calling ResolveSeed() (which would return different values)
        //   - UI seed display diverging from actual generated map seed
        Settings->bRandomSeed = true;
        Settings->Seed = 0; // Ignored when bRandomSeed=true; pipeline will resolve
    }
}

void SRTSMapGeneratorWindow::OnOverlaySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    CurrentOverlayOption = NewSelection;
    if (Subsystem.IsValid() && NewSelection.IsValid())
    {
        ERTSDebugOverlayMode Mode = StringToOverlayMode(*NewSelection.Get());
        Subsystem->SetOverlayMode(Mode);
    }
}

void SRTSMapGeneratorWindow::OnViewportOverlayToggled(ECheckBoxState NewState)
{
    if (!GEditor) return;

    FEditorModeTools& ModeTools = GEditor->GetEditorModeManager();
    if (NewState == ECheckBoxState::Checked)
    {
        ModeTools.ActivateMode(FRTSMapForgeEdMode::EM_RTSMapForge);
    }
    else
    {
        ModeTools.DeactivateMode(FRTSMapForgeEdMode::EM_RTSMapForge);
    }
}

ECheckBoxState SRTSMapGeneratorWindow::GetViewportOverlayState() const
{
    if (!GEditor) return ECheckBoxState::Unchecked;
    return GEditor->GetEditorModeManager().IsModeActive(FRTSMapForgeEdMode::EM_RTSMapForge)
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

FText SRTSMapGeneratorWindow::GetCurrentOverlayText() const
{
    return CurrentOverlayOption.IsValid() ? FText::FromString(*CurrentOverlayOption.Get()) : FText::GetEmpty();
}

TSharedRef<SWidget> SRTSMapGeneratorWindow::MakeOverlayOptionWidget(TSharedPtr<FString> InOption)
{
    return SNew(STextBlock).Text(FText::FromString(*InOption.Get()));
}

FString SRTSMapGeneratorWindow::OverlayModeToString(ERTSDebugOverlayMode Mode)
{
    switch (Mode)
    {
        case ERTSDebugOverlayMode::None:           return TEXT("None");
        case ERTSDebugOverlayMode::Heightmap:       return TEXT("Heightmap");
        case ERTSDebugOverlayMode::WaterCliff:      return TEXT("Water & Cliffs");
        case ERTSDebugOverlayMode::Walkable:        return TEXT("Walkable");
        case ERTSDebugOverlayMode::Buildable:       return TEXT("Buildable");
        case ERTSDebugOverlayMode::Slope:          return TEXT("Slope");
        case ERTSDebugOverlayMode::Regions:         return TEXT("Regions");
        case ERTSDebugOverlayMode::Biomes:          return TEXT("Biomes");
        case ERTSDebugOverlayMode::TacticalZones:   return TEXT("Tactical Zones");
        case ERTSDebugOverlayMode::Influence:       return TEXT("Influence Map");
        case ERTSDebugOverlayMode::CombatHeat:      return TEXT("Combat Heat");
        case ERTSDebugOverlayMode::ChokePoints:     return TEXT("Choke Points");
        default:                                    return TEXT("Unknown");
    }
}

ERTSDebugOverlayMode SRTSMapGeneratorWindow::StringToOverlayMode(const FString& Str)
{
    for (int32 i = 0; i <= static_cast<int32>(ERTSDebugOverlayMode::ChokePoints); ++i)
    {
        ERTSDebugOverlayMode Mode = static_cast<ERTSDebugOverlayMode>(i);
        if (OverlayModeToString(Mode) == Str) return Mode;
    }
    return ERTSDebugOverlayMode::Heightmap;
}

#undef LOCTEXT_NAMESPACE
