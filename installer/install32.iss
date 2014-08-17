#define MyAppName "Knossos"
#define MyAppVersion "4.0.1"
#define MyAppPublisher "Knossos"
#define MyAppURL "http://www.KnossosTool.org"
#define MyAppExeName "knossos32.exe"
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
SetupIconFile=../logo.ico
WizardSmallImageFile=logo_small.bmp
WizardImageFile=bar.bmp
Compression=lzma2/ultra64
SolidCompression=yes
UsePreviousAppDir=no
UsePreviousGroup=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#KNOSSOS_SRC_PATH}{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[UninstallDelete]
Type: files; Name: "{app}"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName} {#MyAppVersion}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: postinstall skipifsilent nowait