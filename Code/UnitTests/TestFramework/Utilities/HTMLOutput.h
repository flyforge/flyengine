#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <fstream>
#include <iostream>
#include <sstream>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <shellapi.h>
#endif

struct plOutputToHTML
{
  static std::ofstream htmlFile;

  static void OutputToHTML(plTestOutput::Enum type, const char* szMsg)
  {
    if (type != plTestOutput::StartOutput && !htmlFile.is_open())
      return;

    static plInt32 iIndentation = 0;
    static bool bError = false;
    static bool bDetails = false;
    static std::string sSubTest;
    static std::string sDuration;
    static std::ostringstream details;

    auto FlushDetails = [&]() {
      if (bDetails)
      {
        bDetails = false;
        htmlFile << "<tr>\n<td colspan=\"4\" class=\"title\" />" << details.str() << "\n</tr>";
      }
      details.str("");
    };

    switch (type)
    {
      case plTestOutput::StartOutput:
      {
        iIndentation = 0;
        bError = false;
        bDetails = false;
        sSubTest.clear();
        sDuration.clear();

        plTestFramework::GetInstance()->CreateOutputFolder();

        std::string sOutputFile = std::string(plTestFramework::GetInstance()->GetAbsOutputPath()) + "/UnitTestsLog.htm";
        const char* szTestName = plTestFramework::GetInstance()->GetTestName();

        const char* szStyle = "body { margin: 0; padding: 20px; font-size: 12px; font-family: Arial, Sans-Serif; background-color: #fff; "
                              "text-align: center; }"
                              "#container { margin: 20px auto; width: 900px; text-align: left; }"
                              "table { border-collapse: collapse; width: 100%; }"
                              "table, td { font-size: 12px; border: solid #000 1px; padding: 5px; }"
                              "td { background-color: #66ff66; }"
                              "td.category { background-color: #ccc; font-weight: bold; }"
                              "td.title { background-color: #fff; }"
                              "td.error { background-color: #ff6666; }"
                              "td.details { background-color: #ffff00; }"
                              "p.success { background-color: #66ff66; }"
                              "p.error { background-color: #ff6666; };";

        htmlFile.open(sOutputFile.c_str());
        if (htmlFile.is_open())
        {
          htmlFile << "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n<head>"
                      "<title>Log - "
                   << szTestName
                   << "</title>\n"
                      "<style type=\"text/css\" media=\"screen\">\n"
                   << szStyle
                   << "</style></head>\n"
                      "<body>\n<div id=\"container\">\n<h1>Log - "
                   << szTestName
                   << "</h1>\n"
                      "<table>\n<tr>\n<td class=\"title\">Testmethod</td>\n<td class=\"title\">Result</td>"
                      "<td class=\"title\">Duration</td>\n<td class=\"title\">Details</td>\n</tr>";
        }
      }
      break;
      case plTestOutput::BeginBlock:
        iIndentation++;

        if (iIndentation == 1)
        {
          // Test start
          htmlFile << "<tr>\n<td colspan=\"4\" class=\"category\" />" << szMsg << "\n</tr>";
        }
        else if (iIndentation == 2)
        {
          // Sub-test start
          FlushDetails();
          bError = false;
          sSubTest = szMsg;
        }
        else
        {
          details << szMsg << "<br/><BLOCKQUOTE>";
        }
        break;

      case plTestOutput::EndBlock:
        if (iIndentation == 1)
        {
          // Test end
          FlushDetails();
        }
        if (iIndentation == 2)
        {
          // Sub-test end
          if (bError)
          {
            htmlFile << "<tr>\n<td class=\"error\">" << sSubTest
                     << "</td>\n"
                        "<td class=\"error\">Failed</td>\n<td class=\"error\">"
                     << sDuration
                     << "</td>\n"
                        "<td class=\"error\"></td>\n</tr>";
          }
          else
          {
            htmlFile << "<tr>\n<td>" << sSubTest << "</td>\n<td>Passed</td>\n<td>" << sDuration << "</td>\n<td></td>\n</tr>";
          }
          FlushDetails();
        }
        else
        {
          details << "</BLOCKQUOTE>";
        }

        iIndentation--;
        break;

      case plTestOutput::Duration:
        sDuration = szMsg;
        break;

      case plTestOutput::Error:
        bError = true;
        bDetails = true;
        details << "<p class=\"error\">" << szMsg << "</p>";
        htmlFile.flush();
        break;

      case plTestOutput::Warning:
      case plTestOutput::Message:
      case plTestOutput::ImportantInfo:
        bDetails = true;

      case plTestOutput::Details:
        details << szMsg << "<br/>";
        break;

      case plTestOutput::ImageDiffFile:
        details << "<a href=\"" << szMsg << "\" target=\"_blank\">View Image Comparison Result</a><br/>";
        break;

      case plTestOutput::Success:
        // iIndentation 1 and 2 are test and sub-test level and are handled explicitly.
        // Anything above is custom success message within a test that we want to log directly.
        if (iIndentation > 2)
        {
          details << "<p class=\"success\">" << szMsg << "</p>";
        }
        break;

      case plTestOutput::FinalResult:
      {
        htmlFile << "</table>\n<h2>" << szMsg << "</h2>";
        htmlFile << "</div>\n</body>\n</html>";
        htmlFile.close();

        std::string sOutputFile = std::string(plTestFramework::GetInstance()->GetAbsOutputPath()) + "/UnitTestsLog.htm";

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
        TestSettings settings = plTestFramework::GetInstance()->GetSettings();
        if (settings.m_bOpenHtmlOutputOnError && bError)
        {
          // opens the html file in a browser
          ShellExecuteA(nullptr, "open", sOutputFile.c_str(), nullptr, nullptr, SW_SHOW);
        }
#endif
      }
      break;
      default:
        break;
    }
  }
};

std::ofstream plOutputToHTML::htmlFile;
