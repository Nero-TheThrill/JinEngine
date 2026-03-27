#pragma once

#include <string>
#include <vector>
#include "glm.hpp"

class LoadingState;

/**
 * @brief Enumerates the shader stages supported by the engine.
 *
 * @details
 * This enum is used when attaching shader source code or shader files to a
 * Shader object. Each value is converted to the corresponding OpenGL shader
 * stage during compilation.
 *
 * @note
 * Tessellation stages must be provided as a valid pair. If only one of
 * TessControl or TessEval is attached, program linking will fail.
 */
enum class ShaderStage
{
    Vertex,
    Fragment,
    Geometry,
    TessControl,
    TessEval,
    Compute
};

class RenderManager;
class Material;

using GLuint = unsigned int;
using GLint = int;
using GLenum = unsigned int;
using FilePath = std::string;

/**
 * @brief Wraps an OpenGL shader program and manages shader compilation, attachment, linking, and uniform submission.
 *
 * @details
 * Shader owns a single OpenGL program object created in the constructor.
 * Individual shader stages can be attached either from files or from raw source strings,
 * then linked into a final program with Link().
 *
 * The class also provides convenience overloads for sending commonly used uniform types.
 * Uniform submission silently skips missing uniforms by returning early when the queried
 * uniform location is -1.
 *
 * Internally, the class tracks which shader objects were attached so that they can be
 * deleted in the destructor and detached after a successful link.
 *
 * Instancing support is detected after program linking by checking whether the linked
 * shader exposes an attribute named `i_Model`. Material uses this information to decide
 * whether instanced rendering can be enabled for a mesh.
 *
 * @note
 * This class does not automatically bind the shader before sending uniforms.
 * The caller is expected to ensure the program is active when uniform values are submitted.
 *
 * @note
 * This class is closely integrated with Material, RenderManager, and LoadingState,
 * which are declared as friends to access internal helper functions during resource setup.
 */
class Shader {
    friend Material;
    friend RenderManager;
    friend LoadingState;
public:
    /**
     * @brief Creates a new OpenGL program object.
     *
     * @details
     * The constructor initializes the program handle by calling glCreateProgram()
     * and marks instancing support as disabled until a successful link check is performed.
     *
     * @note
     * At this point, no shader stages are attached yet.
     */
    Shader();

    /**
     * @brief Destroys the shader program and all tracked attached shader objects.
     *
     * @details
     * The destructor deletes every shader object stored in attachedShaders and then
     * deletes the OpenGL program object itself.
     *
     * @note
     * Shader objects are detached from the program after a successful link, but they are
     * still kept in attachedShaders so they can be properly deleted here.
     */
    ~Shader();

    /**
     * @brief Sends an integer uniform to the currently active shader program.
     *
     * @details
     * Queries the uniform location from the linked program using the provided name.
     * If the uniform does not exist or is optimized out, the function returns without error.
     *
     * @param name Uniform variable name declared in the shader.
     * @param value Integer value to upload.
     *
     * @note
     * The shader program should already be bound with Use() before calling this function.
     */
    void SendUniform(const std::string& name, int value) const;

    /**
     * @brief Sends a float uniform to the currently active shader program.
     *
     * @details
     * Queries the uniform location from the linked program using the provided name.
     * If the uniform does not exist or is optimized out, the function returns without error.
     *
     * @param name Uniform variable name declared in the shader.
     * @param value Float value to upload.
     *
     * @note
     * The shader program should already be bound with Use() before calling this function.
     */
    void SendUniform(const std::string& name, float value) const;

    /**
     * @brief Sends a vec2 uniform to the currently active shader program.
     *
     * @details
     * Queries the uniform location from the linked program using the provided name.
     * If the uniform does not exist or is optimized out, the function returns without error.
     *
     * @param name Uniform variable name declared in the shader.
     * @param value 2D vector value to upload.
     *
     * @note
     * The shader program should already be bound with Use() before calling this function.
     */
    void SendUniform(const std::string& name, const glm::vec2& value) const;

    /**
     * @brief Sends a vec3 uniform to the currently active shader program.
     *
     * @details
     * Queries the uniform location from the linked program using the provided name.
     * If the uniform does not exist or is optimized out, the function returns without error.
     *
     * @param name Uniform variable name declared in the shader.
     * @param value 3D vector value to upload.
     *
     * @note
     * The shader program should already be bound with Use() before calling this function.
     */
    void SendUniform(const std::string& name, const glm::vec3& value) const;

    /**
     * @brief Sends a vec4 uniform to the currently active shader program.
     *
     * @details
     * Queries the uniform location from the linked program using the provided name.
     * If the uniform does not exist or is optimized out, the function returns without error.
     *
     * @param name Uniform variable name declared in the shader.
     * @param value 4D vector value to upload.
     *
     * @note
     * The shader program should already be bound with Use() before calling this function.
     */
    void SendUniform(const std::string& name, const glm::vec4& value) const;

    /**
     * @brief Sends a mat4 uniform to the currently active shader program.
     *
     * @details
     * Queries the uniform location from the linked program using the provided name.
     * If the uniform does not exist or is optimized out, the function returns without error.
     * The matrix is uploaded without transposition.
     *
     * @param name Uniform variable name declared in the shader.
     * @param value 4x4 matrix value to upload.
     *
     * @note
     * The shader program should already be bound with Use() before calling this function.
     */
    void SendUniform(const std::string& name, const glm::mat4& value) const;

