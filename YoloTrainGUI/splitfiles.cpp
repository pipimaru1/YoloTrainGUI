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
    else //誤差蓄積法でファイルをN個おきに分ける splitunit単位
    {
        const float r = static_cast<float>(trainPercent) / 100.0f;
        float acc = 0.0f;

		int count_train_image = 0;
        int count_train_label = 0;
		int count_valid_image = 0;
		int count_valid_label = 0;
		bool _ret = false;

        // 0 以下が来ても壊れないようにガード
        int splitunit = _splitunit;
        if (splitunit <= 0) splitunit = 1;

        size_t i = 0;
        const size_t N = jpgFiles.size();

        while (i < N)
        {
            // このブロックの先頭とサイズ
            size_t blockBegin = i;
            size_t blockSize = std::min<size_t>(splitunit, N - blockBegin);

            // 誤差拡散で「このブロック train か valid か」を決定
            acc += r;
            bool blockIsTrain = false;
            if (acc >= 1.0f) {
                blockIsTrain = true;
                acc -= 1.0f;
            }

            // ブロック内のファイルを一気にコピー（なるべく連続）
            for (size_t j = 0; j < blockSize; ++j, ++i)
            {
                const auto& img = jpgFiles[i];

                fs::path lblSrc = srcLabels / (img.stem().wstring() + L".txt");

                if (blockIsTrain) {
                    _ret=fs::copy_file(img, trainImages / img.filename(),
                        fs::copy_options::overwrite_existing);
                    if (_ret)
                        ++count_train_image;

                    if (fs::exists(lblSrc)) {
                        _ret = fs::copy_file(lblSrc, trainLabels / lblSrc.filename(),
                            fs::copy_options::overwrite_existing);
                        if (_ret)
							++count_train_label;
                    }
                }
                else {
                    _ret = fs::copy_file(img, validImages / img.filename(),
                        fs::copy_options::overwrite_existing);
                    if (_ret)
						++count_valid_image;

                    if (fs::exists(lblSrc)) {
                        _ret = fs::copy_file(lblSrc, validLabels / lblSrc.filename(),
                            fs::copy_options::overwrite_existing);
                        if (_ret)
							++count_valid_label;
                    }
                }
            }
        }
        AppendLog(L"[SPLIT] Done. total=" + std::to_wstring(total) +
            L" used=" + std::to_wstring(used));
        AppendLog(RET);

        AppendLog(L"[SPLIT] Done. Cpied total=" + std::to_wstring(
            count_train_image + count_valid_image + count_train_label + count_valid_label
        ));
        AppendLog(RET);

        AppendLog(L" train_image=" + std::to_wstring(count_train_image) + RET);
        AppendLog(L" train_label=" + std::to_wstring(count_train_label) + RET);
        AppendLog(L" valid_image=" + std::to_wstring(count_valid_image) + RET);
        AppendLog(L" valid_label=" + std::to_wstring(count_valid_label) + RET);

		float actualTrainPct = count_train_image/(float)(count_train_image + count_valid_image) * 100.0f;
		AppendLog(L"[SPLIT] Actual train%=" + std::to_wstring(actualTrainPct) + L" %"+RET);
    }

    /*
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
    */
}
