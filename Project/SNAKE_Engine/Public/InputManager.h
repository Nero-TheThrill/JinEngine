#pragma once
#include <bitset>
#include "vec2.hpp"

class Camera2D;
class SNAKE_Engine;
struct GLFWwindow;
/**
 * @brief Handles keyboard, mouse, and scroll input events.
 *
 * @details
 * InputManager maintains current and previous key/mouse states using bitsets,
 * allowing queries for pressed, released, and held events. It also tracks
 * mouse position (window and world-space with a given camera) and scroll deltas.
 *
 * Input is updated per-frame via Update() and Reset() is called when the
 * window resizes (to resync staged state). Callbacks (OnKey, OnMouseButton)
 * are fed by GLFW.
 *
 * Example usage:
 * @code
 * if (engineContext.inputManager->IsKeyPressed(KEY_SPACE)) {
 *     // Trigger jump action
 * }
 * glm::vec2 mouseWorld = engineContext.inputManager->GetMouseWorldPos(camera);
 * @endcode
 */
class InputManager
{
    friend SNAKE_Engine;

public:
    InputManager() :window(nullptr), mouseX(0.0), mouseY(0.0) {}

    /**
     * @brief Checks if a key is currently held down.
     * @param key Key code (see @ref InputKey).
     * @return True if held.
     */
    [[nodiscard]] bool IsKeyDown(int key) const;

    /**
     * @brief Checks if a key was pressed this frame.
     * @param key Key code.
     * @return True if pressed this frame.
     */
    [[nodiscard]] bool IsKeyPressed(int key) const;

    /**
     * @brief Checks if a key was released this frame.
     * @param key Key code.
     * @return True if released this frame.
     */
    [[nodiscard]] bool IsKeyReleased(int key) const;

    /**
     * @brief Checks if a mouse button is currently held down.
     * @param button Mouse button code (see @ref InputMouseButton).
     * @return True if held.
     */
    [[nodiscard]] bool IsMouseButtonDown(int button) const;

    /**
     * @brief Checks if a mouse button was pressed this frame.
     * @param button Mouse button code.
     * @return True if pressed this frame.
     */
    [[nodiscard]] bool IsMouseButtonPressed(int button) const;

    /**
     * @brief Checks if a mouse button was released this frame.
     * @param button Mouse button code.
     * @return True if released this frame.
     */
    [[nodiscard]] bool IsMouseButtonReleased(int button) const;

    /**
     * @brief Checks if a mouse button is dragging (held + moved).
     * @param button Mouse button code.
     * @return True if dragging.
     */
    [[nodiscard]] bool IsMouseButtonDragging(int button) const;

    /**
     * @brief Returns the current mouse X coordinate (window space).
     */
    [[nodiscard]] double GetMouseX() const;

    /**
     * @brief Returns the current mouse Y coordinate (window space).
     */
    [[nodiscard]] double GetMouseY() const;

    /**
     * @brief Returns the current mouse position in window coordinates.
     */
    [[nodiscard]] glm::vec2 GetMousePos() const;

    /**
     * @brief Returns the mouse X position transformed to world space.
     * @param camera Camera2D used for conversion.
     */
    [[nodiscard]] double GetMouseWorldX(Camera2D* camera) const;

    /**
     * @brief Returns the mouse Y position transformed to world space.
     * @param camera Camera2D used for conversion.
     */
    [[nodiscard]] double GetMouseWorldY(Camera2D* camera) const;

    /**
     * @brief Returns the mouse position transformed to world space.
     * @param camera Camera2D used for conversion.
     */
    [[nodiscard]] glm::vec2 GetMouseWorldPos(Camera2D* camera) const;

    /**
     * @brief Adds scroll delta received from a scroll callback.
     * @param dx Horizontal scroll delta.
     * @param dy Vertical scroll delta.
     */
    void AddScroll(double dx, double dy);

    /**
     * @brief Returns accumulated scroll delta (x,y).
     */
    [[nodiscard]] glm::vec2 GetScrollDelta() const;
    [[nodiscard]] double GetScrollXDelta() const;
    [[nodiscard]] double GetScrollYDelta() const;

