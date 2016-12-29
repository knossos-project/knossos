#define MyAppName "KNOSSOS"
#define MyAppVersion "4.2"
#define MyAppPublisher "Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V."
#define MyAppURL "https://knossostool.org"
#define ExeName64 "knossos64.exe"
#define ExeName32 "knossos32.exe"
#define python64File "python-2.7.13.amd64.msi"
#define python32File "python-2.7.13.msi"
#define License "../LICENSE.txt"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
;byparam=$p
SignTool=byparam C:\Program Files (x86)\Windows Kits\10\bin\x64\signtool.exe sign /f … /t http://timestamp.comodoca.com/authenticode /p … $f
AppId={{AF7E2805-C45C-40E7-8354-90D121683915}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
ArchitecturesInstallIn64BitMode=x64
DefaultDirName={pf}\{#MyAppName} {#MyAppVersion}
DefaultGroupName={#MyAppName} {#MyAppVersion}
LicenseFile={#License}
OutputBaseFilename=win-setup-{#MyAppName} {#MyAppVersion}
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
Name: python64setup; Description: 64 Bit &Python 2.7.13 (installation recommended if you are unsure); GroupDescription: Python runtime environment (required); Check: Is64BitInstallMode
Name: python32setup; Description: 32 Bit &Python 2.7.13 (installation recommended if you are unsure); GroupDescription: Python runtime environment (required); Check: not Is64BitInstallMode
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked

[Files]
Source: {#ExeName64}; DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode
Source: {#ExeName32}; DestDir: {app}; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: {#python64File}; DestDir: {tmp}; Flags: deleteafterinstall ignoreversion; Check: Is64BitInstallMode
Source: {#python32File}; DestDir: {tmp}; Flags: deleteafterinstall ignoreversion; Check: not Is64BitInstallMode

[UninstallDelete]
Type: files; Name: {app}

[Icons]
Name: {group}\{#MyAppName}; Filename: {app}/{#ExeName64}; Check: Is64BitInstallMode
Name: {group}\{#MyAppName}; Filename: {app}/{#ExeName32}; Check: not Is64BitInstallMode
Name: {group}\Uninstall; Filename: {uninstallexe}
Name: "{commondesktop}\{#MyAppName} {#MyAppVersion}"; Filename: {app}/{#ExeName64}; Tasks: desktopicon; Check: Is64BitInstallMode
Name: "{commondesktop}\{#MyAppName} {#MyAppVersion}"; Filename: {app}/{#ExeName32}; Tasks: desktopicon; Check: not Is64BitInstallMode

[Run]
Filename: {tmp}/{#python32File}; Flags: hidewizard shellexec waituntilterminated; Tasks: python32setup  
Filename: {tmp}/{#python64File}; Flags: hidewizard shellexec waituntilterminated; Tasks: python64setup
Filename: {app}/{#ExeName64}; Description: {cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}; Flags: postinstall nowait; Check: Is64BitInstallMode
Filename: {app}/{#ExeName32}; Description: {cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}; Flags: postinstall nowait; Check: not Is64BitInstallMode