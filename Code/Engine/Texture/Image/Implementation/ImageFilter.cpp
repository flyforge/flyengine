#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageFilter.h>

plSimdFloat plImageFilter::GetWidth() const
{
  return m_width;
}

plImageFilter::plImageFilter(float width)
  : m_width(width)
{
}

plImageFilterBox::plImageFilterBox(float width)
  : plImageFilter(width)
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

plImageFilterTriangle::plImageFilterTriangle(float width)
  : plImageFilter(width)
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
    return plMath::Sin(plAngle::Radian(x)) / x;
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

plImageFilterSincWithKaiserWindow::plImageFilterSincWithKaiserWindow(float width, float beta)
  : plImageFilter(width)
  , m_beta(beta)
  , m_invBesselBeta(1.0f / modifiedBessel0(m_beta))
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
    return sinc(x * plSimdFloat(plMath::Pi<float>())) * modifiedBessel0(m_beta * xSq.GetSqrt()) * m_invBesselBeta;
  }
}

plImageFilterWeights::plImageFilterWeights(const plImageFilter& filter, plUInt32 srcSamples, plUInt32 dstSamples)
{
  // Filter weights repeat after the common phase
  plUInt32 commonPhase = plMath::GreatestCommonDivisor(srcSamples, dstSamples);

  srcSamples /= commonPhase;
  dstSamples /= commonPhase;

  m_dstSamplesReduced = dstSamples;

  m_sourceToDestScale = float(dstSamples) / float(srcSamples);
  m_destToSourceScale = float(srcSamples) / float(dstSamples);

  plSimdFloat filterScale, invFilterScale;

  if (dstSamples > srcSamples)
  {
    // When upsampling, reconstruct the source by applying the filter in source space and resampling
    filterScale = 1.0f;
    invFilterScale = 1.0f;
  }
  else
  {
    // When downsampling, widen the filter in order to narrow its frequency spectrum, which effectively combines reconstruction + low-pass
    // filter
    filterScale = m_destToSourceScale;
    invFilterScale = m_sourceToDestScale;
  }

  m_widthInSourceSpace = filter.GetWidth() * filterScale;

  m_numWeights = plUInt32(plMath::Ceil(m_widthInSourceSpace * plSimdFloat(2.0f))) + 1;

  m_weights.SetCountUninitialized(dstSamples * m_numWeights);

  for (plUInt32 dstSample = 0; dstSample < dstSamples; ++dstSample)
  {
    plSimdFloat dstSampleInSourceSpace = (plSimdFloat(dstSample) + plSimdFloat(0.5f)) * m_destToSourceScale;

    plInt32 firstSourceIdx = GetFirstSourceSampleIndex(dstSample);

    plSimdFloat totalWeight = 0.0f;

    for (plUInt32 weightIdx = 0; weightIdx < m_numWeights; ++weightIdx)
    {
      plSimdFloat sourceSample = plSimdFloat(firstSourceIdx + plInt32(weightIdx)) + plSimdFloat(0.5f);

      plSimdFloat weight = filter.SamplePoint((dstSampleInSourceSpace - sourceSample) * invFilterScale);
      totalWeight += weight;
      m_weights[dstSample * m_numWeights + weightIdx] = weight;
    }

    // Normalize weights
    plSimdFloat invWeight = 1.0f / totalWeight;

    for (plUInt32 weightIdx = 0; weightIdx < m_numWeights; ++weightIdx)
    {
      m_weights[dstSample * m_numWeights + weightIdx] *= invWeight;
    }
  }
}

plUInt32 plImageFilterWeights::GetNumWeights() const
{
  return m_numWeights;
}

plSimdFloat plImageFilterWeights::GetWeight(plUInt32 dstSampleIndex, plUInt32 weightIndex) const
{
  PLASMA_ASSERT_DEBUG(weightIndex < m_numWeights, "Invalid weight index {} (should be < {})", weightIndex, m_numWeights);

  return plSimdFloat(m_weights[(dstSampleIndex % m_dstSamplesReduced) * m_numWeights + weightIndex]);
}



PLASMA_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFilter);
