#pragma once
#include <bitset>
#include "vec2.hpp"

class Camera2D;
class JinEngine;
struct GLFWwindow;

/**
 * @brief Central input state manager for keyboard, mouse buttons, cursor position, and scroll input.
 *
 * @details
 * InputManager collects raw input events from GLFW callbacks and converts them into
 * frame-stable queryable states for gameplay and engine systems.
 *
 * The current implementation uses a staged input model:
 * - GLFW callbacks write into stagedKeyState and stagedMouseState through OnKey() and OnMouseButton().
 * - Update() copies staged states into current states once per frame.
 * - previous states are preserved so pressed/released transitions can be queried.
 *
 * This allows user code to call:
 * - IsKeyDown()
 * - IsKeyPressed()
 * - IsKeyReleased()
 * - IsMouseButtonDown()
 * - IsMouseButtonPressed()
 * - IsMouseButtonReleased()
 * - IsMouseButtonDragging()
 *
 * Cursor position is sampled every frame from GLFW, and scroll input is accumulated
 * asynchronously through AddScroll() before being exposed as per-frame deltas in Update().
 *
 * InputManager is initialized and driven by JinEngine:
 * - JinEngine calls Init() with the GLFW window handle.
 * - JinEngine calls Update() once per frame before game-state update.
 *
 * @note
 * This class stores input state only. It does not own the window.
 *
 * @note
 * World-space mouse queries require a valid Camera2D pointer.
 */
class InputManager
{
    friend JinEngine;

public:
    /**
     * @brief Constructs the input manager with default empty state.
     *
     * @details
     * The current implementation initializes the stored GLFW window pointer to nullptr
     * and cursor position to (0, 0). Bitsets and scroll values use their default zero state.
     */
    InputManager() :window(nullptr), mouseX(0.0), mouseY(0.0) {}

    /**
     * @brief Returns whether a key is currently held down.
     *
     * @details
     * This checks the current frame's committed key state.
     *
     * @param key Key code, typically one of the InputKey values.
     * @return True if the key is currently down. :contentReference[oaicite:7]{index=7}
     */
    [[nodiscard]] bool IsKeyDown(int key) const;

    /**
     * @brief Returns whether a key was pressed during the current frame.
     *
     * @details
     * A key is considered pressed when it is down in currentKeyState and was not down
     * in previousKeyState. This makes it suitable for one-frame trigger actions. :contentReference[oaicite:8]{index=8}
     *
     * @param key Key code, typically one of the InputKey values.
     * @return True only on the transition frame from up to down.
     */
    [[nodiscard]] bool IsKeyPressed(int key) const;

    /**
     * @brief Returns whether a key was released during the current frame.
     *
     * @details
     * A key is considered released when it is not down in currentKeyState but was down
     * in previousKeyState. :contentReference[oaicite:9]{index=9}
     *
     * @param key Key code, typically one of the InputKey values.
     * @return True only on the transition frame from down to up.
     */
    [[nodiscard]] bool IsKeyReleased(int key) const;

    /**
     * @brief Returns whether a mouse button is currently held down.
     *
     * @param button Mouse button code, typically one of the InputMouseButton values.
     * @return True if the button is currently down. :contentReference[oaicite:10]{index=10}
     */
    [[nodiscard]] bool IsMouseButtonDown(int button) const;

    /**
     * @brief Returns whether a mouse button was pressed during the current frame.
     *
     * @details
     * A mouse button is considered pressed when it is down in currentMouseState and was
     * not down in previousMouseState. :contentReference[oaicite:11]{index=11}
     *
     * @param button Mouse button code.
     * @return True only on the transition frame from up to down.
     */
    [[nodiscard]] bool IsMouseButtonPressed(int button) const;

    /**
     * @brief Returns whether a mouse button was released during the current frame.
     *
     * @details
     * A mouse button is considered released when it is not down in currentMouseState
     * but was down in previousMouseState. :contentReference[oaicite:12]{index=12}
     *
     * @param button Mouse button code.
     * @return True only on the transition frame from down to up.
     */
    [[nodiscard]] bool IsMouseButtonReleased(int button) const;

    /**
     * @brief Returns whether a mouse button is being held continuously across frames.
     *
     * @details
     * The current implementation returns true when the button is down in both
     * currentMouseState and previousMouseState. It does not check cursor movement directly,
     * so this represents continuous hold rather than geometric drag distance. :contentReference[oaicite:13]{index=13}
     *
     * @param button Mouse button code.
     * @return True if the button remained held from the previous frame into the current frame.
     */
    [[nodiscard]] bool IsMouseButtonDragging(int button) const;

