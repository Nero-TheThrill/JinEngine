#pragma once
#include <bitset>
#include "vec2.hpp"

class Camera2D;
class JinEngine;
struct GLFWwindow;

/**
 * @brief Manages keyboard, mouse button, cursor position, and scroll input states.
 *
 * @details
 * InputManager provides a frame-based input querying system built on top of GLFW callbacks.
 * It tracks both current and previous input states to allow detection of:
 * - key/button down
 * - key/button pressed (down this frame)
 * - key/button released (released this frame)
 *
 * Input events are first written into staged states via GLFW callbacks,
 * then committed to current states during the engine update phase.
 *
 * Mouse position and scroll input are accumulated and processed separately.
 * World-space mouse coordinates are calculated using a Camera2D projection.
 */
class InputManager
{
    friend JinEngine;

public:

    /**
     * @brief Constructs an InputManager with default-initialized state.
     *
     * @details
     * The window pointer is initialized to nullptr.
     * Mouse position values are initialized to zero.
     */
    InputManager() :window(nullptr), mouseX(0.0), mouseY(0.0) {}

    /**
     * @brief Checks whether a key is currently held down.
     *
     * @param key Key code to query.
     * @return true if the key is currently pressed.
     */
    [[nodiscard]] bool IsKeyDown(int key) const;

    /**
     * @brief Checks whether a key was pressed during the current frame.
     *
     * @details
     * Returns true only if the key is down in the current state
     * and was not down in the previous state.
     *
     * @param key Key code to query.
     * @return true if the key was pressed this frame.
     */
    [[nodiscard]] bool IsKeyPressed(int key) const;

    /**
     * @brief Checks whether a key was released during the current frame.
     *
     * @param key Key code to query.
     * @return true if the key was released this frame.
     */
    [[nodiscard]] bool IsKeyReleased(int key) const;

    /**
     * @brief Checks whether a mouse button is currently held down.
     *
     * @param button Mouse button index to query.
     * @return true if the mouse button is currently pressed.
     */
    [[nodiscard]] bool IsMouseButtonDown(int button) const;

    /**
     * @brief Checks whether a mouse button was pressed during the current frame.
     *
     * @param button Mouse button index to query.
     * @return true if the mouse button was pressed this frame.
     */
    [[nodiscard]] bool IsMouseButtonPressed(int button) const;

    /**
     * @brief Checks whether a mouse button was released during the current frame.
     *
     * @param button Mouse button index to query.
     * @return true if the mouse button was released this frame.
     */
    [[nodiscard]] bool IsMouseButtonReleased(int button) const;

    /**
     * @brief Checks whether a mouse button is being held while the mouse is moving.
     *
     * @details
     * This is typically used for drag operations.
     *
     * @param button Mouse button index to query.
     * @return true if the button is down and the cursor is moving.
     */
    [[nodiscard]] bool IsMouseButtonDragging(int button) const;

    /**
     * @brief Returns the current mouse X position in screen space.
     */
    [[nodiscard]] double GetMouseX() const;

    /**
     * @brief Returns the current mouse Y position in screen space.
     */
    [[nodiscard]] double GetMouseY() const;

    /**
     * @brief Returns the current mouse position in screen space.
     */
    [[nodiscard]] glm::vec2 GetMousePos() const;

    /**
     * @brief Returns the mouse X position in world space.
     *
     * @param camera Camera used to convert screen space to world space.
     */
    [[nodiscard]] double GetMouseWorldX(Camera2D* camera) const;

    /**
     * @brief Returns the mouse Y position in world space.
     *
     * @param camera Camera used to convert screen space to world space.
     */
    [[nodiscard]] double GetMouseWorldY(Camera2D* camera) const;

    /**
     * @brief Returns the mouse position in world space.
     *
     * @param camera Camera used to convert screen space to world space.
     */
    [[nodiscard]] glm::vec2 GetMouseWorldPos(Camera2D* camera) const;

    /**
     * @brief Accumulates scroll input delta values.
     *
     * @details
     * This function is typically called from a GLFW scroll callback.
     * The accumulated values are later converted into per-frame deltas.
     *
     * @param dx Horizontal scroll delta.
     * @param dy Vertical scroll delta.
     */
    void AddScroll(double dx, double dy);

    /**
     * @brief Returns the scroll delta for the current frame.
     */
    [[nodiscard]] glm::vec2 GetScrollDelta() const;

    /**
     * @brief Returns the horizontal scroll delta for the current frame.
     */
    [[nodiscard]] double GetScrollXDelta() const;

    /**
     * @brief Returns the vertical scroll delta for the current frame.
     */
    [[nodiscard]] double GetScrollYDelta() const;

    /**
     * @brief Checks whether the scroll wheel was moved upward this frame.
     */
    [[nodiscard]] bool IsScrolledUp() const;

    /**
     * @brief Checks whether the scroll wheel was moved downward this frame.
     */
    [[nodiscard]] bool IsScrolledDown() const;

    /**
     * @brief Receives keyboard input events from GLFW.
     *
     * @details
     * This function updates the staged key state and does not immediately
     * affect the current input state.
     */
    void OnKey(int key, int, int action, int);

    /**
     * @brief Receives mouse button input events from GLFW.
     *
     * @details
     * This function updates the staged mouse state and does not immediately
     * affect the current input state.
     */
    void OnMouseButton(int button, int action, int);

    /**
     * @brief Resets per-frame transient input data.
     *
     * @details
     * This function clears scroll deltas and updates previous input states.
     * It is not intended to be called every frame manually.
     */
    void Reset();

private:

    /**
     * @brief Initializes the InputManager with a GLFW window.
     *
     * @param _window GLFW window handle.
     */
    void Init(GLFWwindow* _window);

    /**
     * @brief Updates internal input state transitions.
     *
     * @details
     * Commits staged input states to current states and stores previous states.
     * This function is called internally by the engine update loop.
     */
    void Update();

    GLFWwindow* window;

    static constexpr int MAX_KEYS = 349;
    static constexpr int MAX_MOUSE_BUTTONS = 9;

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
