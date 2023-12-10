
Node %Lerp
{
  string %Category { "Math/Interpolation" }
  unsigned_int8 %Color { 37, 59, 51 }

  InputPin %x
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  InputPin %y
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  InputPin %factor
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
    string %Tooltip { "How much to interpolate between x and y. At 0 the output is x, at 1 the output is y." }
  }

  OutputPin %result
  {
    string %Type { "float" }
    unsigned_int8 %Color { 75, 161, 204 }
    string %Inline { "lerp(ToBiggerType($in0, $in1), ToBiggerType($in1, $in0), ToFloat1($in2))" }
    string %Tooltip { "Linear interpolation between x and y according to factor." }
  }
}
