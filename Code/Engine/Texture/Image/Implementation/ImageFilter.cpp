#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageFilter.h>

plSimdFloat plImageFilter::GetWidth() const
{
  return m_fWidth;
}

plImageFilter::plImageFilter(float width)
  : m_fWidth(width)
{
}

plImageFilterBox::plImageFilterBox(float fWidth)
  : plImageFilter(fWidth)
{
}

plSimdFloat plImageFilterBox::SamplePoint(const plSimdFloat& x) const
{
  plSimdFloat absX = x.Abs();

  if (absX <= GetWidth())
  {
    return 1.0f;
  }
  else
  {
    return 0.0f;
  }
}

plImageFilterTriangle::plImageFilterTriangle(float fWidth)
  : plImageFilter(fWidth)
{
}

plSimdFloat plImageFilterTriangle::SamplePoint(const plSimdFloat& x) const
{
  plSimdFloat absX = x.Abs();

  plSimdFloat width = GetWidth();

  if (absX <= width)
  {
    return width - absX;
  }
  else
  {
    return 0.0f;
  }
}

static plSimdFloat sinc(const plSimdFloat& x)
{
  plSimdFloat absX = x.Abs();

  // Use Taylor expansion for small values to avoid division
  if (absX < 0.0001f)
  {
    // sin(x) / x = (x - x^3/6 + x^5/120 - ...) / x = 1 - x^2/6 + x^4/120 - ...
    return plSimdFloat(1.0f) - x * x * plSimdFloat(1.0f / 6.0f);
  }
  else
  {
    return plMath::Sin(plAngle::MakeFromRadian(x)) / x;
  }
}

static plSimdFloat modifiedBessel0(const plSimdFloat& x)
{
  // Implementation as I0(x) = sum((1/4 * x * x) ^ k / (k!)^2, k, 0, inf), see
  // http://mathworld.wolfram.com/ModifiedBesselFunctionoftheFirstKind.html

  plSimdFloat sum = 1.0f;

  plSimdFloat xSquared = x * x * plSimdFloat(0.25f);

  plSimdFloat currentTerm = xSquared;

  for (plUInt32 i = 2; currentTerm > 0.001f; ++i)
  {
    sum += currentTerm;
    currentTerm *= xSquared / plSimdFloat(i * i);
  }

  return sum;
}

plImageFilterSincWithKaiserWindow::plImageFilterSincWithKaiserWindow(float fWidth, float fBeta)
  : plImageFilter(fWidth)
  , m_fBeta(fBeta)
  , m_fInvBesselBeta(1.0f / modifiedBessel0(m_fBeta))
{
}

plSimdFloat plImageFilterSincWithKaiserWindow::SamplePoint(const plSimdFloat& x) const
{
  plSimdFloat scaledX = x / GetWidth();

  plSimdFloat xSq = 1.0f - scaledX * scaledX;

  if (xSq <= 0.0f)
  {
    return 0.0f;
  }
  else
  {
    return sinc(x * plSimdFloat(plMath::Pi<float>())) * modifiedBessel0(m_fBeta * xSq.GetSqrt()) * m_fInvBesselBeta;
  }
}

plImageFilterWeights::plImageFilterWeights(const plImageFilter& filter, plUInt32 uiSrcSamples, plUInt32 uiDstSamples)
{
  // Filter weights repeat after the common phase
  plUInt32 commonPhase = plMath::GreatestCommonDivisor(uiSrcSamples, uiDstSamples);

  uiSrcSamples /= commonPhase;
  uiDstSamples /= commonPhase;

  m_uiDstSamplesReduced = uiDstSamples;

  m_fSourceToDestScale = float(uiDstSamples) / float(uiSrcSamples);
  m_fDestToSourceScale = float(uiSrcSamples) / float(uiDstSamples);

  plSimdFloat filterScale, invFilterScale;

  if (uiDstSamples > uiSrcSamples)
  {
    // When upsampling, reconstruct the source by applying the filter in source space and resampling
    filterScale = 1.0f;
    invFilterScale = 1.0f;
  }
  else
  {
    // When downsampling, widen the filter in order to narrow its frequency spectrum, which effectively combines reconstruction + low-pass
    // filter
    filterScale = m_fDestToSourceScale;
    invFilterScale = m_fSourceToDestScale;
  }

  m_fWidthInSourceSpace = filter.GetWidth() * filterScale;

  m_uiNumWeights = plUInt32(plMath::Ceil(m_fWidthInSourceSpace * plSimdFloat(2.0f))) + 1;

  m_Weights.SetCountUninitialized(uiDstSamples * m_uiNumWeights);

  for (plUInt32 dstSample = 0; dstSample < uiDstSamples; ++dstSample)
  {
    plSimdFloat dstSampleInSourceSpace = (plSimdFloat(dstSample) + plSimdFloat(0.5f)) * m_fDestToSourceScale;

    plInt32 firstSourceIdx = GetFirstSourceSampleIndex(dstSample);

    plSimdFloat totalWeight = 0.0f;

    for (plUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      plSimdFloat sourceSample = plSimdFloat(firstSourceIdx + plInt32(weightIdx)) + plSimdFloat(0.5f);

      plSimdFloat weight = filter.SamplePoint((dstSampleInSourceSpace - sourceSample) * invFilterScale);
      totalWeight += weight;
      m_Weights[dstSample * m_uiNumWeights + weightIdx] = weight;
    }

    // Normalize weights
    plSimdFloat invWeight = 1.0f / totalWeight;

    for (plUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      m_Weights[dstSample * m_uiNumWeights + weightIdx] *= invWeight;
    }
  }
}

plUInt32 plImageFilterWeights::GetNumWeights() const
{
  return m_uiNumWeights;
}

plSimdFloat plImageFilterWeights::GetWeight(plUInt32 uiDstSampleIndex, plUInt32 uiWeightIndex) const
{
  PL_ASSERT_DEBUG(uiWeightIndex < m_uiNumWeights, "Invalid weight index {} (should be < {})", uiWeightIndex, m_uiNumWeights);

  return plSimdFloat(m_Weights[(uiDstSampleIndex % m_uiDstSamplesReduced) * m_uiNumWeights + uiWeightIndex]);
}


