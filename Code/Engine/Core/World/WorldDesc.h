#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/World/CoordinateSystem.h>
#include <Core/World/SpatialSystem.h>

class plTimeStepSmoothing;

/// \brief Describes the initial state of a world.
struct plWorldDesc
{
  PL_DECLARE_POD_TYPE();

  plWorldDesc(plStringView sWorldName) { m_sName.Assign(sWorldName); }

  plHashedString m_sName;
  plUInt64 m_uiRandomNumberGeneratorSeed = 0;

  plUniquePtr<plSpatialSystem> m_pSpatialSystem;
  bool m_bAutoCreateSpatialSystem = true; ///< automatically create a default spatial system if none is set

  plSharedPtr<plCoordinateSystemProvider> m_pCoordinateSystemProvider;
  plUniquePtr<plTimeStepSmoothing> m_pTimeStepSmoothing; ///< if nullptr, plDefaultTimeStepSmoothing will be used

  bool m_bReportErrorWhenStaticObjectMoves = true;

  plTime m_MaxComponentInitializationTimePerFrame = plTime::MakeFromHours(10000); // max time to spend on component initialization per frame
};
