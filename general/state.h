#ifndef CONTEXT_H
#define CONTEXT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <map>
#include <string>
#include <unordered_map>

struct BindingPoint {
    GLenum target;
    unsigned int point;
    bool operator==(const BindingPoint &rhs) const {
        return this->target == rhs.target && this->point == rhs.point;
    }
};

struct BindingPointHasher {
    std::size_t operator()(const BindingPoint& lhs) const {
        std::size_t h1 = std::hash<unsigned int>{}(lhs.point);
        std::size_t h2 = std::hash<GLenum>{}(lhs.target);
        return h1 ^ (h2 << 1);
    }
};

class State {
    public:
        GLFWwindow *window;
        State(std::string title, glm::uvec2 size);

        //Map of target-buffer binds
        std::map<GLenum, unsigned int> targets;

        //Map of enabled things
        std::unordered_map<GLenum, bool> enabled;

        //Map of bindingPoint-buffer binds
        std::unordered_map<BindingPoint, unsigned int, BindingPointHasher> bindingPoints;

        //Map of texture bindings, 192 is the maximum defined by GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
        unsigned int texUnitBindings[192];

        unsigned int boundVaoID = 0;
        unsigned int usedShaderID = 0;
        unsigned int activeTexUnit = 0;
        unsigned int boundFramebufID = 0;

        static const unsigned int MAJOR_VER = 3;
        static const unsigned int MINOR_VER = 3;

        static inline std::unordered_map<unsigned int, State*> hack;
        static State& getContext();

        float getDeltaTime();

        void bindBuffer(GLenum target, unsigned int id);
        void bindBufferBase(GLenum target, unsigned int index, unsigned int id);
        void bindVertexArray(unsigned int id);
        void bindFramebuffer(unsigned int id);

        void activateTexUnit(unsigned int unit);
        void bindTexture(GLenum target, unsigned int unit, unsigned int id);

        void useProgram(unsigned int id);

        void setViewport(glm::uvec2 size);
        glm::uvec2 getViewport();

        void setWindowSize(glm::uvec2 size);
        void viewportToWindowSize();
        glm::uvec2 getWindowSize();

        void enable(GLenum option);
        void disable(GLenum option);
        void setDepthFunc(GLenum option);
    protected:
        float deltaTime;
        float prevTime;
    private:
        glm::uvec2 viewportSize;
        glm::uvec2 windowSize;

        GLenum depthFunc;

        static void initGLFW();
        static void initGLAD();

        void createWindow(std::string title, glm::uvec2 size);
};

#endif
