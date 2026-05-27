# YoloTrainGUI 説明書
<img width="742" height="996" alt="image" src="https://github.com/user-attachments/assets/36c639d7-2995-4e8d-8ac1-11e6860c39b8" />

YoloTrainGUI は、YOLO 系モデルの学習作業を支援する Windows GUI アプリケーションです。  
C++ / Win32 API で作成されており、アプリ自身が学習処理を実装するのではなく、外部の YOLOv5 系 `train.py` を Python で起動します。

主な用途は次のとおりです。

- 共有フォルダ上の画像データとラベルデータをローカル一時フォルダへコピーする
- 画像とラベルを `train` / `valid` 用に分割する
- `train.py`、`data.yaml`、`hyp.yaml`、`cfg.yaml`、初期重み、Python 実行環境を GUI で指定する
- YOLO 学習コマンドを組み立てて実行する
- 学習ログを画面に表示し、実行コマンド履歴を保存する

> 画面画像は後から追加してください。
>
> 例:  
> `![YoloTrainGUI メイン画面](./images/YoloTrainGUI_main.png)`

---

## 1. アプリケーション構成

リポジトリの主な構成は次のとおりです。

```text
YoloTrainGUI-master/
├─ YoloTrainGUI.sln
└─ YoloTrainGUI/
   ├─ YoloTrainGUI.cpp
   ├─ YoloTrainGUI.h
   ├─ YoloTrainGUI.rc
   ├─ YoloTrainGUI.vcxproj
   ├─ Resource.h
   ├─ Tooltip.hpp
   ├─ richedit.cpp
   ├─ pch.cpp / pch.h
   ├─ mru_history.ini
   └─ history.txt
```

| ファイル | 内容 |
|---|---|
| `YoloTrainGUI.sln` | Visual Studio ソリューションファイル |
| `YoloTrainGUI.vcxproj` | C++ プロジェクトファイル |
| `YoloTrainGUI.cpp` | GUI の処理、設定保存、データコピー、分割、学習コマンド実行処理 |
| `YoloTrainGUI.rc` | ダイアログ画面のリソース定義 |
| `Resource.h` | ボタンや入力欄などのリソース ID 定義 |
| `mru_history.ini` | 入力履歴・設定値を保存する設定ファイル |
| `history.txt` | 実行した学習コマンドの履歴 |

---

## 2. ビルド方法

### 2.1 必要な開発環境

ビルドには以下が必要です。

- Windows 10 / Windows 11
- Visual Studio 2022
- Visual Studio の「C++ によるデスクトップ開発」ワークロード
- MSVC v143 ツールセット
- Windows 10 SDK または Windows 11 SDK

このアプリは Win32 API ベースの GUI アプリケーションです。  
OpenCV などの外部 C++ ライブラリは使用していません。

ソース上で使用している主な Windows ライブラリは次のとおりです。

- `Comctl32.lib`
- `Ole32.lib`
- `Shell32.lib`
- `Shlwapi.lib`
- `Uuid.lib`

これらは `YoloTrainGUI.cpp` 内の `#pragma comment(lib, ...)` によりリンクされます。

### 2.2 Visual Studio でのビルド

1. `YoloTrainGUI.sln` を Visual Studio 2022 で開きます。
2. 構成を選択します。
   - 推奨: `Release | x64`
   - デバッグ時: `Debug | x64`
3. メニューから **ビルド > ソリューションのビルド** を実行します。
4. ビルドに成功すると、構成に応じた出力フォルダに `YoloTrainGUI.exe` が生成されます。

一般的な出力先の例です。

```text
YoloTrainGUI-master\x64\Release\YoloTrainGUI.exe
YoloTrainGUI-master\x64\Debug\YoloTrainGUI.exe
```

### 2.3 C++17 設定について

ソースコードでは `std::filesystem` を使用しています。  
そのため、C++17 以上でビルドしてください。

プロジェクトでは `Debug | x64` に `stdcpp17` が設定されていますが、構成によっては明示されていない場合があります。  
ビルド時に `std::filesystem` 関連のエラーが出る場合は、Visual Studio のプロジェクト設定で次を指定してください。

```text
C/C++ > 言語 > C++ 言語標準 > ISO C++17 標準 (/std:c++17)
```

### 2.4 ビルド時の注意点

添付ソースの `YoloTrainGUI.rc` には、次のような記述があります。

```rc
LTEXT "patience", -1,        85  250, 30, 8
```

