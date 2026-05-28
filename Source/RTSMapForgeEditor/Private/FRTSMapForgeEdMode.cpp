#include "FRTSMapForgeEdMode.h"
#include "URTSMapForgeEditorSubsystem.h"
#include "Editor.h"

const FEditorModeID FRTSMapForgeEdMode::EM_RTSMapForge = TEXT("EM_RTSMapForge");

void FRTSMapForgeEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
    Super::Render(View, Viewport, PDI);
    if (!GEditor) return;
    URTSMapForgeEditorSubsystem* Sub = GEditor->GetEditorSubsystem<URTSMapForgeEditorSubsystem>();
    if (!Sub || !Sub->HasValidGrid()) return;
    FRTSDebugRenderer Renderer;
    Renderer.RenderOverlay(Sub->GetGrid(), Sub->GetOverlayMode(), PDI, FMatrix::Identity, 10.0f);
}
