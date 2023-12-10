Node %Time
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  OutputPin %Global
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "GlobalTime" }
    string %Tooltip { "Real time. Always at the same speed, unaffected by world simulation speed." }
  }

  OutputPin %World
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "WorldTime" }
    string %Tooltip { "World simulation time. Affected by simulation speed (slow-motion) and world paused state." }
  }
}

Node %UV
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  string %CodeVertexShader { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  string %CodeGeometryShader { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  string %CodePixelDefines { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  OutputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    string %Inline { "G.Input.TexCoord0" }
    string %Tooltip { "The UV 0 texture coordinate." }
  }
}

Node %UV2
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  string %CodeVertexShader { "
#ifndef USE_TEXCOORD1
  #define USE_TEXCOORD1
#endif
" }

  string %CodeGeometryShader { "
#ifndef USE_TEXCOORD1
  #define USE_TEXCOORD1
#endif
" }
  
  string %CodePixelDefines { "
#ifndef USE_TEXCOORD1
  #define USE_TEXCOORD1
#endif
" }

  OutputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    string %Inline { "G.Input.TexCoord1" }
    string %Tooltip { "The UV 0 texture coordinate." }
  }
}

Node %UV_Scroll
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  string %CodeVertexShader { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  string %CodeGeometryShader { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  string %CodePixelDefines { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  InputPin %Speed
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    bool %Expose { true }
    string %DefaultValue { "1, 1" }
  }

  InputPin %Scale
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    bool %Expose { true }
    string %DefaultValue { "1, 1" }
  }

  InputPin %Offset
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    bool %Expose { true }
    string %DefaultValue { "0, 0" }
  }

  OutputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    string %Inline { "(G.Input.TexCoord0 * $in1 + $in2) + frac(WorldTime * $in0)" }
    string %Tooltip { "The scrolled UV 0 texture coordinate." }
  }
}

Node %VertexPosition
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  OutputPin %Position
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "G.Input.Position" }
    string %Tooltip { "The vertex position. For vertex shaders this is the local position, for pixel shaders it is the transformed position." }
  }
}

Node %VertexWorldPosition
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  OutputPin %Position
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "G.Input.WorldPosition" }
    string %Tooltip { "The vertex world space position." }
  }
}

Node %VertexNormal
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  string %CodeVertexShader { "
#ifndef USE_NORMAL
  #define USE_NORMAL
#endif
" }

  string %CodeGeometryShader { "
#ifndef USE_NORMAL
  #define USE_NORMAL
#endif
" }

  string %CodePixelDefines { "
#ifndef USE_NORMAL
  #define USE_NORMAL
#endif
" }

  OutputPin %Normal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "G.Input.Normal" }
    string %Tooltip { "The vertex normal. For vertex shaders this is in local space, for pixel shaders it is in world space." }
  }
}

Node %VertexTangent
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  string %CodeVertexShader { "
#ifndef USE_TANGENT
  #define USE_TANGENT
#endif
" }

  string %CodeGeometryShader { "
#ifndef USE_TANGENT
  #define USE_TANGENT
#endif
" }

  string %CodePixelDefines { "
#ifndef USE_TANGENT
  #define USE_TANGENT
#endif
" }

  OutputPin %Tangent
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "G.Input.Tangent" }
    string %Tooltip { "The vertex tangent. For vertex shaders this is the local tangent, for pixel shaders it is the transformed tangent." }
  }
}

Node %VertexColor
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  string %CodeVertexShader { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
" }

  string %CodeGeometryShader { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
" }
  
  string %CodePixelDefines { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
" }

  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "G.Input.Color0" }
    string %Tooltip { "The vertex color" }
  }
}

Node %VertexColor2
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  string %CodeVertexShader { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
#ifndef USE_COLOR1
  #define USE_COLOR1
#endif
" }

  string %CodeGeometryShader { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
#ifndef USE_COLOR1
  #define USE_COLOR1
#endif
" }
  
  string %CodePixelDefines { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
#ifndef USE_COLOR1
  #define USE_COLOR1
#endif
" }

  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "G.Input.Color1" }
    string %Tooltip { "The second vertex color" }
  }
}

Node %InstanceData
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }
  
  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "GetInstanceData().Color" }
    string %Tooltip { "Per instance color." }
  }
  
}

Node %Camera
{
  string %Category { "Input" }
  unsigned_int8 %Color { 107, 75, 22 }

  OutputPin %Position
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "GetCameraPosition()" }
    string %Tooltip { "Global camera position." }
  }

  OutputPin %Forwards
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "GetCameraDirForwards()" }
    string %Tooltip { "Forward direction vector of the camera." }
  }

  OutputPin %Right
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "GetCameraDirRight()" }
    string %Tooltip { "Right direction vector of the camera." }
  }

  OutputPin %Up
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "GetCameraDirUp()" }
    string %Tooltip { "Up direction vector of the camera." }
  }

  OutputPin %Exposure
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "Exposure" }
    string %Tooltip { "Current exposure value of the camera." }
  }
}