`85` と `250` の間にカンマがないため、環境によってはリソースコンパイルエラーになる可能性があります。  
エラーになる場合は、次のように修正してください。

```rc
LTEXT "patience", -1,        85, 250, 30, 8
```

また、`Resource.h` では一部の ID 名が複数回定義されています。  
ビルド警告が出る場合は、未使用の古い定義を整理してください。

---

## 3. 実行に必要な環境

YoloTrainGUI は、YOLO 学習コマンドを組み立てて Python を起動するアプリです。  
そのため、実際の学習には以下が必要です。

- YOLOv5 系のソースコード
- `train.py`
- Python 実行環境
- PyTorch
- YOLOv5 の依存パッケージ
- 必要に応じて CUDA / GPU 対応 PyTorch
- 学習用の画像データ
- YOLO 形式のラベルデータ
- `data.yaml`
- 必要に応じて `hyp.yaml`、`cfg.yaml`、初期重み `.pt`

このアプリの画面名は `YOLOv5 Trainer GUI` であり、実装されている引数も YOLOv5 の `train.py` に近い形式です。

---

## 4. 起動方法

### 4.1 通常起動

ビルド後、`YoloTrainGUI.exe` をダブルクリックして起動します。

設定ファイルと履歴ファイルをアプリと同じ場所で管理したい場合は、コマンドプロンプトから実行フォルダに移動して起動します。

```bat
cd /d C:\path\to\YoloTrainGUI
YoloTrainGUI.exe
```

### 4.2 コマンドライン引数

現在の実装では、起動時のコマンドライン引数は使用していません。  
設定値は画面入力と設定ファイルから読み込まれます。

### 4.3 起動時に読み込まれるファイル

起動時に、カレントディレクトリの `mru_history.ini` を読み込みます。  
入力履歴や一部のチェック状態が保存されている場合、画面のコンボボックスに反映されます。

```text
mru_history.ini
history.txt
```

> ご要望の `default.ini` に相当する設定ファイルは、現在の実装では `mru_history.ini` です。  
> アプリに `default.ini` を読み込ませたい場合は、`YoloTrainGUI.cpp` の `GetIniFile()` を変更します。
>
> ```cpp
> static fs::path GetIniFile()
> {
>     return L"default.ini";
> }
> ```

---

## 5. 設定ファイル

### 5.1 設定ファイル名

現在の実装で使用される設定ファイル名は次のとおりです。

```text
mru_history.ini
```

ソースコード上では、次の関数で指定されています。

```cpp
static fs::path GetIniFile()
{
    return L"mru_history.ini";
}
```

過去のコメントでは `%LOCALAPPDATA%\YoloV5Trainer\mru_history.ini` に保存する構想もありますが、現在は有効になっていません。  
そのため、起動時のカレントディレクトリにある `mru_history.ini` が読み書きされます。

### 5.2 ファイル形式

`mru_history.ini` は、一般的な `key=value` 形式ではなく、セクションごとに番号付きで値を保存する形式です。

```ini
[SectionName]
0:first value
1:second value
2:third value
```

各セクションでは、`0:` の値が最新または優先値として扱われます。  
起動時は、セクション内の値がコンボボックスの候補として追加され、先頭の値が表示されます。

文字コードは UTF-8 です。

### 5.3 主な設定セクション

| セクション | 内容 |
|---|---|
| `[Image Data]` | 学習画像フォルダの履歴 |
| `[Label Data]` | YOLO ラベルフォルダの履歴 |
| `[Temp Dir]` | ローカル一時フォルダの履歴 |
| `[WorkDir]` | YOLOv5 の作業ディレクトリ |
| `[train.py]` | 使用する `train.py` のパス |
| `[data.yaml]` | 学習データ定義 YAML のパス |
| `[hyp.yaml]` | ハイパーパラメータ YAML のパス |
| `[cfg.yaml]` | モデル構造 YAML のパス |
| `[weights.pt]` | 初期重み `.pt` のパス |
| `[Python.exe]` | Python 実行ファイルのパス、または `python.exe` |
| `[Environment]` | Conda などの環境名 |
| `[Activate.bat]` | 学習開始時に保存される環境指定の履歴 |
| `[epochs]` | エポック数 |
| `[patience]` | early stopping の patience 値 |
| `[batch]` | バッチサイズ |
| `[imgsz]` | 入力画像サイズ |
| `[device]` | 使用デバイス。例: `0`, `cpu` |
| `[Name]` | 学習実行名。YOLOv5 の `--name` に対応 |
| `[project]` | 出力先プロジェクト。YOLOv5 の `--project` に対応 |
| `[cache]` | `--cache` のチェック状態。`1` が有効、`0` が無効 |
| `[resume]` | 再開学習用 checkpoint のパス |
| `[resume_chk]` | Resume チェックボックスの状態 |
| `[exist_ok]` | Allow existing output チェックボックスの状態 |
| `[TrainPercent]` | train/valid 分割時の train 比率 |
| `[Reduction]` | 使用データの削減率 |