    /**
     * @brief Returns the current cursor x position in window space.
     *
     * @details
     * The value is refreshed in Update() using glfwGetCursorPos(). :contentReference[oaicite:14]{index=14}
     *
     * @return Mouse x coordinate in window space.
     */
    [[nodiscard]] double GetMouseX() const;

    /**
     * @brief Returns the current cursor y position in window space.
     *
     * @details
     * The value is refreshed in Update() using glfwGetCursorPos(). :contentReference[oaicite:15]{index=15}
     *
     * @return Mouse y coordinate in window space.
     */
    [[nodiscard]] double GetMouseY() const;

    /**
     * @brief Returns the current cursor position in window space.
     *
     * @return Mouse position as a vec2 in window coordinates. :contentReference[oaicite:16]{index=16}
     */
    [[nodiscard]] glm::vec2 GetMousePos() const;

    /**
     * @brief Converts the current window-space cursor x coordinate into world-space x.
     *
     * @details
     * The current implementation uses the camera's position, screen width, and zoom:
     *
     *     (camera->GetPosition().x + mouseX - camera->GetScreenWidth() / 2.f) / camera->GetZoom()
     *
     * This assumes a camera-centered screen-space convention consistent with Camera2D and
     * the engine's orthographic rendering flow.
     *
     * @param camera Camera used for the conversion.
     * @return World-space mouse x coordinate.
     */
    [[nodiscard]] double GetMouseWorldX(Camera2D* camera) const;

    /**
     * @brief Converts the current window-space cursor y coordinate into world-space y.
     *
     * @details
     * The current implementation uses the camera's position, screen height, and zoom:
     *
     *     (camera->GetPosition().y + camera->GetScreenHeight() / 2.f - mouseY) / camera->GetZoom()
     *
     * The subtraction from screen height reflects the engine's world-space convention
     * relative to top-left based cursor coordinates.
     *
     * @param camera Camera used for the conversion.
     * @return World-space mouse y coordinate.
     */
    [[nodiscard]] double GetMouseWorldY(Camera2D* camera) const;

    /**
     * @brief Converts the current cursor position into world-space using a camera.
     *
     * @param camera Camera used for the conversion.
     * @return World-space mouse position. :contentReference[oaicite:19]{index=19}
     */
    [[nodiscard]] glm::vec2 GetMouseWorldPos(Camera2D* camera) const;

    /**
     * @brief Accumulates scroll wheel input.
     *
     * @details
     * WindowManager's GLFW scroll callback forwards wheel movement into this function.
     * The values are stored in accumulation variables and consumed on the next Update(),
     * where they become the per-frame scroll deltas.
     *
     * @param dx Horizontal scroll delta from the callback.
     * @param dy Vertical scroll delta from the callback.
     */
    void AddScroll(double dx, double dy);

    /**
     * @brief Returns the scroll delta accumulated for the current frame.
     *
     * @return Scroll delta as (x, y). :contentReference[oaicite:21]{index=21}
     */
    [[nodiscard]] glm::vec2 GetScrollDelta() const;

    /**
     * @brief Returns the horizontal scroll delta for the current frame.
     *
     * @return Current frame's horizontal scroll delta. :contentReference[oaicite:22]{index=22}
     */
    [[nodiscard]] double GetScrollXDelta() const;

    /**
     * @brief Returns the vertical scroll delta for the current frame.
     *
     * @return Current frame's vertical scroll delta. :contentReference[oaicite:23]{index=23}
     */
    [[nodiscard]] double GetScrollYDelta() const;

    /**
     * @brief Returns whether the wheel moved upward during the current frame.
     *
     * @return True when scrollDeltaY is greater than zero. :contentReference[oaicite:24]{index=24}
     */
    [[nodiscard]] bool IsScrolledUp() const;

    /**
     * @brief Returns whether the wheel moved downward during the current frame.
     *
     * @return True when scrollDeltaY is less than zero. :contentReference[oaicite:25]{index=25}
     */
    [[nodiscard]] bool IsScrolledDown() const;

    /**
     * @brief Receives a keyboard event from the window callback layer.
     *
     * @details
     * The current implementation validates that the key is within GLFW's valid range
     * and updates stagedKeyState according to the action:
     * - GLFW_PRESS sets the bit to true
     * - GLFW_RELEASE sets the bit to false
     * - GLFW_REPEAT keeps the key staged as down if it was not already staged :contentReference[oaicite:26]{index=26}
     *
     * This function is intended to be called by the engine's GLFW key callback bridge,
     * not by typical gameplay code. :contentReference[oaicite:27]{index=27}
     *
     * @param key GLFW-style key code.
     * @param Unused scan code parameter.
     * @param action GLFW action code.
     * @param Unused modifier flags parameter.
     */
    void OnKey(int key, int, int action, int);

