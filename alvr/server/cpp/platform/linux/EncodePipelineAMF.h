#pragma once
#include "EncodePipeline.h"
#include "ffmpeg_helper.h"

#include <functional>
#include <vulkan/vulkan.h>

#include "../win32/amf/public/common/AMFFactory.h"
#include "../win32/amf/public/include/components/VideoEncoderVCE.h"
#include "../win32/amf/public/include/components/VideoEncoderHEVC.h"
#include "../win32/amf/public/include/components/VideoConverter.h"
#include "../win32/amf/public/include/components/PreProcessing.h"
#include "../win32/amf/public/include/core/VulkanAMF.h"
#include "../win32/amf/public/common/AMFSTL.h"
#include "../win32/amf/public/common/Thread.h"

typedef std::function<void(amf::AMFDataPtr)> AMFDataReceiver;

class AMFPipeline;

class AMFPipe
{
public:
    AMFPipe(amf::AMFComponentPtr src, AMFDataReceiver receiver);
    virtual ~AMFPipe();

    void doPassthrough();
protected:
    amf::AMFComponentPtr m_amfComponentSrc;
    AMFDataReceiver m_receiver;
};

class AMFSolidPipe : public AMFPipe
{
public:
    AMFSolidPipe(amf::AMFComponentPtr src, amf::AMFComponentPtr dst);
protected:
    void Passthrough(amf::AMFDataPtr data);

    amf::AMFComponentPtr m_amfComponentDst;
};

class AMFPipeline
{
public:
    AMFPipeline();
    ~AMFPipeline();

    void Connect(AMFPipe *pipe);
    void Run();
protected:
    std::vector<AMFPipe*> m_pipes;
};

enum EncoderQualityPreset {
    QUALITY = 0,
    BALANCED = 1,
    SPEED = 2
};

namespace alvr
{

// Video encoder for AMD VCE.
class EncodePipelineAMF : public EncodePipeline
{
public:
    EncodePipelineAMF(VkContext &context, Renderer *render, VkFormat format, uint32_t width, uint32_t height);

    void PushFrame(uint64_t targetTimestampNs, bool idr) override;
    bool GetEncoded(std::vector<uint8_t> &out, uint64_t *pts) override;
    void SetBitrate(int64_t bitrate) override;

private:
    amf::AMFComponentPtr MakeConverter(amf::AMF_SURFACE_FORMAT inputFormat, int width, int height, amf::AMF_SURFACE_FORMAT outputFormat);
    amf::AMFComponentPtr MakePreprocessor(amf::AMF_SURFACE_FORMAT inputFormat, int width, int height);
    amf::AMFComponentPtr MakeEncoder(amf::AMF_SURFACE_FORMAT inputFormat, int width, int height, int codec, int refreshRate);
    amf::AMFFactory *m_amfFactory = nullptr;
    amf::AMFContextPtr m_amfContext;
    std::unique_ptr<AMFPipeline> m_pipeline;
    std::vector<amf::AMFComponentPtr> m_amfComponents;

    VkInstance m_vkInstance;
    VkPhysicalDevice m_vkPhysicalDevice;
    VkDevice m_vkDevice;
    VkQueue m_vkQueue;
    Renderer *m_render;

    bool m_use10bit;
    bool m_usePreProc;
    uint32_t m_preProcSigma;
    uint32_t m_preProcTor;
    EncoderQualityPreset m_encoderQualityPreset;
    amf::AMF_SURFACE_FORMAT m_surfaceFormat;

    int m_codec;
    int m_refreshRate;
    int m_renderWidth;
    int m_renderHeight;
    int m_bitrateInMBits;

    void Receive(amf::AMFDataPtr data);
    void ApplyFrameProperties(const amf::AMFSurfacePtr &surface, bool insertIDR);

    std::vector<uint8_t> m_outBuffer;
    uint64_t m_targetTimestampNs;
};

};
