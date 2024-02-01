#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

plImageView::plImageView()
{
  Clear();
}

plImageView::plImageView(const plImageHeader& header, plConstByteBlobPtr imageData)
{
  ResetAndViewExternalStorage(header, imageData);
}

void plImageView::Clear()
{
  plImageHeader::Clear();
  m_SubImageOffsets.Clear();
  m_DataPtr.Clear();
}

bool plImageView::IsValid() const
{
  return !m_DataPtr.IsEmpty();
}

void plImageView::ResetAndViewExternalStorage(const plImageHeader& header, plConstByteBlobPtr imageData)
{
  static_cast<plImageHeader&>(*this) = header;

  plUInt64 dataSize = ComputeLayout();

  PL_IGNORE_UNUSED(dataSize);
  PL_ASSERT_DEV(imageData.GetCount() == dataSize, "Provided image storage ({} bytes) doesn't match required data size ({} bytes)",
    imageData.GetCount(), dataSize);

  // Const cast is safe here as we will only perform non-const access if this is an plImage which owns mutable access to the storage
  m_DataPtr = plBlobPtr<plUInt8>(const_cast<plUInt8*>(static_cast<const plUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

plResult plImageView::SaveTo(plStringView sFileName) const
{
  PL_LOG_BLOCK("Writing Image", sFileName);

  if (m_Format == plImageFormat::UNKNOWN)
  {
    plLog::Error("Cannot write image '{0}' - image data is invalid or empty", sFileName);
    return PL_FAILURE;
  }

  plFileWriter writer;
  if (writer.Open(sFileName) == PL_FAILURE)
  {
    plLog::Error("Failed to open image file '{0}'", sFileName);
    return PL_FAILURE;
  }

  plStringView it = plPathUtils::GetFileExtension(sFileName);

  if (plImageFileFormat* pFormat = plImageFileFormat::GetWriterFormat(it.GetStartPointer()))
  {
    if (pFormat->WriteImage(writer, *this, it.GetStartPointer()) != PL_SUCCESS)
    {
      plLog::Error("Failed to write image file '{0}'", sFileName);
      return PL_FAILURE;
    }

    return PL_SUCCESS;
  }

  plLog::Error("No known image file format for extension '{0}'", it);
  return PL_FAILURE;
}

const plImageHeader& plImageView::GetHeader() const
{
  return *this;
}

plImageView plImageView::GetRowView(
  plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/, plUInt32 y /*= 0*/, plUInt32 z /*= 0*/, plUInt32 uiPlaneIndex /*= 0*/) const
{
  plImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the subformat
  plImageFormat::Enum subFormat = plImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * plImageFormat::GetBlockWidth(subFormat) / plImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(plImageFormat::GetBlockHeight(m_Format, 0) * plImageFormat::GetBlockHeight(subFormat) / plImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(plImageFormat::GetBlockDepth(subFormat) / plImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(plImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex));

  plUInt64 offset = 0;

  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  offset += z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  offset += y * GetRowPitch(uiMipLevel, uiPlaneIndex);

  plBlobPtr<const plUInt8> dataSlice = m_DataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel, uiPlaneIndex));
  return plImageView(header, plConstByteBlobPtr(dataSlice.GetPtr(), dataSlice.GetCount()));
}

void plImageView::ReinterpretAs(plImageFormat::Enum format)
{
  PL_ASSERT_DEBUG(
    plImageFormat::IsCompressed(format) == plImageFormat::IsCompressed(GetImageFormat()), "Cannot reinterpret compressed and non-compressed formats");

  PL_ASSERT_DEBUG(plImageFormat::GetBitsPerPixel(GetImageFormat()) == plImageFormat::GetBitsPerPixel(format),
    "Cannot reinterpret between formats of different sizes");

  SetImageFormat(format);
}

plUInt64 plImageView::ComputeLayout()
{
  m_SubImageOffsets.Clear();
  m_SubImageOffsets.Reserve(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices * GetPlaneCount());

  plUInt64 uiDataSize = 0;

  for (plUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for (plUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for (plUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        for (plUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); uiPlaneIndex++)
        {
          m_SubImageOffsets.PushBack(uiDataSize);

          uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * GetDepth(uiMipLevel);
        }
      }
    }
  }

  // Push back total size as a marker
  m_SubImageOffsets.PushBack(uiDataSize);

  return uiDataSize;
}

void plImageView::ValidateSubImageIndices(plUInt32 uiMipLevel, plUInt32 uiFace, plUInt32 uiArrayIndex, plUInt32 uiPlaneIndex) const
{
  PL_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  PL_ASSERT_DEV(uiFace < m_uiNumFaces, "Invalid uiFace");
  PL_ASSERT_DEV(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
  PL_ASSERT_DEV(uiPlaneIndex < GetPlaneCount(), "Invalid plane index");
}

const plUInt64& plImageView::GetSubImageOffset(plUInt32 uiMipLevel, plUInt32 uiFace, plUInt32 uiArrayIndex, plUInt32 uiPlaneIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  return m_SubImageOffsets[uiPlaneIndex + GetPlaneCount() * (uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex))];
}

plImage::plImage()
{
  Clear();
}

plImage::plImage(const plImageHeader& header)
{
  ResetAndAlloc(header);
}

plImage::plImage(const plImageHeader& header, plByteBlobPtr externalData)
{
  ResetAndUseExternalStorage(header, externalData);
}

plImage::plImage(plImage&& other)
{
  ResetAndMove(std::move(other));
}

plImage::plImage(const plImageView& other)
{
  ResetAndCopy(other);
}

void plImage::operator=(plImage&& rhs)
{
  ResetAndMove(std::move(rhs));
}

void plImage::Clear()
{
  m_InternalStorage.Clear();

  plImageView::Clear();
}

void plImage::ResetAndAlloc(const plImageHeader& header)
{
  const plUInt64 requiredSize = header.ComputeDataSize();

  // it is debatable whether this function should reuse external storage, at all
  // however, it is especially dangerous to rely on the external storage being big enough, since many functions just take an plImage as a
  // destination parameter and expect it to behave correctly when any of the Reset functions is called on it; it is not intuitive, that
  // Reset may fail due to how the image was previously reset

  // therefore, if external storage is insufficient, fall back to internal storage

  if (!UsesExternalStorage() || m_DataPtr.GetCount() < requiredSize)
  {
    m_InternalStorage.SetCountUninitialized(requiredSize);
    m_DataPtr = m_InternalStorage.GetBlobPtr<plUInt8>();
  }

  plImageView::ResetAndViewExternalStorage(header, plConstByteBlobPtr(m_DataPtr.GetPtr(), m_DataPtr.GetCount()));
}

void plImage::ResetAndUseExternalStorage(const plImageHeader& header, plByteBlobPtr externalData)
{
  m_InternalStorage.Clear();

  plImageView::ResetAndViewExternalStorage(header, externalData);
}

void plImage::ResetAndMove(plImage&& other)
{
  static_cast<plImageHeader&>(*this) = other.GetHeader();

  if (other.UsesExternalStorage())
  {
    m_InternalStorage.Clear();
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = other.m_DataPtr;
    other.Clear();
  }
  else
  {
    m_InternalStorage = std::move(other.m_InternalStorage);
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = m_InternalStorage.GetBlobPtr<plUInt8>();
    other.Clear();
  }
}

void plImage::ResetAndCopy(const plImageView& other)
{
  ResetAndAlloc(other.GetHeader());

  memcpy(GetBlobPtr<plUInt8>().GetPtr(), other.GetBlobPtr<plUInt8>().GetPtr(), static_cast<size_t>(other.GetBlobPtr<plUInt8>().GetCount()));
}

plResult plImage::LoadFrom(plStringView sFileName)
{
  PL_LOG_BLOCK("Loading Image", sFileName);

  PL_PROFILE_SCOPE(plPathUtils::GetFileNameAndExtension(sFileName).GetStartPointer());

  plFileReader reader;
  if (reader.Open(sFileName) == PL_FAILURE)
  {
    plLog::Warning("Failed to open image file '{0}'", plArgSensitive(sFileName, "File"));
    return PL_FAILURE;
  }

  plStringView it = plPathUtils::GetFileExtension(sFileName);

  if (plImageFileFormat* pFormat = plImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImage(reader, *this, it.GetStartPointer()) != PL_SUCCESS)
    {
      plLog::Warning("Failed to read image file '{0}'", plArgSensitive(sFileName, "File"));
      return PL_FAILURE;
    }

    return PL_SUCCESS;
  }

  plLog::Warning("No known image file format for extension '{0}'", it);

  return PL_FAILURE;
}

