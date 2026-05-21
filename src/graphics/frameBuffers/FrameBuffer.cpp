#include "Framebuffer.h"
#include <iostream>


using namespace Lengine;


static bool IsMultisampled(uint32_t samples)
{
    return samples > 1;
}

Framebuffer::Framebuffer(const FramebufferSpecification& spec)
    : m_Spec(spec)
{
    Invalidate();
}

Framebuffer::~Framebuffer()
{
    Destroy();
}

void Framebuffer::Invalidate()
{
    if (m_FBO)
        Destroy();

    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    bool multisample = IsMultisampled(m_Spec.samples);

    // ---------- Color Attachments ----------
    m_ColorAttachments.resize(m_Spec.colorAttachmentCount);
    glGenTextures(m_Spec.colorAttachmentCount, m_ColorAttachments.data());

    for (uint32_t i = 0; i < m_Spec.colorAttachmentCount; i++)
    {
        if (multisample)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachments[i]);
            glTexImage2DMultisample(
                GL_TEXTURE_2D_MULTISAMPLE,
                m_Spec.samples,
                m_Spec.colorFormats[i],
                m_Spec.width,
                m_Spec.height,
                GL_TRUE
            );

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0 + i,
                GL_TEXTURE_2D_MULTISAMPLE,
                m_ColorAttachments[i],
                0
            );
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[i]);

            GLenum format = GetFormatFromInternal(m_Spec.colorFormats[i]);
            GLenum type = GetTypeFromInternal(m_Spec.colorFormats[i]);

            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                m_Spec.colorFormats[i],  // internal format
                m_Spec.width,
                m_Spec.height,
                0,
                format,
                type,
                nullptr
            );

            GLenum filter =
                m_Spec.filtering.empty()
                ? GL_LINEAR
                : m_Spec.filtering[i];

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);


            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0 + i,
                GL_TEXTURE_2D,
                m_ColorAttachments[i],
                0
            );
        }
    }

    // Draw buffers
    std::vector<GLenum> buffers;
    for (uint32_t i = 0; i < m_Spec.colorAttachmentCount; i++)
        buffers.push_back(GL_COLOR_ATTACHMENT0 + i);

    glDrawBuffers((GLsizei)buffers.size(), buffers.data());

    // ---------- Depth ----------
    if (m_Spec.useDepth)
    {
        bool multisample = IsMultisampled(m_Spec.samples);

        glGenTextures(1, &m_DepthAttachment);

        if (multisample)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_DepthAttachment);

            glTexImage2DMultisample(
                GL_TEXTURE_2D_MULTISAMPLE,
                m_Spec.samples,
                GL_DEPTH24_STENCIL8,
                m_Spec.width,
                m_Spec.height,
                GL_TRUE
            );

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_DEPTH_STENCIL_ATTACHMENT,
                GL_TEXTURE_2D_MULTISAMPLE,
                m_DepthAttachment,
                0
            );
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);

            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_DEPTH24_STENCIL8,
                m_Spec.width,
                m_Spec.height,
                0,
                GL_DEPTH_STENCIL,
                GL_UNSIGNED_INT_24_8,
                nullptr
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_DEPTH_STENCIL_ATTACHMENT,
                GL_TEXTURE_2D,
                m_DepthAttachment,
                0
            );
        }
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "[Framebuffer] ERROR: Incomplete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Destroy()
{
    if (m_DepthAttachment)
    {
        glDeleteTextures(1, &m_DepthAttachment);
        m_DepthAttachment = 0;
    }

    if (!m_ColorAttachments.empty())
    {
        glDeleteTextures((GLsizei)m_ColorAttachments.size(), m_ColorAttachments.data());
        m_ColorAttachments.clear();
    }

    if (m_FBO)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}

void Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    m_Spec.width = width;
    m_Spec.height = height;
    Invalidate();
}

uint32_t Framebuffer::GetColorAttachment(uint32_t index) const
{
    return m_ColorAttachments[index];
}


uint32_t Framebuffer::GetDepthAttachment() const
{
    return m_DepthAttachment;
}