### 5.4 設定ファイル例

以下は `mru_history.ini`、または `default.ini` として使う場合の例です。

```ini
[Image Data]
0:D:\yolo_dataset\images

[Label Data]
0:D:\yolo_dataset\labels

[Temp Dir]
0:D:\Data\yolo_temp

[WorkDir]
0:C:\Programming\yolov5

[train.py]
0:C:\Programming\yolov5\train.py

[data.yaml]
0:C:\Programming\yolov5\data\custom.yaml

[hyp.yaml]
0:C:\Programming\yolov5\data\hyps\hyp.scratch-low.yaml

[cfg.yaml]
0:C:\Programming\yolov5\models\yolov5s.yaml

[weights.pt]
0:C:\Programming\yolov5\yolov5s.pt

[Python.exe]
0:python.exe

[Environment]
0:yolov5_cuda

[epochs]
0:100

[patience]
0:50

[batch]
0:16

[imgsz]
0:640

[device]
0:0

[Name]
0:train_custom_001

[project]
0:runs/train

[cache]
0:1

[resume]
0:C:\Programming\yolov5\runs\train\exp\weights\last.pt

[resume_chk]
0:0

[exist_ok]
0:0

[TrainPercent]
0:80

[Reduction]
0:1.0
```

### 5.5 設定保存のタイミング

設定は主に次のタイミングで保存されます。

- `Start Training` を押して学習コマンドを実行したとき
- アプリを閉じるとき
- `data.yaml`、`hyp.yaml`、`cfg.yaml` を `Edit` で開いたとき

実行した学習コマンドは `history.txt` に追記されます。  
`history.txt` はコマンド実行履歴であり、設定読み込みには使用されません。

---

## 6. 画面項目

### 6.1 データ準備エリア

| 項目 | 説明 |
|---|---|
| `Images (shared)` | 元画像フォルダを指定します。共有フォルダやネットワークドライブ上の画像フォルダを指定できます。 |
| `Labels (shared)` | YOLO 形式のラベル `.txt` が格納されたフォルダを指定します。 |
| `Temp (local)` | ローカル一時フォルダを指定します。コピー先、分割後データセットの作成先になります。 |
| `Copy Images/Labels to Temp` | 画像とラベルを `Temp` 配下へコピーします。 |
| `Train%` | train データに割り当てる割合です。初期値は `80` です。 |
| `Reduction` | 使用する画像数の割合です。初期値は `1.0` です。 |
| `Split train/valid` | コピー済みデータを train / valid に分割します。 |

### 6.2 YOLOv5 実行エリア

| 項目 | 説明 |
|---|---|
| `WorkDir (./yolov5)` | YOLOv5 リポジトリのルートディレクトリを指定します。 |
| `GitSafe` | `git config --global --add safe.directory` を実行し、Git の safe.directory に WorkDir を追加します。 |
| `train.py` | YOLOv5 の `train.py` を指定します。 |
| `data.yaml` | データセット定義ファイルを指定します。`Edit` で関連付けエディタを起動できます。 |
| `hyp.yaml` | ハイパーパラメータファイルを指定します。未指定でも実行可能です。 |
| `cfg.yaml` | モデル設定ファイルを指定します。未指定でも実行可能です。 |
| `Init Weights.pt` | 初期重みファイルを指定します。 |
| `Python.exe` | 使用する Python 実行ファイルを指定します。未指定時は `python` が使用されます。 |
| `Environment Name` | Conda 環境名などを指定します。未指定の場合、環境 activate は行われません。 |

### 6.3 Options

| 項目 | YOLOv5 引数 | 説明 |
|---|---|---|
| `epochs` | `--epochs` | 学習エポック数 |
| `patience` | `--patience` | early stopping の patience 値 |
| `batch` | `--batch` | バッチサイズ |
| `imgsz` | `--img` | 入力画像サイズ |
| `device` | `--device` | 使用デバイス。例: `0`, `cpu` |
| `cache` | `--cache` | データセットをキャッシュします。 |
| `name` | `--name` | 学習実行名 |
| `project` | `--project` | 学習結果の出力先 |
| `Resume (--resume)` | `--resume` | checkpoint から再開するための設定 |
| `Allow existing output (--exist-ok)` | `--exist-ok` | 既存出力先を許可する設定 |

