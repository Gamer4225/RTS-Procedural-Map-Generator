#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"

/**
 * Editor mode that renders the RTS map grid overlay in the level editor viewport.
 * Activated via the generator window's "Show Viewport Overlay" checkbox.
 */
class FRTSMapForgeEdMode : public FEdMode
{
public:
    static const FEditorModeID EM_RTSMapForge;

    FRTSMapForgeEdMode();
    virtual ~FRTSMapForgeEdMode();

    // FEdMode interface
    virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
    virtual void Enter() override;
    virtual void Exit() override;

    // Selection blocking
    virtual bool AllowWidgetMove() override { return false; }
    virtual bool ShouldDrawWidget() const override { return false; }
    virtual bool UsesTransformWidget() const override { return false; }
};