    /**
     * @brief Returns true if user scrolled up in this frame.
     */
    [[nodiscard]] bool IsScrolledUp() const;

    /**
     * @brief Returns true if user scrolled down in this frame.
     */
    [[nodiscard]] bool IsScrolledDown() const;

    /**
     * @brief GLFW key callback entry point.
     * @details Called internally by the engine to stage key events.
     */
    void OnKey(int key, int, int action, int);

    /**
     * @brief GLFW mouse button callback entry point.
     * @details Called internally by the engine to stage mouse button events.
     */
    void OnMouseButton(int button, int action, int);

    /**
     * @brief Resets staged state after window resize.
     * @note Called on framebuffer size change, not every frame.
     */
    void Reset();

private:
    /**
     * @brief Initializes the manager with a GLFW window.
     */
    void Init(GLFWwindow* _window);

    /**
     * @brief Updates state each frame (commits staged input).
     */
    void Update();

    GLFWwindow* window;

    static constexpr int MAX_KEYS = 349;          ///< Number of supported keys.
    static constexpr int MAX_MOUSE_BUTTONS = 9;   ///< Number of supported mouse buttons.

    std::bitset<MAX_KEYS> currentKeyState;
    std::bitset<MAX_KEYS> previousKeyState;
    std::bitset<MAX_MOUSE_BUTTONS> currentMouseState;
    std::bitset<MAX_MOUSE_BUTTONS> previousMouseState;

    double mouseX;
    double mouseY;

    double scrollAccumX = 0.0;
    double scrollAccumY = 0.0;
    double scrollDeltaX = 0.0;
    double scrollDeltaY = 0.0;

    std::bitset<MAX_KEYS> stagedKeyState;
    std::bitset<MAX_MOUSE_BUTTONS> stagedMouseState;
};


enum InputKey
{
    KEY_UNKNOWN = -1,
    KEY_SPACE = 32,
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_MINUS = 45,
    KEY_PERIOD = 46,
    KEY_SLASH = 47,
    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,
    KEY_SEMICOLON = 59,
    KEY_EQUAL = 61,
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,
    KEY_LEFT_BRACKET = 91,
    KEY_BACKSLASH = 92,
    KEY_RIGHT_BRACKET = 93,
    KEY_GRAVE_ACCENT = 96,

    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_TAB = 258,
    KEY_BACKSPACE = 259,
    KEY_INSERT = 260,
    KEY_DELETE = 261,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
    KEY_PAGE_UP = 266,
    KEY_PAGE_DOWN = 267,
    KEY_HOME = 268,
    KEY_END = 269,
    KEY_CAPS_LOCK = 280,
    KEY_SCROLL_LOCK = 281,
    KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283,
    KEY_PAUSE = 284,

    KEY_F1 = 290,
    KEY_F2 = 291,
    KEY_F3 = 292,
    KEY_F4 = 293,
    KEY_F5 = 294,
    KEY_F6 = 295,
    KEY_F7 = 296,
    KEY_F8 = 297,
    KEY_F9 = 298,
    KEY_F10 = 299,
    KEY_F11 = 300,
    KEY_F12 = 301,

    KEY_LEFT_SHIFT = 340,
    KEY_LEFT_CONTROL = 341,
    KEY_LEFT_ALT = 342,
    KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_RIGHT_SUPER = 347,

    KEY_MENU = 348
};

enum InputMouseButton
{
    MOUSE_BUTTON_1 = 0,
    MOUSE_BUTTON_LEFT = MOUSE_BUTTON_1,
    MOUSE_BUTTON_2 = 1,
    MOUSE_BUTTON_RIGHT = MOUSE_BUTTON_2,
    MOUSE_BUTTON_3 = 2,
    MOUSE_BUTTON_MIDDLE = MOUSE_BUTTON_3,
    MOUSE_BUTTON_4 = 3,
    MOUSE_BUTTON_5 = 4,
    MOUSE_BUTTON_6 = 5,
    MOUSE_BUTTON_7 = 6,
    MOUSE_BUTTON_8 = 7,

    MOUSE_BUTTON_LAST = MOUSE_BUTTON_8
};
