#define MyAppName "Knossos"
#define MyAppVersion "4.0 Beta 2"
#define MyAppPublisher "Knossos"
#define MyAppURL "http://www.KnossosTool.org"
#define MyAppExeName "knossos.exe"
#define KNOSSOS_SRC_PATH ""
#define License "LICENSE"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{AF7E2805-C45C-40E7-8354-90D121683915}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName} {#MyAppVersion}
DefaultGroupName={#MyAppName} {#MyAppVersion}
LicenseFile={#License}
OutputBaseFilename=win32-Setup-Knossos {#MyAppVersion}
SetupIconFile=logo.ico
WizardSmallImageFile=logo.bmp
WizardImageFile=bar.bmp
Compression=lzma2/ultra64
SolidCompression=yes
UsePreviousAppDir=no
UsePreviousGroup=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Dirs]
Name: "{app}\sqldrivers"
Name: "{app}\platforms"

[Files]
Source: "{#KNOSSOS_SRC_PATH}knossos.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}glut32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}icudt51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}icuin51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}icuuc51.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}libcurl-4.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#KNOSSOS_SRC_PATH}libpython2.7.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
;Source: "{#KNOSSOS_SRC_PATH}PythonQt.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5CLucene.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5Help.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5OpenGl.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5Sql.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5Test.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#KNOSSOS_SRC_PATH}logo.ico"; DestDir: "{app}"; Flags: ignoreversion

Source: "{#KNOSSOS_SRC_PATH}platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#KNOSSOS_SRC_PATH}sqldrivers\*"; DestDir: "{app}\sqldrivers"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
[UninstallDelete]
Type: files; Name: "{app}"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\knossos.exe" ; IconFilename: "{app}\logo.ico"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName} {#MyAppVersion}"; Filename: "{app}\knossos.exe"; Tasks: desktopicon ; IconFilename: "{app}\logo.ico"

[Run]
;Filename: "{app}\tools\{#pythonSetup}"; Description: "Install Python (python runtime environment required, installation recommended if you are unsure)" ; Flags: shellexec postinstall skipifsilent
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: postinstall skipifsilent nowait