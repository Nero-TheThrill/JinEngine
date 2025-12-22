#pragma once

#include <string>
#include <vector>
#include "glm.hpp"

class LoadingState;

/**
 * @brief Identifies which programmable stage a shader source belongs to.
 *
 * @details
 * ShaderStage is used when compiling and attaching a shader module to a program.
 * In the implementation, each stage is mapped to the corresponding OpenGL shader
 * type (e.g., Vertex -> GL_VERTEX_SHADER).
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
 * @brief Wraps an OpenGL shader program and provides module attachment, linking, and uniform updates.
 *
 * @details
 * Shader owns an OpenGL program object created with glCreateProgram() and is responsible for:
 * - Compiling shader modules from files or source strings.
 * - Attaching compiled modules to the program.
 * - Linking the program and validating stage combinations (tessellation must be provided as TCS+TES pairs).
 * - Detaching shader modules after a successful link (modules are still kept in attachedShaders and deleted on destruction).
 * - Binding/unbinding the program for rendering.
 * - Sending common uniform types by name.
 *
 * Instancing support is detected after linking by checking whether the attribute name "i_Model"
 * exists in the linked program (glGetAttribLocation != -1). The result is cached in isSupportInstancing
 * and can be queried by friend systems via SupportsInstancing().
 *
 * Program creation, attachment, linking, and instancing queries are restricted to friend classes
 * (Material / RenderManager / LoadingState) to keep program lifecycle controlled by the rendering system.
 */
class Shader {
    friend Material;
    friend RenderManager;
    friend LoadingState;
public:
    /**
     * @brief Creates an empty OpenGL program object.
     *
     * @details
     * Constructs the Shader and immediately creates an OpenGL program via glCreateProgram().
     * The program has no attached modules until AttachFromFile/AttachFromSource is used.
     */
    Shader();

    /**
     * @brief Destroys the shader program and deletes any compiled modules tracked by this object.
     *
     * @details
     * Deletes all shader objects stored in attachedShaders and then deletes the OpenGL program.
     * This assumes the program will no longer be used by any pipeline when destruction occurs.
     */
    ~Shader();

    /**
     * @brief Sends an integer uniform to the program.
     *
     * @details
     * Looks up the uniform location with glGetUniformLocation(). If the uniform is not found
     * (location == -1), the call is ignored.
     *
     * @param name  Uniform name as declared in GLSL.
     * @param value Integer value to upload.
     */
    void SendUniform(const std::string& name, int value) const;

    /**
     * @brief Sends a float uniform to the program.
     *
     * @details
     * Looks up the uniform location with glGetUniformLocation(). If the uniform is not found
     * (location == -1), the call is ignored.
     *
     * @param name  Uniform name as declared in GLSL.
     * @param value Float value to upload.
     */
    void SendUniform(const std::string& name, float value) const;

    /**
     * @brief Sends a vec2 uniform to the program.
     *
     * @details
     * Looks up the uniform location with glGetUniformLocation(). If the uniform is not found
     * (location == -1), the call is ignored.
     *
     * @param name  Uniform name as declared in GLSL.
     * @param value Vector value to upload.
     */
    void SendUniform(const std::string& name, const glm::vec2& value) const;

    /**
     * @brief Sends a vec3 uniform to the program.
     *
     * @details
     * Looks up the uniform location with glGetUniformLocation(). If the uniform is not found
     * (location == -1), the call is ignored.
     *
     * @param name  Uniform name as declared in GLSL.
     * @param value Vector value to upload.
     */
    void SendUniform(const std::string& name, const glm::vec3& value) const;

    /**
     * @brief Sends a vec4 uniform to the program.
     *
     * @details
     * Looks up the uniform location with glGetUniformLocation(). If the uniform is not found
     * (location == -1), the call is ignored.
     *
     * @param name  Uniform name as declared in GLSL.
     * @param value Vector value to upload.
     */
    void SendUniform(const std::string& name, const glm::vec4& value) const;

    /**
     * @brief Sends a mat4 uniform to the program.
     *
     * @details
     * Looks up the uniform location with glGetUniformLocation(). If the uniform is not found
     * (location == -1), the call is ignored.
     *
     * The matrix is uploaded using glUniformMatrix4fv with transpose set to GL_FALSE.
     *
     * @param name  Uniform name as declared in GLSL.
     * @param value Matrix value to upload.
     */
    void SendUniform(const std::string& name, const glm::mat4& value) const;

