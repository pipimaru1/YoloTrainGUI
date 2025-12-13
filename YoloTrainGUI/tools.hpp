#pragma once




std::wstring GetComboText(HWND hCombo);
void SetComboText(HWND hCombo, const std::wstring& s);

uint64_t CountFiles(const fs::path& root);
bool EnsureDir(const fs::path& p);

//bool PickFolder(HWND owner, std::wstring& outPath, const std::wstring& initialDir = L"");
bool PickFolder(HWND owner, std::wstring& outPath);
// ディレクトリ選択ダイアログを表示する
//bool PickFolderEx(HWND owner, std::wstring& outPath, const std::wstring& initialDir, std::wstring _title);
// コンボボックスからディレクトリを選択して、ディレクトリ選択ダイアログを表示する
//bool PickFolderEx(HWND hDlg, UINT _ComboID, std::wstring title=L"");

bool PickFile(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec, std::wstring& outPath);
bool PickFileEx(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec,std::wstring& outPath, const std::wstring& initialDir, std::wstring _title);
bool PickFileEx(HWND hDlg, UINT _ComboID, const COMDLG_FILTERSPEC* spec, std::wstring title = L"");

// YoloTrainGUI.h に移動
//bool PickFolderEx1(HWND owner, std::wstring& outPath, std::wstring initialDir, std::wstring _title);
//bool PickFolderEx2(HWND hDlg, UINT _ComboID, std::wstring _title);//上記関数のラッパー
//bool PickFolderEx3(HWND hDlg, UINT _ComboID, std::wstring _title, std::wstring _SECSTR);


bool UpdateYoloYamlTrainVal(
    const std::filesystem::path& yamlPath,
    const std::wstring& baseDirW, // 例: L"C:\\yolodata"
    std::wstring& outMessage      // ログ用（UIに出すなど）
);

void SplitDataset(const fs::path& sourceRoot, const fs::path& destRoot,
    int trainPercent, double reductionFactor, 
    bool _shuffle,  //= true, 
    int _splitunit  // = 1
);

std::string ToUTF8(const std::wstring& w);
std::wstring FromUTF8(const std::string& s);

// MRU utilities
std::vector<std::wstring> LoadMRU(const std::wstring& section);
void SaveMRU(const std::wstring& section, const std::wstring& value, size_t maxItems);
void LoadMRUToCombo(HWND hCombo, const std::wstring& section);
void LoadMRUToTextControl(HWND hEditText, const std::wstring& section);
std::wstring LoadMRUToString(const std::wstring& section);
bool LoadFlagFromIni(const wchar_t* key, bool def); // = false)
