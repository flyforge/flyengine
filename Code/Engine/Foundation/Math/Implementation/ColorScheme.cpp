#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>

plColor plColorScheme::s_Colors[Count][10] = {
  {
    plColorGammaUB(201, 42, 42),   // oc-red-9
    plColorGammaUB(224, 49, 49),   // oc-red-8
    plColorGammaUB(240, 62, 62),   // oc-red-7
    plColorGammaUB(250, 82, 82),   // oc-red-6
    plColorGammaUB(255, 107, 107), // oc-red-5
    plColorGammaUB(255, 135, 135), // oc-red-4
    plColorGammaUB(255, 168, 168), // oc-red-3
    plColorGammaUB(255, 201, 201), // oc-red-2
    plColorGammaUB(255, 227, 227), // oc-red-1
    plColorGammaUB(255, 245, 245), // oc-red-0
  },
  {
    plColorGammaUB(166, 30, 77),   // oc-pink-9
    plColorGammaUB(194, 37, 92),   // oc-pink-8
    plColorGammaUB(214, 51, 108),  // oc-pink-7
    plColorGammaUB(230, 73, 128),  // oc-pink-6
    plColorGammaUB(240, 101, 149), // oc-pink-5
    plColorGammaUB(247, 131, 172), // oc-pink-4
    plColorGammaUB(250, 162, 193), // oc-pink-3
    plColorGammaUB(252, 194, 215), // oc-pink-2
    plColorGammaUB(255, 222, 235), // oc-pink-1
    plColorGammaUB(255, 240, 246), // oc-pink-0
  },
  {
    plColorGammaUB(134, 46, 156),  // oc-grape-9
    plColorGammaUB(156, 54, 181),  // oc-grape-8
    plColorGammaUB(174, 62, 201),  // oc-grape-7
    plColorGammaUB(190, 75, 219),  // oc-grape-6
    plColorGammaUB(204, 93, 232),  // oc-grape-5
    plColorGammaUB(218, 119, 242), // oc-grape-4
    plColorGammaUB(229, 153, 247), // oc-grape-3
    plColorGammaUB(238, 190, 250), // oc-grape-2
    plColorGammaUB(243, 217, 250), // oc-grape-1
    plColorGammaUB(248, 240, 252), // oc-grape-0
  },
  {
    plColorGammaUB(95, 61, 196),   // oc-violet-9
    plColorGammaUB(103, 65, 217),  // oc-violet-8
    plColorGammaUB(112, 72, 232),  // oc-violet-7
    plColorGammaUB(121, 80, 242),  // oc-violet-6
    plColorGammaUB(132, 94, 247),  // oc-violet-5
    plColorGammaUB(151, 117, 250), // oc-violet-4
    plColorGammaUB(177, 151, 252), // oc-violet-3
    plColorGammaUB(208, 191, 255), // oc-violet-2
    plColorGammaUB(229, 219, 255), // oc-violet-1
    plColorGammaUB(243, 240, 255), // oc-violet-0
  },
  {
    plColorGammaUB(54, 79, 199),   // oc-indigo-9
    plColorGammaUB(59, 91, 219),   // oc-indigo-8
    plColorGammaUB(66, 99, 235),   // oc-indigo-7
    plColorGammaUB(76, 110, 245),  // oc-indigo-6
    plColorGammaUB(92, 124, 250),  // oc-indigo-5
    plColorGammaUB(116, 143, 252), // oc-indigo-4
    plColorGammaUB(145, 167, 255), // oc-indigo-3
    plColorGammaUB(186, 200, 255), // oc-indigo-2
    plColorGammaUB(219, 228, 255), // oc-indigo-1
    plColorGammaUB(237, 242, 255), // oc-indigo-0
  },
  {
    plColorGammaUB(24, 100, 171),  // oc-blue-9
    plColorGammaUB(25, 113, 194),  // oc-blue-8
    plColorGammaUB(28, 126, 214),  // oc-blue-7
    plColorGammaUB(34, 139, 230),  // oc-blue-6
    plColorGammaUB(51, 154, 240),  // oc-blue-5
    plColorGammaUB(77, 171, 247),  // oc-blue-4
    plColorGammaUB(116, 192, 252), // oc-blue-3
    plColorGammaUB(165, 216, 255), // oc-blue-2
    plColorGammaUB(208, 235, 255), // oc-blue-1
    plColorGammaUB(231, 245, 255), // oc-blue-0
  },
  {
    plColorGammaUB(11, 114, 133),  // oc-cyan-9
    plColorGammaUB(12, 133, 153),  // oc-cyan-8
    plColorGammaUB(16, 152, 173),  // oc-cyan-7
    plColorGammaUB(21, 170, 191),  // oc-cyan-6
    plColorGammaUB(34, 184, 207),  // oc-cyan-5
    plColorGammaUB(59, 201, 219),  // oc-cyan-4
    plColorGammaUB(102, 217, 232), // oc-cyan-3
    plColorGammaUB(153, 233, 242), // oc-cyan-2
    plColorGammaUB(197, 246, 250), // oc-cyan-1
    plColorGammaUB(227, 250, 252), // oc-cyan-0
  },
  {
    plColorGammaUB(8, 127, 91),    // oc-teal-9
    plColorGammaUB(9, 146, 104),   // oc-teal-8
    plColorGammaUB(12, 166, 120),  // oc-teal-7
    plColorGammaUB(18, 184, 134),  // oc-teal-6
    plColorGammaUB(32, 201, 151),  // oc-teal-5
    plColorGammaUB(56, 217, 169),  // oc-teal-4
    plColorGammaUB(99, 230, 190),  // oc-teal-3
    plColorGammaUB(150, 242, 215), // oc-teal-2
    plColorGammaUB(195, 250, 232), // oc-teal-1
    plColorGammaUB(230, 252, 245), // oc-teal-0
  },
  {
    plColorGammaUB(43, 138, 62),   // oc-green-9
    plColorGammaUB(47, 158, 68),   // oc-green-8
    plColorGammaUB(55, 178, 77),   // oc-green-7
    plColorGammaUB(64, 192, 87),   // oc-green-6
    plColorGammaUB(81, 207, 102),  // oc-green-5
    plColorGammaUB(105, 219, 124), // oc-green-4
    plColorGammaUB(140, 233, 154), // oc-green-3
    plColorGammaUB(178, 242, 187), // oc-green-2
    plColorGammaUB(211, 249, 216), // oc-green-1
    plColorGammaUB(235, 251, 238), // oc-green-0
  },
  {
    plColorGammaUB(92, 148, 13),   // oc-lime-9
    plColorGammaUB(102, 168, 15),  // oc-lime-8
    plColorGammaUB(116, 184, 22),  // oc-lime-7
    plColorGammaUB(130, 201, 30),  // oc-lime-6
    plColorGammaUB(148, 216, 45),  // oc-lime-5
    plColorGammaUB(169, 227, 75),  // oc-lime-4
    plColorGammaUB(192, 235, 117), // oc-lime-3
    plColorGammaUB(216, 245, 162), // oc-lime-2
    plColorGammaUB(233, 250, 200), // oc-lime-1
    plColorGammaUB(244, 252, 227), // oc-lime-0
  },
  {
    plColorGammaUB(230, 119, 0),   // oc-yellow-9
    plColorGammaUB(240, 140, 0),   // oc-yellow-8
    plColorGammaUB(245, 159, 0),   // oc-yellow-7
    plColorGammaUB(250, 176, 5),   // oc-yellow-6
    plColorGammaUB(252, 196, 25),  // oc-yellow-5
    plColorGammaUB(255, 212, 59),  // oc-yellow-4
    plColorGammaUB(255, 224, 102), // oc-yellow-3
    plColorGammaUB(255, 236, 153), // oc-yellow-2
    plColorGammaUB(255, 243, 191), // oc-yellow-1
    plColorGammaUB(255, 249, 219), // oc-yellow-0
  },
  {
    plColorGammaUB(217, 72, 15),   // oc-orange-9
    plColorGammaUB(232, 89, 12),   // oc-orange-8
    plColorGammaUB(247, 103, 7),   // oc-orange-7
    plColorGammaUB(253, 126, 20),  // oc-orange-6
    plColorGammaUB(255, 146, 43),  // oc-orange-5
    plColorGammaUB(255, 169, 77),  // oc-orange-4
    plColorGammaUB(255, 192, 120), // oc-orange-3
    plColorGammaUB(255, 216, 168), // oc-orange-2
    plColorGammaUB(255, 232, 204), // oc-orange-1
    plColorGammaUB(255, 244, 230), // oc-orange-0
  },
  {
    plColorGammaUB(33, 37, 41),    // oc-gray-9
    plColorGammaUB(52, 58, 64),    // oc-gray-8
    plColorGammaUB(73, 80, 87),    // oc-gray-7
    plColorGammaUB(134, 142, 150), // oc-gray-6
    plColorGammaUB(173, 181, 189), // oc-gray-5
    plColorGammaUB(206, 212, 218), // oc-gray-4
    plColorGammaUB(222, 226, 230), // oc-gray-3
    plColorGammaUB(233, 236, 239), // oc-gray-2
    plColorGammaUB(241, 243, 245), // oc-gray-1
    plColorGammaUB(248, 249, 250), // oc-gray-0
  },
  {
    plColorGammaUB(0, 82, 65),      // oc-plasma-9
    plColorGammaUB(0, 91, 73),      // oc-plasma-8
    plColorGammaUB(0, 122, 97),     // oc-plasma-7
    plColorGammaUB(0, 122, 97),     // oc-plasma-6
    plColorGammaUB(0, 152, 122),     // oc-plasma-5
    plColorGammaUB(0, 152, 122),    // oc-plasma-4
    plColorGammaUB(1, 181, 145),    // oc-plasma-3
    plColorGammaUB(11, 222, 179),   // oc-plasma-2
    plColorGammaUB(6, 250, 201),    // oc-plasma-1
    plColorGammaUB(75, 251, 216),   // oc-plasma-0
  },
  {
    plColorGammaUB(0, 0, 0),    // oc-plasma-9
    plColorGammaUB(2, 2, 2),    // oc-plasma-8
    plColorGammaUB(5, 5, 5),   // oc-plasma-7
    plColorGammaUB(10, 10, 10),   // oc-plasma-6
    plColorGammaUB(12, 12, 12),  // oc-plasma-5
    plColorGammaUB(15, 15, 15),  // oc-plasma-4
    plColorGammaUB(20, 20, 20),  // oc-plasma-3
    plColorGammaUB(25, 25, 25), // oc-plasma-2
    plColorGammaUB(30, 30, 30),  // oc-plasma-1
    plColorGammaUB(40, 40, 40), // oc-plasma-0
  }};