> 現在のソースでは、`Allow existing output (--exist-ok)` のチェック状態は設定ファイルに保存されますが、学習コマンドへ `--exist-ok` を追加する処理は確認できません。  
> 必要な場合は `DoTrain()` に `--exist-ok` を追加してください。

---

## 7. 操作手順

### 7.1 学習データを準備する

画像フォルダとラベルフォルダを用意します。  
ラベルは YOLO 形式の `.txt` ファイルで、画像ファイルと同じベース名にしてください。

例:

```text
dataset_source/
├─ images/
│  ├─ img001.jpg
│  ├─ img002.jpg
│  └─ img003.jpg
└─ labels/
   ├─ img001.txt
   ├─ img002.txt
   └─ img003.txt
```

現在の分割処理では、対象画像は `.jpg` ファイルです。  
`.png` や `.bmp` などを使用する場合は、ソースコードの `SplitDataset()` を拡張してください。

### 7.2 画像とラベルを Temp にコピーする

1. `Images (shared)` に画像フォルダを指定します。
2. `Labels (shared)` にラベルフォルダを指定します。
3. `Temp (local)` にローカル一時フォルダを指定します。
4. `Copy Images/Labels to Temp` を押します。

コピー実行時、以下のフォルダが作成されます。

```text
TempDir/
└─ source/
   ├─ images/
   └─ labels/
```

既に `TempDir/source/images` または `TempDir/source/labels` が存在する場合、コピー前に削除されます。

### 7.3 train / valid に分割する

1. `Train%` に train データの割合を入力します。
   - 例: `80`
2. `Reduction` に使用データの割合を入力します。
   - すべて使用する場合: `1.0`
   - 半分だけ使う場合: `0.5`
3. `Split train/valid` を押します。

分割後、以下の構成が作成されます。

```text
TempDir/
├─ source/
│  ├─ images/
│  └─ labels/
└─ dataset/
   ├─ train/
   │  ├─ images/
   │  └─ labels/
   └─ valid/
      ├─ images/
      └─ labels/
```

分割処理では、画像ファイルをランダムにシャッフルしてから train / valid にコピーします。  
ラベルファイルは、画像ファイル名と同じベース名の `.txt` が存在する場合にコピーされます。

例:

```text
画像:  sample001.jpg
ラベル: sample001.txt
```

`Reduction` は `0.001` から `1.0` の範囲に補正されます。  
`Train%` は `1` から `99` の範囲に補正されます。

### 7.4 data.yaml を準備する

YOLOv5 の `data.yaml` を準備します。  
上記の `TempDir/dataset` を使用する場合の例です。

```yaml
train: D:/Data/yolo_temp/dataset/train/images
val: D:/Data/yolo_temp/dataset/valid/images

nc: 2
names: ['class0', 'class1']
```

`data.yaml` 欄の `Edit` ボタンを押すと、Windows の関連付けエディタでファイルを開けます。  
`hyp.yaml`、`cfg.yaml` も同様に `Edit` ボタンで開けます。

### 7.5 YOLOv5 の実行設定を入力する

次の項目を設定します。

| 項目 | 入力例 |
|---|---|
| `WorkDir` | `C:\Programming\yolov5` |
| `train.py` | `C:\Programming\yolov5\train.py` |
| `data.yaml` | `C:\Programming\yolov5\data\custom.yaml` |
| `hyp.yaml` | `C:\Programming\yolov5\data\hyps\hyp.scratch-low.yaml` |
| `cfg.yaml` | `C:\Programming\yolov5\models\yolov5s.yaml` |
| `Init Weights.pt` | `C:\Programming\yolov5\yolov5s.pt` |
| `Python.exe` | `python.exe` または `C:\ProgramData\anaconda3\python.exe` |
| `Environment Name` | `yolov5_cuda` など |

`Environment Name` が入力されている場合、学習コマンドの先頭に次の処理が追加されます。

```bat
call activate <Environment Name> &&
```

Conda 環境を使う場合は、環境名を入力する運用が分かりやすいです。

### 7.6 学習を開始する

`Start Training` を押すと、入力値をもとに学習コマンドが作成され、`cmd.exe /C` 経由で実行されます。

