#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Texture/TextureDLL.h>

/// \brief Represents a function used for filtering an image.
class PLASMA_TEXTURE_DLL plImageFilter
{
public:
  /// \brief Samples the filter function at a single point. Note that the distribution isn't necessarily normalized.
  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const = 0;

  /// \brief Returns the width of the filter; outside of the interval [-width, width], the filter function is always zero.
  plSimdFloat GetWidth() const;

protected:
  plImageFilter(float width);

private:
  plSimdFloat m_width;
};

/// \brief Box filter
class PLASMA_TEXTURE_DLL plImageFilterBox : public plImageFilter
{
public:
  plImageFilterBox(float width = 0.5f);

  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const override;
};

/// \brief Triangle filter
class PLASMA_TEXTURE_DLL plImageFilterTriangle : public plImageFilter
{
public:
  plImageFilterTriangle(float width = 1.0f);

  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const override;
};

/// \brief Kaiser-windowed sinc filter
class PLASMA_TEXTURE_DLL plImageFilterSincWithKaiserWindow : public plImageFilter
{
public:
  /// \brief Construct a sinc filter with a Kaiser window of the given window width and beta parameter.
  /// Note that the beta parameter (equaling alpha * pi in the mathematical definition of the Kaiser window) is often incorrectly alpha by other
  /// filtering tools.
  plImageFilterSincWithKaiserWindow(float windowWidth = 3.0f, float beta = 4.0f);

  virtual plSimdFloat SamplePoint(const plSimdFloat& x) const override;

private:
  plSimdFloat m_beta;
  plSimdFloat m_invBesselBeta;
};

/// \brief Pre-computes the required filter weights for rescaling a sequence of image samples.
class PLASMA_TEXTURE_DLL plImageFilterWeights
{
public:
  /// \brief Pre-compute the weights for the given filter for scaling between the given number of samples.
  plImageFilterWeights(const plImageFilter& filter, plUInt32 srcSamples, plUInt32 dstSamples);

  /// \brief Returns the number of weights.
  plUInt32 GetNumWeights() const;

  /// \brief Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
  plSimdFloat GetWeight(plUInt32 dstSampleIndex, plUInt32 weightIndex) const;

  /// \brief Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
  inline plInt32 GetFirstSourceSampleIndex(plUInt32 dstSampleIndex) const;

  plArrayPtr<const float> ViewWeights() const;

private:
  plHybridArray<float, 16> m_weights;
  plSimdFloat m_widthInSourceSpace;
  plSimdFloat m_sourceToDestScale;
  plSimdFloat m_destToSourceScale;
  plUInt32 m_numWeights;
  plUInt32 m_dstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>
