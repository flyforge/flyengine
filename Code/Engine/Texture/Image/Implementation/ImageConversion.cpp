#include <Texture/TexturePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

PL_ENUMERABLE_CLASS_IMPLEMENTATION(plImageConversionStep);

namespace
{
  struct TableEntry
  {
    TableEntry() = default;

    TableEntry(const plImageConversionStep* pStep, const plImageConversionEntry& entry)
    {
      m_step = pStep;
      m_sourceFormat = entry.m_sourceFormat;
      m_targetFormat = entry.m_targetFormat;
      m_numChannels = plMath::Min(plImageFormat::GetNumChannels(entry.m_sourceFormat), plImageFormat::GetNumChannels(entry.m_targetFormat));

      float sourceBpp = plImageFormat::GetExactBitsPerPixel(m_sourceFormat);
      float targetBpp = plImageFormat::GetExactBitsPerPixel(m_targetFormat);

      m_flags = entry.m_flags;

      // Base cost is amount of bits processed
      m_cost = sourceBpp + targetBpp;

      // Penalty for non-inplace conversion
      if ((m_flags & plImageConversionFlags::InPlace) == 0)
      {
        m_cost *= 2;
      }

      // Penalize formats that aren't aligned to powers of two
      if (!plImageFormat::IsCompressed(m_sourceFormat) && !plImageFormat::IsCompressed(m_targetFormat))
      {
        auto sourceBppInt = static_cast<plUInt32>(sourceBpp);
        auto targetBppInt = static_cast<plUInt32>(targetBpp);
        if (!plMath::IsPowerOf2(sourceBppInt) || !plMath::IsPowerOf2(targetBppInt))
        {
          m_cost *= 2;
        }
      }

      m_cost += entry.m_additionalPenalty;
    }

    const plImageConversionStep* m_step = nullptr;
    plImageFormat::Enum m_sourceFormat = plImageFormat::UNKNOWN;
    plImageFormat::Enum m_targetFormat = plImageFormat::UNKNOWN;
    plBitflags<plImageConversionFlags> m_flags;
    float m_cost = plMath::MaxValue<float>();
    plUInt32 m_numChannels = 0;

    static TableEntry chain(const TableEntry& a, const TableEntry& b)
    {
      if (plImageFormat::GetExactBitsPerPixel(a.m_sourceFormat) > plImageFormat::GetExactBitsPerPixel(a.m_targetFormat) &&
          plImageFormat::GetExactBitsPerPixel(b.m_sourceFormat) < plImageFormat::GetExactBitsPerPixel(b.m_targetFormat))
      {
        // Disallow chaining conversions which first reduce to a smaller intermediate and then go back to a larger one, since
        // we end up throwing away information.
        return {};
      }

      TableEntry entry;
      entry.m_step = a.m_step;
      entry.m_cost = a.m_cost + b.m_cost;
      entry.m_sourceFormat = a.m_sourceFormat;
      entry.m_targetFormat = a.m_targetFormat;
      entry.m_flags = a.m_flags;
      entry.m_numChannels = plMath::Min(a.m_numChannels, b.m_numChannels);
      return entry;
    }

    bool operator<(const TableEntry& other) const
    {
      if (m_numChannels > other.m_numChannels)
        return true;

      if (m_numChannels < other.m_numChannels)
        return false;

      return m_cost < other.m_cost;
    }

    bool isAdmissible() const
    {
      if (m_numChannels == 0)
        return false;

      return m_cost < plMath::MaxValue<float>();
    }
  };

  plMutex s_conversionTableLock;
  plHashTable<plUInt32, TableEntry> s_conversionTable;
  bool s_conversionTableValid = false;

  constexpr plUInt32 MakeKey(plImageFormat::Enum a, plImageFormat::Enum b) { return a * plImageFormat::NUM_FORMATS + b; }
  constexpr plUInt32 MakeTypeKey(plImageFormatType::Enum a, plImageFormatType::Enum b) { return (a << 16) + b; }

