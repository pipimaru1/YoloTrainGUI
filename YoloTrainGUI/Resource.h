#pragma once

#define IDD_MAIN                        101

// Annotation (Network)
#define IDC_COMBO_IMG                   1001
#define IDC_BTN_BROWSE_IMG              1002
#define IDC_COMBO_LABEL                 1003
#define IDC_BTN_BROWSE_LABEL            1004

// Local Temp & Split
#define IDC_COMBO_TEMP                  1005
#define IDC_BTN_BROWSE_TEMP             1006
#define IDC_BTN_COPY                    1007
#define IDC_EDIT_TRAINPCT               1010
#define IDC_EDIT_REDUCTION              1011
#define IDC_BTN_SPLIT                   1012

// YOLOv5 section
#define IDC_COMBO_WORKDIR               1020
#define IDC_BTN_BROWSE_WORKDIR          1021
#define IDC_COMBO_TRAINPY               1022
#define IDC_BTN_BROWSE_TRAINPY          1023
#define IDC_COMBO_YAML                  1024
#define IDC_BTN_BROWSE_YAML             1025
#define IDC_COMBO_HYP                   1026
#define IDC_BTN_BROWSE_HYP              1027
#define IDC_COMBO_CFG                   1028
#define IDC_BTN_BROWSE_CFG              1029
#define IDC_COMBO_WEIGHTS               1030
#define IDC_BTN_BROWSE_WEIGHTS          1031
#define IDC_COMBO_PYTHON                1032
#define IDC_BTN_BROWSE_PYTHON           1033
#define IDC_COMBO_ACTIVATE              1034
#define IDC_BTN_BROWSE_ACTIVATE         1035

#define IDC_BTN_EDIT_YAML               1036
#define IDC_BTN_EDIT_HYP                1037
#define IDC_BTN_EDIT_CFG                1038

// Options
#define IDC_COMBO_EPOCHS                1040
#define IDC_COMBO_BATCHSIZE             1041
#define IDC_COMBO_IMGSZ                 1042
#define IDC_COMBO_DEVICE                1043
#define IDC_CHK_CACHE                   1044
#define IDC_COMBO_NAME                  1045
#define IDC_EDIT_PROJECT                1046

// Run
#define IDC_BTN_TRAIN                   1050
#define IDC_BTN_STOP                    1051

// Progress & Log
#define IDC_PROGRESS                    1060
#define IDC_LOG                         1070

// tooltips
#define IDC_STC_TEMP					1073

#define IDC_STATIC				   2300

//#define IDC_STATIC_PATIENCE      2301
#define IDC_COMBO_PATIENCE         2302
//#define IDC_SPIN_PATIENCE        2303
#define IDC_CHECK_RESUME           2304
#define IDC_COMBO_RESUME_PATH      2305
#define IDC_BTN_RESUME_BROWSE      2306
#define IDC_CHECK_EXISTOK          2307

#define IDC_CMB_PATIENCE          1801  // --patience のドロップダウン
#define IDC_CMB_RESUME            1802  // --resume のドロップダウン
#define IDC_BTN_RESUME_BROWSE     1803  // 参照... ボタン（ファイル選択）
#define IDC_EDIT_RESUME_PATH      1804  // 選択した .pt 表示用（読み取り専用エディット推奨）
#define IDC_CHK_EXIST_OK          1805  // --exist-ok のチェックボックス

#define IDC_BTN_SAFE_DIR		  1901 // 安全なGITディレクトリを作成するボタン
