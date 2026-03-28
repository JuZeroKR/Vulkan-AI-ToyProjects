#include "Shader.h"
#include "Logger.h"
#include <algorithm>
#include <vector>

using namespace std;

namespace caaVk
{
    std::string extractFilename(const std::string& spvFilename)
    {
        if (spvFilename.length() < 4 || spvFilename.substr(spvFilename.length() - 4) != ".spv") {
            exitWithMessage("Shader file does not have .spv extension: {}", spvFilename);
        }

        // 寃쎈줈? 留덉?留?.spv ?쒓굅 ex: path/triangle.vert.spv -> triangle.vert
        size_t lastSlash = spvFilename.find_last_of("/\\");
        size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
        size_t end = spvFilename.length();
        size_t lastDot = spvFilename.find_last_of('.');
        if (lastDot != std::string::npos && lastDot > start)
            end = lastDot;

        return spvFilename.substr(start, end - start);
    }

    Shader::Shader(Context& ctx, string spvFilename) : ctx_(ctx)
    {
        name_ = extractFilename(spvFilename);

        auto shaderCode = readSpvFile(spvFilename);
        shaderModule_ = createShaderModule(shaderCode);
        reflectModule_ = createRefModule(shaderCode);
        stage_ = static_cast<VkShaderStageFlagBits>(reflectModule_.shader_stage);
    }

    Shader::Shader(Shader&& other) noexcept
        : ctx_(other.ctx_), // Copy reference (references can't be moved)
        shaderModule_(other.shaderModule_), reflectModule_(other.reflectModule_),
        stage_(other.stage_), name_(std::move(other.name_))
    {
        other.shaderModule_ = VK_NULL_HANDLE;
        other.reflectModule_ = {}; // Zero-initialize the struct
        other.stage_ = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }

    Shader::~Shader()
    {
        cleanup();
    }

    void Shader::cleanup()
    {
        if (reflectModule_._internal != nullptr) {
            spvReflectDestroyShaderModule(&reflectModule_);
        }
        if (shaderModule_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(ctx_.device(), shaderModule_, nullptr);
            shaderModule_ = VK_NULL_HANDLE;
        }
    }

    vector<char> Shader::readSpvFile(const string& spvFilename)
    {
        if (spvFilename.length() < 4 || spvFilename.substr(spvFilename.length() - 4) != ".spv") {
            exitWithMessage("Shader file does not have .spv extension: {}", spvFilename);
        }

        ifstream is(spvFilename, ios::binary | ios::in | ios::ate);
        if (!is.is_open()) {
            exitWithMessage("Could not open shader file: {}", spvFilename);
        }

        size_t shaderSize = static_cast<size_t>(is.tellg());
        if (shaderSize == 0 || shaderSize % 4 != 0) {
            exitWithMessage("Shader file size is invalid (must be >0 and multiple of 4): {}",
                spvFilename);
        }
        is.seekg(0, ios::beg);

        vector<char> shaderCode(shaderSize);
        is.read(shaderCode.data(), shaderSize);
        is.close();

        return shaderCode;
    }

