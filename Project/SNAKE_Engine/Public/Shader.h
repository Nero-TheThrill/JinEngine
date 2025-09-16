#pragma once

#include <string>
#include <vector>
#include "glm.hpp"

class LoadingState;

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
 * @brief Encapsulates an OpenGL shader program.
 *
 * @details
 * Provides functionality to attach and compile shader sources from files or strings,
 * link them into a program, and set uniform variables. Also tracks whether the program
 * supports GPU instancing based on the presence of specific attributes.
 *
 * Shader objects are owned and managed by RenderManager and Material. They are typically
 * created during resource loading and bound before rendering objects.
 */
class Shader {
    friend Material;
    friend RenderManager;
    friend LoadingState;
public:
    /**
     * @brief Constructs an empty shader program object.
     *
     * @details
     * Initializes an OpenGL program object handle. Shaders must be attached and linked
     * before use.
     */
    Shader();

    /**
     * @brief Destroys the shader program.
     *
     * @details
     * Deletes the OpenGL program and attached shaders.
     */
    ~Shader();

    /**
     * @brief Sends an integer uniform value to the shader.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Integer value to upload.
     */
    void SendUniform(const std::string& name, int value) const;

    /**
     * @brief Sends a float uniform value to the shader.
     *
     * @param name Name of the uniform variable in the shader.
     * @param value Float value to upload.
     */
    void SendUniform(const std::string& name, float value) const;

    /**
     * @brief Sends a vec2 uniform value to the shader.
     *
     * @param name Name of the uniform variable.
     * @param value 2D vector to upload.
     */
    void SendUniform(const std::string& name, const glm::vec2& value) const;

    /**
     * @brief Sends a vec3 uniform value to the shader.
     *
     * @param name Name of the uniform variable.
     * @param value 3D vector to upload.
     */
    void SendUniform(const std::string& name, const glm::vec3& value) const;

    /**
     * @brief Sends a vec4 uniform value to the shader.
     *
     * @param name Name of the uniform variable.
     * @param value 4D vector to upload.
     */
    void SendUniform(const std::string& name, const glm::vec4& value) const;

    /**
     * @brief Sends a mat4 uniform value to the shader.
     *
     * @param name Name of the uniform variable.
     * @param value 4x4 matrix to upload.
     */
    void SendUniform(const std::string& name, const glm::mat4& value) const;

    /**
     * @brief Gets the OpenGL program ID of this shader.
     *
     * @return The program object handle.
     */
    [[nodiscard]] GLuint GetProgramID() const { return programID; }

    /**
     * @brief Binds the shader program for use in rendering.
     */
    void Use() const;

    /**
     * @brief Unbinds the shader program.
     *
     * @details
     * Typically resets the active program to 0.
     */
    void Unuse() const;
private:
    /**
     * @brief Checks if this shader program supports instancing.
     *
     * @return True if instancing attributes are available, false otherwise.
     */
    [[nodiscard]] bool SupportsInstancing() const;

    /**
     * @brief Links all attached shaders into a complete program.
     *
     * @return True if linking succeeded, false otherwise.
     */
    bool Link();

    /**
     * @brief Attaches and compiles a shader from a file.
     *
     * @param stage Shader stage (vertex, fragment, compute, etc.).
     * @param filepath Path to the shader source file.
     * @return True if compilation and attachment succeeded.
     */
    bool AttachFromFile(ShaderStage stage, const FilePath& filepath);

    /**
     * @brief Attaches and compiles a shader from a source string.
     *
     * @param stage Shader stage (vertex, fragment, compute, etc.).
     * @param source Shader GLSL source code.
     * @return True if compilation and attachment succeeded.
     */
    bool AttachFromSource(ShaderStage stage, const std::string& source);

    /**
     * @brief Loads GLSL shader source from a file.
     *
     * @param filepath Path to the file.
     * @param success Set to 1 if load succeeded, 0 otherwise.
     * @return Shader source as a string.
     */
    [[nodiscard]] std::string LoadShaderSource(const FilePath& filepath, GLint& success);

    /**
     * @brief Compiles a shader stage from GLSL source code.
     *
     * @param stage Shader stage to compile.
     * @param source GLSL shader source code.
     * @param success Set to 1 if compilation succeeded, 0 otherwise.
     * @return OpenGL shader object ID.
     */
    [[nodiscard]] GLuint CompileShader(ShaderStage stage, const std::string& source, GLint& success);

    /**
     * @brief Detects and caches whether instancing is supported by this program.
     *
     * @details
     * Called after linking to check for instancing-specific attributes.
     */
    void CheckSupportsInstancing();

    GLuint programID; ///< OpenGL program object ID.
    std::vector<GLuint> attachedShaders; ///< List of attached shader object IDs.
    std::vector<ShaderStage> attachedStages; ///< Shader stages that are attached.

    bool isSupportInstancing; ///< Cached result of instancing support check.
};
