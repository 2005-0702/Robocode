; =====================================================================
;  RobocodeBlocks.iss  ―  Robocode Blocks のインストーラーを作る設定（Inno Setup 6.3 以上）
;
;  このファイルは、リポジトリの installer フォルダに置く。
;    Robocode\              （.sln のあるフォルダ）
;      Robocode.sln
;      web\index.html
;      x64\Release\Robocode.exe      ← Visual Studio で Release | x64 をビルドするとできる
;      installer\
;        RobocodeBlocks.iss          ← このファイル
;        jdk\bin\javac.exe …         ← Adoptium の JDK（.zip）を展開して、フォルダ名を jdk にする
;        MicrosoftEdgeWebview2Setup.exe  ← （なくてもよい）WebView2 が入っていない PC のため
;        robocode-setup.jar              ← （なくてもよい）Robocode もいっしょに入れたいとき
;
;  Inno Setup でこのファイルを開いて「Build → Compile」を押すと、
;  installer\Output\RobocodeBlocks_Setup.exe ができる。
; =====================================================================

#define AppName "Robocode Blocks"
#define AppVersion "1.0"
#define AppExe "Robocode.exe"

[Setup]
; AppId はこのアプリの番号。バージョンを上げても変えないこと（変えると別のアプリとして入ってしまう）
AppId={{2BD10331-E53B-434A-B384-0C56C7E5A3AF}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Robocode Blocks チーム
; 使う人のフォルダ（%LOCALAPPDATA%\Programs\Robocode Blocks）に入れる。管理者の許可はいらない
;   Program Files に入れると、WebView2 がアプリの横にデータのフォルダを作れず、画面が出なくなるため
PrivilegesRequired=lowest
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=RobocodeBlocks_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
UninstallDisplayIcon={app}\{#AppExe}

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "デスクトップにショートカットを作る"; GroupDescription: "ショートカット:"
#if FileExists(AddBackslash(SourcePath) + "robocode-setup.jar")
Name: "installrobocode"; Description: "Robocode もインストールする（まだ入れていないとき）"; GroupDescription: "Robocode:"
#endif

[Files]
; アプリ本体（Visual Studio の Release | x64 のビルド結果）
Source: "..\x64\Release\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\x64\Release\WebView2Loader.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; 画面（index.html など）
Source: "..\web\*"; DestDir: "{app}\web"; Flags: ignoreversion recursesubdirs createallsubdirs
; JDK（javac と java）。exe の横の jdk フォルダにあれば、アプリはそれを使う（JAVA_HOME・PATH の設定はいらない）
Source: "jdk\*"; DestDir: "{app}\jdk"; Flags: ignoreversion recursesubdirs createallsubdirs
; WebView2 ランタイムのインストーラー（入っていない PC のときだけ、あとで実行する）
#if FileExists(AddBackslash(SourcePath) + "MicrosoftEdgeWebview2Setup.exe")
Source: "MicrosoftEdgeWebview2Setup.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
#endif
; Robocode のインストーラー（えらんだときだけ）
#if FileExists(AddBackslash(SourcePath) + "robocode-setup.jar")
Source: "robocode-setup.jar"; DestDir: "{tmp}"; Flags: deleteafterinstall; Tasks: installrobocode
#endif

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
#if FileExists(AddBackslash(SourcePath) + "MicrosoftEdgeWebview2Setup.exe")
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; Parameters: "/silent /install"; StatusMsg: "WebView2 ランタイムをインストールしています…"; Check: NeedsWebView2
#endif
#if FileExists(AddBackslash(SourcePath) + "robocode-setup.jar")
; Robocode のインストーラーは、アプリといっしょに入れた java で動かす（Java が入っていない PC でも動く）
Filename: "{app}\jdk\bin\javaw.exe"; Parameters: "-jar ""{tmp}\robocode-setup.jar"""; WorkingDir: "{tmp}"; StatusMsg: "Robocode のインストーラーを開いています…"; Flags: waituntilterminated; Tasks: installrobocode
#endif
Filename: "{app}\{#AppExe}"; Description: "Robocode Blocks を起動する"; Flags: nowait postinstall skipifsilent

[Code]
// WebView2 ランタイムが入っているか（Microsoft の説明にある場所を見る）
function WebView2Version(Root: Integer; Key: String): String;
var
  V: String;
begin
  Result := '';
  if RegQueryStringValue(Root, Key, 'pv', V) then
    if (V <> '') and (V <> '0.0.0.0') then Result := V;
end;

function NeedsWebView2: Boolean;
var
  Guid: String;
begin
  Guid := '{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';
  Result := (WebView2Version(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\' + Guid) = '') and
            (WebView2Version(HKLM, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\' + Guid) = '') and
            (WebView2Version(HKCU, 'Software\Microsoft\EdgeUpdate\Clients\' + Guid) = '');
end;