    /*
    Create Shader Module with internal alignment fix
    */
    VkShaderModule Shader::createShaderModule(const vector<char>& shaderCode)
    {
        // Vulkan requires pCode to be aligned to 4 bytes.
        // We copy to a temporary vector<uint32_t> to guarantee this.
        vector<uint32_t> alignedCode(shaderCode.size() / 4);
        memcpy(alignedCode.data(), shaderCode.data(), shaderCode.size());

        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo shaderModuleCI{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shaderModuleCI.codeSize = shaderCode.size();
        shaderModuleCI.pCode = alignedCode.data();
        
        // ?붿뒪?ъ뿉 ?덈뒗 ?먯씠???뚯뒪(二쇰줈 SPIR-V 諛붿씠?덈━)瑜?GPU媛 ?댄빐?????덈룄濡?Vulkan 媛앹껜
        /*
        VkPipelineShaderStageCreateInfo : Vertex ?뱀? Fragment 泥섎━ 留↔?.
        */
        VkResult result = vkCreateShaderModule(ctx_.device(), &shaderModuleCI, nullptr, &shaderModule);
        if (result != VK_SUCCESS) {
            printLog("vkCreateShaderModule failed for shader: {} with result: {}", name_, getResultString(result));
            check(result);
        }

        return shaderModule;
    }

    /*
    Create Spirit-Reflect Module
    */
    SpvReflectShaderModule Shader::createRefModule(const vector<char>& shaderCode)
    {
        // Aligned copy for reflection as well
        vector<uint32_t> alignedCode(shaderCode.size() / 4);
        memcpy(alignedCode.data(), shaderCode.data(), shaderCode.size());

        SpvReflectShaderModule reflectShaderModule;
        SpvReflectResult reflectResult = spvReflectCreateShaderModule(
            shaderCode.size(), alignedCode.data(),
            &reflectShaderModule);
        if (reflectResult != SPV_REFLECT_RESULT_SUCCESS) {
            exitWithMessage("Failed to reflect shader module: {}",
                getSpvReflectResultString(reflectResult));
        }

        return reflectShaderModule;
    }

    vector<VkVertexInputAttributeDescription> Shader::makeVertexInputAttributeDescriptions() const
    {
        vector<VkVertexInputAttributeDescription> attributes;

        // Only meaningful for vertex shaders
        if (stage_ != VK_SHADER_STAGE_VERTEX_BIT) {
            exitWithMessage("Only for vertex shaders.");
            return attributes;
        }

        // Enumerate input variables
        uint32_t varCount = reflectModule_.input_variable_count;
        if (varCount == 0 || reflectModule_.input_variables == nullptr) {
            printLog("[Warning] No input variables found in shader: {}", name_);
            return attributes;
        }

        attributes.reserve(varCount);

        vector<const SpvReflectInterfaceVariable*> inputVars(reflectModule_.input_variables,
            reflectModule_.input_variables + varCount);
        sort(inputVars.begin(), inputVars.end(),
            [](const SpvReflectInterfaceVariable* a, const SpvReflectInterfaceVariable* b) {
                return a->location < b->location;
            }); // Sort by location

        // Example: match your struct layout
        // struct Vertex { float position[3]; float color[3]; };
        // print("{}, Stage: {}\n", name_, vkShaderStageFlagBitsToString(stage));
        uint32_t offset = 0;
        for (uint32_t i = 0; i < varCount; ++i) {
            const SpvReflectInterfaceVariable* var = inputVars[i];

            if (var->location == uint32_t(-1) || string(var->name) == "gl_VertexIndex") {
                // print("  No attribute\n"); // ?곗씠?붿뿉 in???녿뒗 寃쎌슦
                continue;
            }

            assert(i == var->location);

            VkVertexInputAttributeDescription desc = {};
            desc.location = var->location;
            desc.binding = 0;
            desc.format = getVkFormatFromSpvReflectFormat(var->format);
            desc.offset = offset; // Offset? ?섏쨷??怨꾩궛???덉젙

            attributes.push_back(desc);

            // print("  Attribute: name = {}, location = {}, binding = {}, format = {}, offset = {}\n",
            //       string(var->name), desc.location, desc.binding, vkFormatToString(desc.format),
            //       desc.offset);

            offset += getFormatSize(desc.format);
        }

        return attributes;
    }

    const char* getShaderStageString(SpvReflectShaderStageFlagBits stage)
    {
        switch (stage) {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
            return "Vertex";
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return "Tessellation Control";
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return "Tessellation Evaluation";
        case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
            return "Geometry";
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
            return "Fragment";
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
            return "Compute";
        default:
            return "Unknown";
        }
    }

    const char* getDescriptorTypeString(SpvReflectDescriptorType type)
    {
        switch (type) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return "Sampler";
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return "Combined Image Sampler";
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return "Sampled Image";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return "Storage Image";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return "Uniform Texel Buffer";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return "Storage Texel Buffer";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return "Uniform Buffer";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return "Storage Buffer";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return "Dynamic Uniform Buffer";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return "Dynamic Storage Buffer";
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return "Input Attachment";
        default:
            return "Unknown";
        }
    }


