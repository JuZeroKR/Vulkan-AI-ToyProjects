#include "InferenceEngine.h"
#include "Logger.h"
#include <fstream>
#include <string>

namespace caaVk {

    InferenceEngine::InferenceEngine(Context& context)
        : context_(context)
    {
        // Pure CPU Mode for AI (prevents all Vulkan initialization conflicts)
        net_ = std::make_unique<ncnn::Net>();
        net_->opt.use_vulkan_compute = false;
    }

    InferenceEngine::~InferenceEngine() {
        if (net_) net_->clear();
    }

    bool InferenceEngine::loadModel(const std::string& paramPath, const std::string& binPath) {
        if (net_->load_param(paramPath.c_str()) != 0) {
            return false;
        }
        if (net_->load_model(binPath.c_str()) != 0) {
            return false;
        }
        return true;
    }

    ncnn::Mat InferenceEngine::run(uint32_t width, uint32_t height, const unsigned char* pixelData) {
        // 1. Create ncnn::Mat from pixels (RGBA to RGB conversion)
        // Image2D uses RGBA (4 bytes), so we must use PIXEL_RGBA2RGB
        ncnn::Mat in = ncnn::Mat::from_pixels(pixelData, ncnn::Mat::PIXEL_RGBA2RGB, width, height);

        // 2. Normalization (EfficientNet-B0 / ImageNet standard)
        const float mean_vals[3] = { 123.675f, 116.28f, 103.53f };
        const float norm_vals[3] = { 0.01712475f, 0.017507f, 0.017429f };
        in.substract_mean_normalize(mean_vals, norm_vals);

        // 3. Prepare extractor
        ncnn::Extractor ex = net_->create_extractor();
        
        // 4. Set input (use the correct blob name from the new model: 'in0')
        ex.input("in0", in);

        // 5. Extract output (use the correct blob name from the new model: 'out0')
        ncnn::Mat out;
        if (ex.extract("out0", out) != 0) {
            return ncnn::Mat();
        }

        return out;
    }

    bool InferenceEngine::loadLabels(const std::string& path) {
        labels_.clear();
        std::ifstream f(path);
        if (!f.is_open()) return false;

        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) {
                labels_.push_back(line);
            }
        }
        return !labels_.empty();
    }

    std::string InferenceEngine::getLabel(int index) const {
        if (index >= 0 && index < static_cast<int>(labels_.size())) {
            return labels_[index];
        }
        return "Class " + std::to_string(index);
    }

}