plResult plImage::Convert(plImageFormat::Enum targetFormat)
{
  return plImageConversion::Convert(*this, *this, targetFormat);
}

plImageView plImageView::GetSubImageView(plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/) const
{
  plImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);
  header.SetWidth(GetWidth(uiMipLevel));
  header.SetHeight(GetHeight(uiMipLevel));
  header.SetDepth(GetDepth(uiMipLevel));
  header.SetImageFormat(m_Format);

  const plUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, 0);
  plUInt64 size = *(&offset + GetPlaneCount()) - offset;

  plBlobPtr<const plUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return plImageView(header, plConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

plImage plImage::GetSubImageView(plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/)
{
  plImageView constView = plImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  // Create an plImage attached to the view. Const cast is safe here since we own the storage.
  return plImage(
    constView.GetHeader(), plByteBlobPtr(const_cast<plUInt8*>(constView.GetBlobPtr<plUInt8>().GetPtr()), constView.GetBlobPtr<plUInt8>().GetCount()));
}

plImageView plImageView::GetPlaneView(plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/, plUInt32 uiPlaneIndex /*= 0*/) const
{
  plImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  plImageFormat::Enum subFormat = plImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * plImageFormat::GetBlockWidth(subFormat) / plImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * plImageFormat::GetBlockHeight(subFormat) / plImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(GetDepth(uiMipLevel) * plImageFormat::GetBlockDepth(subFormat) / plImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  const plUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  plUInt64 size = *(&offset + 1) - offset;

  plBlobPtr<const plUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return plImageView(header, plConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

plImage plImage::GetPlaneView(plUInt32 uiMipLevel /* = 0 */, plUInt32 uiFace /* = 0 */, plUInt32 uiArrayIndex /* = 0 */, plUInt32 uiPlaneIndex /* = 0 */)
{
  plImageView constView = plImageView::GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);

  // Create an plImage attached to the view. Const cast is safe here since we own the storage.
  return plImage(
    constView.GetHeader(), plByteBlobPtr(const_cast<plUInt8*>(constView.GetBlobPtr<plUInt8>().GetPtr()), constView.GetBlobPtr<plUInt8>().GetCount()));
}

plImage plImage::GetSliceView(plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/, plUInt32 z /*= 0*/, plUInt32 uiPlaneIndex /*= 0*/)
{
  plImageView constView = plImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z, uiPlaneIndex);

  // Create an plImage attached to the view. Const cast is safe here since we own the storage.
  return plImage(
    constView.GetHeader(), plByteBlobPtr(const_cast<plUInt8*>(constView.GetBlobPtr<plUInt8>().GetPtr()), constView.GetBlobPtr<plUInt8>().GetCount()));
}

plImageView plImageView::GetSliceView(plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/, plUInt32 z /*= 0*/, plUInt32 uiPlaneIndex /*= 0*/) const
{
  plImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  plImageFormat::Enum subFormat = plImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * plImageFormat::GetBlockWidth(subFormat) / plImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * plImageFormat::GetBlockHeight(subFormat) / plImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(plImageFormat::GetBlockDepth(subFormat) / plImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  plUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) + z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  plUInt64 size = GetDepthPitch(uiMipLevel, uiPlaneIndex);

  plBlobPtr<const plUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return plImageView(header, plConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

bool plImage::UsesExternalStorage() const
{
  return m_InternalStorage.GetBlobPtr<plUInt8>() != m_DataPtr;
}


