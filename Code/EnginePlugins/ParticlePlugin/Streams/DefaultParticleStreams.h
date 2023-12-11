#pragma once

#include <ParticlePlugin/Streams/ParticleStream.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStream_ZeroInit final : public plParticleStream
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStream_ZeroInit, plParticleStream);

protected:
  // base class implementation already zero fills the stream data
  // virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_Position final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_Position, plParticleStreamFactory);

public:
  plParticleStreamFactory_Position();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleStream_Position final : public plParticleStream
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStream_Position, plParticleStream);

protected:
  virtual void Initialize(plParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_Size final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_Size, plParticleStreamFactory);

public:
  plParticleStreamFactory_Size();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleStream_Size final : public plParticleStream
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStream_Size, plParticleStream);

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_Color final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_Color, plParticleStreamFactory);

public:
  plParticleStreamFactory_Color();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleStream_Color final : public plParticleStream
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStream_Color, plParticleStream);

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_Velocity final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_Velocity, plParticleStreamFactory);

public:
  plParticleStreamFactory_Velocity();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleStream_Velocity final : public plParticleStream
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStream_Velocity, plParticleStream);

protected:
  virtual void Initialize(plParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// LIFETIME STREAM
//////////////////////////////////////////////////////////////////////////

// always default initialized by the behavior

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_LastPosition final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_LastPosition, plParticleStreamFactory);

public:
  plParticleStreamFactory_LastPosition();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_RotationSpeed final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_RotationSpeed, plParticleStreamFactory);

public:
  plParticleStreamFactory_RotationSpeed();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_RotationOffset final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_RotationOffset, plParticleStreamFactory);

public:
  plParticleStreamFactory_RotationOffset();
};

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_EffectID final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_EffectID, plParticleStreamFactory);

public:
  plParticleStreamFactory_EffectID();
};

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_OnOff final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_OnOff, plParticleStreamFactory);

public:
  plParticleStreamFactory_OnOff();
};

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_Axis final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_Axis, plParticleStreamFactory);

public:
  plParticleStreamFactory_Axis();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleStream_Axis final : public plParticleStream
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStream_Axis, plParticleStream);

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_TrailData final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_TrailData, plParticleStreamFactory);

public:
  plParticleStreamFactory_TrailData();
};

//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamFactory_Variation final : public plParticleStreamFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStreamFactory_Variation, plParticleStreamFactory);

public:
  plParticleStreamFactory_Variation();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleStream_Variation final : public plParticleStream
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleStream_Variation, plParticleStream);

protected:
  virtual void Initialize(plParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plParticleSystemInstance* m_pOwner;
};