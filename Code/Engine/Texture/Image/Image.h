#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>

#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/ImageHeader.h>

/// \brief A class referencing image data and holding metadata about the image.
class PLASMA_TEXTURE_DLL plImageView : protected plImageHeader
{
public:
  /// \brief Constructs an empty image view.
  plImageView();

  /// \brief Constructs an image view with the given header and image data.
  plImageView(const plImageHeader& header, plConstByteBlobPtr imageData);

  /// \brief Constructs an empty image view.
  void Clear();

  /// \brief Returns false if the image view does not reference any data yet.
  bool IsValid() const;

  /// \brief Constructs an image view with the given header and image data.
  void ResetAndViewExternalStorage(const plImageHeader& header, plConstByteBlobPtr imageData);

  /// \brief Convenience function to save the image to the given file.
  plResult SaveTo(plStringView sFileName) const;

  /// \brief Returns the header this image was constructed from.
  const plImageHeader& GetHeader() const;

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  plBlobPtr<const T> GetBlobPtr() const;

  plConstByteBlobPtr GetByteBlobPtr() const;

  /// \brief Returns a view to the given sub-image.
  plImageView GetSubImageView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0) const;

  /// \brief Returns a view to a sub-plane.
  plImageView GetPlaneView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to z slice of the image.
  plImageView GetSliceView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 z = 0, plUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to a row of pixels resp. blocks.
  plImageView GetRowView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 y = 0, plUInt32 z = 0, plUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  const T* GetPixelPointer(
    plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 x = 0, plUInt32 y = 0, plUInt32 z = 0, plUInt32 uiPlaneIndex = 0) const;

  /// \brief Reinterprets the image with a given format; the format must have the same size in bits per pixel as the current one.
  void ReinterpretAs(plImageFormat::Enum format);

public:
  using plImageHeader::GetDepth;
  using plImageHeader::GetHeight;
  using plImageHeader::GetWidth;

  using plImageHeader::GetNumArrayIndices;
  using plImageHeader::GetNumFaces;
  using plImageHeader::GetNumMipLevels;
  using plImageHeader::GetPlaneCount;

  using plImageHeader::GetImageFormat;

  using plImageHeader::GetNumBlocksX;
  using plImageHeader::GetNumBlocksY;
  using plImageHeader::GetNumBlocksZ;

  using plImageHeader::GetDepthPitch;
  using plImageHeader::GetRowPitch;

protected:
  plUInt64 ComputeLayout();

  void ValidateSubImageIndices(plUInt32 uiMipLevel, plUInt32 uiFace, plUInt32 uiArrayIndex, plUInt32 uiPlaneIndex) const;
  template <typename T>
  void ValidateDataTypeAccessor(plUInt32 uiPlaneIndex) const;

  const plUInt64& GetSubImageOffset(plUInt32 uiMipLevel, plUInt32 uiFace, plUInt32 uiArrayIndex, plUInt32 uiPlaneIndex) const;

  plHybridArray<plUInt64, 16> m_subImageOffsets;
  plBlobPtr<plUInt8> m_dataPtr;
};

/// \brief A class containing image data and associated meta data.
///
/// This class is a lightweight container for image data and the description required for interpreting the data,
/// such as the image format, its dimensions, number of sub-images (i.e. cubemap faces, mip levels and array sub-images).
/// However, it does not provide any methods for interpreting or  modifying of the image data.
///
/// The sub-images are stored in a predefined order compatible with the layout of DDS files, that is, it first stores
/// the mip chain for each image, then all faces in a case of a cubemap, then the individual images of an image array.
class PLASMA_TEXTURE_DLL plImage : public plImageView
{
  /// Use Reset() instead
  void operator=(const plImage& rhs) = delete;

  /// Use Reset() instead
  void operator=(const plImageView& rhs) = delete;

  /// \brief Constructs an image with the given header; allocating internal storage for it.
  explicit plImage(const plImageHeader& header);

  /// \brief Constructs an image with the given header backed by user-supplied external storage.
  explicit plImage(const plImageHeader& header, plByteBlobPtr externalData);

  /// \brief Constructor from image view (copies the image data to internal storage)
  explicit plImage(const plImageView& other);

public:
  PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Constructs an empty image.
  plImage();

  /// \brief Move constructor
  plImage(plImage&& other);

  void operator=(plImage&& rhs);

  /// \brief Constructs an empty image. If the image is attached to an external storage, the attachment is discarded.
  void Clear();

  /// \brief Constructs an image with the given header and ensures sufficient storage is allocated.
  ///
  /// \note If this plImage was previously attached to external storage, this will reuse that storage.
  /// However, if the external storage is not sufficiently large, ResetAndAlloc() will detach from it and allocate internal storage.
  void ResetAndAlloc(const plImageHeader& header);

  /// \brief Constructs an image with the given header and attaches to the user-supplied external storage.
  ///
  /// The user is responsible to keep the external storage alive as long as this plImage is alive.
  void ResetAndUseExternalStorage(const plImageHeader& header, plByteBlobPtr externalData);

  /// \brief Moves the given data into this object.
  ///
  /// If \a other is attached to an external storage, this object will also be attached to it,
  /// so life-time requirements for the external storage are now bound to this instance.
  void ResetAndMove(plImage&& other);

  /// \brief Constructs from an image view. Copies the image data to internal storage.
  ///
  /// If the image is currently attached to external storage, the attachment is discarded.
  void ResetAndCopy(const plImageView& other);

  /// \brief Convenience function to load the image from the given file.
  plResult LoadFrom(plStringView sFileName);

  /// \brief Convenience function to convert the image to the given format.
  plResult Convert(plImageFormat::Enum targetFormat);

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  plBlobPtr<T> GetBlobPtr();

  plByteBlobPtr GetByteBlobPtr();

  using plImageView::GetBlobPtr;
  using plImageView::GetByteBlobPtr;

  /// \brief Returns a view to the given sub-image.
  plImage GetSubImageView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0);

  using plImageView::GetSubImageView;

  /// \brief Returns a view to a sub-plane.
  plImage GetPlaneView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 uiPlaneIndex = 0);

  using plImageView::GetPlaneView;

  /// \brief Returns a view to z slice of the image.
  plImage GetSliceView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 z = 0, plUInt32 uiPlaneIndex = 0);

  using plImageView::GetSliceView;

  /// \brief Returns a view to a row of pixels resp. blocks.
  plImage GetRowView(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 y = 0, plUInt32 z = 0, plUInt32 uiPlaneIndex = 0);

  using plImageView::GetRowView;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  T* GetPixelPointer(plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0, plUInt32 x = 0, plUInt32 y = 0, plUInt32 z = 0, plUInt32 uiPlaneIndex = 0);

  using plImageView::GetPixelPointer;

private:
  bool UsesExternalStorage() const;

  plBlob m_internalStorage;
};

#include <Texture/Image/Implementation/Image_inl.h>
