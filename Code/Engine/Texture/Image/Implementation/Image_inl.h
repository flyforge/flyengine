#pragma once

template <typename T>
struct plImageSizeofHelper
{
  static constexpr size_t Size = sizeof(T);
};

template <>
struct plImageSizeofHelper<void>
{
  static constexpr size_t Size = 1;
};

template <>
struct plImageSizeofHelper<const void>
{
  static constexpr size_t Size = 1;
};

template <typename T>
plBlobPtr<const T> plImageView::GetBlobPtr() const
{
  for (plUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<T>(uiPlaneIndex);
  }
  return plBlobPtr<const T>(reinterpret_cast<T*>(static_cast<plUInt8*>(m_DataPtr.GetPtr())), m_DataPtr.GetCount() / plImageSizeofHelper<T>::Size);
}

inline plConstByteBlobPtr plImageView::GetByteBlobPtr() const
{
  for (plUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<plUInt8>(uiPlaneIndex);
  }
  return plConstByteBlobPtr(static_cast<plUInt8*>(m_DataPtr.GetPtr()), m_DataPtr.GetCount());
}

template <typename T>
plBlobPtr<T> plImage::GetBlobPtr()
{
  plBlobPtr<const T> constPtr = plImageView::GetBlobPtr<T>();

  return plBlobPtr<T>(const_cast<T*>(static_cast<const T*>(constPtr.GetPtr())), constPtr.GetCount());
}

inline plByteBlobPtr plImage::GetByteBlobPtr()
{
  plConstByteBlobPtr constPtr = plImageView::GetByteBlobPtr();

  return plByteBlobPtr(const_cast<plUInt8*>(constPtr.GetPtr()), constPtr.GetCount());
}

template <typename T>
const T* plImageView::GetPixelPointer(plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/, plUInt32 x /*= 0*/,
  plUInt32 y /*= 0*/, plUInt32 z /*= 0*/, plUInt32 uiPlaneIndex /*= 0*/) const
{
  ValidateDataTypeAccessor<T>(uiPlaneIndex);
  PL_ASSERT_DEV(x < GetNumBlocksX(uiMipLevel, uiPlaneIndex), "Invalid x coordinate");
  PL_ASSERT_DEV(y < GetNumBlocksY(uiMipLevel, uiPlaneIndex), "Invalid y coordinate");
  PL_ASSERT_DEV(z < GetNumBlocksZ(uiMipLevel, uiPlaneIndex), "Invalid z coordinate");

  plUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) +
                    z * GetDepthPitch(uiMipLevel, uiPlaneIndex) +
                    y * GetRowPitch(uiMipLevel, uiPlaneIndex) +
                    x * plImageFormat::GetBitsPerBlock(m_Format, uiPlaneIndex) / 8;
  return reinterpret_cast<const T*>(&m_DataPtr[offset]);
}

template <typename T>
T* plImage::GetPixelPointer(
  plUInt32 uiMipLevel /*= 0*/, plUInt32 uiFace /*= 0*/, plUInt32 uiArrayIndex /*= 0*/, plUInt32 x /*= 0*/, plUInt32 y /*= 0*/, plUInt32 z /*= 0*/, plUInt32 uiPlaneIndex /*= 0*/)
{
  return const_cast<T*>(plImageView::GetPixelPointer<T>(uiMipLevel, uiFace, uiArrayIndex, x, y, z, uiPlaneIndex));
}


template <typename T>
void plImageView::ValidateDataTypeAccessor(plUInt32 uiPlaneIndex) const
{
  plUInt32 bytesPerBlock = plImageFormat::GetBitsPerBlock(GetImageFormat(), uiPlaneIndex) / 8;
  PL_IGNORE_UNUSED(bytesPerBlock);
  PL_ASSERT_DEV(bytesPerBlock % plImageSizeofHelper<T>::Size == 0, "Accessor type is not suitable for interpreting contained data");
}
