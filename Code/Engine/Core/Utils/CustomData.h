#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>


/// \brief A base class for user-defined data assets.
///
/// Allows users to define their own asset types that can be created, edited and referenced in the editor without writing an editor plugin.
///
/// In order to do that, subclass plCustomData,
/// and put the macro PL_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData) into the header next to your custom type.
/// Also put the macro PL_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData) into the implementation file.
///
/// Those will also define resource and resource handle types, such as YourCustomDataResource and YourCustomDataResourceHandle.
///
/// For a full example see SampleCustomData in the SampleGamePlugin.
class PL_CORE_DLL plCustomData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plCustomData, plReflectedClass);

public:
  /// \brief Loads the serialized custom data using a robust serialization-based method.
  ///
  /// This function does not need to be overridden. It will work, even if the properties change.
  /// It is only virtual in case you want to hook into the deserialization process.
  virtual void Load(class plAbstractObjectGraph& ref_graph, class plRttiConverterContext& ref_context, const class plAbstractObjectNode* pRootNode);
};

/// \brief Base class for resources that represent different implementations of plCustomData
///
/// These resources are automatically generated using these macros:
///   PL_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData)
///   PL_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData)
///
/// Put the former into a header next to YourCustomData and the latter into a cpp file.
///
/// This builds these types:
///   YourCustomDataResource
///   YourCustomDataResourceHandle
///
/// You can then use these to reference this resource type for example in components.
/// For a full example search the SampleGamePlugin for SampleCustomDataResource and SampleCustomDataResourceHandle and see how they are used.
class PL_CORE_DLL plCustomDataResourceBase : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plCustomDataResourceBase, plResource);

public:
  plCustomDataResourceBase();
  ~plCustomDataResourceBase();

protected:
  virtual void CreateAndLoadData(plAbstractObjectGraph& ref_graph, plRttiConverterContext& ref_context, const plAbstractObjectNode* pRootNode) = 0;
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  plResourceLoadDesc UpdateContent_Internal(plStreamReader* Stream, const plRTTI& rtti);
};

/// \brief Template resource type for sub-classed plCustomData types.
///
/// See plCustomDataResourceBase for details.
template <typename T>
class plCustomDataResource : public plCustomDataResourceBase
{
public:
  plCustomDataResource();
  ~plCustomDataResource();

  /// \brief Provides read access to the custom data type.
  ///
  /// Returns nullptr, if the resource wasn't loaded successfully.
  const T* GetData() const { return GetLoadingState() == plResourceState::Loaded ? reinterpret_cast<const T*>(m_Data) : nullptr; }

protected:
  virtual void CreateAndLoadData(plAbstractObjectGraph& graph, plRttiConverterContext& context, const plAbstractObjectNode* pRootNode) override;

  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;

  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  struct alignas(PL_ALIGNMENT_OF(T))
  {
    plUInt8 m_Data[sizeof(T)];
  };
};

/// \brief Helper macro to declare a plCustomDataResource<T> and a matching resource handle
///
/// See plCustomDataResourceBase for details.
#define PL_DECLARE_CUSTOM_DATA_RESOURCE(SELF)                              \
  class SELF##Resource : public plCustomDataResource<SELF>                 \
  {                                                                        \
    PL_ADD_DYNAMIC_REFLECTION(SELF##Resource, plCustomDataResource<SELF>); \
    PL_RESOURCE_DECLARE_COMMON_CODE(SELF##Resource);                       \
  };                                                                       \
                                                                           \
  using SELF##ResourceHandle = plTypedResourceHandle<SELF##Resource>

/// \brief Helper macro to define a plCustomDataResource<T>
///
/// See plCustomDataResourceBase for details.
#define PL_DEFINE_CUSTOM_DATA_RESOURCE(SELF)                                                 \
  PL_BEGIN_DYNAMIC_REFLECTED_TYPE(SELF##Resource, 1, plRTTIDefaultAllocator<SELF##Resource>) \
  PL_END_DYNAMIC_REFLECTED_TYPE;                                                             \
                                                                                             \
  PL_RESOURCE_IMPLEMENT_COMMON_CODE(SELF##Resource)


#include <Core/Utils/Implementation/CustomData_inl.h>
