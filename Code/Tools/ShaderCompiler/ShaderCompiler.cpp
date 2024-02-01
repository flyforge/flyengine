#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <ShaderCompiler/ShaderCompiler.h>

plCommandLineOptionString opt_Shader("_ShaderCompiler", "-shader", "\
One or multiple paths to shader files or folders containing shaders.\n\
Paths are separated with semicolons.\n\
Paths may be absolute or relative to the -project directory.\n\
If a path to a folder is specified, all .plShader files in that folder are compiled.\n\
\n\
This option has to be specified.",
  "");

plCommandLineOptionPath opt_Project("_ShaderCompiler", "-project", "\
Path to the folder of the project, for which shaders should be compiled.",
  "");

plCommandLineOptionString opt_Platform("_ShaderCompiler", "-platform", "The name of the platform for which to compile the shaders.\n\
Examples:\n\
  -platform DX11_SM50\n\
  -platform VULKAN\n\
  -platform ALL",
  "DX11_SM50");

plCommandLineOptionBool opt_IgnoreErrors("_ShaderCompiler", "-IgnoreErrors", "If set, a compile error won't stop other shaders from being compiled.", false);

plCommandLineOptionDoc opt_Perm("_ShaderCompiler", "-perm", "<string list>", "List of permutation variables to set to fixed values.\n\
Spaces are used to separate multiple arguments, therefore each argument mustn't use spaces.\n\
In the form of 'SOME_VAR=VALUE'\n\
Examples:\n\
  -perm BLEND_MODE=BLEND_MODE_OPAQUE\n\
  -perm TWO_SIDED=FALSE MSAA=TRUE\n\
\n\
If a permutation variable is not set to a fixed value, all shader permutations for that variable will generated and compiled.\n\
",
  "");

plShaderCompilerApplication::plShaderCompilerApplication()
  : plGameApplication("plShaderCompiler", nullptr)
{
}

plResult plShaderCompilerApplication::BeforeCoreSystemsStartup()
{
  {
    plStringBuilder cmdHelp;
    if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested, "_ShaderCompiler"))
    {
      plLog::Print(cmdHelp);
      return PL_FAILURE;
    }
  }

  plStartup::AddApplicationTag("tool");
  plStartup::AddApplicationTag("shadercompiler");

  // only print important messages
  plLog::SetDefaultLogLevel(plLogMsgType::InfoMsg);

  PL_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  auto cmd = plCommandLineUtils::GetGlobalInstance();

  m_sShaderFiles = opt_Shader.GetOptionValue(plCommandLineOption::LogMode::Always);
  PL_ASSERT_ALWAYS(!m_sShaderFiles.IsEmpty(), "Shader file has not been specified. Use the -shader command followed by a path");

  m_sAppProjectPath = opt_Project.GetOptionValue(plCommandLineOption::LogMode::Always);
  PL_ASSERT_ALWAYS(!m_sAppProjectPath.IsEmpty(), "Project directory has not been specified. Use the -project command followed by a path");

  m_sPlatforms = opt_Platform.GetOptionValue(plCommandLineOption::LogMode::Always);

  m_bIgnoreErrors = opt_IgnoreErrors.GetOptionValue(plCommandLineOption::LogMode::Always);

  const plUInt32 pvs = cmd->GetStringOptionArguments("-perm");

  for (plUInt32 pv = 0; pv < pvs; ++pv)
  {
    plStringBuilder var = cmd->GetStringOption("-perm", pv);

    const char* szEqual = var.FindSubString("=");

    if (szEqual == nullptr)
    {
      plLog::Error("Permutation Variable declaration contains no equal sign: '{0}'", var);
      continue;
    }

    plStringBuilder val = szEqual + 1;
    var.SetSubString_FromTo(var.GetData(), szEqual);

    val.Trim(" \t");
    var.Trim(" \t");

    plLog::Dev("Fixed permutation variable: {0} = {1}", var, val);
    m_FixedPermVars[var].PushBack(val);
  }

  return PL_SUCCESS;
}


void plShaderCompilerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  plStartup::StartupHighLevelSystems();
}

