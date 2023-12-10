Node %ConstantColor
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 41, 49, 84 }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "$prop0" }
  }

  Property %Color
  {
    string %Type { "color" }
    string %DefaultValue { "255, 255, 255, 255" }
  }
}

Node %Constant1
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 41, 49, 84 }

  OutputPin %Value
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float" }
    string %DefaultValue { "0" }
  }
}

Node %Constant2
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 41, 49, 84 }

  OutputPin %Value
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 204, 204, 75 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float2" }
    string %DefaultValue { "0, 0" }
  }
}

Node %Constant3
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 41, 49, 84 }

  OutputPin %Value
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 90, 153, 70 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float3" }
    string %DefaultValue { "0, 0, 0" }
  }
}

Node %Constant4
{
  string %Category { "Constants" }
  unsigned_int8 %Color { 41, 49, 84 }

  OutputPin %Value
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 107, 70, 24 }
    string %Inline { "$prop0" }
  }

  Property %Value
  {
    string %Type { "float4" }
    string %DefaultValue { "0, 0, 0, 0" }
  }
}


