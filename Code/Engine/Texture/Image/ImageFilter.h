#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Texture/TextureDLL.h>

/// \brief Represents a function used for filtering an image.
class PL_TEXTURE_DLL plImageFilter
{
public:
  /// \brief Samples the filter function at a single point. Note that the distribution isn't necessarily normalized.
  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const = 0;

  /// \brief Returns the width of the filter; outside of the interval [-width, width], the filter function is always zero.
  plSimdFloat GetWidth() const;

protected:
  plImageFilter(float width);

private:
  plSimdFloat m_fWidth;
};

/// \brief Box filter
class PL_TEXTURE_DLL plImageFilterBox : public plImageFilter
{
public:
  plImageFilterBox(float fWidth = 0.5f);

  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const override;
};

/// \brief Triangle filter
class PL_TEXTURE_DLL plImageFilterTriangle : public plImageFilter
{
public:
  plImageFilterTriangle(float fWidth = 1.0f);

  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const override;
};

/// \brief Kaiser-windowed sinc filter
class PL_TEXTURE_DLL plImageFilterSincWithKaiserWindow : public plImageFilter
{
public:
  /// \brief Construct a sinc filter with a Kaiser window of the given window width and beta parameter.
  /// Note that the beta parameter (equaling alpha * pi in the mathematical definition of the Kaiser window) is often incorrectly alpha by other
  /// filtering tools.
  plImageFilterSincWithKaiserWindow(float fWindowWidth = 3.0f, float fBeta = 4.0f);

  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const override;

private:
  plSimdFloat m_fBeta;
  plSimdFloat m_fInvBesselBeta;
};

/// \brief Pre-computes the required filter weights for rescaling a sequence of image samples.
class PL_TEXTURE_DLL plImageFilterWeights
{
public:
  /// \brief Pre-compute the weights for the given filter for scaling between the given number of samples.
  plImageFilterWeights(const plImageFilter& filter, plUInt32 uiSrcSamples, plUInt32 uiDstSamples);

  /// \brief Returns the number of weights.
  plUInt32 GetNumWeights() const;

  /// \brief Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
  plSimdFloat GetWeight(plUInt32 uiDstSampleIndex, plUInt32 uiWeightIndex) const;

  /// \brief Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
  inline plInt32 GetFirstSourceSampleIndex(plUInt32 uiDstSampleIndex) const;

  plArrayPtr<const float> ViewWeights() const;

private:
  plHybridArray<float, 16> m_Weights;
  plSimdFloat m_fWidthInSourceSpace;
  plSimdFloat m_fSourceToDestScale;
  plSimdFloat m_fDestToSourceScale;
  plUInt32 m_uiNumWeights;
  plUInt32 m_uiDstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>
