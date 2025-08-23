#define _WIN32_WINNT 0x0601
#include "pch.h"

#include "tools.hpp"

// ------------------------------
// ComboBox helpers
// ------------------------------

// �R���{�{�b�N�X�̃e�L�X�g���擾�E�ݒ肷��
std::wstring GetComboText(HWND hCombo)
{
    int len = (int)SendMessageW(hCombo, WM_GETTEXTLENGTH, 0, 0);
    std::wstring s(len, L'\0');
    SendMessageW(hCombo, WM_GETTEXT, len + 1, (LPARAM)s.data());
    return s;
}
void SetComboText(HWND hCombo, const std::wstring& s)
{
    SendMessageW(hCombo, WM_SETTEXT, 0, (LPARAM)s.c_str());
}


// ------------------------------
// IFileDialog helpers
// ------------------------------

//bool PickFolder(HWND owner, std::wstring& outPath, const std::wstring& initialDir)
bool PickFolder(HWND owner, std::wstring& outPath)
{
    IFileDialog* pfd = nullptr;
	// IFileDialog�̃C���X�^���X���쐬����
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) 
        return false;
    else{
		// �_�C�A���O�{�b�N�X�̃I�v�V�������擾���āA�t�H���_�I�����[�h�ɕύX����
        DWORD opts = 0; pfd->GetOptions(&opts);
        pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

		// �����\���t�H���_�̎w��i�ȗ��j
        //if (!initialDir.empty()) 
        //{
        //    IShellItem* psiInit = nullptr;
        //    if (SUCCEEDED(SHCreateItemFromParsingName(initialDir.c_str(), nullptr, IID_PPV_ARGS(&psiInit)))) {
        //        // �����ڂ����w��F���[�U�[�����t�H���_�ɂ��ړ��ł���
        //        pfd->SetDefaultFolder(psiInit);
        //        // �u�K�����̃t�H���_���J���ĊJ�n�������v�ꍇ�� SetFolder ��
        //        // pfd->SetFolder(psiInit);
        //        psiInit->Release();
        //    }
        //}

        hr = pfd->Show(owner);
        if (FAILED(hr))
        {
            pfd->Release();
            return false;
        }else{
            IShellItem* psi = nullptr;
            hr = pfd->GetResult(&psi);
            if (FAILED(hr))
            {
                pfd->Release();
                return false;
            }else{
                PWSTR psz = nullptr;
                psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
                if (psz)
                {
                    outPath = psz;
                    CoTaskMemFree(psz);
                }else{
                    psi->Release();
                    pfd->Release();
                    return !outPath.empty();
                }
            }
        }
    }
}

// �g���ŁF�����t�H���_�w��A�^�C�g���ύX�Ή�
static bool PickFolderEx(HWND owner, std::wstring& outPath, const std::wstring& initialDir, std::wstring _title)
{
    Microsoft::WRL::ComPtr<IFileDialog> pfd;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
        return false;

    DWORD opts = 0; pfd->GetOptions(&opts);
    pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

    // ���^�C�g����ύX
    pfd->SetTitle(_title.c_str());

    // 1) �����t�H���_������
    if (!initialDir.empty() && std::filesystem::exists(initialDir)) {
        // ���[�g�� "C:\" �̂悤�ɖ��� '\' ��K���t����
        std::wstring dir = initialDir;
        if (dir.size() == 2 && dir[1] == L':') dir += L'\\';

        Microsoft::WRL::ComPtr<IShellItem> psiInit;
        if (SUCCEEDED(SHCreateItemFromParsingName(dir.c_str(), nullptr, IID_PPV_ARGS(&psiInit)))) {
            // �u�K�����̃t�H���_�ŊJ���v
            pfd->SetFolder(psiInit.Get());
            // ����ł� OS ���̗��������P�[�X�ւ̕ی��Ƃ��Ċ���l���d�˂Ă���
            pfd->SetDefaultFolder(psiInit.Get());
        }
    }

    // �O��t�@�C�������c���Ă���Ƌz���邱�Ƃ�����̂Ŗ����N���A
    pfd->SetFileName(L"");

    if (FAILED(pfd->Show(owner))) return false;

    Microsoft::WRL::ComPtr<IShellItem> psi;
    if (FAILED(pfd->GetResult(&psi))) return false;

    PWSTR psz = nullptr;
    psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
    if (!psz) return false;
    outPath = psz; CoTaskMemFree(psz);
    return !outPath.empty();
}

//��L�֐��̃��b�p�[
bool PickFolderEx(HWND hDlg, UINT _ComboID, std::wstring _title)
{
    std::wstring _p;
    std::wstring _initdir = GetComboText(GetDlgItem(hDlg, _ComboID));

    if (PickFolderEx(hDlg, _p, _initdir, _title))
    {
        SetComboText(GetDlgItem(hDlg, _ComboID), _p);
        return true;
    }

    return false;
}


// �t�@�C���I���_�C�A���O��\������
bool PickFile(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec, std::wstring& outPath)
{
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) return false;
    pfd->SetFileTypes(nSpec, spec);
    hr = pfd->Show(owner);
    if (FAILED(hr)) { pfd->Release(); return false; }
    IShellItem* psi = nullptr;
    hr = pfd->GetResult(&psi);
    if (FAILED(hr)) { pfd->Release(); return false; }
    PWSTR psz = nullptr;
    psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
    if (psz) { outPath = psz; CoTaskMemFree(psz); }
    psi->Release();
    pfd->Release();
    return !outPath.empty();
}

// �g���ŁF�����t�H���_�w��A�^�C�g���ύX�Ή�
 bool PickFileEx(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec,
    std::wstring& outPath, const std::wstring& initialDir, std::wstring _title)
{
    Microsoft::WRL::ComPtr<IFileDialog> pfd;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
        return false;

    pfd->SetFileTypes(nSpec, spec);

    if (!initialDir.empty() && std::filesystem::exists(initialDir)) {
        std::wstring dir = initialDir;
        if (dir.size() == 2 && dir[1] == L':') 
            dir += L'\\';
        Microsoft::WRL::ComPtr<IShellItem> psiInit;
        if (SUCCEEDED(SHCreateItemFromParsingName(dir.c_str(), nullptr, IID_PPV_ARGS(&psiInit)))) {
            pfd->SetFolder(psiInit.Get());
            pfd->SetDefaultFolder(psiInit.Get());
        }
    }
    pfd->SetFileName(L""); // �O�̂���

    // ���^�C�g����ύX
    pfd->SetTitle(_title.c_str());

    if (FAILED(pfd->Show(owner))) 
        return false;

    Microsoft::WRL::ComPtr<IShellItem> psi;
    if (FAILED(pfd->GetResult(&psi))) 
        return false;

    PWSTR psz = nullptr;
    psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
    if (!psz) 
        return false;
    outPath = psz; CoTaskMemFree(psz);
    return !outPath.empty();
}
 bool PickFileEx(HWND hDlg, UINT _ComboID, const COMDLG_FILTERSPEC* spec, std::wstring title)
 {
     std::wstring _p;
     std::wstring _initdir = GetComboText(GetDlgItem(hDlg, _ComboID));
     if (PickFileEx(hDlg, spec, 1, _p, _initdir, title))
     {
         SetComboText(GetDlgItem(hDlg, _ComboID), _p);
         return true;
     }
     return false;
 }