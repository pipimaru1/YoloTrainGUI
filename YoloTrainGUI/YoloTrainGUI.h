#pragma once

#include "pch.h"
#include "resource.h"

// ------------------------------
// Globals
// ------------------------------
extern HINSTANCE g_hInst;
extern HWND g_hDlg;
extern std::atomic<HANDLE> g_hChildProc;
extern std::mutex g_logMutex;
extern std::wstring g_logBuffer;

void LogAppendANSI(const std::wstring& s);
void AppendLog(const std::wstring& s);

//////////////////////////////////////////////////////////////////////////////////////
// Get text from a control
//////////////////////////////////////////////////////////////////////////////////////
class TrainParams {
    std::wstring src_dir;
    std::wstring src_dir_image;
    std::wstring src_dir_label;

    std::wstring RatioTrain;
    std::wstring RatioUse; //for Hyperparameter tuning データを減らしたい時に使う

    std::wstring workdir;
    std::wstring trainpy;
    std::wstring datayaml;
    std::wstring hypyaml;
    std::wstring cfgyaml;
    std::wstring weights;
    std::wstring python;
    std::wstring activate;
    std::wstring resume;
    std::wstring epochs;
    std::wstring patience;
    std::wstring batch;
    std::wstring imgsz;
    std::wstring device;
    std::wstring _NAME;
    std::wstring project;

    std::wstring http_proxy; // HTTP プロキシ設定（v8/v11用）
    std::wstring https_proxy; // HTTPS プロキシ設定（v8/v11用）

    int chkResume = 0; // 0 or 1
    int chkCache = 0; // 0 or 1
    int chkUseHyp = 0; // 0 or 1
    int exist_ok = 0; // 0 or 1
    int chkUseProxy = 0; // 0 or 1

    // v8 only
    std::wstring task;    // detect/segment/pose/classify
    std::wstring backend; // "YOLOV5" or "YOLOV8" or "YOLO11"

public:
    TrainParams() : backend(L"yolov5"), workdir(L""), trainpy(L"train.py"), datayaml(L""), hypyaml(L""), cfgyaml(L""),
        weights(L""), python(L"python"), activate(L""), resume(L""), epochs(L"300"), patience(L"50"), batch(L"16"), imgsz(L"640"),
        device(L"0"), _NAME(L""), project(L"runs/train"), task(L"detect")
    {
        chkResume = 0;
        chkCache = 0;
        chkUseHyp = 0;
        exist_ok = 0;
    }

    //メソッド
    int ReadControls(HWND hDlg);
    int SaveCurrentSettingsToIni(HWND hDlg);
    void DoTrain();
//    void DoTrain_old();

};
