#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Declarations.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QApplication>
#include <QRect>

QScreen& plWidgetUtils::GetClosestScreen(const QPoint& point)
{
  QScreen* pClosestScreen = QApplication::screenAt(point);
  if (pClosestScreen == nullptr)
  {
    QList<QScreen*> screens = QApplication::screens();
    float fShortestDistance = plMath::Infinity<float>();
    for (QScreen* pScreen : screens)
    {
      const QRect geom = pScreen->geometry();
      plBoundingBox plGeom = plBoundingBox::MakeFromCenterAndHalfExtents(plVec3(geom.center().x(), geom.center().y(), 0), plVec3(geom.width() / 2.0f, geom.height() / 2.0f, 0));
      const plVec3 plPoint(point.x(), point.y(), 0);
      if (plGeom.Contains(plPoint))
      {
        return *pScreen;
      }
      float fDistance = plGeom.GetDistanceSquaredTo(plPoint);
      if (fDistance < fShortestDistance)
      {
        fShortestDistance = fDistance;
        pClosestScreen = pScreen;
      }
    }
    PLASMA_ASSERT_DEV(pClosestScreen != nullptr, "There are no screens connected, UI cannot function.");
  }
  return *pClosestScreen;
}

void plWidgetUtils::AdjustGridDensity(
  double& ref_fFinestDensity, double& ref_fRoughDensity, plUInt32 uiWindowWidth, double fViewportSceneWidth, plUInt32 uiMinPixelsForStep)
{
  const double fMaxStepsFitInWindow = (double)uiWindowWidth / (double)uiMinPixelsForStep;

  const double fStartDensity = ref_fFinestDensity;

  plInt32 iFactor = 1;
  double fNewDensity = ref_fFinestDensity;
  plInt32 iFactors[2] = {5, 2};
  plInt32 iLastFactor = 0;

  while (true)
  {
    const double fStepsAtDensity = fViewportSceneWidth / fNewDensity;

    if (fStepsAtDensity < fMaxStepsFitInWindow)
      break;

    iFactor *= iFactors[iLastFactor];
    fNewDensity = fStartDensity * iFactor;

    iLastFactor = (iLastFactor + 1) % 2;
  }

  ref_fFinestDensity = fStartDensity * iFactor;

  iFactor *= iFactors[iLastFactor];
  ref_fRoughDensity = fStartDensity * iFactor;
}

void plWidgetUtils::ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = plMath::RoundDown((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = plMath::RoundUp((double)viewportSceneRect.right(), fGridStops);
}

void plWidgetUtils::ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY)
{
  out_fMinY = plMath::RoundDown((double)viewportSceneRect.top(), fGridStops);
  out_fMaxY = plMath::RoundUp((double)viewportSceneRect.bottom(), fGridStops);
}
