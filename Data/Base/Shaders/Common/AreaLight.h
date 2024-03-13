#pragma once

inline float3 GetSpecularDominantDirArea(float3 N, float3 R, float roughness)
{
  float lerpFactor = (1 - roughness);

  return normalize(lerp(N, R, lerpFactor));
}

float IlluminanceSphereOrDisk(float cosTheta, float sinSigmaSqr)
{
  float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
  float illuminance = 0.0f;

  if (cosTheta * cosTheta > sinSigmaSqr)
  {
    illuminance = PI * sinSigmaSqr * saturate(cosTheta);
  }
  else
  {
    float x = sqrt(1.0f / sinSigmaSqr - 1.0f); // For a disk this simplify to x = d / r
    float y = -x * (cosTheta / sinTheta);
    float sinThetaSqrtY = sinTheta * sqrt(1.0f - y * y);
    illuminance = (cosTheta * acos(y) - x * sinThetaSqrtY) * sinSigmaSqr + atan(sinThetaSqrtY / x);
  }

  return max(illuminance, 0.0f);
}

AccumulatedLight SphereLightShading(plPerLightData lightData, plMaterialData matData, float3 L, float3 V, float attenuation)
{
  float3 N = matData.worldNormal;
  float3 lightVector = lightData.position - matData.worldPosition;

  float3 R = 2 * dot(V, N) * N - V;
  R = GetSpecularDominantDirArea(N, R, matData.roughness);

  float3 closestPointOnRay = dot(lightVector, R) * R;
  float3 centerToRay = closestPointOnRay - lightVector;
  float invDistToRay = rsqrt(dot(centerToRay, centerToRay));
  float3 closestPointOnSphere = lightVector + centerToRay * saturate(lightData.width * invDistToRay);

  L = normalize(closestPointOnSphere);

  float3 H = normalize(L + V);

  float NoV = saturate(dot(N, V));
  float NoL = saturate(dot(N, L));
  float NoH = saturate(dot(N, H));

  float VoH = saturate(dot(V, H));
  float LoH = saturate(dot(L, H));

  float sqrDistance = dot(lightData.position - matData.worldPosition, lightVector);

  // float cosTheta = clamp(NoL, -0.999, 0.999); // Clamp to avoid edge case
  //                                                           // We need to prevent the object penetrating into the surface
  //                                                           // and we must avoid divide by 0, thus the 0.9999f
  // float sqrLightRadius = lightData.width * lightData.width;
  // float sinSigmaSqr = min(sqrLightRadius / sqrDistance, 0.9999f);
  // luminance = IlluminanceSphereOrDisk(cosTheta, sinSigmaSqr);

  float3 kS = 1.0f;
  float3 kD = 1.0f;

  float3 specular = 0.0f;
  float3 diffuse  = 0.0f;


  // Specular
#if defined(USE_MATERIAL_SPECULAR_ANISOTROPIC)
  // Specular
  if (matData.anisotropic != 0.0f)
  {
    specular += BRDF_Specular_Anisotropic(matData, H, NoV, NoL, NoH, LoH) * NoL;
  }
  else
#endif
  {
    specular += BRDF_Specular_Isotropic(matData, NoV, NoL, NoH, VoH, LoH) * NoL ;
  }

#if defined(USE_MATERIAL_SPECULAR_CLEARCOAT)
  // Specular clearcoat
  if (matData.clearcoat != 0.0f)
  {
    float cNoH = saturate(dot(matData.clearcoatNormal, H));
    float cNoL = saturate(dot(matData.clearcoatNormal, L));

    specular += BRDF_Specular_Clearcoat(matData, cNoH, VoH) * cNoL;
  }
#endif

#if defined(USE_MATERIAL_SPECULAR_SHEEN)
  // Specular sheen
  if (matData.sheen != 0.0f)
  {
    specular += BRDF_Specular_Sheen(matData, NoV, NoL, NoH) * NoL;
  }
#endif

  specular /= PI;

  // Diffuse
  diffuse  = BRDF_Diffuse(matData, NoV, NoL, VoH, NoH) * kD * NoL;


  return InitializeLight(diffuse, specular * (attenuation) );
}

float3 ClosestPointOnLine(float3 a, float3 b, float3 c)
{
  float3 ab = b - a;
  float t = dot(c - a, ab) / dot(ab, ab);
  return a + t * ab;
}

float3 ClosestPointOnSegment(float3 a, float3 b, float3 c)
{
  float3 ab = b - a;
  float t = dot(c - a, ab) / dot(ab, ab);
  return a + saturate(t) * ab;
}