作成されるコマンド例です。

```bat
call activate yolov5_cuda && cd /d C:\Programming\yolov5 && python.exe C:\Programming\yolov5\train.py --data C:\Programming\yolov5\data\custom.yaml --epochs 100 --patience 50 --batch 16 --img 640 --device 0 --hyp C:\Programming\yolov5\data\hyps\hyp.scratch-low.yaml --cfg C:\Programming\yolov5\models\yolov5s.yaml --weights C:\Programming\yolov5\yolov5s.pt --cache --name train_custom_001 --project runs/train
```

学習プロセスの標準出力と標準エラーは、画面下部のログ欄に表示されます。  
同時に、実行したコマンドは `history.txt` にタイムスタンプ付きで追記されます。

### 7.7 学習を停止する

`Stop` を押すと、起動中の子プロセスを終了します。  
現在の実装では `TerminateProcess()` による停止です。  
環境によっては、`cmd.exe` 配下で起動された Python プロセスが残る場合があります。停止後は必要に応じてタスクマネージャーで Python プロセスを確認してください。

---

## 8. 学習コマンドの生成仕様

`Start Training` 押下時、`DoTrain()` により次の順序でコマンドが組み立てられます。

```text
call activate <Environment> &&
cd /d <WorkDir> &&
<Python.exe> <train.py>
  --data <data.yaml>
  --epochs <epochs>
  --patience <patience>
  --resume <resume>
  --batch <batch>
  --img <imgsz>
  --device <device>
  --hyp <hyp.yaml>
  --cfg <cfg.yaml>
  --weights <weights.pt>
  --cache
  --name <name>
  --project <project>
```

空欄の項目は基本的にコマンドへ追加されません。  
ただし、`WorkDir`、`train.py`、`data.yaml` は必須です。これらが空欄の場合、学習は開始されません。

パスに空白や `&` が含まれる場合は、自動的にダブルクォーテーションで囲まれます。

---

## 9. GitSafe ボタン

YOLOv5 の作業ディレクトリが Git 管理フォルダで、Git の `safe.directory` エラーが発生する場合があります。  
`GitSafe` ボタンを押すと、`WorkDir` に入力されているフォルダに対して次のコマンドを実行します。

```bat
git config --global --add safe.directory "<WorkDir>"
```

Git が PATH に登録されていない場合は失敗します。  
その場合は Git for Windows をインストールするか、Git の実行ファイルが PATH に含まれるように設定してください。

---

## 10. ログと履歴

### 10.1 画面ログ

画面下部のログ欄には、次の内容が表示されます。

- コピー処理のログ
- 分割処理のログ
- 実行した学習コマンド
- `train.py` の標準出力
- `train.py` の標準エラー
- プロセス終了コード

### 10.2 history.txt

学習を開始すると、実行したコマンドが `history.txt` に追記されます。

例:

```text
2025-08-17 11:04:02  call activate yolov5 && cd /d D:\Programing\yolov5 && python.exe D:\Programing\yolov5\train.py --data D:\Programing\yolov5\data\coco_test.yaml --epochs 100 --patience 50 --batch 16 --img 640 --device 0 --cfg D:\Programing\yolov5\models\yolov5s.yaml --weights D:\Programing\yolov5\yolov5s.pt --cache --name TESTGUI
```

`history.txt` は実行履歴確認用です。  
アプリ起動時の設定復元には `mru_history.ini` が使われます。

---

## 11. 運用上の注意

### 11.1 画像形式

現在の分割処理では `.jpg` のみを対象にしています。  
`.png`、`.jpeg`、`.bmp` などを使いたい場合は、`SplitDataset()` の拡張が必要です。

### 11.2 ラベルファイル名

ラベルは画像と同じベース名の `.txt` である必要があります。

```text
OK:
  camera_001.jpg
  camera_001.txt

NG:
  camera_001.jpg
  label_001.txt
```

### 11.3 Temp フォルダの既存データ

`Copy Images/Labels to Temp` 実行時、既存の `TempDir/source/images` と `TempDir/source/labels` は削除されます。  
`Split train/valid` 実行時、既存の `TempDir/dataset` は削除されます。

重要なデータを `TempDir` 配下に直接置かないようにしてください。

### 11.4 Resume と exist-ok

画面上には `Resume (--resume)` と `Allow existing output (--exist-ok)` がありますが、添付ソースでは以下の点に注意が必要です。