// We could use a lower brightness here for our dark UI but the colors looks much nicer at higher brightness so we just apply a scale factor instead.
static constexpr plUInt8 DarkUIBrightness = 3;
static constexpr plUInt8 DarkUIHighBrightness = 7;
static constexpr plUInt8 DarkUIGrayBrightness = 4; // gray is too dark at UIBrightness
static constexpr float DarkUISaturation = 0.95f;
static constexpr plColor DarkUIFactor = plColor(0.5f, 0.5f, 0.5f, 1.0f);
plColor plColorScheme::s_DarkUIColors[Count] = {
  GetColor(plColorScheme::Red, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Pink, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Grape, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Violet, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Indigo, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Blue, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Cyan, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Teal, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Green, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Lime, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Yellow, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Orange, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Gray, DarkUIGrayBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::PlasmaBranding, DarkUIHighBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(plColorScheme::Black, DarkUIHighBrightness, DarkUISaturation) * DarkUIFactor,
};

static constexpr plUInt8 LightUIBrightness = 4;
static constexpr plUInt8 LightUIHighBrightness = 8;
static constexpr plUInt8 LightUIGrayBrightness = 5; // gray is too dark at UIBrightness
static constexpr float LightUISaturation = 1.0f;
plColor plColorScheme::s_LightUIColors[Count] = {
  GetColor(plColorScheme::Red, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Pink, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Grape, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Violet, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Indigo, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Blue, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Cyan, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Teal, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Green, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Lime, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Yellow, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Orange, LightUIBrightness, LightUISaturation),
  GetColor(plColorScheme::Gray, LightUIGrayBrightness, LightUISaturation),
  GetColor(plColorScheme::PlasmaBranding, LightUIHighBrightness, LightUISaturation),
  GetColor(plColorScheme::Black, LightUIHighBrightness, LightUISaturation),
};

// static
plColor plColorScheme::GetColor(float fIndex, plUInt8 uiBrightness, float fSaturation /*= 1.0f*/, float fAlpha /*= 1.0f*/)
{
  plUInt32 uiIndexA, uiIndexB;
  float fFrac;
  GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

  const plColor a = s_Colors[uiIndexA][uiBrightness];
  const plColor b = s_Colors[uiIndexB][uiBrightness];
  const plColor c = plMath::Lerp(a, b, fFrac);
  const float l = c.GetLuminance();
  return plMath::Lerp(plColor(l, l, l), c, fSaturation).WithAlpha(fAlpha);
}

plColorScheme::CategoryColorFunc plColorScheme::s_CategoryColorFunc = nullptr;

plColor plColorScheme::GetCategoryColor(plStringView sCategory, CategoryColorUsage usage)
{
  if (s_CategoryColorFunc != nullptr)
  {
    return s_CategoryColorFunc(sCategory, usage);
  }

  plInt8 iBrightnessOffset = -3;
  plUInt8 uiSaturationStep = 0;

  if (usage == plColorScheme::CategoryColorUsage::BorderIconColor)
  {
    // don't color these icons at all
    return plColor::MakeZero();
  }

  if (usage == plColorScheme::CategoryColorUsage::MenuEntryIcon || usage == plColorScheme::CategoryColorUsage::AssetMenuIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == plColorScheme::CategoryColorUsage::ViewportIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 2;
  }
  else if (usage == plColorScheme::CategoryColorUsage::OverlayIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == plColorScheme::CategoryColorUsage::SceneTreeIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == plColorScheme::CategoryColorUsage::BorderColor)
  {
    iBrightnessOffset = -3;
    uiSaturationStep = 0;
  }

  const plUInt8 uiBrightness = (plUInt8)plMath::Clamp<plInt32>(DarkUIBrightness + iBrightnessOffset, 0, 9);
  const float fSaturation = DarkUISaturation - (uiSaturationStep * 0.2f);

  if (const char* sep = sCategory.FindSubString("/"))
  {
    // chop off everything behind the first separator
    sCategory = plStringView(sCategory.GetStartPointer(), sep);
  }

  if (sCategory.IsEqual_NoCase("AI"))
    return plColorScheme::GetColor(plColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Animation"))
    return plColorScheme::GetColor(plColorScheme::Pink, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Construction"))
    return plColorScheme::GetColor(plColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Custom"))
    return plColorScheme::GetColor(plColorScheme::PlasmaBranding, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Effects"))
    return plColorScheme::GetColor(plColorScheme::Grape, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Gameplay"))
    return plColorScheme::GetColor(plColorScheme::Indigo, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Input"))
    return plColorScheme::GetColor(plColorScheme::Red, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Lighting"))
    return plColorScheme::GetColor(plColorScheme::Violet, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Logic"))
    return plColorScheme::GetColor(plColorScheme::Teal, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Physics"))
    return plColorScheme::GetColor(plColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Prefabs"))
    return plColorScheme::GetColor(plColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Rendering"))
    return plColorScheme::GetColor(plColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Terrain"))
    return plColorScheme::GetColor(plColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Scripting"))
    return plColorScheme::GetColor(plColorScheme::Green, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Sound"))
    return plColorScheme::GetColor(plColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Utilities") || sCategory.IsEqual_NoCase("Editing"))
    return plColorScheme::GetColor(plColorScheme::Gray, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("XR"))
    return plColorScheme::GetColor(plColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  plLog::Warning("Color for category '{}' is undefined.", sCategory);
  return plColor::MakeZero();
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_ColorScheme);
