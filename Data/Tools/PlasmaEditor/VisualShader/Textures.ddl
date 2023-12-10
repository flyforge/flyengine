Node %NormalTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 97, 6, 14 }
  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0;
SamplerState $prop0_AutoSampler;
" }

  Property %Name
  {
    string %Type { "identifier" }
    string %DefaultValue { "NormalTexture" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue {"{ 4dc82890-39e3-4bfc-a97d-86a984d4d3db }" }// Neutral normal map
  }

  InputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %Normal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "normalize(DecodeNormalTexture($prop0.Sample($prop0_AutoSampler, ToFloat2($in0))))" }
    string %Tooltip { "Normal in Tangent Space" }
  }
}

Node %Texture2D
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 97, 6, 14 }
  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0;
SamplerState $prop0_AutoSampler;
" }

  Property %Name
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomTexture" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  InputPin %Sampler
  {
    string %Type { "sampler" }
    unsigned_int8 %Color { 0, 96, 96 }
    string %DefaultValue { "$prop0_AutoSampler" }
    string %Tooltip { "Optional sampler state to use." }
  }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0))" }
  }

  OutputPin %Red
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).w" }
  }
}

Node %Texture3Way
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 97, 6, 14 }
  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0;
SamplerState $prop0_AutoSampler;
" }

  Property %Name
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomTexture" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue {"" }
  }
  
  Property %Tiling
  {
    string %Type { "float" }
    string %DefaultValue { "1" }
  }
  
  InputPin %WorldNormal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %DefaultValue { "G.Input.Normal" }
    string %DefineWhenUsingDefaultValue { "USE_NORMAL" }
  }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "SampleTexture3Way($prop0, $prop0_AutoSampler, $in0, G.Input.WorldPosition, $prop2)" }
  }
}

Node %NormalTexture3Way
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 97, 6, 14 }
  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0;
SamplerState $prop0_AutoSampler;
" }

  Property %Name
  {
    string %Type { "identifier" }
    string %DefaultValue { "NormalTexture" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue {"{ 4dc82890-39e3-4bfc-a97d-86a984d4d3db }" }// Neutral normal map
  }

  Property %Tiling
  {
    string %Type { "float" }
    string %DefaultValue { "1" }
  }

  InputPin %WorldNormal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %DefaultValue { "G.Input.Normal" }
    string %DefineWhenUsingDefaultValue { "USE_NORMAL" }
  }

  OutputPin %Normal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline{"normalize(DecodeNormalTexture(SampleTexture3Way($prop0, $prop0_AutoSampler, $in0, G.Input.WorldPosition, $prop2)))"}
    string %Tooltip { "Normal in Tangent Space" }
  }
}

Node %BlendNormals
{
  string %Category { "Texturing" }
  unsigned_int8 %Color { 216, 86, 0 }

  InputPin %BaseNormal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
  }
  
  InputPin %DetailNormal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
  }

  OutputPin %Normal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "BlendNormals($in0, $in1)" }
    string %Tooltip { "Blended Normal" }
  }
}
