#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

plResult plTypeScriptBinding::SetupProjectCode()
{
  plStringBuilder sAbsPathToPlasmaTemplate, sAbsPathToData;
  PLASMA_SUCCEED_OR_RETURN(plFileSystem::ResolvePath(":plugins/TypeScript/pl-template.ts", &sAbsPathToPlasmaTemplate, nullptr));

  sAbsPathToData = sAbsPathToPlasmaTemplate;
  sAbsPathToData.PathParentDirectory();

  plStringBuilder sPlasmaFileContent;

  // read pl.ts
  {
    plFileReader filePlasma;
    PLASMA_SUCCEED_OR_RETURN(filePlasma.Open(sAbsPathToPlasmaTemplate));
    sPlasmaFileContent.ReadAll(filePlasma);
  }

  // Remove all files from the Project/TypeScript folder that are also in Plugins/TypeScript
#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)
  {
    plStringBuilder sTargetPath;

    plFileSystemIterator it;
    for (it.StartSearch(sAbsPathToData, plFileSystemIteratorFlags::ReportFilesRecursive); it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sTargetPath);

      sTargetPath.MakeRelativeTo(sAbsPathToData).IgnoreResult();
      sTargetPath.Prepend(":project/TypeScript/");

      plFileSystem::DeleteFile(sTargetPath);
    }
  }
#endif

  RemoveAutoGeneratedCode(sPlasmaFileContent);

  GenerateComponentsFile(":project/TypeScript/pl/AllComponents.ts");
  InjectComponentImportExport(sPlasmaFileContent, "./pl/AllComponents");

  GenerateMessagesFile(":project/TypeScript/pl/AllMessages.ts");
  InjectMessageImportExport(sPlasmaFileContent, "./pl/AllMessages");

  GenerateEnumsFile(":project/TypeScript/pl/AllEnums.ts", s_RequiredEnums);
  GenerateEnumsFile(":project/TypeScript/pl/AllFlags.ts", s_RequiredFlags);

  InjectEnumImportExport(sPlasmaFileContent, "./pl/AllEnums");
  InjectFlagsImportExport(sPlasmaFileContent, "./pl/AllFlags");

  {
    plDeferredFileWriter fileOut;
    fileOut.SetOutput(":project/TypeScript/pl.ts", true);
    PLASMA_SUCCEED_OR_RETURN(fileOut.WriteBytes(sPlasmaFileContent.GetData(), sPlasmaFileContent.GetElementCount()));
    PLASMA_SUCCEED_OR_RETURN(fileOut.Close());
  }

  return PLASMA_SUCCESS;
}

void plTypeScriptBinding::GetTsName(const plRTTI* pRtti, plStringBuilder& out_sName)
{
  out_sName = pRtti->GetTypeName();
  out_sName.TrimWordStart("pl");
}

void plTypeScriptBinding::GenerateComponentCode(plStringBuilder& out_Code, const plRTTI* pRtti)
{
  plStringBuilder sComponentType, sParentType;
  GetTsName(pRtti, sComponentType);

  GetTsName(pRtti->GetParentType(), sParentType);

  out_Code.AppendFormat("export class {0} extends {1}\n", sComponentType, sParentType);
  out_Code.Append("{\n");
  out_Code.AppendFormat("  public static GetTypeNameHash(): number { return {}; }\n", plHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()));
  GenerateExposedFunctionsCode(out_Code, pRtti);
  GeneratePropertiesCode(out_Code, pRtti);
  out_Code.Append("}\n\n");
}

static void CreateComponentTypeList(plSet<const plRTTI*>& ref_found, plDynamicArray<const plRTTI*>& ref_sorted, const plRTTI* pRtti)
{
  if (ref_found.Contains(pRtti))
    return;
  
  if (!pRtti->IsDerivedFrom<plComponent>())
    return;

  if (pRtti == plGetStaticRTTI<plComponent>() || pRtti == plGetStaticRTTI<plTypeScriptComponent>())
    return;

  ref_found.Insert(pRtti);
  CreateComponentTypeList(ref_found, ref_sorted, pRtti->GetParentType());

  ref_sorted.PushBack(pRtti);
}

static void CreateComponentTypeList(plDynamicArray<const plRTTI*>& out_sorted)
{
  plSet<const plRTTI*> found;
  out_sorted.Reserve(100);
  
  plHybridArray<const plRTTI*, 64> alphabetical;
  plRTTI::ForEachDerivedType<plComponent>([&](const plRTTI* pRtti) { alphabetical.PushBack(pRtti); });
  alphabetical.Sort([](const plRTTI* p1, const plRTTI* p2) -> bool { return p1->GetTypeName().Compare(p2->GetTypeName()) < 0; });

  for (auto pRtti : alphabetical)
  {
    CreateComponentTypeList(found, out_sorted, pRtti);
  }
}

void plTypeScriptBinding::GenerateAllComponentsCode(plStringBuilder& out_Code)
{
  plDynamicArray<const plRTTI*> sorted;
  CreateComponentTypeList(sorted);

  for (auto pRtti : sorted)
  {
    GenerateComponentCode(out_Code, pRtti);
  }
}

void plTypeScriptBinding::GenerateComponentsFile(const char* szFile)
{
  plStringBuilder sFileContent;

  sFileContent =
    R"(
import __GameObject = require("TypeScript/pl/GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("TypeScript/pl/Component")
export import Component = __Component.Component;

import __Vec2 = require("TypeScript/pl/Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("TypeScript/pl/Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("TypeScript/pl/Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("TypeScript/pl/Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("TypeScript/pl/Quat")
export import Quat = __Quat.Quat;

import __Transform = require("TypeScript/pl/Transform")
export import Transform = __Transform.Transform;

import __Color = require("TypeScript/pl/Color")
export import Color = __Color.Color;

import __Time = require("TypeScript/pl/Time")
export import Time = __Time.Time;

import __Angle = require("TypeScript/pl/Angle")
export import Angle = __Angle.Angle;

import Enum = require("./AllEnums")
import Flags = require("./AllFlags")

declare function __CPP_ComponentProperty_get(component: Component, id: number);
declare function __CPP_ComponentProperty_set(component: Component, id: number, value);
declare function __CPP_ComponentFunction_Call(component: Component, id: number, ...args);

)";

  GenerateAllComponentsCode(sFileContent);

  plDeferredFileWriter file;
  file.SetOutput(szFile, true);

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()).IgnoreResult();

  if (file.Close().Failed())
  {
    plLog::Error("Failed to write file '{}'", szFile);
    return;
  }
}

void plTypeScriptBinding::InjectComponentImportExport(plStringBuilder& content, const char* szComponentFile)
{
  plDynamicArray<const plRTTI*> sorted;
  CreateComponentTypeList(sorted);

  plStringBuilder sImportExport, sTypeName;

  sImportExport.Format(R"(import __AllComponents = require("{}")
)",
    szComponentFile);

  for (const plRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0} = __AllComponents.{0};\n", sTypeName);
  }

  AppendToTextFile(content, sImportExport);
}
