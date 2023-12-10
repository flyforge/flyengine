#pragma once

/// \file

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Strings/HashedString.h>

class plRTTI;
class plAbstractObjectNode;
class plAbstractObjectGraph;
class plGraphPatch;
class plGraphPatchContext;
class plGraphVersioning;

/// \brief Tuple used for identifying patches and tracking patch progression.
struct plVersionKey
{
  plVersionKey() = default;
  plVersionKey(plStringView sType, plUInt32 uiTypeVersion)
  {
    m_sType.Assign(sType);
    m_uiTypeVersion = uiTypeVersion;
  }
  PLASMA_DECLARE_POD_TYPE();
  plHashedString m_sType;
  plUInt32 m_uiTypeVersion;
};

/// \brief Hash helper class for plVersionKey
struct plGraphVersioningHash
{
  PLASMA_FORCE_INLINE static plUInt32 Hash(const plVersionKey& a)
  {
    auto typeNameHash = a.m_sType.GetHash();
    plUInt32 uiHash = plHashingUtils::xxHash32(&typeNameHash, sizeof(typeNameHash));
    uiHash = plHashingUtils::xxHash32(&a.m_uiTypeVersion, sizeof(a.m_uiTypeVersion), uiHash);
    return uiHash;
  }

  PLASMA_ALWAYS_INLINE static bool Equal(const plVersionKey& a, const plVersionKey& b)
  {
    return a.m_sType == b.m_sType && a.m_uiTypeVersion == b.m_uiTypeVersion;
  }
};

/// \brief A class that overlaps plReflectedTypeDescriptor with the properties needed for patching.
struct PLASMA_FOUNDATION_DLL plTypeVersionInfo
{
  const char* GetTypeName() const;
  void SetTypeName(const char* szName);
  const char* GetParentTypeName() const;
  void SetParentTypeName(const char* szName);

  plHashedString m_sTypeName;
  plHashedString m_sParentTypeName;
  plUInt32 m_uiTypeVersion;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plTypeVersionInfo);

/// \brief Handles the patching of a node. Is passed into the patch
///  classes to provide utility functions and track the node's patching progress.
class PLASMA_FOUNDATION_DLL plGraphPatchContext
{
public:
  /// \brief Ensures that the base class named szType is at version uiTypeVersion.
  ///  If bForcePatch is set, the current version of the base class is reset back to force the execution
  ///  of this patch if necessary. This is mainly necessary for backwards compatibility with patches that
  ///  were written before the type information of all base classes was written to the doc.
  void PatchBaseClass(const char* szType, plUInt32 uiTypeVersion, bool bForcePatch = false); // [tested]

  /// \brief Renames current class type.
  void RenameClass(const char* szTypeName); // [tested]

  /// \brief Renames current class type.
  void RenameClass(const char* szTypeName, plUInt32 uiVersion);

  /// \brief Changes the base class hierarchy to the given one.
  void ChangeBaseClass(plArrayPtr<plVersionKey> baseClasses); // [tested]

private:
  friend class plGraphVersioning;
  plGraphPatchContext(plGraphVersioning* pParent, plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph);
  void Patch(plAbstractObjectNode* pNode);
  void Patch(plUInt32 uiBaseClassIndex, plUInt32 uiTypeVersion, bool bForcePatch);
  void UpdateBaseClasses();

private:
  plGraphVersioning* m_pParent = nullptr;
  plAbstractObjectGraph* m_pGraph = nullptr;
  plAbstractObjectNode* m_pNode = nullptr;
  plDynamicArray<plVersionKey> m_BaseClasses;
  plUInt32 m_uiBaseClassIndex = 0;
  mutable plHashTable<plHashedString, plTypeVersionInfo> m_TypeToInfo;
};

/// \brief Singleton that allows version patching of plAbstractObjectGraph.
///
/// Patching is automatically executed of plAbstractObjectGraph de-serialize functions.
class PLASMA_FOUNDATION_DLL plGraphVersioning
{
  PLASMA_DECLARE_SINGLETON(plGraphVersioning);

public:
  plGraphVersioning();
  ~plGraphVersioning();

  /// \brief Patches all nodes inside pGraph to the current version. pTypesGraph is the graph of serialized
  /// used types in pGraph at the time of saving. If not provided, any base class is assumed to be at max version.
  void PatchGraph(plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph = nullptr);

private:
  friend class plGraphPatchContext;

  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, GraphVersioning);

  void PluginEventHandler(const plPluginEvent& EventData);
  void UpdatePatches();
  plUInt32 GetMaxPatchVersion(const plHashedString& sType) const;

  plHashTable<plHashedString, plUInt32> m_MaxPatchVersion; ///< Max version the given type can be patched to.
  plDynamicArray<const plGraphPatch*> m_GraphPatches;
  plHashTable<plVersionKey, const plGraphPatch*, plGraphVersioningHash> m_NodePatches;
};
