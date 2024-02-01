#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

plStateMap::plStateMap() = default;
plStateMap::~plStateMap() = default;


void plStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void plStateMap::StoreBool(const plTempHashedString& sName, bool value)
{
  m_Bools[sName] = value;
}

void plStateMap::StoreInteger(const plTempHashedString& sName, plInt64 value)
{
  m_Integers[sName] = value;
}

void plStateMap::StoreDouble(const plTempHashedString& sName, double value)
{
  m_Doubles[sName] = value;
}

void plStateMap::StoreVec3(const plTempHashedString& sName, const plVec3& value)
{
  m_Vec3s[sName] = value;
}

void plStateMap::StoreColor(const plTempHashedString& sName, const plColor& value)
{
  m_Colors[sName] = value;
}

void plStateMap::StoreString(const plTempHashedString& sName, const plString& value)
{
  m_Strings[sName] = value;
}

void plStateMap::RetrieveBool(const plTempHashedString& sName, bool& out_bValue, bool bDefaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(sName, out_bValue))
  {
    out_bValue = bDefaultValue;
  }
}

void plStateMap::RetrieveInteger(const plTempHashedString& sName, plInt64& out_iValue, plInt64 iDefaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(sName, out_iValue))
  {
    out_iValue = iDefaultValue;
  }
}

void plStateMap::RetrieveDouble(const plTempHashedString& sName, double& out_fValue, double fDefaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(sName, out_fValue))
  {
    out_fValue = fDefaultValue;
  }
}

void plStateMap::RetrieveVec3(const plTempHashedString& sName, plVec3& out_vValue, plVec3 vDefaultValue /*= plVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(sName, out_vValue))
  {
    out_vValue = vDefaultValue;
  }
}

void plStateMap::RetrieveColor(const plTempHashedString& sName, plColor& out_value, plColor defaultValue /*= plColor::White*/)
{
  if (!m_Colors.TryGetValue(sName, out_value))
  {
    out_value = defaultValue;
  }
}

void plStateMap::RetrieveString(const plTempHashedString& sName, plString& out_sValue, plStringView sDefaultValue /*= {} */)
{
  if (!m_Strings.TryGetValue(sName, out_sValue))
  {
    out_sValue = sDefaultValue;
  }
}


