#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Streams/DefaultParticleStreams.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream_ZeroInit, 1, plRTTIDefaultAllocator<plParticleStream_ZeroInit>)
PL_END_DYNAMIC_REFLECTED_TYPE;



//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_Position, 1, plRTTIDefaultAllocator<plParticleStreamFactory_Position>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream_Position, 1, plRTTIDefaultAllocator<plParticleStream_Position>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_Position::plParticleStreamFactory_Position()
  : plParticleStreamFactory("Position", plProcessingStream::DataType::Float4, plGetStaticRTTI<plParticleStream_Position>())
{
}

void plParticleStream_Position::Initialize(plParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void plParticleStream_Position::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plProcessingStreamIterator<plVec4> itData(m_pStream, uiNumElements, uiStartIndex);

  const plVec4 defValue = m_pOwner->GetTransform().m_vPosition.GetAsVec4(0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_Size, 1, plRTTIDefaultAllocator<plParticleStreamFactory_Size>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream_Size, 1, plRTTIDefaultAllocator<plParticleStream_Size>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_Size::plParticleStreamFactory_Size()
  : plParticleStreamFactory("Size", plProcessingStream::DataType::Half, plGetStaticRTTI<plParticleStream_Size>())
{
}

void plParticleStream_Size::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plProcessingStreamIterator<plFloat16> itData(m_pStream, uiNumElements, uiStartIndex);

  const float defValue = 1.0f;
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_Color, 1, plRTTIDefaultAllocator<plParticleStreamFactory_Color>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream_Color, 1, plRTTIDefaultAllocator<plParticleStream_Color>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_Color::plParticleStreamFactory_Color()
  : plParticleStreamFactory("Color", plProcessingStream::DataType::Half4, plGetStaticRTTI<plParticleStream_Color>())
{
}

void plParticleStream_Color::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plProcessingStreamIterator<plColorLinear16f> itData(m_pStream, uiNumElements, uiStartIndex);

  const plColorLinear16f defValue(1.0f, 1.0f, 1.0f, 1.0f);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_Velocity, 1, plRTTIDefaultAllocator<plParticleStreamFactory_Velocity>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream_Velocity, 1, plRTTIDefaultAllocator<plParticleStream_Velocity>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_Velocity::plParticleStreamFactory_Velocity()
  : plParticleStreamFactory("Velocity", plProcessingStream::DataType::Float3, plGetStaticRTTI<plParticleStream_Velocity>())
{
}

void plParticleStream_Velocity::Initialize(plParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void plParticleStream_Velocity::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plProcessingStreamIterator<plVec3> itData(m_pStream, uiNumElements, uiStartIndex);

  const plVec3 startVel = m_pOwner->GetParticleStartVelocity();

  while (!itData.HasReachedEnd())
  {
    itData.Current() = startVel;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_LastPosition, 1, plRTTIDefaultAllocator<plParticleStreamFactory_LastPosition>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_LastPosition::plParticleStreamFactory_LastPosition()
  : plParticleStreamFactory("LastPosition", plProcessingStream::DataType::Float3, plGetStaticRTTI<plParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_RotationSpeed, 1, plRTTIDefaultAllocator<plParticleStreamFactory_RotationSpeed>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_RotationSpeed::plParticleStreamFactory_RotationSpeed()
  : plParticleStreamFactory("RotationSpeed", plProcessingStream::DataType::Half, plGetStaticRTTI<plParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_RotationOffset, 1, plRTTIDefaultAllocator<plParticleStreamFactory_RotationOffset>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_RotationOffset::plParticleStreamFactory_RotationOffset()
  : plParticleStreamFactory("RotationOffset", plProcessingStream::DataType::Half, plGetStaticRTTI<plParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_EffectID, 1, plRTTIDefaultAllocator<plParticleStreamFactory_EffectID>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_EffectID::plParticleStreamFactory_EffectID()
  : plParticleStreamFactory("EffectID", plProcessingStream::DataType::Int, plGetStaticRTTI<plParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_OnOff, 1, plRTTIDefaultAllocator<plParticleStreamFactory_OnOff>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_OnOff::plParticleStreamFactory_OnOff()
  : plParticleStreamFactory("OnOff", plProcessingStream::DataType::Int, plGetStaticRTTI<plParticleStream_ZeroInit>())
{
  // TODO: smaller data type
  // TODO: "Byte" type results in memory corruptions
}

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_Axis, 1, plRTTIDefaultAllocator<plParticleStreamFactory_Axis>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream_Axis, 1, plRTTIDefaultAllocator<plParticleStream_Axis>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_Axis::plParticleStreamFactory_Axis()
  : plParticleStreamFactory("Axis", plProcessingStream::DataType::Float3, plGetStaticRTTI<plParticleStream_Axis>())
{
}

void plParticleStream_Axis::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plProcessingStreamIterator<plVec3> itData(m_pStream, uiNumElements, uiStartIndex);

  const plVec3 defValue(1, 0, 0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_TrailData, 1, plRTTIDefaultAllocator<plParticleStreamFactory_TrailData>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_TrailData::plParticleStreamFactory_TrailData()
  : plParticleStreamFactory("TrailData", plProcessingStream::DataType::Short2, plGetStaticRTTI<plParticleStream_ZeroInit>())
{
}


//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////


PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStreamFactory_Variation, 1, plRTTIDefaultAllocator<plParticleStreamFactory_Variation>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleStream_Variation, 1, plRTTIDefaultAllocator<plParticleStream_Variation>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleStreamFactory_Variation::plParticleStreamFactory_Variation()
  : plParticleStreamFactory("Variation", plProcessingStream::DataType::Int, plGetStaticRTTI<plParticleStream_Variation>())
{
}

void plParticleStream_Variation::Initialize(plParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void plParticleStream_Variation::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plProcessingStreamIterator<plUInt32> itData(m_pStream, uiNumElements, uiStartIndex);

  const plVec3 startVel = m_pOwner->GetParticleStartVelocity();

  plRandom& rng = m_pOwner->GetOwnerEffect()->GetRNG();

  while (!itData.HasReachedEnd())
  {
    itData.Current() = rng.UInt();
    itData.Advance();
  }
}



PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_DefaultParticleStreams);
