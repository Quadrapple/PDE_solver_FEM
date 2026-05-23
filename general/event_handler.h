#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "state.h"
#include <vector>

struct Cursor {
    glm::dvec2 pos;
    glm::dvec2 prevPos;
};

struct MouseClick {
    int button;
    int action;
    int mods;
};

struct KeyPress {
    int key;
    int scancode;
    int action;
    int mods;
};


class KeyPressListener {
    public:
        virtual void onKeyboardAction(KeyPress press) = 0;
};

class KeyHoldListener {
    public:
        virtual void onKeyHold(const bool keysHeld[]) = 0;
};

class CursorListener {
    public:
        virtual void onCursorMove(Cursor cursor) = 0;
};

class MouseClickListener {
    public:
        virtual void onMouseClick(MouseClick click) = 0;
};

class FrameListener {
    public:
        virtual void onFrameEnd() = 0;
};

//TODO: remove all statics where possible
class EventHandler : public State {
    public:
        EventHandler(std::string title, glm::uvec2 size);

        static void errorCallback(int error, const char *description);
        static void mouseClickCallback(GLFWwindow *window, int button, int action, int mods);
        static void cursorCallback(GLFWwindow *window, double xpos, double ypos);
        static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
        static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

        //Not handled by GLFW
        static void processKeysHeldDown(GLFWwindow* window);
        static void processFrameEnd(GLFWwindow *window);

        static void addListener(MouseClickListener* listener);
        static void addListener(KeyPressListener* listener);
        static void addListener(CursorListener* listener);
        static void addListener(KeyHoldListener* listener);
        static void addListener(FrameListener* listener);

        void pollEvents();

        void setCursorMode(GLenum mode);

        void disableMouse();
        void enableMouse();

    private:
        static inline glm::dvec2 prevCursorPos = glm::dvec2(0,0);

        static inline GLenum cursorMode;
        static inline bool mouseEnabled = false;
        static inline bool keyboardState[GLFW_KEY_LAST];

        static inline std::vector<MouseClickListener*> mouseListeners;
        static inline std::vector<KeyPressListener*> keyPressListeners;
        static inline std::vector<KeyHoldListener*> keyHoldListeners;
        static inline std::vector<CursorListener*> cursorListeners;
        static inline std::vector<FrameListener*> frameListeners;

        void setCallbacks();
        void calcDeltaTime();

        class ControlKeyListener : public KeyPressListener {
            public:
                ControlKeyListener(EventHandler *ctx);
                virtual void onKeyboardAction(KeyPress press) override;
            private:
                bool keyPressed = false;
                EventHandler *ctx;
        };

        ControlKeyListener cl;

};

#endif
