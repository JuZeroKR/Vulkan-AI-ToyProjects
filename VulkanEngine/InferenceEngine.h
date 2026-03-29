#pragma once

#include "Context.h"
#include "Image2D.h"
#include <net.h>
#include <gpu.h>
#include <memory>
#include <string>

namespace caaVk {

    class InferenceEngine {
    public:
        InferenceEngine(Context& context);
        ~InferenceEngine();

        bool loadModel(const std::string& paramPath, const std::string& binPath);
        
        // Pass pixels for robust cross-context processing
        ncnn::Mat run(uint32_t width, uint32_t height, const unsigned char* pixelData);

        // Label management
        bool loadLabels(const std::string& path);
        std::string getLabel(int index) const;

    private:
        Context& context_;
        std::unique_ptr<ncnn::Net> net_;
        std::vector<std::string> labels_;
    };

}
