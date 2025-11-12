#define _WIN32_WINNT 0x0601
#include "pch.h"
#include "tools.hpp"
#include "resource.h"
//#include "resource_user.h"
#include "YoloTrainGUI.h"
#include "Tooltip.hpp"


// ------------------------------
// Split Dataset (based on your divfiles.cpp)  filecite：使用している分割ロジックは添付コードに準拠
// ------------------------------
void SplitDataset(
    const fs::path& sourceRoot, 
    const fs::path& destRoot,
    int trainPercent, 
    double reductionFactor, 
    bool _shuffle,
    int _splitunit)
{
    fs::path srcImages = sourceRoot / "images";
    fs::path srcLabels = sourceRoot / "labels";

    fs::path trainImages = destRoot / "train" / "images";
    fs::path trainLabels = destRoot / "train" / "labels";
    fs::path validImages = destRoot / "valid" / "images";
    fs::path validLabels = destRoot / "valid" / "labels";

    EnsureDir(trainImages); EnsureDir(trainLabels);
    EnsureDir(validImages); EnsureDir(validLabels);

    // collect *.jpg / *.JPG
    std::vector<fs::path> jpgFiles;
    for (auto& e : fs::directory_iterator(srcImages)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        if (ext == L".jpg") jpgFiles.push_back(e.path());
    }

    // shuffle
    if (_shuffle)
    {
        std::random_device rd; std::mt19937 g(rd());
        std::shuffle(jpgFiles.begin(), jpgFiles.end(), g);
		AppendLog(L"[SPLIT] Shuffled dataset.");
        AppendLog(RET);
    }

	// reduction データの間引き(0.001～1.0)
    if (reductionFactor < 0.001) 
        reductionFactor = 0.001;
    if (reductionFactor > 1.0) 
        reductionFactor = 1.0;
    size_t total = jpgFiles.size();
    size_t used = (size_t)std::ceil(total * reductionFactor);
    if (used < jpgFiles.size()){
        jpgFiles.resize(used); //データの切り捨て
        AppendLog(L"[SPLIT] Reduced dataset: total=" + std::to_wstring(total) +
            L" used=" + std::to_wstring(used) +
			L" factor=" + std::to_wstring(reductionFactor));
        AppendLog(RET);
    }

    // train count
    if (trainPercent < 1) 
        trainPercent = 1;
    if (trainPercent > 99) 
        trainPercent = 99;
    
    ///////////////////////////////////////////////////////////
    // シャッフルしてから先頭N%をtrain、残りをvalid 
    // 古いコードで必ずしもこのコードを使わなくてもいいが
    // 実績があるので念のため残す
	// _splitunit は無視される
	if (_shuffle)
    {
        size_t nTrain = (size_t)std::ceil(used * (trainPercent / 100.0));
        // copy train
        for (size_t i = 0; i < nTrain && i < jpgFiles.size(); ++i) {
            const auto& img = jpgFiles[i];
            fs::copy_file(img, trainImages / img.filename(), fs::copy_options::overwrite_existing);
            fs::path lblSrc = srcLabels / (img.stem().wstring() + L".txt");
            if (fs::exists(lblSrc)) {
                fs::copy_file(lblSrc, trainLabels / lblSrc.filename(), fs::copy_options::overwrite_existing);
            }
        }
        // copy valid (rest)
        for (size_t i = nTrain; i < jpgFiles.size(); ++i) {
            const auto& img = jpgFiles[i];
            fs::copy_file(img, validImages / img.filename(), fs::copy_options::overwrite_existing);
            fs::path lblSrc = srcLabels / (img.stem().wstring() + L".txt");
            if (fs::exists(lblSrc)) {
                fs::copy_file(lblSrc, validLabels / lblSrc.filename(), fs::copy_options::overwrite_existing);
            }
        }
        AppendLog(L"[SPLIT] Done. total=" + std::to_wstring(total) +
            L" used=" + std::to_wstring(used) +
            L" train=" + std::to_wstring(nTrain) +
            L" valid=" + std::to_wstring(used - nTrain));
        AppendLog(RET);
    }
    else //誤差蓄積法でファイルをN個おきに分ける
    {
        float _r = (float)trainPercent / 100.0f;
        float acc = 0.0f;

        for (size_t i = 0; i < jpgFiles.size(); ++i)
        {
            acc += _r;
            if (acc >= 1.0)
            {
                const auto& img = jpgFiles[i];
                fs::copy_file(img, trainImages / img.filename(), fs::copy_options::overwrite_existing);
                fs::path lblSrc = srcLabels / (img.stem().wstring() + L".txt");
                if (fs::exists(lblSrc)) {
                    fs::copy_file(lblSrc, trainLabels / lblSrc.filename(), fs::copy_options::overwrite_existing);
                }
                acc -= 1.0f;
            }
            else
            {
                const auto& img = jpgFiles[i];
                fs::copy_file(img, validImages / img.filename(), fs::copy_options::overwrite_existing);
                fs::path lblSrc = srcLabels / (img.stem().wstring() + L".txt");
                if (fs::exists(lblSrc)) {
                    fs::copy_file(lblSrc, validLabels / lblSrc.filename(), fs::copy_options::overwrite_existing);
                }
            }
        }
    }
}
