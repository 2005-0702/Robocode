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
;        robocode\                       ← （なくてもよい）Robocode をいっしょに入れたいとき
;                                          開発の PC で Robocode をこのフォルダにインストールしておく
;                                          （robocode\libs\robocode.jar がある状態）
;
;  Inno Setup でこのファイルを開いて「Build → Compile」を押すと、
;  installer\Output\RobocodeBlocks_Setup.exe ができる。
; =====================================================================

#define AppName "Robocode Blocks"
#define AppVersion "1.0"
#define AppExe "Robocode.exe"
; アイコン：Robocode フォルダ（.vcxproj の横）の Robocode.ico
#define AppIcon "Robocode_Icon.ico"
; いっしょに入れる Robocode のバージョン（installer\robocode に入れた Robocode に合わせて書きかえる）
#define RobocodeVersion "1.10.1"

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
; 入れる場所を選ぶ画面を、毎回出す（もう入っているときも）
;   最初は前に入れた場所が入っている。そのまま「次へ」なら上書き、変えれば新しい場所に入る
DisableDirPage=no
OutputDir=Output
OutputBaseFilename=RobocodeBlocks_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
UninstallDisplayIcon={app}\{#AppIcon}
; インストーラー（RobocodeBlocks_Setup.exe）そのもののアイコン
SetupIconFile=..\Robocode\{#AppIcon}

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "デスクトップにショートカットを作る"; GroupDescription: "ショートカット:"
#if DirExists(AddBackslash(SourcePath) + "robocode")
Name: "installrobocode"; Description: "Robocode {#RobocodeVersion} をインストールする（もう入っているときは上書き）"; GroupDescription: "いっしょに入れるもの:"
#endif
#if FileExists(AddBackslash(SourcePath) + "MicrosoftEdgeWebview2Setup.exe")
Name: "installwebview2"; Description: "WebView2 ランタイムをインストールする（もう入っているときは更新）"; GroupDescription: "いっしょに入れるもの:"
#endif

[Files]
; アプリ本体（Visual Studio の Release | x64 のビルド結果）
Source: "..\x64\Release\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\x64\Release\WebView2Loader.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; ショートカットに使うアイコン（exe の中のアイコンではなく、このファイルを直接使う）
Source: "..\Robocode\{#AppIcon}"; DestDir: "{app}"; Flags: ignoreversion
; 画面（index.html など）
Source: "..\web\*"; DestDir: "{app}\web"; Flags: ignoreversion recursesubdirs createallsubdirs
; JDK（javac と java）。exe の横の jdk フォルダにあれば、アプリはそれを使う（JAVA_HOME・PATH の設定はいらない）
Source: "jdk\*"; DestDir: "{app}\jdk"; Flags: ignoreversion recursesubdirs createallsubdirs
; WebView2 ランタイムのインストーラー（えらんだときだけ、あとで実行する。もう入っていれば更新になる）
#if FileExists(AddBackslash(SourcePath) + "MicrosoftEdgeWebview2Setup.exe")
Source: "MicrosoftEdgeWebview2Setup.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Tasks: installwebview2
#endif
; Robocode（えらんだときだけ）。入れる場所は [Code] の RobocodeDir で決める
;   もう入っている Robocode のほうが新しいときは、上書きしないで別の場所に入れる
#if DirExists(AddBackslash(SourcePath) + "robocode")
Source: "robocode\*"; DestDir: "{code:RobocodeDir}"; Flags: ignoreversion recursesubdirs createallsubdirs; Tasks: installrobocode
#endif

[Icons]
; IconFilename：ショートカットのアイコンを、いっしょに入れた .ico から取る
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"; IconFilename: "{app}\{#AppIcon}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; IconFilename: "{app}\{#AppIcon}"; Tasks: desktopicon

[Run]
#if FileExists(AddBackslash(SourcePath) + "MicrosoftEdgeWebview2Setup.exe")
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; Parameters: "/silent /install"; StatusMsg: "WebView2 ランタイムをインストールしています…"; Tasks: installwebview2
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

function NeedsWebView2: Boolean; forward;   // 下で作る

function WebView2Installed: Boolean;
begin
  Result := not NeedsWebView2;
end;

// ---------------------------------------------------------------------
//  Robocode：もう入っているか・バージョン・入れる場所
// ---------------------------------------------------------------------
var
  RobocodeTarget: String;   // Robocode を入れる場所（「次へ」を押したときに決める）

// Robocode のフォルダか（libs\robocode.jar があるか）
function IsRobocodeFolder(Dir: String): Boolean;
begin
  Result := (Dir <> '') and FileExists(AddBackslash(Dir) + 'libs\robocode.jar');
end;

// もう入っている Robocode のフォルダ（なければ ''）
//   アプリが覚えているフォルダ（%LOCALAPPDATA%\RobocodeBlocks\robocode_folder.txt）→ C:\robocode の順に見る
function InstalledRobocodeFolder: String;
var
  Saved: AnsiString;
begin
  Result := '';
  if LoadStringFromFile(ExpandConstant('{localappdata}\RobocodeBlocks\robocode_folder.txt'), Saved) then
  begin
    Saved := Trim(Saved);
    if IsRobocodeFolder(String(Saved)) then
    begin
      Result := String(Saved);
      exit;
    end;
  end;
  if IsRobocodeFolder('C:\robocode') then Result := 'C:\robocode';
end;

// libs の中の「robocode.core-1.9.5.4.jar」のような名前から、バージョンを読む（分からなければ ''）
function VersionFromJar(Dir, Prefix: String): String;
var
  FR: TFindRec;
  Name: String;
begin
  Result := '';
  if FindFirst(AddBackslash(Dir) + 'libs\' + Prefix + '*.jar', FR) then
  begin
    try
      Name := FR.Name;
      Result := Copy(Name, Length(Prefix) + 1, Length(Name) - Length(Prefix) - 4);
    finally
      FindClose(FR);
    end;
  end;
end;

function InstalledRobocodeVersion(Dir: String): String;
begin
  Result := VersionFromJar(Dir, 'robocode.core-');
  if Result = '' then Result := VersionFromJar(Dir, 'robocode.api-');
end;

// 「1.9.5.4」の先頭の数字を取り出して、残りを Rest に入れる
function TakeNumber(var Rest: String): Integer;
var
  P: Integer;
begin
  P := Pos('.', Rest);
  if P = 0 then
  begin
    Result := StrToIntDef(Rest, 0);
    Rest := '';
  end else begin
    Result := StrToIntDef(Copy(Rest, 1, P - 1), 0);
    Rest := Copy(Rest, P + 1, Length(Rest));
  end;
end;

// バージョンをくらべる：A のほうが古い → 負、同じ → 0、新しい → 正
function CompareVersion(A, B: String): Integer;
var
  X, Y: Integer;
begin
  Result := 0;
  while (Result = 0) and ((A <> '') or (B <> '')) do
  begin
    X := TakeNumber(A);
    Y := TakeNumber(B);
    Result := X - Y;
  end;
end;

// [Files] の DestDir から呼ばれる
function RobocodeDir(Param: String): String;
begin
  if RobocodeTarget = '' then RobocodeTarget := 'C:\robocode';
  Result := RobocodeTarget;
end;

// Robocode を入れる場所を決める（もう入っているときは、バージョンをくらべる）
procedure DecideRobocodeTarget;
var
  Existing, Ver, Other: String;
begin
  Existing := InstalledRobocodeFolder;
  if Existing = '' then
  begin
    RobocodeTarget := 'C:\robocode';   // まだ入っていない
    exit;
  end;
  Other := Existing + '-{#RobocodeVersion}';   // 別の場所（例：C:\robocode-1.9.5.4）
  Ver := InstalledRobocodeVersion(Existing);
  if Ver = '' then
  begin
    // 入っている Robocode のバージョンが分からないときは、聞く
    if MsgBox('もう入っている Robocode（' + Existing + '）のバージョンが分かりませんでした。' + #13#10 +
              '上書きしますか？' + #13#10#13#10 +
              '「はい」：上書きする' + #13#10 +
              '「いいえ」：上書きしないで、別の場所（' + Other + '）に入れる',
              mbConfirmation, MB_YESNO) = IDYES then
      RobocodeTarget := Existing
    else
      RobocodeTarget := Other;
  end
  else if CompareVersion('{#RobocodeVersion}', Ver) < 0 then
  begin
    // 入っているほうが新しい → 上書きしない
    RobocodeTarget := Other;
    MsgBox('もう入っている Robocode（' + Ver + '）のほうが、いっしょに入れる Robocode（{#RobocodeVersion}）より新しいので、' +
           '上書きしないで、別の場所に入れます。' + #13#10#13#10 + Other, mbInformation, MB_OK);
  end
  else
    RobocodeTarget := Existing;   // 同じか、入れるほうが新しい → 上書き
end;

// 「いっしょに入れるもの」は、もう入っていれば、最初はチェックを外しておく（入れるかどうかは選べる）
var
  TasksPageShown: Boolean;

procedure CurPageChanged(CurPageID: Integer);
begin
  if (CurPageID = wpSelectTasks) and not TasksPageShown then
  begin
    TasksPageShown := True;
#if DirExists(AddBackslash(SourcePath) + "robocode")
    if InstalledRobocodeFolder <> '' then WizardSelectTasks('!installrobocode');
#endif
#if FileExists(AddBackslash(SourcePath) + "MicrosoftEdgeWebview2Setup.exe")
    if WebView2Installed then WizardSelectTasks('!installwebview2');
#endif
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
#if DirExists(AddBackslash(SourcePath) + "robocode")
  if (CurPageID = wpSelectTasks) and WizardIsTaskSelected('installrobocode') then DecideRobocodeTarget;
#endif
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
