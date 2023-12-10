#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/IdTable.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class plOpenXR;


class PLASMA_OPENXRPLUGIN_DLL plOpenXRSpatialAnchors : public plXRSpatialAnchorsInterface
{
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(plOpenXRSpatialAnchors, plXRSpatialAnchorsInterface);

public:
  plOpenXRSpatialAnchors(plOpenXR* pOpenXR);
  ~plOpenXRSpatialAnchors();

  plXRSpatialAnchorID CreateAnchor(const plTransform& globalTransform) override;
  plResult DestroyAnchor(plXRSpatialAnchorID id) override;
  plResult TryGetAnchorTransform(plXRSpatialAnchorID id, plTransform& out_globalTransform) override;

private:
  friend class plOpenXR;
  struct AnchorData
  {
    PLASMA_DECLARE_POD_TYPE();
    XrSpatialAnchorMSFT m_Anchor;
    XrSpace m_Space;
  };

  plOpenXR* m_pOpenXR = nullptr;

  plIdTable<plXRSpatialAnchorID, AnchorData> m_Anchors;
};
