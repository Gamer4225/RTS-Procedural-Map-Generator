#pragma once
#include "Tools/UEdMode.h"
#include "Core/FRTSGrid.h"
#include "Visualization/FRTSDebugRenderer.h"
class RTSMAPFORGEEDITOR_API FRTSMapForgeEdMode : public UEdMode
{
public:
    static const FEditorModeID EM_RTSMapForge;
    virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
};