    /**
     * @brief Returns the OpenGL program ID owned by this Shader.
     */
    [[nodiscard]] GLuint GetProgramID() const { return programID; }

    /**
     * @brief Binds this program for subsequent draw calls.
     *
     * @details
     * Calls glUseProgram(programID).
     */
    void Use() const;

    /**
     * @brief Unbinds any program from the current OpenGL context.
     *
     * @details
     * Calls glUseProgram(0).
     */
    void Unuse() const;
private:

    /**
     * @brief Returns whether the linked program supports instanced rendering conventions used by this engine.
     *
     * @details
     * The engine considers a program "instancing-capable" when the attribute "i_Model" is present
     * in the linked program. This value is determined in CheckSupportsInstancing() after Link().
     */
    [[nodiscard]] bool SupportsInstancing() const;

    /**
     * @brief Links the attached shader modules into a complete program.
     *
     * @details
     * Validates that tessellation stages are provided as a pair:
     * - TessControl must be present if and only if TessEval is present.
     *
     * On link failure, the OpenGL program info log is printed and false is returned.
     * On success:
     * - Instancing capability is detected via CheckSupportsInstancing().
     * - All attached modules are detached from the program (glDetachShader) while still being
     *   tracked in attachedShaders for deletion during destruction.
     *
     * @return True if linking succeeds; false otherwise.
     */
    bool Link();

    /**
     * @brief Loads, compiles, and attaches a shader module from a file.
     *
     * @details
     * This performs:
     * - LoadShaderSource(filepath)
     * - CompileShader(stage, source)
     * - glAttachShader(programID, shader)
     *
     * On success, the created shader object and its stage are tracked in attachedShaders/attachedStages.
     *
     * @param stage    Stage of the shader source.
     * @param filepath Path to the shader file.
     * @return True if the file loads and the shader compiles and attaches; false otherwise.
     */
    bool AttachFromFile(ShaderStage stage, const FilePath& filepath);

    /**
     * @brief Compiles and attaches a shader module from an in-memory source string.
     *
     * @details
     * This performs:
     * - CompileShader(stage, source)
     * - glAttachShader(programID, shader)
     *
     * On success, the created shader object and its stage are tracked in attachedShaders/attachedStages.
     *
     * @param stage  Stage of the shader source.
     * @param source GLSL source code.
     * @return True if the shader compiles and attaches; false otherwise.
     */
    bool AttachFromSource(ShaderStage stage, const std::string& source);

    /**
     * @brief Loads an entire shader file into a string.
     *
     * @details
     * Opens the file and streams its content into a string. If the file cannot be opened,
     * success is set to GL_FALSE and an empty string is returned.
     *
     * @param filepath Shader file path.
     * @param success  Output flag set to GL_TRUE on success or GL_FALSE on failure.
     * @return Shader source string (may be empty on failure).
     */
    [[nodiscard]] std::string LoadShaderSource(const FilePath& filepath, GLint& success);

    /**
     * @brief Compiles a shader module from a source string.
     *
     * @details
     * Creates an OpenGL shader object for the specified stage, sets the source,
     * and compiles it. On compilation failure, the stage name and info log are reported.
     *
     * The returned shader object is still returned even when compilation fails; the caller
     * is expected to check the success flag before attaching it.
     *
     * @param stage   Shader stage to compile for.
     * @param source  GLSL source code.
     * @param success Output flag set to GL_TRUE on success or GL_FALSE on failure.
     * @return OpenGL shader object ID.
     */
    [[nodiscard]] GLuint CompileShader(ShaderStage stage, const std::string& source, GLint& success);

    /**
     * @brief Detects whether the linked program contains the "i_Model" attribute for instancing.
     *
     * @details
     * Calls glGetAttribLocation(programID, "i_Model") and caches the result as a boolean.
     */
    void CheckSupportsInstancing();

    GLuint programID;
    std::vector<GLuint> attachedShaders;
    std::vector<ShaderStage> attachedStages;

    bool isSupportInstancing;
};
