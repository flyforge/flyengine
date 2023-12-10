#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A simple registry that stores name/value pairs of types that are common to store game state
///
class PLASMA_CORE_DLL plStateMap
{
public:
  plStateMap();
  ~plStateMap();

  /// void Load(plStreamReader& stream);
  /// void Save(plStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const plTempHashedString& name, bool value);
  void StoreInteger(const plTempHashedString& name, plInt64 value);
  void StoreDouble(const plTempHashedString& name, double value);
  void StoreVec3(const plTempHashedString& name, const plVec3& value);
  void StoreColor(const plTempHashedString& name, const plColor& value);
  void StoreString(const plTempHashedString& name, const plString& value);

  void RetrieveBool(const plTempHashedString& name, bool& out_Value, bool defaultValue = false);
  void RetrieveInteger(const plTempHashedString& name, plInt64& out_Value, plInt64 defaultValue = 0);
  void RetrieveDouble(const plTempHashedString& name, double& out_Value, double defaultValue = 0);
  void RetrieveVec3(const plTempHashedString& name, plVec3& out_Value, plVec3 defaultValue = plVec3(0));
  void RetrieveColor(const plTempHashedString& name, plColor& out_Value, plColor defaultValue = plColor::White);
  void RetrieveString(const plTempHashedString& name, plString& out_Value, plStringView sDefaultValue = {});

private:
  plHashTable<plTempHashedString, bool> m_Bools;
  plHashTable<plTempHashedString, plInt64> m_Integers;
  plHashTable<plTempHashedString, double> m_Doubles;
  plHashTable<plTempHashedString, plVec3> m_Vec3s;
  plHashTable<plTempHashedString, plColor> m_Colors;
  plHashTable<plTempHashedString, plString> m_Strings;
};
