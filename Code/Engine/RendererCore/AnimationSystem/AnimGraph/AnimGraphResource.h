#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

class plAnimGraphInstance;
class plAnimGraphNode;

//////////////////////////////////////////////////////////////////////////

using plAnimGraphResourceHandle = plTypedResourceHandle<class plAnimGraphResource>;

struct PLASMA_RENDERERCORE_DLL plAnimationClipMapping : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimationClipMapping, plReflectedClass);

  plHashedString m_sClipName;
  plAnimationClipResourceHandle m_hClip;

  const char* GetClipName() const { return m_sClipName.GetData(); }
  void SetClipName(const char* szName) { m_sClipName.Assign(szName); }

  const char* GetClip() const;
  void SetClip(const char* szName);
};

class PLASMA_RENDERERCORE_DLL plAnimGraphResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimGraphResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plAnimGraphResource);

public:
  plAnimGraphResource();
  ~plAnimGraphResource();

  const plAnimGraph& GetAnimationGraph() const { return m_AnimGraph; }

  plArrayPtr<const plString> GetIncludeGraphs() const { return m_IncludeGraphs; }
  const plDynamicArray<plAnimationClipMapping>& GetAnimationClipMapping() const { return m_AnimationClipMapping; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plDynamicArray<plString> m_IncludeGraphs;
  plDynamicArray<plAnimationClipMapping> m_AnimationClipMapping;
  plAnimGraph m_AnimGraph;
};
