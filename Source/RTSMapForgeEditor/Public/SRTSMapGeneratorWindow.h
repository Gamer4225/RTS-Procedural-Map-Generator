#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Visualization/FRTSDebugRenderer.h"

class URTSMapForgeEditorSubsystem;
class URTSGenerationSettings;

/**
 * Main Slate window for the RTS MapForge generator.
 * Provides parameter controls, generation trigger, overlay toggles,
 * minimap preview, and validation readout.
 *
 * FIX Problem 1: Generate button is disabled while IsGenerating() is true.
 *               Tick() detects async completion and calls RefreshReadouts()
 *               without blocking the editor thread.
 */
class SRTSMapGeneratorWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRTSMapGeneratorWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    TWeakObjectPtr<URTSMapForgeEditorSubsystem> Subsystem;
    TWeakObjectPtr<URTSGenerationSettings>      Settings;

    // Preview brush — updated from subsystem texture
    FSlateBrush PreviewBrush;
    FVector2D   PreviewDesiredSize = FVector2D(256.0f, 256.0f);

    // Overlay options for combo box
    TArray<TSharedPtr<FString>>  OverlayOptions;
    TSharedPtr<FString>          CurrentOverlayOption;

    // Cached readout strings
    FString ScoreText;
    FString ValidationText;
    bool    bLastHadGrid   = false;
    bool    bWasGenerating = false; // FIX Problem 1: Track generating state for Tick refresh

    void InitializeSettings();
    void RefreshReadouts();
    void OnGenerateClicked();
    void OnRandomizeSeedClicked();
    void OnOverlaySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
    void OnViewportOverlayToggled(ECheckBoxState NewState);
    ECheckBoxState GetViewportOverlayState() const;
    FText GetCurrentOverlayText() const;

    TSharedRef<SWidget> MakeOverlayOptionWidget(TSharedPtr<FString> InOption);

    static FString           OverlayModeToString(ERTSDebugOverlayMode Mode);
    static ERTSDebugOverlayMode StringToOverlayMode(const FString& Str);
};