- `exist_ok` は設定ファイルに保存されますが、`--exist-ok` を学習コマンドへ追加する処理が確認できません。
- Resume のチェック判定で、チェックボックスではなく resume パス用コンボボックスの ID を参照しているように見えます。

必要に応じて `DoTrain()` の実装を確認・修正してください。

修正例の考え方です。

```cpp
if (IsDlgButtonChecked(g_hDlg, IDC_CHECK_RESUME) == BST_CHECKED) {
    std::wstring resume = GetText(g_hDlg, IDC_CMB_RESUME);
    if (resume.empty()) ss << L" --resume";
    else ss << L" --resume " << Quote(resume);
}

if (IsDlgButtonChecked(g_hDlg, IDC_CHK_EXIST_OK) == BST_CHECKED) {
    ss << L" --exist-ok";
}
```

---

## 12. トラブルシュート

### 12.1 `workdir/train.py/data.yaml is required.` と表示される

`WorkDir`、`train.py`、`data.yaml` のいずれかが空欄です。  
3つすべてを指定してから `Start Training` を押してください。

### 12.2 Python や torch が見つからない

Python 実行環境が正しく選択されていない可能性があります。

確認項目:

- `Python.exe` が正しいか
- `Environment Name` が正しい Conda 環境名か
- その環境に PyTorch がインストールされているか
- CUDA を使う場合、PyTorch と CUDA の組み合わせが合っているか

コマンドプロンプトで次のように確認できます。

```bat
call activate yolov5_cuda
python -c "import torch; print(torch.__version__); print(torch.cuda.is_available())"
```

### 12.3 Git の safe.directory エラーが出る

`WorkDir` を指定した状態で `GitSafe` を押してください。  
または、コマンドプロンプトで次を実行してください。

```bat
git config --global --add safe.directory "C:\Programming\yolov5"
```

### 12.4 分割後に labels が少ない

画像ファイルと同名の `.txt` が存在しない場合、ラベルはコピーされません。

例:

```text
画像:  abc.jpg
必要なラベル: abc.txt
```

また、現在の分割対象画像は `.jpg` のみです。

### 12.5 ビルド時にリソースエラーが出る

`YoloTrainGUI.rc` の `patience` 行でカンマ抜けがある可能性があります。  
次のように修正してください。

```rc
LTEXT "patience", -1,        85, 250, 30, 8
```

### 12.6 `std::filesystem` のエラーが出る

C++17 が有効になっていない可能性があります。  
プロジェクト設定で `/std:c++17` を指定してください。

---

## 13. 開発者向け補足

### 13.1 設定保存先を `%LOCALAPPDATA%` に戻す場合

ソースには `%LOCALAPPDATA%\YoloV5Trainer` を使うための関数 `GetStoreDir()` が残っています。  
現在はコメントアウトされ、カレントディレクトリの `mru_history.ini` / `history.txt` を使っています。

ユーザーごとに設定を分けたい場合は、次のように戻すことができます。

```cpp
static fs::path GetIniFile()
{
    return GetStoreDir() / L"mru_history.ini";
}

static fs::path GetHistoryFile()
{
    return GetStoreDir() / L"history.txt";
}
```

### 13.2 `default.ini` に変更する場合

アプリケーション配布時に `default.ini` という名前に統一したい場合は、次のように変更します。

```cpp
static fs::path GetIniFile()
{
    return L"default.ini";
}
```

この場合、配布時には `YoloTrainGUI.exe` と同じフォルダに `default.ini` を配置してください。

### 13.3 YOLOv8 などに対応する場合

現在の実装は YOLOv5 の `train.py` 引数に合わせています。  
YOLOv8 以降では CLI や Python API の形式が異なるため、`DoTrain()` のコマンド生成処理を変更する必要があります。

---

## 14. まとめ

YoloTrainGUI は、YOLOv5 系の学習に必要なデータ準備、train/valid 分割、学習コマンド作成、実行ログ確認を GUI で支援するアプリケーションです。  
学習本体は Python 側の `train.py` が担当し、C++ アプリは設定入力とプロセス起動を担当します。

運用時は、以下の流れで使用します。

1. 画像フォルダとラベルフォルダを指定する
2. ローカル Temp フォルダへコピーする
3. train / valid に分割する
4. `data.yaml` を準備する
5. YOLOv5 の `WorkDir`、`train.py`、Python 環境、学習オプションを指定する
6. `Start Training` で学習を開始する
7. ログ画面と `history.txt` で実行状況を確認する
