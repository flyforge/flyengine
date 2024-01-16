template <typename T>
plCustomDataResource<T>::plCustomDataResource() = default;

template <typename T>
plCustomDataResource<T>::~plCustomDataResource() = default;

template <typename T>
void plCustomDataResource<T>::CreateAndLoadData(plAbstractObjectGraph& ref_graph, plRttiConverterContext& ref_context, const plAbstractObjectNode* pRootNode)
{
  T* pData = reinterpret_cast<T*>(m_Data);

  if (GetLoadingState() == plResourceState::Loaded)
  {
    plMemoryUtils::Destruct(pData);
  }

  plMemoryUtils::Construct(pData);

  if (pRootNode)
  {
    // pRootNode is empty when the resource file is empty
    // no need to attempt to load it then
    pData->Load(ref_graph, ref_context, pRootNode);
  }
}

template <typename T>
plResourceLoadDesc plCustomDataResource<T>::UnloadData(Unload WhatToUnload)
{
  if (GetData() != nullptr)
  {
    plMemoryUtils::Destruct(GetData());
  }

  return plCustomDataResourceBase::UnloadData(WhatToUnload);
}

template <typename T>
plResourceLoadDesc plCustomDataResource<T>::UpdateContent(plStreamReader* Stream)
{
  return UpdateContent_Internal(Stream, *plGetStaticRTTI<T>());
}

template <typename T>
void plCustomDataResource<T>::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plCustomDataResource<T>);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}