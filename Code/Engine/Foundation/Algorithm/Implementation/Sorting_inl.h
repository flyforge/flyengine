
template <typename Container, typename Comparer>
void plSorting::QuickSort(Container& inout_container, const Comparer& comparer)
{
  if (inout_container.IsEmpty())
    return;

  QuickSort(inout_container, 0, inout_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void plSorting::QuickSort(plArrayPtr<T>& inout_arrayPtr, const Comparer& comparer)
{
  if (inout_arrayPtr.IsEmpty())
    return;

  QuickSort(inout_arrayPtr, 0, inout_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void plSorting::InsertionSort(Container& inout_container, const Comparer& comparer)
{
  if (inout_container.IsEmpty())
    return;

  InsertionSort(inout_container, 0, inout_container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void plSorting::InsertionSort(plArrayPtr<T>& inout_arrayPtr, const Comparer& comparer)
{
  if (inout_arrayPtr.IsEmpty())
    return;

  InsertionSort(inout_arrayPtr, 0, inout_arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void plSorting::QuickSort(Container& inout_container, plUInt32 uiStartIndex, plUInt32 uiEndIndex, const Comparer& in_comparer)
{
  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(inout_container, uiStartIndex, uiEndIndex, in_comparer);
    }
    else
    {
      const plUInt32 uiPivotIndex = Partition(inout_container, uiStartIndex, uiEndIndex, in_comparer);

      plUInt32 uiFirstHalfEndIndex = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      plUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(in_comparer, inout_container[uiFirstHalfEndIndex], inout_container[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(in_comparer, inout_container[uiPivotIndex], inout_container[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
        QuickSort(inout_container, uiStartIndex, uiFirstHalfEndIndex, in_comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(inout_container, uiSecondHalfStartIndex, uiEndIndex, in_comparer);
    }
  }
}

template <typename Container, typename Comparer>
plUInt32 plSorting::Partition(Container& inout_container, plUInt32 uiLeft, plUInt32 uiRight, const Comparer& comparer)
{
  plUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, inout_container[uiLeft], inout_container[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, inout_container[uiRight], inout_container[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, inout_container[uiLeft], inout_container[uiPivotIndex]))
    {
      // left < pivot < right
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, inout_container[uiLeft], inout_container[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, inout_container[uiRight], inout_container[uiPivotIndex]))
    {
      // right < pivot < left
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  plMath::Swap(inout_container[uiPivotIndex], inout_container[uiRight]); // move pivot to right

  plUInt32 uiIndex = uiLeft;
  for (plUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, inout_container[i], inout_container[uiRight]))
    {
      plMath::Swap(inout_container[i], inout_container[uiIndex]);
      ++uiIndex;
    }
  }

  plMath::Swap(inout_container[uiIndex], inout_container[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename T, typename Comparer>
void plSorting::QuickSort(plArrayPtr<T>& inout_arrayPtr, plUInt32 uiStartIndex, plUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = inout_arrayPtr.GetPtr();

  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(inout_arrayPtr, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      const plUInt32 uiPivotIndex = Partition(ptr, uiStartIndex, uiEndIndex, comparer);

      plUInt32 uiFirstHalfEndIndex = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      plUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(comparer, ptr[uiFirstHalfEndIndex], ptr[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(comparer, ptr[uiPivotIndex], ptr[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
        QuickSort(inout_arrayPtr, uiStartIndex, uiFirstHalfEndIndex, comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(inout_arrayPtr, uiSecondHalfStartIndex, uiEndIndex, comparer);
    }
  }
}

template <typename T, typename Comparer>
plUInt32 plSorting::Partition(T* pPtr, plUInt32 uiLeft, plUInt32 uiRight, const Comparer& comparer)
{
  plUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, pPtr[uiRight], pPtr[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiPivotIndex]))
    {
      // left < pivot < right
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, pPtr[uiLeft], pPtr[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, pPtr[uiRight], pPtr[uiPivotIndex]))
    {
      // right < pivot < left
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  plMath::Swap(pPtr[uiPivotIndex], pPtr[uiRight]); // move pivot to right

  plUInt32 uiIndex = uiLeft;
  for (plUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, pPtr[i], pPtr[uiRight]))
    {
      plMath::Swap(pPtr[i], pPtr[uiIndex]);
      ++uiIndex;
    }
  }

  plMath::Swap(pPtr[uiIndex], pPtr[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename Container, typename Comparer>
void plSorting::InsertionSort(Container& inout_container, plUInt32 uiStartIndex, plUInt32 uiEndIndex, const Comparer& comparer)
{
  for (plUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    plUInt32 uiHoleIndex = i;
    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, inout_container[uiHoleIndex], inout_container[uiHoleIndex - 1]))
    {
      plMath::Swap(inout_container[uiHoleIndex], inout_container[uiHoleIndex - 1]);
      --uiHoleIndex;
    }
  }
}

template <typename T, typename Comparer>
void plSorting::InsertionSort(plArrayPtr<T>& inout_arrayPtr, plUInt32 uiStartIndex, plUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = inout_arrayPtr.GetPtr();

  for (plUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    plUInt32 uiHoleIndex = i;
    T valueToInsert = std::move(ptr[uiHoleIndex]);

    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, valueToInsert, ptr[uiHoleIndex - 1]))
    {
      --uiHoleIndex;
    }

    const plUInt32 uiMoveCount = i - uiHoleIndex;
    if (uiMoveCount > 0)
    {
      plMemoryUtils::RelocateOverlapped(ptr + uiHoleIndex + 1, ptr + uiHoleIndex, uiMoveCount);
      plMemoryUtils::MoveConstruct(ptr + uiHoleIndex, std::move(valueToInsert));
    }
    else
    {
      ptr[uiHoleIndex] = std::move(valueToInsert);
    }
  }
}
