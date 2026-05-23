#include "render_target.h"
#include "state.h"
#include <stdexcept>

RenderTarget::RenderTarget() : id(0) {
    glGenFramebuffers(1, &id);
}

RenderTarget::RenderTarget(glm::uvec2 size) : size(size) {
    glGenFramebuffers(1, &id);

    bind();
    *this << std::make_shared<Texture>(GL_TEXTURE_2D, GL_RGB, size, GL_RGB, nullptr)
          << Renderbuffer(GL_DEPTH24_STENCIL8, size);

    assertComplete();
}

RenderTarget::RenderTarget(glm::uvec2 size, unsigned int samples) : size(size) {
    glGenFramebuffers(1, &id);

    bind();
    *this << std::make_shared<Texture>(GL_TEXTURE_2D_MULTISAMPLE, GL_RGB, size, 4, true)
          << Renderbuffer(GL_DEPTH24_STENCIL8, size, samples);

    assertComplete();
}

RenderTarget::RenderTarget(const std::shared_ptr<Texture> color) {
    glGenFramebuffers(1, &id);

    bind();
    *this << color
          << Renderbuffer(GL_DEPTH24_STENCIL8, size);

    assertComplete();
}

void RenderTarget::assertComplete() const {
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Error! Framebuffer incomplete");
    }
}

void RenderTarget::bind() const {
    State::getContext().bindFramebuffer(id);
}

void RenderTarget::blit(const RenderTarget &other) const {
    bind();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getId());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, other.getId());
    glBlitFramebuffer(0, 0, other.size.x, other.size.y, 0, 0, this->size.x, this->size.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

Texture& RenderTarget::getColor(unsigned int attachment) const {
    return *colorAttachments[attachment];
}

unsigned int RenderTarget::getId() const {
    return id;
}

glm::vec2 RenderTarget::getSize() const {
    return size;
}

RenderTarget& RenderTarget::operator<<(const Renderbuffer &rhs) {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rhs.id);
    rbo = rhs;

    return *this;
}

void RenderTarget::clear() const {
    bind();
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

RenderTarget& RenderTarget::operator<<(const std::shared_ptr<Texture> rhs) {
    rhs->bind(0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachments.size(), rhs->getType(), rhs->getId(), 0);
    colorAttachments.push_back(rhs);

    return *this;
}

Renderbuffer::Renderbuffer(GLenum type, glm::uvec2 size) : type(type), size(size) {
    glGenRenderbuffers(1, &id);
    glBindRenderbuffer(GL_RENDERBUFFER, id);
    glRenderbufferStorage(GL_RENDERBUFFER, type, size.x, size.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

Renderbuffer::Renderbuffer(GLenum type, glm::uvec2 size, unsigned int samples) : type(type), size(size) {
    glGenRenderbuffers(1, &id);
    glBindRenderbuffer(GL_RENDERBUFFER, id);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, type, size.x, size.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

Renderbuffer::Renderbuffer() : id(0) {} 
