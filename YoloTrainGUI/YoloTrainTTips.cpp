#define _WIN32_WINNT 0x0601
#include "pch.h"
#include "tools.hpp"
#include "resource.h"
//#include "resource_user.h"
#include "YoloTrainGUI.h"
#include "Tooltip.hpp"

// ツールチップ
Tooltip _ToolTipMainDlg; // グローバルなツールチップオブジェクト
Tooltip _ToolTipMultiCpDlg; // グローバルなツールチップオブジェクト

std::wstring strTipTempDir =
L"TempDir\r\n"
L"├─ source\r\n"
L"│   ├─ images (from shared)\r\n"
L"│   └─ labels (from shared)\r\n"
L"├─ train\r\n"
L"│   ├─ images\r\n"
L"│   └─ labels\r\n"
L"└─ valid\r\n"
L"    ├─ images\r\n"
L"    └─ labels\r\n"
L"\r\n"
L"元データは ./TempDir/source にコピーされます。\r\n"
L"トレーニング用と検証用に分割されると、\r\n"
L"./TempDir/train および./TempDir/valid にコピーされます。\r\n"
L"【注意】yolov8はRAMディスク使えません!!【注意】\r\n";

std::wstring strTipDataYaml =
L"クラシフィケーション定義とデータフォルダを記載します。 \r\n"
L"下記を参考に記述してください。絶対パスの方が安定します。\r\n"
L"\r\n"
L"train: C:/TempDir/train/images \r\n"
L"val : C:/TempDir/valid/images \r\n";

std::wstring strTipTrainPy =
L"train.pyを指定します。\r\n"
L"通常はパスを入れずに train.py とだけ記述してください。\r\n";

std::wstring strTipPythonExe =
L"python.exeを指定します。\r\n"
L"通常はパスを入れずに Python.exe とだけ記述してください。\r\n";

std::wstring strTipWorkDir =
L"Yolovの作業ディレクトリを指定します。\r\n"
L"train.pyのあるディレクトリを指定してください。\r\n"
L"./yolov5や./yolov8, ./yolo11 等を指定します。\r\n";

std::wstring strGitSafe =
L"Yolo作業ディレクトリに必要なファイルを\r\n"
L"gitでダウンロードすることがあります\r\n"
L"このボタンを押して、gitsafe に指定してください。\r\n";

std::wstring strChkEnv =
L"Pythonの環境設定のリストを表示します。\r\n"
L"Condaに対応しています。\r\n";

std::wstring strTipCfgYaml =
L"不要な時は空欄にしてください。\r\n"
L"ptファイルがあれば不要です。\r\n";

std::wstring strTipAutoEditYamlBtn =
L"Tempフォルダをdata.yamlに適用します。\r\n";


