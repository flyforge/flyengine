#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>


/// \brief A base class for user-defined data assets.
///
/// Allows users to define their own asset types that can be created, edited and referenced in the editor
/// without writing an editor plugin. In order to do that, subclass plCustomData, put
/// PLASMA_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData) macro in you header and
/// PLASMA_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData) macro in your implementation file.
/// Those will also define resource and resource handle types such as YourCustomDataResource and
/// YourCustomDataResourceHandle.
class PLASMA_CORE_DLL plCustomData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCustomData, plReflectedClass);

public:
  virtual void Load(class plAbstractObjectGraph& ref_graph, class plRttiConverterContext& ref_context, const class plAbstractObjectNode* pRootNode);
};


class PLASMA_CORE_DLL plCustomDataResourceBase : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCustomDataResourceBase, plResource);

public:
  plCustomDataResourceBase();
  ~plCustomDataResourceBase();

protected:
  virtual void CreateAndLoadData(plAbstractObjectGraph& ref_graph, plRttiConverterContext& ref_context, const plAbstractObjectNode* pRootNode) = 0;
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  plResourceLoadDesc UpdateContent_Internal(plStreamReader* Stream, const plRTTI& rtti);
};


template <typename T>
class plCustomDataResource : public plCustomDataResourceBase
{
public:
  plCustomDataResource();
  ~plCustomDataResource();

  const T* GetData() const { return GetLoadingState() == plResourceState::Loaded ? reinterpret_cast<const T*>(m_Data) : nullptr; }

protected:
  virtual void CreateAndLoadData(plAbstractObjectGraph& graph, plRttiConverterContext& context, const plAbstractObjectNode* pRootNode) override;

  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;

  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  struct alignas(PLASMA_ALIGNMENT_OF(T))
  {
    plUInt8 m_Data[sizeof(T)];
  };
};


#define PLASMA_DECLARE_CUSTOM_DATA_RESOURCE(SELF)                              \
  class SELF##Resource : public plCustomDataResource<SELF>                 \
  {                                                                        \
    PLASMA_ADD_DYNAMIC_REFLECTION(SELF##Resource, plCustomDataResource<SELF>); \
    PLASMA_RESOURCE_DECLARE_COMMON_CODE(SELF##Resource);                       \
  };                                                                       \
                                                                           \
  using SELF##ResourceHandle = plTypedResourceHandle<SELF##Resource>

#define PLASMA_DEFINE_CUSTOM_DATA_RESOURCE(SELF)                                                 \
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(SELF##Resource, 1, plRTTIDefaultAllocator<SELF##Resource>) \
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;                                                             \
                                                                                             \
  PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(SELF##Resource)


#include <Core/CustomData/Implementation/CustomData_inl.h>