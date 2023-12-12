#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class plRenderDataBatch
{
private:
  struct SortableRenderData
  {
    PLASMA_DECLARE_POD_TYPE();

    const plRenderData* m_pRenderData;
    plUInt64 m_uiSortingKey;
  };

public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief This function should return true if the given render data should be filtered and not rendered.
  typedef plDelegate<bool(const plRenderData*)> Filter;

  template <typename T>
  class Iterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    void Next();

    bool IsValid() const;

    void operator++();

  private:
    friend class plRenderDataBatch;

    Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd, Filter filter);

    Filter m_Filter;
    const SortableRenderData* m_pCurrent;
    const SortableRenderData* m_pEnd;
  };

  plUInt32 GetCount() const;

  template <typename T>
  const T* GetFirstData() const;

  template <typename T>
  Iterator<T> GetIterator(plUInt32 uiStartIndex = 0, plUInt32 uiCount = plInvalidIndex) const;

private:
  friend class plExtractedRenderData;
  friend class plRenderDataBatchList;

  Filter m_Filter;
  plArrayPtr<SortableRenderData> m_Data;
};

class plRenderDataBatchList
{
public:
  plUInt32 GetBatchCount() const;

  plRenderDataBatch GetBatch(plUInt32 uiIndex) const;

private:
  friend class plExtractedRenderData;

  plRenderDataBatch::Filter m_Filter;
  plArrayPtr<const plRenderDataBatch> m_Batches;
};

#include <RendererCore/Pipeline/Implementation/RenderDataBatch_inl.h>