float RectangleSolidAngle(float3 worldPos,
  float3 p0, float3 p1,
  float3 p2, float3 p3)
{
  float3 v0 = p0 - worldPos;
  float3 v1 = p1 - worldPos;
  float3 v2 = p2 - worldPos;
  float3 v3 = p3 - worldPos;

  float3 n0 = normalize(cross(v0, v1));
  float3 n1 = normalize(cross(v1, v2));
  float3 n2 = normalize(cross(v2, v3));
  float3 n3 = normalize(cross(v3, v0));


  float g0 = acos(dot(-n0, n1));
  float g1 = acos(dot(-n1, n2));
  float g2 = acos(dot(-n2, n3));
  float g3 = acos(dot(-n3, n0));

  return g0 + g1 + g2 + g3 - 2 * PI;
}

AccumulatedLight TubeLightShading(plPerLightData lightData, plMaterialData matData, float3 L, float3 V, float attenuation) {
  float3 N = matData.worldNormal;
  float3 lightVector = lightData.position - matData.worldPosition;

  float3 lightLeft = normalize(RGB10ToFloat3(lightData.upDirection) * 2.0f - 1.0f);
  float lightLength = lightData.length;

  float3 P0 = lightData.position - lightLeft * lightLength * 0.5f;
  float3 P1 = lightData.position + lightLeft * lightLength * 0.5f;


  float3 forward = normalize(ClosestPointOnLine(P0, P1, matData.worldPosition) - matData.worldPosition);
  float3 left = lightLeft;
  float3 up = cross(lightLeft, forward);

  float3 p0 = lightData.position - left * (0.5 * lightLength) + lightData.width * up;
  float3 p1 = lightData.position - left * (0.5 * lightLength) - lightData.width * up;
  float3 p2 = lightData.position + left * (0.5 * lightLength) - lightData.width * up;
  float3 p3 = lightData.position + left * (0.5 * lightLength) + lightData.width * up;


  float solidAngle = RectangleSolidAngle(matData.worldPosition, p0, p1, p2, p3);

  // float3 spherePosition = ClosestPointOnSegment(P0, P1, matData.worldPosition);
  // float3 sphereUnormL = spherePosition - matData.worldPosition;
  // float3 sphereL = normalize(sphereUnormL);
  // float sqrSphereDistance = dot(sphereUnormL, sphereUnormL);
  //
  // luminanceModifier = PI * saturate(dot(sphereL, N)) * ((lightData.width * lightData.width) / sqrSphereDistance);

  float3 R = 2 * dot(V, N) * N - V;
  R = GetSpecularDominantDirArea(N, R, matData.roughness);

  // First, the closest point to the ray on the segment
  float3 L0 = P0 - matData.worldPosition;
  float3 L1 = P1 - matData.worldPosition;
  float3 Ld = L1 - L0;

  float t = dot(R, L0) * dot(R, Ld) - dot(L0, Ld);
  t /= dot(Ld, Ld) - (dot(R, Ld) * dot(R, Ld));

  L = (L0 + saturate(t) * Ld);

  // Then I place a sphere on that point and calculate the lisght vector like for sphere light.
  float3 centerToRay = dot(L, R) * R - L;
  float3 closestPoint = L + centerToRay * saturate(lightData.width / length(centerToRay));
  L = normalize(closestPoint);

  float3 H = normalize(L + V);

  float NoV = saturate(dot(N, V));
  float NoL = saturate(dot(N, L));
  float NoH = saturate(dot(N, H));

  float VoH = saturate(dot(V, H));
  float LoH = saturate(dot(L, H));


  float sqrDistance = dot(lightData.position - matData.worldPosition, lightVector);


  float3 kS = 1.0f;
  float3 kD = 1.0f;

  float3 specular = 0.0f;
  float3 diffuse  = 0.0f;


  // Specular
#if defined(USE_MATERIAL_SPECULAR_ANISOTROPIC)
  // Specular
  if (matData.anisotropic != 0.0f)
  {
    specular += BRDF_Specular_Anisotropic(matData, H, NoV, NoL, NoH, LoH) * NoL;
  }
  else
#endif
  {
    specular += BRDF_Specular_Isotropic(matData, NoV, NoL, NoH, VoH, LoH) * NoL;
  }

#if defined(USE_MATERIAL_SPECULAR_CLEARCOAT)
  // Specular clearcoat
  if (matData.clearcoat != 0.0f)
  {
    float cNoH = saturate(dot(matData.clearcoatNormal, H));
    float cNoL = saturate(dot(matData.clearcoatNormal, L));

    specular += BRDF_Specular_Clearcoat(matData, cNoH, VoH) * cNoL;
  }
#endif

#if defined(USE_MATERIAL_SPECULAR_SHEEN)
  // Specular sheen
  if (matData.sheen != 0.0f)
  {
    specular += BRDF_Specular_Sheen(matData, NoV, NoL, NoH) * NoL;
  }
#endif

  // Diffuse
  diffuse  = BRDF_Diffuse(matData, NoV, NoL, VoH, NoH) * NoL;

  return InitializeLight(diffuse, specular * (attenuation));
}