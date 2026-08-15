#define MyAppName "EchoValhalla SuperPlugin"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "EchoValhalla Audio"
#define MyAppURL "https://github.com/echovalhalla/superplugin"
#define MyVST3Name "EchoValhalla SuperPlugin.vst3"

[Setup]
AppId={{C38A1397-62F1-4E70-9884-282697F8674B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commoncf}\VST3
DisableDirPage=no
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=EULA.txt
OutputBaseFilename=EchoValhalla_SuperPlugin_Setup_v1.0.0
OutputDir=.
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "dist\{#MyVST3Name}\*"; DestDir: "{app}\{#MyVST3Name}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
