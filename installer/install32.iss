#define MyAppName "Knossos"
#define MyAppVersion "4.1"
#define MyAppPublisher "Knossos"
#define MyAppURL "http://www.knossostool.org"
#define MyAppExeName "knossos32.exe"
#define pythonSetup "python-2.7.9.msi"
#define License "LICENSE"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{01CE4BB4-056B-455A-90EA-F326DFF9DA75}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName} {#MyAppVersion}
DefaultGroupName={#MyAppName} {#MyAppVersion} 32 Bit
LicenseFile={#License}
OutputBaseFilename=win32-Setup-Knossos {#MyAppVersion}
SetupIconFile=../resources/icons/logo.ico
WizardSmallImageFile=logo_small.bmp
WizardImageFile=bar.bmp
Compression=lzma2/ultra64
SolidCompression=yes
UsePreviousAppDir=no
UsePreviousGroup=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: pythonsetup; Description: 32 Bit &Python 2.7.9 (installation recommended if you are unsure); GroupDescription: Python runtime environment (required)
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked

[Files]
Source: {#MyAppExeName}; DestDir: {app}; Flags: ignoreversion
Source: {#PythonSetup}; DestDir: {tmp}; Flags: deleteafterinstall ignoreversion

[UninstallDelete]
Type: files; Name: {app}

[Icons]
Name: {group}\{#MyAppName}; Filename: {app}/{#MyAppExeName}
Name: {group}\Uninstall; Filename: {uninstallexe}
Name: "{commondesktop}\{#MyAppName} {#MyAppVersion}"; Filename: {app}/{#MyAppExeName}; Tasks: desktopicon

[Run]
Filename: {tmp}/{#PythonSetup}; Flags: hidewizard shellexec waituntilterminated; Tasks: pythonsetup
Filename: {app}/{#MyAppExeName}; Description: {cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}; Flags: postinstall nowait