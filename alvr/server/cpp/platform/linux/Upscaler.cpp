#include "Upscaler.h"
#include "alvr_server/bindings.h"

#include <math.h>

#define A_CPU
#include "shader/ffx_a.h"
#include "shader/ffx_fsr1.h"

Upscaler::Upscaler(Renderer *render, Method method, VkFormat format, uint32_t width, uint32_t height, float scale, uint32_t sharpness)
    : r(render)
    , m_method(method)
    , m_width(width)
    , m_height(height)
    , m_scale(scale)
    , m_sharpness(sharpness)
{
    m_output.width = m_width * m_scale; // align?
    m_output.height = m_height * m_scale; // align?
    if (m_method == FSR) {
        initFSR();
    } else if (m_method == NIS) {
        initNIS();
    } else {
        throw std::runtime_error("Upscaler: Unknown method");
    }
}

Upscaler::Output Upscaler::GetOutput()
{
    return m_output;
}

std::vector<RenderPipeline*> Upscaler::GetPipelines()
{
    return m_pipelines;
}

void Upscaler::initFSR()
{
    std::vector<VkSpecializationMapEntry> entries;

    // EASU
    RenderPipeline *easu = new RenderPipeline(r);
    easu->SetShader(FSR_EASU_SHADER_COMP_SPV_PTR, FSR_EASU_SHADER_COMP_SPV_LEN);

    FsrEasuCon(&m_fsrEasuConstants.con0, &m_fsrEasuConstants.con1, &m_fsrEasuConstants.con2, &m_fsrEasuConstants.con3,
               m_width, m_height, m_width, m_height, m_output.width, m_output.height);

#define ENTRY(x) entries.push_back({(uint32_t)entries.size(), offsetof(FsrEasuConstants, x), sizeof(FsrEasuConstants::x)});
    ENTRY(con0);
    ENTRY(con0_1);
    ENTRY(con0_2);
    ENTRY(con0_3);
    ENTRY(con1);
    ENTRY(con1_1);
    ENTRY(con1_2);
    ENTRY(con1_3);
    ENTRY(con2);
    ENTRY(con2_1);
    ENTRY(con2_2);
    ENTRY(con2_3);
    ENTRY(con3);
    ENTRY(con3_1);
    ENTRY(con3_2);
    ENTRY(con3_3);
#undef ENTRY

    easu->SetConstants(&m_fsrEasuConstants, sizeof(m_fsrEasuConstants), std::move(entries));
    easu->SetPixelsPerGroup(16, 16);

    m_pipelines.push_back(easu);

    // RCAS
    RenderPipeline *rcas = new RenderPipeline(r);
    rcas->SetShader(FSR_RCAS_SHADER_COMP_SPV_PTR, FSR_RCAS_SHADER_COMP_SPV_LEN);

    FsrRcasCon(&m_fsrRcasConstants.con0, m_sharpness / 10.0);

#define ENTRY(x) entries.push_back({(uint32_t)entries.size(), offsetof(FsrRcasConstants, x), sizeof(FsrRcasConstants::x)});
    ENTRY(con0);
    ENTRY(con0_1);
    ENTRY(con0_2);
    ENTRY(con0_3);
#undef ENTRY

    rcas->SetConstants(&m_fsrEasuConstants, sizeof(m_fsrEasuConstants), std::move(entries));
    rcas->SetPixelsPerGroup(16, 16);

    m_pipelines.push_back(rcas);
}

void Upscaler::initNIS()
{
}