  struct IntermediateBuffer
  {
    IntermediateBuffer(plUInt32 uiBitsPerBlock)
      : m_bitsPerBlock(uiBitsPerBlock)
    {
    }
    plUInt32 m_bitsPerBlock;
  };

  plUInt32 allocateScratchBufferIndex(plHybridArray<IntermediateBuffer, 16>& ref_scratchBuffers, plUInt32 uiBitsPerBlock, plUInt32 uiExcludedIndex)
  {
    int foundIndex = -1;

    for (plUInt32 bufferIndex = 0; bufferIndex < plUInt32(ref_scratchBuffers.GetCount()); ++bufferIndex)
    {
      if (bufferIndex == uiExcludedIndex)
      {
        continue;
      }

      if (ref_scratchBuffers[bufferIndex].m_bitsPerBlock == uiBitsPerBlock)
      {
        foundIndex = bufferIndex;
        break;
      }
    }

    if (foundIndex >= 0)
    {
      // Reuse existing scratch buffer
      return foundIndex;
    }
    else
    {
      // Allocate new scratch buffer
      ref_scratchBuffers.PushBack(IntermediateBuffer(uiBitsPerBlock));
      return ref_scratchBuffers.GetCount() - 1;
    }
  }
} // namespace

plImageConversionStep::plImageConversionStep()
{
  s_conversionTableValid = false;
}

plImageConversionStep::~plImageConversionStep()
{
  s_conversionTableValid = false;
}

plResult plImageConversion::BuildPath(plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat, bool bSourceEqualsTarget,
  plHybridArray<plImageConversion::ConversionPathNode, 16>& ref_path_out, plUInt32& ref_uiNumScratchBuffers_out)
{
  PL_LOCK(s_conversionTableLock);

  ref_path_out.Clear();
  ref_uiNumScratchBuffers_out = 0;

  if (sourceFormat == targetFormat)
  {
    ConversionPathNode node;
    node.m_sourceFormat = sourceFormat;
    node.m_targetFormat = targetFormat;
    node.m_inPlace = bSourceEqualsTarget;
    node.m_sourceBufferIndex = 0;
    node.m_targetBufferIndex = 0;
    node.m_step = nullptr;
    ref_path_out.PushBack(node);
    return PL_SUCCESS;
  }

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  for (plImageFormat::Enum current = sourceFormat; current != targetFormat;)
  {
    plUInt32 currentTableIndex = MakeKey(current, targetFormat);

    TableEntry entry;

    if (!s_conversionTable.TryGetValue(currentTableIndex, entry))
    {
      return PL_FAILURE;
    }

    plImageConversion::ConversionPathNode step;
    step.m_sourceFormat = entry.m_sourceFormat;
    step.m_targetFormat = entry.m_targetFormat;
    step.m_inPlace = entry.m_flags.IsAnySet(plImageConversionFlags::InPlace);
    step.m_step = entry.m_step;

    current = entry.m_targetFormat;

    ref_path_out.PushBack(step);
  }

  plHybridArray<IntermediateBuffer, 16> scratchBuffers;
  scratchBuffers.PushBack(IntermediateBuffer(plImageFormat::GetBitsPerBlock(targetFormat)));

  for (int i = ref_path_out.GetCount() - 1; i >= 0; --i)
  {
    if (i == ref_path_out.GetCount() - 1)
      ref_path_out[i].m_targetBufferIndex = 0;
    else
      ref_path_out[i].m_targetBufferIndex = ref_path_out[i + 1].m_sourceBufferIndex;

    if (i > 0)
    {
      if (ref_path_out[i].m_inPlace)
      {
        ref_path_out[i].m_sourceBufferIndex = ref_path_out[i].m_targetBufferIndex;
      }
      else
      {
        plUInt32 bitsPerBlock = plImageFormat::GetBitsPerBlock(ref_path_out[i].m_sourceFormat);

        ref_path_out[i].m_sourceBufferIndex = allocateScratchBufferIndex(scratchBuffers, bitsPerBlock, ref_path_out[i].m_targetBufferIndex);
      }
    }
  }

  if (bSourceEqualsTarget)
  {
    // Enforce constraint that source == target
    ref_path_out[0].m_sourceBufferIndex = 0;

    // Did we accidentally break the in-place invariant?
    if (ref_path_out[0].m_sourceBufferIndex == ref_path_out[0].m_targetBufferIndex && !ref_path_out[0].m_inPlace)
    {
      if (ref_path_out.GetCount() == 1)
      {
        // Only a single step, so we need to add a copy step
        plImageConversion::ConversionPathNode copy;
        copy.m_inPlace = false;
        copy.m_sourceFormat = sourceFormat;
        copy.m_targetFormat = sourceFormat;
        copy.m_sourceBufferIndex = ref_path_out[0].m_sourceBufferIndex;
        copy.m_targetBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, plImageFormat::GetBitsPerBlock(ref_path_out[0].m_sourceFormat), ref_path_out[0].m_sourceBufferIndex);
        ref_path_out[0].m_sourceBufferIndex = copy.m_targetBufferIndex;
        copy.m_step = nullptr;
        ref_path_out.Insert(copy, 0);
      }
      else
      {
        // Turn second step to non-inplace
        ref_path_out[1].m_inPlace = false;
        ref_path_out[1].m_sourceBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, plImageFormat::GetBitsPerBlock(ref_path_out[1].m_sourceFormat), ref_path_out[0].m_sourceBufferIndex);
        ref_path_out[0].m_targetBufferIndex = ref_path_out[1].m_sourceBufferIndex;
      }
    }
  }
  else
  {
    ref_path_out[0].m_sourceBufferIndex = scratchBuffers.GetCount();
  }

  ref_uiNumScratchBuffers_out = scratchBuffers.GetCount() - 1;

  return PL_SUCCESS;
}

