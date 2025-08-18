#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <commdlg.h>   // GetOpenFileName

// ------------------------------
// ToolTip
// ------------------------------

inline HINSTANCE g_hInst = nullptr;

class Tooltip
{
    std::wstring title;
    HWND      hnd_ToolTip = nullptr;

public:
    void EnsureTooltip(HWND hDlg) {
        static bool s_inited = [] {
            INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_WIN95_CLASSES };
            InitCommonControlsEx(&icc);
            return true;
            }();

        if (hnd_ToolTip) return;

        hnd_ToolTip = CreateWindowExW(
            WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hDlg, nullptr, g_hInst, nullptr);
        if (!hnd_ToolTip) return;

        // �ܕԂ��E�x��
        SendMessageW(hnd_ToolTip, TTM_SETMAXTIPWIDTH, 0, 480);
        SendMessageW(hnd_ToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, 200);  // �z�o�[�J�n�܂Łims�j
        SendMessageW(hnd_ToolTip, TTM_SETDELAYTIME, TTDT_RESHOW, 50);    // �ĕ\���܂�
        SendMessageW(hnd_ToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 8000); // �����ŏ�����܂�

        // �o���[���̃^�C�g���i�C�Ӂj
        SendMessageW(hnd_ToolTip, TTM_SETTITLE, TTI_INFO, (LPARAM)title.c_str());
    }

    // �}�E�X�I�[�o�[�ł̃c�[���`�b�v�\��
    // 1) �ǉ��F�z�o�[�p�c�[����o�^����w���p
    void AddHoverTooltipForCtrl(HWND hDlg, int ctrlId, const wchar_t* _title, const wchar_t* _text)
    {
        title = _title ? _title : L""; // �^�C�g����ۑ��i�K�v�Ȃ��Ŏg���j

        EnsureTooltip(hDlg); // �c�[���`�b�v�쐬���e��ݒ�
        HWND hCtrl = GetDlgItem(hDlg, ctrlId);
        TOOLINFOW ti{};
        // v6 �}�j�t�F�X�g���������ɔz������ V1 �T�C�Y���g���iv6 �Ȃ� sizeof(TTTOOLINFOW) �ł�OK�j
        ti.cbSize = TTTOOLINFOW_V1_SIZE;
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND /*| TTF_TRANSPARENT*/;
        ti.hwnd = hDlg;                 // �e�i�_�C�A���O�j
        ti.uId = (UINT_PTR)hCtrl;      // �c�[�������̃R���g���[�� HWND
        ti.lpszText = const_cast<LPWSTR>(_text);

        SendMessageW(hnd_ToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    }
};