    /**
     * @brief Returns the OpenGL program handle owned by this shader.
     *
     * @return GLuint OpenGL program object ID.
     *
     * @note
     * This is typically used internally by engine systems that need low-level program access.
     */
    [[nodiscard]] GLuint GetProgramID() const { return programID; }

    /**
     * @brief Binds this shader program for subsequent rendering or uniform submission.
     *
     * @details
     * Calls glUseProgram(programID) so that future draw calls and uniform uploads use this program.
     */
    void Use() const;

    /**
     * @brief Unbinds the currently active shader program.
     *
     * @details
     * Calls glUseProgram(0), clearing the current program binding.
     */
    void Unuse() const;
private:

    /**
     * @brief Returns whether the linked shader program supports instanced rendering.
     *
     * @details
     * The value is determined by CheckSupportsInstancing(), which checks whether
     * the linked program exposes an attribute named `i_Model`.
     *
     * @return true if instancing is supported by the linked shader program.
     * @return false otherwise.
     *
     * @note
     * This function is primarily used internally by Material when enabling instancing.
     */
    [[nodiscard]] bool SupportsInstancing() const;

    /**
     * @brief Links all attached shader stages into a final OpenGL program.
     *
     * @details
     * Before linking, this function validates tessellation usage and requires
     * TessControl and TessEval to be attached together as a pair.
     *
     * If linking succeeds:
     * - the program link status is validated,
     * - instancing capability is detected,
     * - all attached shader objects are detached from the program.
     *
     * If linking fails, an error is logged and the function returns false.
     *
     * @return true if program linking succeeds.
     * @return false if validation or linking fails.
     *
     * @note
     * The shader objects are detached after a successful link, but they remain stored
     * in attachedShaders so they can be deleted later by the destructor.
     */
    bool Link();

    /**
     * @brief Loads shader source from a file, compiles it, and attaches it to the program.
     *
     * @details
     * This function first reads the shader file contents using LoadShaderSource().
     * If file loading succeeds, it compiles the source with CompileShader().
     * On successful compilation, the created shader object is attached to the program,
     * and both the shader handle and its stage are tracked internally.
     *
     * @param stage Shader stage to compile and attach.
     * @param filepath Path to the shader source file.
     *
     * @return true if the file is loaded, compiled, and attached successfully.
     * @return false if file loading or compilation fails.
     *
     * @note
     * This function only attaches the stage. The caller must still call Link()
     * to create a usable shader program.
     */
    bool AttachFromFile(ShaderStage stage, const FilePath& filepath);

    /**
     * @brief Compiles shader code from a raw source string and attaches it to the program.
     *
     * @details
     * This function compiles the provided source using CompileShader().
     * On success, the compiled shader object is attached to the program and
     * tracked internally for later detachment and destruction.
     *
     * @param stage Shader stage to compile and attach.
     * @param source Raw GLSL shader source code.
     *
     * @return true if compilation and attachment succeed.
     * @return false if compilation fails.
     *
     * @note
     * This function only attaches the stage. The caller must still call Link()
     * to create a usable shader program.
     */
    bool AttachFromSource(ShaderStage stage, const std::string& source);

    /**
     * @brief Reads the full contents of a shader source file into a string.
     *
     * @details
     * Opens the file at the given path and copies its contents into a string stream.
     * If the file cannot be opened, an error is logged, success is set to GL_FALSE,
     * and an empty string is returned.
     *
     * @param filepath Path to the shader source file.
     * @param success Output flag set to GL_TRUE on success, GL_FALSE on failure.
     *
     * @return The loaded shader source string, or an empty string if loading fails.
     */
    [[nodiscard]] std::string LoadShaderSource(const FilePath& filepath, GLint& success);

    /**
     * @brief Compiles a shader object for the specified stage from source code.
     *
     * @details
     * Creates an OpenGL shader object for the requested stage, uploads the source code,
     * and compiles it. Compilation status is written to the success output parameter.
     * If compilation fails, the OpenGL info log is printed through the engine logger.
     *
     * @param stage Shader stage to compile.
     * @param source GLSL source code for the shader.
     * @param success Output flag set to the compile result returned by OpenGL.
     *
     * @return GLuint Handle of the created shader object.
     *
     * @note
     * The shader object is created regardless of compile success. The caller is responsible
     * for checking the success flag before attaching it to the program.
     */
    [[nodiscard]] GLuint CompileShader(ShaderStage stage, const std::string& source, GLint& success);

    /**
     * @brief Detects whether the linked shader supports instancing.
     *
     * @details
     * Checks the linked program for the attribute named `i_Model`.
     * If the attribute exists, isSupportInstancing is set to true.
     * Otherwise, it remains false.
     *
     * @note
     * This function is called automatically from Link() after a successful program link.
     */
    void CheckSupportsInstancing();

    GLuint programID;
    std::vector<GLuint> attachedShaders;
    std::vector<ShaderStage> attachedStages;

    bool isSupportInstancing;
};