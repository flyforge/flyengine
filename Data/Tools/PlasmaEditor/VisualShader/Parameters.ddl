Node %Parameter1f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 97, 6, 14 }

  string %CodeMaterialParams { "float $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT1($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float" }
    string %DefaultValue { "0" }
  }

  OutputPin %Value
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "$prop0" }
  }
}

Node %Parameter2f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 97, 6, 14 }

  string %CodeMaterialParams { "float2 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT2($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float2" }
    string %DefaultValue { "0, 0" }
  }

  OutputPin %Value
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    string %Inline { "$prop0" }
  }
}

Node %Parameter3f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 97, 6, 14 }

  string %CodeMaterialParams { "float3 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT3($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float3" }
    string %DefaultValue { "0, 0, 0" }
  }

  OutputPin %Value
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "$prop0" }
  }
}

Node %Parameter4f
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 97, 6, 14 }

  string %CodeMaterialParams { "float4 $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "FLOAT4($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "float4" }
    string %DefaultValue { "0, 0, 0, 0" }
  }

  OutputPin %Value
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "$prop0" }
  }
}

Node %ParameterColor
{
  string %Category { "Parameters" }
  unsigned_int8 %Color { 97, 6, 14 }

  string %CodeMaterialParams { "Color $prop0 @Default($prop1);" }
  string %CodeMaterialCB { "COLOR4F($prop0);" }

  Property %ParamName
  {
    string %Type { "identifier" }
    string %DefaultValue { "Parameter" }
  }

  Property %Default
  {
    string %Type { "color" }
    string %DefaultValue { "255, 255, 255, 255" }
  }

  OutputPin %Value
  {
    string %Type { "color" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "$prop0" }
  }
}