void SetTootips(HWND hDlg, Tooltip& ttTmpDir)
{
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_TEMP, L"YOLO GUI", strTipTempDir.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_DATA_YAML, L"YOLO GUI", strTipDataYaml.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_EDIT_YAML, L"YOLO GUI", strTipDataYaml.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_TRAINPY, L"YOLO GUI", strTipTrainPy.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_PYTHONEXE, L"YOLO GUI", strTipPythonExe.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_WORKDIR, L"YOLO GUI", strTipWorkDir.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_SAFE_DIR, L"YOLO GUI", strGitSafe.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VIEW_PYENV, L"YOLO GUI", strChkEnv.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_CFGYAML, L"CFG Yaml", strTipCfgYaml.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BUTTON_APPLY_YAML, L"YOLO GUI", strTipAutoEditYamlBtn.c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_ONLY_WITH_LABEL,
        L"YOLO GUI",
        std::wstring(L"ラベルファイルのあるデータのみコピーします。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_GRP_YOLOVERSION,
        L"YOLO GUI",
        std::wstring(L"Yoloのバージョンを指定します。\r\n対応はyolov5、yolov8、yolo11です。\r\nyolov8とyolo11のラーニング環境は同じです。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RAD_YOLOV5,
        L"YOLO GUI",
        std::wstring(L"Yoloのバージョンを指定します。\r\n対応はyolov5、yolov8、yolo11です。\r\nyolov8とyolo11のラーニング環境は同じです。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RAD_YOLOV8,
        L"YOLO GUI",
        std::wstring(L"Yoloのバージョンを指定します。\r\n対応はyolov5、yolov8、yolo11です。\r\nyolov8とyolo11のラーニング環境は同じです。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_USEPROXY,
        L"YOLO GUI",
        std::wstring(L"プロキシ環境の時はチェックしてください。\r\n自動的に必要ファイルをダウンロードすることがあります。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_PROXY_HTTP,
        L"YOLO GUI",
        std::wstring(L"プロキシ環境の時は指定してください。\r\n自動的に必要ファイルをダウンロードすることがあります。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_PROXY_HTTPS,
        L"YOLO GUI",
        std::wstring(L"プロキシ環境の時は指定してください。\r\n自動的に必要ファイルをダウンロードすることがあります。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_PROXY_HTTP,
        L"YOLO GUI",
        std::wstring(L"プロキシ環境の時は指定してください。\r\n自動的に必要ファイルをダウンロードすることがあります。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_PROXY_HTTPS,
        L"YOLO GUI",
        std::wstring(L"プロキシ環境の時は指定してください。\r\n自動的に必要ファイルをダウンロードすることがあります。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_COMBO_IMG,
        L"YOLO GUI",
        std::wstring(L"[ソース]イメージファイルのあるフォルダを指定").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_COMBO_LABEL,
        L"YOLO GUI",
        std::wstring(L"[ソース]ラベルファイルのあるフォルダを指定").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_COMBO_TEMP,
        L"YOLO GUI",
        std::wstring(L"テンポラリフォルダを指定してください。ソースからこのフォルダにコピーします。\r\n"
            L"【注意】CLR Tempで、このフォルダは丸ごと消されます。専用のフォルダを作成して指定するようにしてください。"
        ).c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_OPEN_COPY_MULTI,
        L"YOLO GUI",
        std::wstring(L"フォルダ毎にTrainとValidを指定する場合はこのボタンを押してください。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_CLEARTMP,
        L"YOLO GUI",
        std::wstring(L"テンポラリフォルダを消去します。【注意】フォルダごと消します。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_COPY,
        L"YOLO GUI",
        std::wstring(L"ソースからテンポラリへコピーを開始します。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_ONLY_WITH_LABEL,
        L"YOLO GUI",
        std::wstring(L"ラベルファイルのあるペアのみコピーします。\r\n"
            L"アノテーション中のデータセットを味見トレーニングするときに便利です。").c_str()
    );

    std::wstring strIDC_RADIO_SPLITUNIT =
        L"TrainとValidに分割するときの、連続するファイル数単位を指定します。\n\r"
        L"連続する似た画像がTrainとValid両方に入ることを抑制します。"
        L"汎化性能の向上の可能性があります。";

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RADIO_SPLITUNIT_1, L"YOLO GUI", strIDC_RADIO_SPLITUNIT.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RADIO_SPLITUNIT_5, L"YOLO GUI", strIDC_RADIO_SPLITUNIT.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RADIO_SPLITUNIT_10, L"YOLO GUI", strIDC_RADIO_SPLITUNIT.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RADIO_SPLITUNIT_20, L"YOLO GUI", strIDC_RADIO_SPLITUNIT.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RADIO_SPLITUNIT_50, L"YOLO GUI", strIDC_RADIO_SPLITUNIT.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_RADIO_SPLITUNIT_100, L"YOLO GUI", strIDC_RADIO_SPLITUNIT.c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_SPLIT,
        L"YOLO GUI",
        std::wstring(L"テンポラリフォルダのデータセットをTrainとValidに分割します。\r\n"
            L"分割したデータは別のフォルダにコピーされます。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_EDIT_TRAINPCT,
        L"YOLO GUI",
        std::wstring(L"Trainに割り当てるデータの割合を%で指定してください。\r\n"
            L"ふつうは80です。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_EDIT_REDUCTION,
        L"YOLO GUI",
        std::wstring(L"使うデータの割合を係数で指定してください。\r\n"
            L"分からなければ1.0を指定してください。\r\n"
            L"大きなデータセットのテストなどでデータを減らしたいときに使います。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_SPLIT_SHUFFLE,
        L"YOLO GUI",
        std::wstring(L"データを分割するときランダムな順番にしてから分割します。\r\n"
            L"毎回結果が変わります。\r\n"
            L"デフォルトでは規則な順番で分割します。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_COMBO_WEIGHTS,
        L"YOLO GUI",
        std::wstring(L"初期ネットワークモデルファイル(*.pt)を指定します。 \r\n"
            L"Yolov5の場合はcfg.yamlとモデルが一致するようにいしてください。\r\n"
            L"Yolov8/11の場合は、ここで指定されたネットワークモデルと同じ構成のモデルになります。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_COMBO_ACTIVATE,
        L"YOLO GUI",
        std::wstring(L"トレインに使用するpython環境名を指定します。 \r\n"
            L"anaconda、minicondaに対応します。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_COMBO_NAME,
        L"YOLO GUI",
        std::wstring(L"トレインのプロジェクト名を指定します。 \r\n"
            L"このプロジェクト名のフォルダの下にトレーニング結果が保存されます。").c_str()
    );
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_EXIST_OK,
        L"YOLO GUI",
        std::wstring(L"トレインの結果は上書きされないように自動敵に連番が付与されますが、 \r\n"
            L"ここをチェックすると、連番は付与されず、上書きされます。").c_str()
    );

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN,
        L"YOLO GUI",
        std::wstring(L"トレインを開始します。").c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_STOP,
        L"YOLO GUI",
        std::wstring(L"トレインを中止します。確実に停止したか確認するには、タスクマネージャーで見てください。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_EDITHISTORY,
        L"YOLO GUI",
        std::wstring(L"トレインのコマンドの履歴を出します。\r\n"
            L"この履歴を編集すればコマンドプロンプトから手動でトレーニングを実行できます。").c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_OPENTENSORBOARD,
        L"YOLO GUI",
        std::wstring(L"TensorBoadを開いてトレーニング進捗を見ます。\r\n"
            L"トレーニング開始前に、下記のコマンドを同じpython環境で実行しておく必要があります。\r\n"
            L"yolo settings tensorboard = True\r\n"
            L"さらに、TensorBordは別のコマンドプロンプトでサーバーを起動しておいてください。\r\n"
            L"起動コマンド例\r\n"
            L"tensorboard --logdir runs/train --port 6006\r\n"
            L"tensorboard --logdir e : \Programming\yolov8\runs\detect --port 6006\r\n"
            L"tensorboard --logdir C : \Programming\yolov8\runs\detect --port 6006"
        ).c_str());

    //本当は関数を分割
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_0, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_1, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_2, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_3, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_4, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_5, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_6, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_7, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_8, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_9, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_10, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_11, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_12, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_13, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_14, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_TRAIN_EN_15, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_0, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_1, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_2, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_3, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_4, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_5, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_6, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_7, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_8, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_9, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_10, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_11, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_12, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_13, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_14, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CHK_VALID_EN_15, L"YOLO GUI", std::wstring(L"このデータの使用/不使用を選択します。").c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_0, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_1, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_2, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_3, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_4, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_5, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_6, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_7, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_8, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_9, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_10, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_11, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_12, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_13, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_14, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_TRAIN_SRC_15, L"YOLO GUI", std::wstring(L"Trainとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_0, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_1, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_2, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_3, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_4, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_5, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_6, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_7, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_8, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_9, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_10, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_11, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_12, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_13, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_14, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_CMB_VALID_SRC_15, L"YOLO GUI", std::wstring(L"Validとして使うデータフォルダを指定します。\r\nimage,lableの親フォルダを指定します。").c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_0, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_1, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_2, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_3, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_4, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_5, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_6, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_7, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_8, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_9, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_10, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_11, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_12, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_13, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_14, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_TRAIN_SRC_15, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_0, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_1, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_2, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_3, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_4, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_5, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_6, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_7, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_8, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_9, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_10, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_11, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_12, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_13, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_14, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STATIC_VALID_SRC_15, L"YOLO GUI", std::wstring(L"データ数を表示します。\r\nimageフォルダの中のjpgファル数をカウントします。").c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_0, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_1, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_2, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_3, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_4, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_5, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_6, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_7, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_8, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_9, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_10, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_11, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_12, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_13, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_14, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_BROWSE_15, L"YOLO GUI", std::wstring(L"trainに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_0, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_1, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_2, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_3, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_4, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_5, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_6, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_7, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_8, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_9, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_10, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_11, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_12, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_13, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_14, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_BROWSE_15, L"YOLO GUI", std::wstring(L"validに使用したいフォルダを指定します。").c_str());

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_0, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_1, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_2, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_3, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_4, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_5, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_6, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_7, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_8, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_9, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_10, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_11, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_12, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_13, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_14, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_TRAIN_SRC_CREAR_15, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_0, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_1, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_2, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_3, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_4, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_5, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_6, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_7, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_8, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_9, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_10, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_11, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_12, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_13, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_14, L"YOLO GUI", L"値をクリアします。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VALID_SRC_CREAR_15, L"YOLO GUI", L"値をクリアします。");


    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_EXPORTLIST_MCOPY, L"YOLO GUI", L"フォルダの組み合わせてを保存します。");
	ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_IMPORTLIST_MCOPY, L"YOLO GUI", L"保存したフォルダの組み合わせを読み込みます。");

    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_CLEARTMP_MCOPY, L"YOLO GUI", L"テンポラリフォルダをクリアします。\r\nコピーする前に必ず実行してください。");
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_COPY_TO_TEMP, L"YOLO GUI", L"指定されたフォルダのデータのコピーを実行します。\r\nメインに戻ってもSplit(ファイル分割)は行わないでください。");

}





    
