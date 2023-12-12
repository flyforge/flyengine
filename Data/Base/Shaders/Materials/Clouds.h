#pragma once

// https://www.guerrilla-games.com/media/News/Files/The-Real-time-Volumetric-Cloudscapes-of-Horizon-Zero-Dawn.pdf
// https://www.guerrilla-games.com/read/nubis-realtime-volumetric-cloudscapes-in-a-nutshell
// https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
// http://www.diva-portal.org/smash/get/diva2:1223894/FULLTEXT01.pdf


#define SINGLE_CLOUD

#define PLANET_RADIUS 6371e3 /* radius of the planet */

#ifdef SINGLE_CLOUD,
#define CLOUD_START 0
#define CLOUD_END 30
#else
#define CLOUD_START 600
#define CLOUD_END 800
#endif


float remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
  return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float HenyeyGreenstein(float3 inLightVector, float3 inViewVector, float inG)
{
  float cos_angle = dot(inLightVector, inViewVector);
  return ((1.0 - inG * inG) / pow((1.0 + inG * inG - 2.0 * inG * cos_angle), 3.0 / 2.0)) / 4.0 * 3.1415;
}

// Noise generation functions (by iq)
float noise1D( float n )
{
  return frac(sin(n)*43758.5453);
}

float GetWeatherData(float2 xy)
{
#ifdef SINGLE_CLOUD
  float grad = length(xy);
  grad = 1.0f - saturate(grad / 10.0f);
  grad = saturate(grad * 1.5f);
  //return grad * 0.4;
  return 0.2;
#else

  float4 noise = NoiseMap.Sample(NoiseMap_AutoSampler, float3(xy.x, xy.y, 0.0f));
  /*float wfbm = noise.x * .625 +
         noise.y * .125 +
           noise.z * .25;

  // cloud shape modeled after the GPU Pro 7 chapter
    float cloud = remap(noise.w, wfbm - 1., 1., 0., 1.);
    cloud = remap(cloud, 1.0f - coverage, 1., 0., 1.); // fake cloud coverage*/

  return remap(noise.x, coverage, 1.0, 0.0, 1.0);
#endif
}

float HeightProfile(float3 p, float start_height, float end_height, float hardness)
{
  float height = p.z - start_height;
  float grad = saturate(height / (end_height - start_height));
  grad = saturate(1.0f - abs(grad - 0.5f) * 2.0f);
  grad = saturate(grad * hardness);

  return grad;
}

float SampleCloudDensity(float3 p, float weatherData)
{
  //return HeightProfile(p, CLOUD_START, CLOUD_END, 8.0f);

  float4 noise = NoiseMap.Sample(NoiseMap_AutoSampler, p * 0.09);

  float low_freq_fbm = (noise.g * 0.625) + (noise.b * 0.25) + (noise.a * 0.125);

  float base_cloud = remap(noise.r, low_freq_fbm - 1.0, 1.0, 0.0, 1.0);

  base_cloud *= HeightProfile(p, CLOUD_START, CLOUD_END, 1.2f);

  float base_cloud_with_coverage = remap(base_cloud, 1.0 - weatherData, 1.0, 0.0, 1.0);
  //base_cloud_with_coverage *= 1.0 - weatherData;
  //float base_cloud_with_coverage = base_cloud * weatherData;

  float3 detailNoise = DetailNoiseMap.Sample(DetailNoiseMap_AutoSampler, p * 0.009).rgb;

  float detailFbm = (detailNoise.r * 0.625) + (detailNoise.g * 0.25) + (detailNoise.b * 0.125);

  //float highFreqMultiplier = lerp(detailFbm, 1.0 - detailFbm, saturate(p.z - CLOUD_START) / (CLOUD_END - CLOUD_START));
  float highFreqMultiplier = 1.0 - detailFbm;

  float final_cloud = remap(base_cloud_with_coverage, highFreqMultiplier * 0.2, 1.0, 0.0, 1.0);

  return max(base_cloud_with_coverage, 0.0);

  //return p.z > CLOUD_START ? 1.0f : 0.0f;
  //return p.z > 1500.0f ? 1.0f : 0.0f;
}

float2 ray_sphere_intersect(
  float3 start, // starting position of the ray
  float3 dir, // the direction of the ray
  float radius // and the sphere radius
) {
  // ray-sphere intersection that assumes
  // the sphere is centered at the origin.
  // No intersection when result.x > result.y
  float a = dot(dir, dir);
  float b = 2.0 * dot(dir, start);
  float c = dot(start, start) - (radius * radius);
  float d = (b*b) - 4.0*a*c;
  if (d < 0.0) return float2(1e5,-1e5);
  return float2(
    (-b - sqrt(d))/(2.0*a),
    (-b + sqrt(d))/(2.0*a)
  );
}