void plImageConversion::RebuildConversionTable()
{
  PL_LOCK(s_conversionTableLock);

  s_conversionTable.Clear();

  // Prime conversion table with known conversions
  for (plImageConversionStep* conversion = plImageConversionStep::GetFirstInstance(); conversion; conversion = conversion->GetNextInstance())
  {
    plArrayPtr<const plImageConversionEntry> entries = conversion->GetSupportedConversions();

    for (plUInt32 subIndex = 0; subIndex < (plUInt32)entries.GetCount(); subIndex++)
    {
      const plImageConversionEntry& subConversion = entries[subIndex];

      if (subConversion.m_flags.IsAnySet(plImageConversionFlags::InPlace))
      {
        PL_ASSERT_DEV(plImageFormat::IsCompressed(subConversion.m_sourceFormat) == plImageFormat::IsCompressed(subConversion.m_targetFormat) &&
                        plImageFormat::GetBitsPerBlock(subConversion.m_sourceFormat) == plImageFormat::GetBitsPerBlock(subConversion.m_targetFormat),
          "In-place conversions are only allowed between formats of the same number of bits per pixel and compressedness");
      }

      if (plImageFormat::GetType(subConversion.m_sourceFormat) == plImageFormatType::PLANAR)
      {
        PL_ASSERT_DEV(plImageFormat::GetType(subConversion.m_targetFormat) == plImageFormatType::LINEAR, "Conversions from planar formats must target linear formats");
      }
      else if (plImageFormat::GetType(subConversion.m_targetFormat) == plImageFormatType::PLANAR)
      {
        PL_ASSERT_DEV(plImageFormat::GetType(subConversion.m_sourceFormat) == plImageFormatType::LINEAR, "Conversions to planar formats must sourced from linear formats");
      }

      plUInt32 tableIndex = MakeKey(subConversion.m_sourceFormat, subConversion.m_targetFormat);

      // Use the cheapest known conversion for each combination in case there are multiple ones
      TableEntry candidate(conversion, subConversion);

      TableEntry existing;

      if (!s_conversionTable.TryGetValue(tableIndex, existing) || candidate < existing)
      {
        s_conversionTable.Insert(tableIndex, candidate);
      }
    }
  }

  for (plUInt32 i = 0; i < plImageFormat::NUM_FORMATS; i++)
  {
    const plImageFormat::Enum format = static_cast<plImageFormat::Enum>(i);
    // Add copy-conversion (from and to same format)
    s_conversionTable.Insert(
      MakeKey(format, format), TableEntry(nullptr, plImageConversionEntry(plImageConversionEntry(format, format, plImageConversionFlags::InPlace))));
  }

  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  for (plUInt32 k = 1; k < plImageFormat::NUM_FORMATS; k++)
  {
    for (plUInt32 i = 1; i < plImageFormat::NUM_FORMATS; i++)
    {
      if (k == i)
      {
        continue;
      }

      plUInt32 tableIndexIK = MakeKey(static_cast<plImageFormat::Enum>(i), static_cast<plImageFormat::Enum>(k));

      TableEntry entryIK;
      if (!s_conversionTable.TryGetValue(tableIndexIK, entryIK))
      {
        continue;
      }

      for (plUInt32 j = 1; j < plImageFormat::NUM_FORMATS; j++)
      {
        if (j == i || j == k)
        {
          continue;
        }

        plUInt32 tableIndexIJ = MakeKey(static_cast<plImageFormat::Enum>(i), static_cast<plImageFormat::Enum>(j));
        plUInt32 tableIndexKJ = MakeKey(static_cast<plImageFormat::Enum>(k), static_cast<plImageFormat::Enum>(j));

        TableEntry entryKJ;
        if (!s_conversionTable.TryGetValue(tableIndexKJ, entryKJ))
        {
          continue;
        }

        TableEntry candidate = TableEntry::chain(entryIK, entryKJ);

        TableEntry existing;
        if (candidate.isAdmissible() && candidate < s_conversionTable[tableIndexIJ])
        {
          // To Convert from format I to format J, first Convert from I to K
          s_conversionTable[tableIndexIJ] = candidate;
        }
      }
    }
  }

  s_conversionTableValid = true;
}