    void Shader::printReflectionInfo()
    {
        std::cout << "=== SPIR-V Shader Reflection Information ===" << std::endl;
        std::cout << "Entry Point: "
            << (reflectModule_.entry_point_name ? reflectModule_.entry_point_name : "Unknown") << std::endl;
        std::cout << "Shader Stage: " << getShaderStageString(reflectModule_.shader_stage) << std::endl;
        std::cout << "Source Language: ";
        switch (reflectModule_.source_language) {
        case SpvSourceLanguageGLSL:
            std::cout << "GLSL";
            break;
        case SpvSourceLanguageHLSL:
            std::cout << "HLSL";
            break;
        case SpvSourceLanguageOpenCL_C:
            std::cout << "OpenCL C";
            break;
        default:
            std::cout << "Unknown (" << reflectModule_.source_language << ")";
            break;
        }
        std::cout << " v" << reflectModule_.source_language_version << std::endl;

        if (reflectModule_.source_file) {
            std::cout << "Source File: " << reflectModule_.source_file << std::endl;
        }

        std::cout << "\n--- Descriptor Bindings ---" << std::endl;
        std::cout << "Total descriptor bindings: " << reflectModule_.descriptor_binding_count << std::endl;

        for (uint32_t i = 0; i < reflectModule_.descriptor_binding_count; ++i) {
            const SpvReflectDescriptorBinding* binding = &reflectModule_.descriptor_bindings[i];
            std::cout << "  Binding " << i << ":" << std::endl;
            std::cout << "    Name: " << (binding->name ? binding->name : "Unknown") << std::endl;
            std::cout << "    Set: " << binding->set << std::endl;
            std::cout << "    Binding: " << binding->binding << std::endl;
            std::cout << "    Type: " << getDescriptorTypeString(binding->descriptor_type) << std::endl;
            std::cout << "    Count: " << binding->count << std::endl;

            if (binding->image.dim != SpvDimMax) {
                std::cout << "    Image Dimension: ";
                switch (binding->image.dim) {
                case SpvDim1D:
                    std::cout << "1D";
                    break;
                case SpvDim2D:
                    std::cout << "2D";
                    break;
                case SpvDim3D:
                    std::cout << "3D";
                    break;
                case SpvDimCube:
                    std::cout << "Cube";
                    break;
                case SpvDimBuffer:
                    std::cout << "Buffer";
                    break;
                default:
                    std::cout << "Unknown";
                    break;
                }
                std::cout << std::endl;
                std::cout << "    Image Format: " << binding->image.image_format << std::endl;
            }
        }

        std::cout << "\n--- Descriptor Sets ---" << std::endl;
        std::cout << "Total descriptor sets: " << reflectModule_.descriptor_set_count << std::endl;
        for (uint32_t i = 0; i < reflectModule_.descriptor_set_count; ++i) {
            const SpvReflectDescriptorSet* set = &reflectModule_.descriptor_sets[i];
            std::cout << "  Set " << set->set << ": " << set->binding_count << " bindings" << std::endl;
        }

        std::cout << "\n--- Input Variables ---" << std::endl;
        std::cout << "Total input variables: " << reflectModule_.input_variable_count << std::endl;
        for (uint32_t i = 0; i < reflectModule_.input_variable_count; ++i) {
            const SpvReflectInterfaceVariable* var = reflectModule_.input_variables[i];
            std::cout << "  Input " << i << ": " << (var->name ? var->name : "Unknown") << std::endl;
            std::cout << "    Location: " << var->location << std::endl;
        }

        std::cout << "\n--- Output Variables ---" << std::endl;
        std::cout << "Total output variables: " << reflectModule_.output_variable_count << std::endl;
        for (uint32_t i = 0; i < reflectModule_.output_variable_count; ++i) {
            const SpvReflectInterfaceVariable* var = reflectModule_.output_variables[i];
            std::cout << "  Output " << i << ": " << (var->name ? var->name : "Unknown") << std::endl;
            std::cout << "    Location: " << var->location << std::endl;
        }

        std::cout << "\n--- Push Constants ---" << std::endl;
        std::cout << "Total push constant blocks: " << reflectModule_.push_constant_block_count << std::endl;
        for (uint32_t i = 0; i < reflectModule_.push_constant_block_count; ++i) {
            const SpvReflectBlockVariable* block = &reflectModule_.push_constant_blocks[i];
            std::cout << "  Push constant block " << i << ":" << std::endl;
            std::cout << "    Name: " << (block->name ? block->name : "Unknown") << std::endl;
            std::cout << "    Size: " << block->size << " bytes" << std::endl;
            std::cout << "    Offset: " << block->offset << std::endl;
        }

        // For compute shaders, show workgroup size
        if (reflectModule_.shader_stage & SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT) {
            std::cout << "\n--- Compute Shader Info ---" << std::endl;
            std::cout << "Local workgroup size: (" << reflectModule_.entry_points[0].local_size.x << ", "
                << reflectModule_.entry_points[0].local_size.y << ", "
                << reflectModule_.entry_points[0].local_size.z << ")" << std::endl;
        }
    }

    
}
