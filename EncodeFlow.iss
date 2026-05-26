[Setup]
AppName=EncodeFlow
AppVersion=1.0.0
DefaultDirName={pf}\EncodeFlow
DefaultGroupName=EncodeFlow
OutputDir=installer
OutputBaseFilename=EncodeFlowSetup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "x64\Release\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\EncodeFlow"; Filename: "{app}\EncodeFlow.exe"
Name: "{commondesktop}\EncodeFlow"; Filename: "{app}\EncodeFlow.exe"

[Run]
Filename: "{app}\EncodeFlow.exe"; Description: "Lancer EncodeFlow"; Flags: nowait postinstall skipifsilent