plResult plImageConversion::Convert(const plImageView& source, plImage& ref_target, plImageFormat::Enum targetFormat)
{
  PL_PROFILE_SCOPE("plImageConversion::Convert");

  plImageFormat::Enum sourceFormat = source.GetImageFormat();

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (&source != &ref_target)
    {
      // copy if not already the same
      ref_target.ResetAndCopy(source);
    }
    return PL_SUCCESS;
  }

  plHybridArray<ConversionPathNode, 16> path;
  plUInt32 numScratchBuffers = 0;
  if (BuildPath(sourceFormat, targetFormat, &source == &ref_target, path, numScratchBuffers).Failed())
  {
    return PL_FAILURE;
  }

  return Convert(source, ref_target, path, numScratchBuffers);
}

plResult plImageConversion::Convert(const plImageView& source, plImage& ref_target, plArrayPtr<ConversionPathNode> path, plUInt32 uiNumScratchBuffers)
{
  PL_ASSERT_DEV(path.GetCount() > 0, "Invalid conversion path");
  PL_ASSERT_DEV(path[0].m_sourceFormat == source.GetImageFormat(), "Invalid conversion path");

  plHybridArray<plImage, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  const plImageView* pSource = &source;

  for (plUInt32 i = 0; i < path.GetCount(); ++i)
  {
    plUInt32 targetIndex = path[i].m_targetBufferIndex;

    plImage* pTarget = targetIndex == 0 ? &ref_target : &intermediates[targetIndex - 1];

    if (ConvertSingleStep(path[i].m_step, *pSource, *pTarget, path[i].m_targetFormat).Failed())
    {
      return PL_FAILURE;
    }

    pSource = pTarget;
  }

  return PL_SUCCESS;
}

plResult plImageConversion::ConvertRaw(
  plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumElements, plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat)
{
  if (uiNumElements == 0)
  {
    return PL_SUCCESS;
  }

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (target.GetPtr() != source.GetPtr())
      memcpy(target.GetPtr(), source.GetPtr(), uiNumElements * plUInt64(plImageFormat::GetBitsPerPixel(sourceFormat)) / 8);
    return PL_SUCCESS;
  }

  if (plImageFormat::IsCompressed(sourceFormat) || plImageFormat::IsCompressed(targetFormat))
  {
    return PL_FAILURE;
  }

  plHybridArray<ConversionPathNode, 16> path;
  plUInt32 numScratchBuffers;
  if (BuildPath(sourceFormat, targetFormat, source.GetPtr() == target.GetPtr(), path, numScratchBuffers).Failed())
  {
    return PL_FAILURE;
  }

  return ConvertRaw(source, target, uiNumElements, path, numScratchBuffers);
}

