#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QApplication>
#include <QMessageBox>

class QColorDialog;
class plQtColorDialog;

class PL_GUIFOUNDATION_DLL plQtUiServices : public QObject
{
  Q_OBJECT

  PL_DECLARE_SINGLETON(plQtUiServices);

public:
  struct Event
  {
    enum Type
    {
      ShowDocumentTemporaryStatusBarText,
      ShowDocumentPermanentStatusBarText,
      ShowGlobalStatusBarText,
      ClickedDocumentPermanentStatusBarText,
      CheckForUpdates,
    };

    enum TextType
    {
      Info,
      Warning,
      Error
    };

    Type m_Type;
    plString m_sText;
    plTime m_Time;
    TextType m_TextType = TextType::Info;
  };

  static plEvent<const plQtUiServices::Event&> s_Events;

  struct TickEvent
  {
    enum class Type
    {
      StartFrame,
      EndFrame,
    };

    Type m_Type;
    plUInt32 m_uiFrame = 0;
    plTime m_Time;
    double m_fRefreshRate = 60.0;
  };

  static plEvent<const plQtUiServices::TickEvent&> s_TickEvent;

public:
  plQtUiServices();

  /// \brief True if the application doesn't show any window and only works in the background
  static bool IsHeadless();

  /// \brief Set to true if the application doesn't show any window and only works in the background
  static void SetHeadless(bool bHeadless);

  /// \brief Shows a non-modal color dialog. The Qt slots are called when the selected color is changed or when the dialog is closed and the result
  /// accepted or rejected.
  void ShowColorDialog(const plColor& color, bool bAlpha, bool bHDR, QWidget* pParent, const char* szSlotCurColChanged, const char* szSlotAccept, const char* szSlotReject);

  /// \brief Might show a message box depending on the given status. If the status is 'failure' the szFailureMsg is shown, including the message in
  /// plStatus. If the status is success a message box with text szSuccessMsg is shown, but only if the status message is not empty or if
  /// bOnlySuccessMsgIfDetails is false.
  static void MessageBoxStatus(const plStatus& s, const char* szFailureMsg, const char* szSuccessMsg = "", bool bOnlySuccessMsgIfDetails = true);

  /// \brief Shows an information message box
  static void MessageBoxInformation(const plFormatString& msg);

  /// \brief Shows an warning message box
  static void MessageBoxWarning(const plFormatString& msg);

  /// \brief Shows a question message box and returns which button the user pressed
  static QMessageBox::StandardButton MessageBoxQuestion(const plFormatString& msg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);

  /// \brief Use this if you need to display a status bar message in any/all documents. Go directly through the document, if you only want to show a
  /// message in a single document window.
  static void ShowAllDocumentsTemporaryStatusBarMessage(const plFormatString& msg, plTime timeOut);

  static void ShowAllDocumentsPermanentStatusBarMessage(const plFormatString& msg, Event::TextType type);

  /// \brief Shows a 'critical' message in all container windows (in red), which does not disappear, until it is replaced with another (empty) string.
  static void ShowGlobalStatusBarMessage(const plFormatString& msg);

  /// \brief Opens the given file in the program that is registered in the OS to handle that file type.
  static bool OpenFileInDefaultProgram(const char* szPath);

  /// \brief Opens the given file or folder in the Explorer
  static void OpenInExplorer(const char* szPath, bool bIsFile);

  static plStatus OpenInClion(const QStringList& arguments);

  static plStatus OpenInRider(const char* szPath);
  
  static plStatus OpenIn10X(const char* szPath);
  
  static plStatus OpenInVisualStudio(const char* szPath);

  /// \brief Attempts to launch Visual Studio Code with the given command line
  static plStatus OpenInVsCode(const QStringList& arguments);

  /// \brief Loads some global state used by plQtUiServices from the registry. E.g. the last position of the color dialog.
  void LoadState();

  /// \brief Saves some global state used by plQtUiServices to the registry.
  void SaveState();

  /// \brief Returns a cached QIcon that was created from an internal Qt resource (e.g. 'QIcon(":QtNamespace/MyIcon.png")' ). Prevents creating the
  /// object over and over.
  ///
  /// If svgTintColor is a non-zero color, and sIdentifier points to an .SVG file, then the first time the icon is requested with that color,
  /// a copy is made, and the SVG content is modified such that white ("#FFFFFF") gets replaced by the requested color.
  /// Thus multiple tints of the same icon can be created for different use cases.
  /// Usually this is used to get different shades of the same icon, such that it looks good on the target background.
  static const QIcon& GetCachedIconResource(plStringView sIdentifier, plColor svgTintColor = plColor::MakeZero());

  /// \brief Returns a cached QImage that was created from an internal Qt resource (e.g. 'QImage(":QtNamespace/MyIcon.png")' ). Prevents creating the
  /// object over and over.
  static const QImage& GetCachedImageResource(const char* szIdentifier);

  /// \brief Returns a cached QPixmap that was created from an internal Qt resource (e.g. 'QPixmap(":QtNamespace/MyIcon.png")' ). Prevents creating
  /// the object over and over.
  static const QPixmap& GetCachedPixmapResource(const char* szIdentifier);

  /// \brief Adds the pattern to the gitignore file.
  ///
  /// If the gitignore file does not exist, it is created.
  /// If the pattern is already present in the file, it is not added again.
  static plResult AddToGitIgnore(const char* szGitIgnoreFile, const char* szPattern);

  /// \brief Raises the 'CheckForUpdates' event
  static void CheckForUpdates();

  void Init();

private Q_SLOTS:
  void TickEventHandler();

private:
  plQtColorDialog* m_pColorDlg;
  QByteArray m_ColorDlgGeometry;

  static plMap<plString, QIcon> s_IconsCache;
  static plMap<plString, QImage> s_ImagesCache;
  static plMap<plString, QPixmap> s_PixmapsCache;
  static bool s_bHeadless;
  static TickEvent s_LastTickEvent;
  bool m_bIsDrawingATM = false;
};
