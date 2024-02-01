#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A simple registry that stores name/value pairs of types that are common to store game state
///
class PL_CORE_DLL plStateMap
{
public:
  plStateMap();
  ~plStateMap();

  /// void Load(plStreamReader& stream);
  /// void Save(plStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const plTempHashedString& sName, bool value);
  void StoreInteger(const plTempHashedString& sName, plInt64 value);
  void StoreDouble(const plTempHashedString& sName, double value);
  void StoreVec3(const plTempHashedString& sName, const plVec3& value);
  void StoreColor(const plTempHashedString& sName, const plColor& value);
  void StoreString(const plTempHashedString& sName, const plString& value);

  void RetrieveBool(const plTempHashedString& sName, bool& out_bValue, bool bDefaultValue = false);
  void RetrieveInteger(const plTempHashedString& sName, plInt64& out_iValue, plInt64 iDefaultValue = 0);
  void RetrieveDouble(const plTempHashedString& sName, double& out_fValue, double fDefaultValue = 0);
  void RetrieveVec3(const plTempHashedString& sName, plVec3& out_vValue, plVec3 vDefaultValue = plVec3(0));
  void RetrieveColor(const plTempHashedString& sName, plColor& out_value, plColor defaultValue = plColor::White);
  void RetrieveString(const plTempHashedString& sName, plString& out_sValue, plStringView sDefaultValue = {});

private:
  plHashTable<plTempHashedString, bool> m_Bools;
  plHashTable<plTempHashedString, plInt64> m_Integers;
  plHashTable<plTempHashedString, double> m_Doubles;
  plHashTable<plTempHashedString, plVec3> m_Vec3s;
  plHashTable<plTempHashedString, plColor> m_Colors;
  plHashTable<plTempHashedString, plString> m_Strings;
};