plResult plImageConversion::ConvertRaw(
  plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumElements, plArrayPtr<ConversionPathNode> path, plUInt32 uiNumScratchBuffers)
{
  PL_ASSERT_DEV(path.GetCount() > 0, "Path of length 0 is invalid.");

  if (uiNumElements == 0)
  {
    return PL_SUCCESS;
  }

  if (plImageFormat::IsCompressed(path.GetPtr()->m_sourceFormat) || plImageFormat::IsCompressed((path.GetEndPtr() - 1)->m_targetFormat))
  {
    return PL_FAILURE;
  }

  plHybridArray<plBlob, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  for (plUInt32 i = 0; i < path.GetCount(); ++i)
  {
    plUInt32 targetIndex = path[i].m_targetBufferIndex;
    plUInt32 targetBpp = plImageFormat::GetBitsPerPixel(path[i].m_targetFormat);

    plByteBlobPtr stepTarget;
    if (targetIndex == 0)
    {
      stepTarget = target;
    }
    else
    {
      plUInt32 expectedSize = static_cast<plUInt32>(targetBpp * uiNumElements / 8);
      intermediates[targetIndex - 1].SetCountUninitialized(expectedSize);
      stepTarget = intermediates[targetIndex - 1].GetByteBlobPtr();
    }

    if (path[i].m_step == nullptr)
    {
      memcpy(stepTarget.GetPtr(), source.GetPtr(), uiNumElements * targetBpp / 8);
    }
    else
    {
      if (static_cast<const plImageConversionStepLinear*>(path[i].m_step)
            ->ConvertPixels(source, stepTarget, uiNumElements, path[i].m_sourceFormat, path[i].m_targetFormat)
            .Failed())
      {
        return PL_FAILURE;
      }
    }

    source = stepTarget;
  }

  return PL_SUCCESS;
}

