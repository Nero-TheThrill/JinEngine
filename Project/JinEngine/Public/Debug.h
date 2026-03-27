#pragma once
#include <iostream>
#include <sstream>
#pragma once

/**
 * @brief Extracts the file name portion from the current source file path.
 *
 * @details
 * This macro uses strrchr() to search for the last '\\' character in __FILE__.
 * If found, it returns a pointer to the character after the last path separator,
 * which corresponds to the file name only.
 * If no separator is found, it falls back to the full __FILE__ string.
 *
 * This is used by the logging macros so that log output shows a short file name
 * instead of the full absolute or relative source path.
 */
#define FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

 /**
  * @brief Logs a normal diagnostic message with file and line information.
  *
  * @details
  * The macro builds a temporary std::ostringstream, streams the user expression into it,
  * converts the result into a std::string, and forwards the message to DebugLogger::Log().
  *
  * The current source file name and source line are also forwarded automatically.
  *
  * @param x Any streamable expression.
  *
  * @code
  * JIN_LOG("Player position: " << x << ", " << y);
  * @endcode
  */
#define JIN_LOG(x)   do { std::ostringstream oss; oss << x; DebugLogger::Log(oss.str(), FILENAME, __LINE__); } while(0)

  /**
   * @brief Logs a warning message with file and line information.
   *
   * @details
   * This macro behaves the same way as JIN_LOG, but forwards the built message
   * to DebugLogger::Warn().
   *
   * @param x Any streamable expression.
   *
   * @code
   * JIN_WRN("Texture not found: " << textureTag);
   * @endcode
   */
#define JIN_WRN(x)   do { std::ostringstream oss; oss << x; DebugLogger::Warn(oss.str(), FILENAME, __LINE__); } while(0)

   /**
    * @brief Logs an error message with file and line information.
    *
    * @details
    * This macro behaves the same way as JIN_LOG, but forwards the built message
    * to DebugLogger::Error().
    *
    * @param x Any streamable expression.
    *
    * @code
    * JIN_ERR("Shader compilation failed");
    * @endcode
    */
#define JIN_ERR(x)   do { std::ostringstream oss; oss << x; DebugLogger::Error(oss.str(), FILENAME, __LINE__); } while(0)


    /**
     * @brief Defines the logging verbosity level used by DebugLogger.
     *
     * @details
     * The logger checks the current level using greater-than-or-equal comparisons.
     * Based on the current Debug.cpp implementation:
     *
     * - None disables all output.
     * - Error allows only error messages.
     * - Warning allows warning and error messages.
     * - Log allows normal logs, warnings, and errors.
     * - All is the highest value and therefore also allows every message. :contentReference[oaicite:1]{index=1}
     *
     * @note
     * Because the implementation uses numeric comparison rather than explicit filtering,
     * the enum order is important for the logger behavior.
     */
enum class LogLevel
{
    None,
    Error,
    Warning,
    Log,
    All
};

/**
 * @brief Static utility class for engine logging.
 *
 * @details
 * DebugLogger stores a single global logging level and provides static functions
 * for writing log, warning, and error messages.
 *
 * In the current implementation:
 * - Log messages are written to std::cout.
 * - Warning messages are written to std::cerr.
 * - Error messages are written to std::cerr. :contentReference[oaicite:2]{index=2}
 *
 * Each output includes a severity prefix, source file name, source line, and message body.
 *
 * Typical usage is not to call Log(), Warn(), or Error() directly, but to use
 * the JIN_LOG, JIN_WRN, and JIN_ERR macros so file/line information is captured automatically.
 */
class DebugLogger
{
public:
    /**
     * @brief Sets the current global logging level.
     *
     * @details
     * All subsequent logging calls use this level to decide whether output should be emitted.
     *
     * @param level New logging level.
     *
     * @code
     * DebugLogger::SetLogLevel(LogLevel::Warning);
     * @endcode
     */
    static void SetLogLevel(LogLevel level);

    /**
     * @brief Returns the current global logging level.
     *
     * @return The currently active LogLevel value.
     */
    static LogLevel GetLogLevel();

    /**
     * @brief Writes a normal log message if the current level allows it.
     *
     * @details
     * The current implementation prints messages in the following format:
     * [LOG] file:line - message
     *
     * Output is written to std::cout when currentLevel >= LogLevel::Log. :contentReference[oaicite:3]{index=3}
     *
     * @param msg Message text to output.
     * @param file Source file name.
     * @param line Source line number.
     *
     * @note
     * Prefer using JIN_LOG instead of calling this function directly.
     */
    static void Log(const std::string& msg, const char* file, int line);

    /**
     * @brief Writes a warning message if the current level allows it.
     *
     * @details
     * The current implementation prints messages in the following format:
     * [WRN] file:line - message
     *
     * Output is written to std::cerr when currentLevel >= LogLevel::Warning. :contentReference[oaicite:4]{index=4}
     *
     * @param msg Message text to output.
     * @param file Source file name.
     * @param line Source line number.
     *
     * @note
     * Prefer using JIN_WRN instead of calling this function directly.
     */
    static void Warn(const std::string& msg, const char* file, int line);

    /**
     * @brief Writes an error message if the current level allows it.
     *
     * @details
     * The current implementation prints messages in the following format:
     * [ERR] file:line - message
     *
     * Output is written to std::cerr when currentLevel >= LogLevel::Error. :contentReference[oaicite:5]{index=5}
     *
     * @param msg Message text to output.
     * @param file Source file name.
     * @param line Source line number.
     *
     * @note
     * Prefer using JIN_ERR instead of calling this function directly.
     */
    static void Error(const std::string& msg, const char* file, int line);

private:
    /**
     * @brief Stores the current global logging level.
     *
     * @details
     * The current Debug.cpp implementation initializes this value to LogLevel::Log,
     * meaning standard logs, warnings, and errors are enabled by default. :contentReference[oaicite:6]{index=6}
     */
    static LogLevel currentLevel;
};