    /**
     * @brief Receives a mouse button event from the window callback layer.
     *
     * @details
     * The current implementation validates that the button is within GLFW's valid range
     * and updates stagedMouseState according to press/release actions. :contentReference[oaicite:28]{index=28}
     *
     * This function is intended to be called by the engine's GLFW mouse callback bridge,
     * not by typical gameplay code. :contentReference[oaicite:29]{index=29}
     *
     * @param button GLFW-style mouse button code.
     * @param action GLFW action code.
     * @param Unused modifier flags parameter.
     */
    void OnMouseButton(int button, int action, int);

    /**
     * @brief Clears all stored input state.
     *
     * @details
     * The current implementation resets previous, current, and staged key/button states.
     * This is used to prevent stale transition states from surviving unusual timing situations.
     *
     * In the current engine flow, EngineTimer::Tick() calls Reset() when a frame delta
     * becomes larger than 0.05 seconds, then clamps dt to 0.0f. :contentReference[oaicite:31]{index=31}
     */
    void Reset();

private:
    /**
     * @brief Binds the input manager to a GLFW window.
     *
     * @details
     * JinEngine calls this during initialization after the window has been created.
     *
     * @param _window GLFW window handle used for cursor sampling.
     */
    void Init(GLFWwindow* _window);

    /**
     * @brief Commits staged input into the current frame state.
     *
     * @details
     * The current implementation performs the following sequence once per frame:
     * 1. Copies current key/button state into previous state
     * 2. Moves accumulated scroll into per-frame deltas and clears accumulators
     * 3. Copies staged key/button state into current state
     * 4. Samples the cursor position from GLFW into mouseX and mouseY :contentReference[oaicite:33]{index=33}
     *
     * JinEngine calls this every frame before state update.
     */
    void Update();

    /**
     * @brief GLFW window handle used for cursor position queries.
     */
    GLFWwindow* window;

    /**
     * @brief Maximum supported key index stored in the internal bitset.
     *
     * @details
     * This matches the currently used GLFW key range up to KEY_MENU (348).
     */
    static constexpr int MAX_KEYS = 349;

    /**
     * @brief Maximum supported mouse button slot count stored in the internal bitset.
     */
    static constexpr int MAX_MOUSE_BUTTONS = 9;

    /**
     * @brief Current committed key state for the active frame.
     */
    std::bitset<MAX_KEYS> currentKeyState;

    /**
     * @brief Previous frame's committed key state.
     */
    std::bitset<MAX_KEYS> previousKeyState;

    /**
     * @brief Current committed mouse button state for the active frame.
     */
    std::bitset<MAX_MOUSE_BUTTONS> currentMouseState;

    /**
     * @brief Previous frame's committed mouse button state.
     */
    std::bitset<MAX_MOUSE_BUTTONS> previousMouseState;

    /**
     * @brief Current cursor x position in window coordinates.
     */
    double mouseX;

    /**
     * @brief Current cursor y position in window coordinates.
     */
    double mouseY;

    /**
     * @brief Accumulated horizontal scroll received since the last Update().
     */
    double scrollAccumX = 0.0;

    /**
     * @brief Accumulated vertical scroll received since the last Update().
     */
    double scrollAccumY = 0.0;

    /**
     * @brief Horizontal scroll delta exposed for the current frame.
     */
    double scrollDeltaX = 0.0;

    /**
     * @brief Vertical scroll delta exposed for the current frame.
     */
    double scrollDeltaY = 0.0;

    /**
     * @brief Staged key state written by callback events before frame commit.
     */
    std::bitset<MAX_KEYS> stagedKeyState;

    /**
     * @brief Staged mouse button state written by callback events before frame commit.
     */
    std::bitset<MAX_MOUSE_BUTTONS> stagedMouseState;

};

/**
 * @brief Engine key code aliases matching the GLFW key values used by InputManager.
 *
 * @details
 * These constants allow gameplay code to query input without including GLFW directly.
 * The numeric values mirror the GLFW key codes expected by OnKey() and the internal bitset layout. :contentReference[oaicite:35]{index=35}
 */
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

/**
 * @brief Engine mouse button aliases matching the GLFW mouse button values used by InputManager.
 *
 * @details
 * These constants mirror the button codes expected by OnMouseButton() and the internal mouse button bitset. :contentReference[oaicite:36]{index=36}
 */
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