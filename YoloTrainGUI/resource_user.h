#pragma once
#define IDC_CMB_PATIENCE          1801
#define IDC_CMB_RESUME            1802
#define IDC_CHK_EXIST_OK          1805
#define IDC_CHECK_RESUME          1806
#define IDC_BTN_RESUME_BROWSE     1807
#define IDC_BTN_SAFE_DIR          1808
#define IDC_STC_DATA_YAML         1809
#define IDC_STC_TRAINPY           1810
#define IDC_STC_PYTHONEXE         1811
#define IDC_BTN_VIEW_PYENV		  1812
#define IDC_BTN_CLEARTMP		  1813
#define IDC_STC_WORKDIR           1814
#define IDC_CHK_USEHYPERPARAM     1815
#define IDC_RAD_YOLOV5			  1816   // 「YOLOv5」ラジオ
#define IDC_RAD_YOLOV8            1817   // 「YOLOv8」ラジオ
#define IDC_RAD_YOLO11            1818   // 「YOLOv8」ラジオ
#define IDC_GRP_YOLOVERSION       1819 

#define IDC_RADIO_SEGMENT	1820
#define IDC_RADIO_POSE		1821
#define IDC_RADIO_CLASSIFY	1822

/*
リソース追加手順
① 事前チェック（1回だけでOK）
	リソース ビューで .rc を右クリック → リソース インクルード…
	シンボル用ヘッダ: resource.h
	読み取り専用ヘッダ:
	#include "resource_user.h"
	#include <winres.h>
	これで**デザイナが resource_user.h のIDを“読むだけ”**になります（書き換えはしない）。

➁ IDを作る（重複防止の肝）
	resource_user.h に 新しいID を追加して保存。
	（あなたの帯域に合わせて番号は任意。例）
	// resource_user.h
	#define IDC_BTN_EXPORT   1820    // 追加するボタンのID
	先に定義が見えていれば、VSは resource.h に同名を自動生成しません。

③ビルドする
	一度ビルドしてIDを読み込ませる。

④ボタン(リソース)を置く
	リソース ビューでダイアログを開く（デザイナ）。
	ツールボックスの 「ボタン」 を配置。
	そのボタンを選択して プロパティを設定：
	ID → IDC_BTN_EXPORT（今作った名前を入力）
	キャプション → 例: Export
	タブ位置（TabStop）→ True（通常は既定でTrue）
	既定ボタンにしたいなら スタイルを 既定のボタン（BS_DEFPUSHBUTTON）に。
	ここで ID がドロップダウンに出ない時は、一度 .rc を閉じて開き直すと反映されます。

⑤イベント処理を接続（Win32 ダイアログ例）
	WM_COMMAND でIDを見るだけです。
	// 例: ダイアログプロシージャ内
	case WM_COMMAND:
	{
		const int id = LOWORD(wParam);
		const int code = HIWORD(wParam);
		if (id == IDC_BTN_EXPORT && code == BN_CLICKED) {
			OnExportClicked(hDlg);   // ここに処理を書く
			return TRUE;
		}
		// 既存の処理…
		break;
	}

ありがちな落とし穴（回避メモ）
先に resource_user.h を書かないでIDを入力すると、VSが resource.h に同名を自動生成→二重定義。
→ 先に resource_user.h を書くのがコツ。
ラベル等が IDC_STATIC のままだとコードから取得できません。
→ 固有IDに置き換える（IDC_STATIC は常に -1 の特別ID、触らない）。
DS_SETFONT 未定義でデザイナが開かない…
→ リソース インクルードの読み取り専用に <winres.h> が入っているか再確認。


rc /l 0x411 /v YoloTrainGUI.rc

*/