plResult plImageConversion::ConvertSingleStep(
  const plImageConversionStep* pStep, const plImageView& source, plImage& target, plImageFormat::Enum targetFormat)
{
  if (!pStep)
  {
    target.ResetAndCopy(source);
    return PL_SUCCESS;
  }

  plImageFormat::Enum sourceFormat = source.GetImageFormat();

  plImageHeader header = source.GetHeader();
  header.SetImageFormat(targetFormat);
  target.ResetAndAlloc(header);

  switch (MakeTypeKey(plImageFormat::GetType(sourceFormat), plImageFormat::GetType(targetFormat)))
  {
    case MakeTypeKey(plImageFormatType::LINEAR, plImageFormatType::LINEAR):
    {
      // we have to do the computation in 64-bit otherwise it might overflow for very large textures (8k x 4k or bigger).
      plUInt64 numElements = plUInt64(8) * target.GetByteBlobPtr().GetCount() / (plUInt64)plImageFormat::GetBitsPerPixel(targetFormat);
      return static_cast<const plImageConversionStepLinear*>(pStep)->ConvertPixels(
        source.GetByteBlobPtr(), target.GetByteBlobPtr(), (plUInt32)numElements, sourceFormat, targetFormat);
    }

    case MakeTypeKey(plImageFormatType::LINEAR, plImageFormatType::BLOCK_COMPRESSED):
      return ConvertSingleStepCompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(plImageFormatType::LINEAR, plImageFormatType::PLANAR):
      return ConvertSingleStepPlanarize(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(plImageFormatType::BLOCK_COMPRESSED, plImageFormatType::LINEAR):
      return ConvertSingleStepDecompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(plImageFormatType::PLANAR, plImageFormatType::LINEAR):
      return ConvertSingleStepDeplanarize(source, target, sourceFormat, targetFormat, pStep);

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return PL_FAILURE;
  }
}

plResult plImageConversion::ConvertSingleStepDecompress(
  const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat, const plImageConversionStep* pStep)
{
  for (plUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (plUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (plUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const plUInt32 width = target.GetWidth(mipLevel);
        const plUInt32 height = target.GetHeight(mipLevel);

        const plUInt32 blockSizeX = plImageFormat::GetBlockWidth(sourceFormat);
        const plUInt32 blockSizeY = plImageFormat::GetBlockHeight(sourceFormat);

        const plUInt32 numBlocksX = source.GetNumBlocksX(mipLevel);
        const plUInt32 numBlocksY = source.GetNumBlocksY(mipLevel);

        const plUInt64 targetRowPitch = target.GetRowPitch(mipLevel);
        const plUInt32 targetBytesPerPixel = plImageFormat::GetBitsPerPixel(targetFormat) / 8;

        // Decompress into a temp memory block so we don't have to explicitly handle the case where the image is not a multiple of the block
        // size
        plHybridArray<plUInt8, 256> tempBuffer;
        tempBuffer.SetCount(numBlocksX * blockSizeX * blockSizeY * targetBytesPerPixel);

        for (plUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (plUInt32 blockY = 0; blockY < numBlocksY; blockY++)
          {
            plImageView sourceRowView = source.GetRowView(mipLevel, face, arrayIndex, blockY, slice);

            if (static_cast<const plImageConversionStepDecompressBlocks*>(pStep)
                  ->DecompressBlocks(sourceRowView.GetByteBlobPtr(), plByteBlobPtr(tempBuffer.GetData(), tempBuffer.GetCount()), numBlocksX,
                    sourceFormat, targetFormat)
                  .Failed())
            {
              return PL_FAILURE;
            }

            for (plUInt32 blockX = 0; blockX < numBlocksX; blockX++)
            {
              plUInt8* targetPointer = target.GetPixelPointer<plUInt8>(mipLevel, face, arrayIndex, blockX * blockSizeX, blockY * blockSizeY, slice);

              // Copy into actual target, clamping to image dimensions
              plUInt32 copyWidth = plMath::Min(blockSizeX, width - blockX * blockSizeX);
              plUInt32 copyHeight = plMath::Min(blockSizeY, height - blockY * blockSizeY);
              for (plUInt32 row = 0; row < copyHeight; row++)
              {
                memcpy(targetPointer, &tempBuffer[(blockX * blockSizeX + row) * blockSizeY * targetBytesPerPixel],
                  plMath::SafeMultiply32(copyWidth, targetBytesPerPixel));
                targetPointer += targetRowPitch;
              }
            }
          }
        }
      }
    }
  }

  return PL_SUCCESS;
}

plResult plImageConversion::ConvertSingleStepCompress(
  const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat, const plImageConversionStep* pStep)
{
  for (plUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (plUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (plUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const plUInt32 sourceWidth = source.GetWidth(mipLevel);
        const plUInt32 sourceHeight = source.GetHeight(mipLevel);

        const plUInt32 numBlocksX = target.GetNumBlocksX(mipLevel);
        const plUInt32 numBlocksY = target.GetNumBlocksY(mipLevel);

        const plUInt32 targetWidth = numBlocksX * plImageFormat::GetBlockWidth(targetFormat);
        const plUInt32 targetHeight = numBlocksY * plImageFormat::GetBlockHeight(targetFormat);

        const plUInt64 sourceRowPitch = source.GetRowPitch(mipLevel);
        const plUInt32 sourceBytesPerPixel = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;

        // Pad image to multiple of block size for compression
        plImageHeader paddedSliceHeader;
        paddedSliceHeader.SetWidth(targetWidth);
        paddedSliceHeader.SetHeight(targetHeight);
        paddedSliceHeader.SetImageFormat(sourceFormat);

        plImage paddedSlice;
        paddedSlice.ResetAndAlloc(paddedSliceHeader);

        for (plUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (plUInt32 y = 0; y < targetHeight; ++y)
          {
            plUInt32 sourceY = plMath::Min(y, sourceHeight - 1);

            memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, 0, y), source.GetPixelPointer<void>(mipLevel, face, arrayIndex, 0, sourceY, slice),
              static_cast<size_t>(sourceRowPitch));

            for (plUInt32 x = sourceWidth; x < targetWidth; ++x)
            {
              memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, x, y),
                source.GetPixelPointer<void>(mipLevel, face, arrayIndex, sourceWidth - 1, sourceY, slice), sourceBytesPerPixel);
            }
          }

          plResult result = static_cast<const plImageConversionStepCompressBlocks*>(pStep)->CompressBlocks(paddedSlice.GetByteBlobPtr(),
            target.GetSliceView(mipLevel, face, arrayIndex, slice).GetByteBlobPtr(), numBlocksX, numBlocksY, sourceFormat, targetFormat);

          if (result.Failed())
          {
            return PL_FAILURE;
          }
        }
      }
    }
  }

  return PL_SUCCESS;
}

