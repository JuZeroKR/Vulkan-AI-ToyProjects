#include "VulkanTool.h"
#include "Logger.h"
#include <unordered_map>


namespace caaVk {

    void check(VkResult result)
    {
        if (result != VK_SUCCESS) {
            exitWithMessage("[Error] {} {} {}", getResultString(result), __FILE__, __LINE__);
        }
    }

    string getResultString(VkResult errorCode)
    {
        switch (errorCode) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
        default:
            return "UNKNOWN_ERROR";
        }
    }

    VkDescriptorType stringToDescriptorType(const string& typeStr)
    {
        static const unordered_map<string, VkDescriptorType> stringToTypeMap = {
            {"SAMPLER", VK_DESCRIPTOR_TYPE_SAMPLER},
            {"COMBINED_IMAGE_SAMPLER", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
            {"SAMPLED_IMAGE", VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
            {"STORAGE_IMAGE", VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
            {"UNIFORM_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER},
            {"STORAGE_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER},
            {"UNIFORM_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
            {"STORAGE_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
            {"UNIFORM_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC},
            {"STORAGE_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC},
            {"INPUT_ATTACHMENT", VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT},
            {"INLINE_UNIFORM_BLOCK", VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK},
            {"ACCELERATION_STRUCTURE_KHR", VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR},
            {"ACCELERATION_STRUCTURE_NV", VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV},
            {"SAMPLE_WEIGHT_IMAGE_QCOM", VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM},
            {"BLOCK_MATCH_IMAGE_QCOM", VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM},
            {"MUTABLE_EXT", VK_DESCRIPTOR_TYPE_MUTABLE_EXT} };

        auto it = stringToTypeMap.find(typeStr);
        if (it != stringToTypeMap.end()) {
            return it->second;
        }

        exitWithMessage("Invalid type {}", typeStr);
        return VK_DESCRIPTOR_TYPE_MAX_ENUM; // Invalid type
    }

    auto getSpvReflectResultString(SpvReflectResult result) -> string
    {
        switch (result) {
        case SPV_REFLECT_RESULT_SUCCESS: return "SUCCESS";
        case SPV_REFLECT_RESULT_NOT_READY: return "NOT_READY";
        case SPV_REFLECT_RESULT_ERROR_PARSE_FAILED: return "PARSE_FAILED";
        case SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED: return "ALLOC_FAILED";
        case SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED: return "RANGE_EXCEEDED";
        case SPV_REFLECT_RESULT_ERROR_NULL_POINTER: return "NULL_POINTER";
        case SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR: return "INTERNAL_ERROR";
        case SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH: return "COUNT_MISMATCH";
        case SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND: return "ELEMENT_NOT_FOUND";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE: return "INVALID_CODE_SIZE";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER: return "INVALID_MAGIC_NUMBER";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF: return "UNEXPECTED_EOF";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE: return "INVALID_ID_REFERENCE";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW: return "SET_NUMBER_OVERFLOW";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS: return "INVALID_STORAGE_CLASS";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION: return "RECURSION";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION: return "INVALID_INSTRUCTION";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA: return "UNEXPECTED_BLOCK_DATA";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE: return "INVALID_BLOCK_MEMBER_REFERENCE";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT: return "INVALID_ENTRY_POINT";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE: return "INVALID_EXECUTION_MODE";
        default: return "UNKNOWN";
        }
    }

    auto getVkFormatFromSpvReflectFormat(SpvReflectFormat format) -> VkFormat
    {
        return static_cast<VkFormat>(format);
    }

    bool BindingComp::operator()(const vector<VkDescriptorSetLayoutBinding>& lhs,
        const vector<VkDescriptorSetLayoutBinding>& rhs) const
    {
        if (lhs.size() != rhs.size()) {
            return lhs.size() < rhs.size();
        }
        for (size_t i = 0; i < lhs.size(); i++) {
            if (lhs[i].binding != rhs[i].binding) return lhs[i].binding < rhs[i].binding;
            if (lhs[i].descriptorType != rhs[i].descriptorType) return lhs[i].descriptorType < rhs[i].descriptorType;
            if (lhs[i].descriptorCount != rhs[i].descriptorCount) return lhs[i].descriptorCount < rhs[i].descriptorCount;
            if (lhs[i].stageFlags != rhs[i].stageFlags) return lhs[i].stageFlags < rhs[i].stageFlags;
        }
        return false;
    }

    size_t BindingHash::operator()(const vector<VkDescriptorSetLayoutBinding>& bindings) const
    {
        size_t h = 0;
        for (const auto& b : bindings) {
            h ^= hash<uint32_t>{}(b.binding) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash<uint32_t>{}(static_cast<uint32_t>(b.descriptorType)) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash<uint32_t>{}(b.descriptorCount) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash<uint32_t>{}(static_cast<uint32_t>(b.stageFlags)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    bool BindingEqual::operator()(const vector<VkDescriptorSetLayoutBinding>& lhs,
        const vector<VkDescriptorSetLayoutBinding>& rhs) const
    {
        if (lhs.size() != rhs.size()) return false;
        for (size_t i = 0; i < lhs.size(); i++) {
            if (lhs[i].binding != rhs[i].binding) return false;
            if (lhs[i].descriptorType != rhs[i].descriptorType) return false;
            if (lhs[i].descriptorCount != rhs[i].descriptorCount) return false;
            if (lhs[i].stageFlags != rhs[i].stageFlags) return false;
        }
        return true;
    }

    auto getFormatSize(VkFormat format) -> uint32_t
    {
        switch (format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
            return 1;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        default:
            return 0;
        }
    }
}