plResult plShaderCompilerApplication::CompileShader(plStringView sShaderFile)
{
  PL_LOG_BLOCK("Compiling Shader", sShaderFile);

  if (ExtractPermutationVarValues(sShaderFile).Failed())
    return PL_FAILURE;

  plHybridArray<plPermutationVar, 16> PermVars;

  const plUInt32 uiMaxPerms = m_PermutationGenerator.GetPermutationCount();

  plLog::Info("Shader has {0} permutations", uiMaxPerms);

  for (plUInt32 perm = 0; perm < uiMaxPerms; ++perm)
  {
    PL_LOG_BLOCK("Compiling Permutation");

    m_PermutationGenerator.GetPermutation(perm, PermVars);
    plShaderCompiler sc;
    if (sc.CompileShaderPermutationForPlatforms(sShaderFile, PermVars, plLog::GetThreadLocalLogSystem(), m_sPlatforms).Failed())
      return PL_FAILURE;
  }

  plLog::Success("Compiled Shader '{0}'", sShaderFile);
  return PL_SUCCESS;
}

plResult plShaderCompilerApplication::ExtractPermutationVarValues(plStringView sShaderFile)
{
  m_PermutationGenerator.Clear();

  plFileReader shaderFile;
  if (shaderFile.Open(sShaderFile).Failed())
  {
    plLog::Error("Could not open file '{0}'", sShaderFile);
    return PL_FAILURE;
  }

  plHybridArray<plHashedString, 16> permVars;
  plHybridArray<plPermutationVar, 16> fixedPermVars;
  plShaderParser::ParsePermutationSection(shaderFile, permVars, fixedPermVars);

  {
    PL_LOG_BLOCK("Permutation Vars");
    for (const auto& s : permVars)
    {
      plLog::Dev(s.GetData());
    }
  }

  // regular permutation variables
  {
    for (const auto& s : permVars)
    {
      plHybridArray<plHashedString, 16> values;
      plShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  // permutation variables that have fixed values
  {
    for (const auto& s : fixedPermVars)
    {
      m_PermutationGenerator.AddPermutation(s.m_sName, s.m_sValue);
    }
  }

  {
    for (auto it = m_FixedPermVars.GetIterator(); it.IsValid(); ++it)
    {
      plHashedString hsname, hsvalue;
      hsname.Assign(it.Key().GetData());
      m_PermutationGenerator.RemovePermutations(hsname);

      for (const auto& val : it.Value())
      {
        hsvalue.Assign(val.GetData());

        m_PermutationGenerator.AddPermutation(hsname, hsvalue);
      }
    }
  }

  return PL_SUCCESS;
}

void plShaderCompilerApplication::PrintConfig()
{
  PL_LOG_BLOCK("ShaderCompiler Config");

  plLog::Info("Project: '{0}'", m_sAppProjectPath);
  plLog::Info("Shader: '{0}'", m_sShaderFiles);
  plLog::Info("Platform: '{0}'", m_sPlatforms);
}

plApplication::Execution plShaderCompilerApplication::Run()
{
  PrintConfig();

  plStringBuilder files = m_sShaderFiles;

  plDynamicArray<plString> shadersToCompile;

  plDynamicArray<plStringView> allFiles;
  files.Split(false, allFiles, ";");

  for (const plStringView& shader : allFiles)
  {
    plStringBuilder file = shader;
    plStringBuilder relPath;

    if (plFileSystem::ResolvePath(file, nullptr, &relPath).Succeeded())
    {
      shadersToCompile.PushBack(relPath);
    }
    else
    {
      if (plPathUtils::IsRelativePath(file))
      {
        file.Prepend(m_sAppProjectPath, "/");
      }

      file.TrimWordEnd("*");
      file.MakeCleanPath();

      if (plOSFile::ExistsDirectory(file))
      {
        plFileSystemIterator fsIt;
        for (fsIt.StartSearch(file, plFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
        {
          if (plPathUtils::HasExtension(fsIt.GetStats().m_sName, "plShader"))
          {
            fsIt.GetStats().GetFullPath(relPath);

            if (relPath.MakeRelativeTo(m_sAppProjectPath).Succeeded())
            {
              shadersToCompile.PushBack(relPath);
            }
          }
        }
      }
      else
      {
        plLog::Error("Could not resolve path to shader '{0}'", file);
      }
    }
  }

  for (const auto& shader : shadersToCompile)
  {
    if (CompileShader(shader).Failed())
    {
      if (!m_bIgnoreErrors)
      {
        return plApplication::Execution::Quit;
      }
    }
  }

  return plApplication::Execution::Quit;
}

PL_CONSOLEAPP_ENTRY_POINT(plShaderCompilerApplication);