plResult plImageConversion::ConvertSingleStepDeplanarize(
  const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat, const plImageConversionStep* pStep)
{
  for (plUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (plUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (plUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const plUInt32 width = target.GetWidth(mipLevel);
        const plUInt32 height = target.GetHeight(mipLevel);

        plHybridArray<plImageView, 2> sourcePlanes;
        for (plUInt32 planeIndex = 0; planeIndex < source.GetPlaneCount(); ++planeIndex)
        {
          const plUInt32 blockSizeX = plImageFormat::GetBlockWidth(sourceFormat, planeIndex);
          const plUInt32 blockSizeY = plImageFormat::GetBlockHeight(sourceFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return PL_FAILURE;
          }

          sourcePlanes.PushBack(source.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const plImageConversionStepDeplanarize*>(pStep)
              ->ConvertPixels(sourcePlanes, target.GetSubImageView(mipLevel, face, arrayIndex), width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return PL_FAILURE;
        }
      }
    }
  }

  return PL_SUCCESS;
}

plResult plImageConversion::ConvertSingleStepPlanarize(
  const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat, const plImageConversionStep* pStep)
{
  for (plUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (plUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (plUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const plUInt32 width = target.GetWidth(mipLevel);
        const plUInt32 height = target.GetHeight(mipLevel);

        plHybridArray<plImage, 2> targetPlanes;
        for (plUInt32 planeIndex = 0; planeIndex < target.GetPlaneCount(); ++planeIndex)
        {
          const plUInt32 blockSizeX = plImageFormat::GetBlockWidth(targetFormat, planeIndex);
          const plUInt32 blockSizeY = plImageFormat::GetBlockHeight(targetFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return PL_FAILURE;
          }

          targetPlanes.PushBack(target.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const plImageConversionStepPlanarize*>(pStep)
              ->ConvertPixels(source.GetSubImageView(mipLevel, face, arrayIndex), targetPlanes, width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return PL_FAILURE;
        }
      }
    }
  }

  return PL_SUCCESS;
}

bool plImageConversion::IsConvertible(plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat)
{
  PL_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  plUInt32 tableIndex = MakeKey(sourceFormat, targetFormat);
  return s_conversionTable.Contains(tableIndex);
}

plImageFormat::Enum plImageConversion::FindClosestCompatibleFormat(
  plImageFormat::Enum format, plArrayPtr<const plImageFormat::Enum> compatibleFormats)
{
  PL_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  TableEntry bestEntry;
  plImageFormat::Enum bestFormat = plImageFormat::UNKNOWN;

  for (plUInt32 targetIndex = 0; targetIndex < plUInt32(compatibleFormats.GetCount()); targetIndex++)
  {
    plUInt32 tableIndex = MakeKey(format, compatibleFormats[targetIndex]);
    TableEntry candidate;
    if (s_conversionTable.TryGetValue(tableIndex, candidate) && candidate < bestEntry)
    {
      bestEntry = candidate;
      bestFormat = compatibleFormats[targetIndex];
    }
  }

  return bestFormat;
}


