#include "FRTSMapForgeEdMode.h"
#include "URTSMapForgeEditorSubsystem.h"
#include "Visualization/FRTSDebugRenderer.h"
#include "Editor.h"

const FEditorModeID FRTSMapForgeEdMode::EM_RTSMapForge(TEXT("EM_RTSMapForge"));

FRTSMapForgeEdMode::FRTSMapForgeEdMode()
{
}

FRTSMapForgeEdMode::~FRTSMapForgeEdMode()
{
}

void FRTSMapForgeEdMode::Enter()
{
    FEdMode::Enter();
}

void FRTSMapForgeEdMode::Exit()
{
    FEdMode::Exit();
}

void FRTSMapForgeEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
    if (!GEditor || !PDI)
    {
        return;
    }

    URTSMapForgeEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<URTSMapForgeEditorSubsystem>();
    if (!Subsystem || !Subsystem->HasValidGrid())
    {
        return;
    }

    const FRTSGrid& Grid = Subsystem->GetGrid();
    ERTSDebugOverlayMode Mode = Subsystem->GetOverlayMode();

    FRTSDebugRenderer Renderer;
    // Render at slight Z offset above the world origin plane.
    // If you have a Landscape, adjust ZOffset to sit just above it.
    Renderer.RenderOverlay(Grid, Mode, PDI, FMatrix::Identity, /*ZOffset=*/10.0f);
}
