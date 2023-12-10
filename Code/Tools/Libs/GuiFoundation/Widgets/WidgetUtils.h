#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

class QRectF;
class QScreen;

namespace plWidgetUtils
{
  /// \brief Contrary to QApplication::screenAt() this function will always succeed with a valid cursor positions
  /// and also with out of bounds cursor positions.
  PLASMA_GUIFOUNDATION_DLL QScreen& GetClosestScreen(const QPoint& point);

  PLASMA_GUIFOUNDATION_DLL void AdjustGridDensity(
    double& ref_fFinestDensity, double& ref_fRoughDensity, plUInt32 uiWindowWidth, double fViewportSceneWidth, plUInt32 uiMinPixelsForStep);

  PLASMA_GUIFOUNDATION_DLL void ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX);

  PLASMA_GUIFOUNDATION_DLL void ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY);
} // namespace plWidgetUtils
