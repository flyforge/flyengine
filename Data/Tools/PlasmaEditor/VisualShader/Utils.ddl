Node %SceneColor
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  InputPin %ScreenPosition
  {
    unsigned_int8 %Color { 204, 204, 75 }
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
  }

  OutputPin %Color
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "SampleSceneColor(ToFloat2($in0))" }
  }
}

Node %SceneDepth
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  InputPin %ScreenPosition
  {
    unsigned_int8 %Color { 204, 204, 75 }
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
  }

  OutputPin %Depth
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "SampleSceneDepth(ToFloat2($in0))" }
  }
}

Node %ScenePosition
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  InputPin %ScreenPosition
  {
    unsigned_int8 %Color { 204, 204, 75 }
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
  }

  OutputPin %Position
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "SampleScenePosition(ToFloat2($in0))" }
  }
}

Node %DepthFade
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  InputPin %FadeDistance
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }

  OutputPin %Fade
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "DepthFade(G.Input.Position.xyw, ToFloat1($in0))" }
  }
}

Node %Fresnel
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  string %CodePixelBody { "

float VisualShaderFresnel(float3 normal, float f0, float exponent)
{
  float3 normalizedViewVector = normalize(GetCameraPosition() - G.Input.WorldPosition);
  float NdotV = saturate(dot(normalize(normal), normalizedViewVector));
  float f = pow(1 - NdotV, exponent);
  return f + (1 - f) * f0;
}

" }
  
  InputPin %Exponent
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "5.0f" }
  }
  
  InputPin %F0
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.04f" }
  }
  
  InputPin %Normal
  {
    unsigned_int8 %Color { 90, 153, 70 }
    string %Type { "float3" }
    string %DefaultValue { "GetNormal()" }
  }

  OutputPin %Fresnel
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "VisualShaderFresnel(ToFloat3($in2), ToFloat1($in1), ToFloat1($in0))" }
  }
}

Node %Refraction
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  string %CodePixelBody { "

float4 VisualShaderRefraction(float3 worldNormal, float IoR, float thickness, float3 tintColor, float newOpacity)
{
  return CalculateRefraction(G.Input.WorldPosition, worldNormal, IoR, thickness, tintColor, newOpacity);
}

" }

  InputPin %Normal
  {
    unsigned_int8 %Color { 90, 153, 70 }
    string %Type { "float3" }
    string %DefaultValue { "GetNormal()" }
  }
  
  InputPin %IoR
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.3f" }
  }
  
  InputPin %Thickness
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }
  
  InputPin %TintColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }
  
  InputPin %NewOpacity
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }
  
  OutputPin %Refraction
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "VisualShaderRefraction(ToFloat3($in0), ToFloat1($in1), ToFloat1($in2), ToFloat3($in3), ToFloat1($in4))" }
  }
}

Node %Blur
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  string %CodePixelBody { "

float4 VisualShaderBlur(float radius, float strength)
{
  return Blur(G.Input.WorldPosition, radius, strength);
}

" }

  InputPin %Radius
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1f" }
  }

  InputPin %Strength
  {
    unsigned_int8 %Color { 75, 161, 204 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1f" }
  }
  
  OutputPin %Blur
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "VisualShaderBlur(ToFloat1($in0), ToFloat1($in1))" }
  }
}
