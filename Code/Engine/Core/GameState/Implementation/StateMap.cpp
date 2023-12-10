#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

plStateMap::plStateMap() {}
plStateMap::~plStateMap() {}


void plStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void plStateMap::StoreBool(const plTempHashedString& name, bool value)
{
  m_Bools[name] = value;
}

void plStateMap::StoreInteger(const plTempHashedString& name, plInt64 value)
{
  m_Integers[name] = value;
}

void plStateMap::StoreDouble(const plTempHashedString& name, double value)
{
  m_Doubles[name] = value;
}

void plStateMap::StoreVec3(const plTempHashedString& name, const plVec3& value)
{
  m_Vec3s[name] = value;
}

void plStateMap::StoreColor(const plTempHashedString& name, const plColor& value)
{
  m_Colors[name] = value;
}

void plStateMap::StoreString(const plTempHashedString& name, const plString& value)
{
  m_Strings[name] = value;
}

void plStateMap::RetrieveBool(const plTempHashedString& name, bool& out_Value, bool defaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void plStateMap::RetrieveInteger(const plTempHashedString& name, plInt64& out_Value, plInt64 defaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void plStateMap::RetrieveDouble(const plTempHashedString& name, double& out_Value, double defaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void plStateMap::RetrieveVec3(const plTempHashedString& name, plVec3& out_Value, plVec3 defaultValue /*= plVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void plStateMap::RetrieveColor(const plTempHashedString& name, plColor& out_Value, plColor defaultValue /*= plColor::White*/)
{
  if (!m_Colors.TryGetValue(name, out_Value))
  {
    out_Value = defaultValue;
  }
}

void plStateMap::RetrieveString(const plTempHashedString& name, plString& out_Value, plStringView sDefaultValue /*= {} */)
{
  if (!m_Strings.TryGetValue(name, out_Value))
  {
    out_Value = sDefaultValue;
  }
}



PLASMA_STATICLINK_FILE(Core, Core_GameState_Implementation_StateMap);
