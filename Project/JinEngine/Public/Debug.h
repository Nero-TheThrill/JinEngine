#pragma once
#include <iostream>
#include <sstream>
#pragma once

/**
 * @brief Extracts the file name from the full __FILE__ path.
 *
 * @details
 * This macro strips the directory path from the __FILE__ macro
 * and returns only the file name portion.
 *
 * On Windows-style paths, it searches for the last '\\' character.
 * If no separator is found, __FILE__ is returned as-is.
 */
#define FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

 /**
  * @brief Logs a normal debug message with file and line information.
  *
  * @details
  * This macro:
  * - Streams the input expression into an std::ostringstream
  * - Converts it to a std::string
  * - Forwards the message to DebugLogger::Log along with file and line metadata
  *
  * Usage example:
  * @code
  * JIN_LOG("Player position: " << position.x << ", " << position.y);
  * @endcode
  */
#define JIN_LOG(x)   do { std::ostringstream oss; oss << x; DebugLogger::Log(oss.str(), FILENAME, __LINE__); } while(0)

  /**
   * @brief Logs a warning message with file and line information.
   *
   * @details
   * This macro behaves identically to JIN_LOG, but routes the message
   * through DebugLogger::Warn instead.
   */
#define JIN_WRN(x)   do { std::ostringstream oss; oss << x; DebugLogger::Warn(oss.str(), FILENAME, __LINE__); } while(0)

   /**
    * @brief Logs an error message with file and line information.
    *
    * @details
    * This macro behaves identically to JIN_LOG, but routes the message
    * through DebugLogger::Error instead.
    */
#define JIN_ERR(x)   do { std::ostringstream oss; oss << x; DebugLogger::Error(oss.str(), FILENAME, __LINE__); } while(0)

    /**
     * @brief Severity level used to filter log output.
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
 * @brief Static logging utility for debug output.
 *
 * @details
 * DebugLogger provides a centralized, static interface for logging
 * messages with different severity levels.
 *
 * Log output can be filtered by setting a global LogLevel.
 * Each log entry includes:
 * - Message string
 * - Source file name
 * - Source line number
 *
 * This class is intended to be used through the JIN_LOG / JIN_WRN / JIN_ERR macros.
 */
class DebugLogger
{
public:
    /**
     * @brief Sets the global log filtering level.
     *
     * @param level Minimum LogLevel that will be printed.
     */
    static void SetLogLevel(LogLevel level);

    /**
     * @brief Returns the current global log filtering level.
     *
     * @return Currently active LogLevel.
     */
    static LogLevel GetLogLevel();

    /**
     * @brief Logs a standard debug message.
     *
     * @param msg Log message text.
     * @param file Source file name.
     * @param line Source line number.
     */
    static void Log(const std::string& msg, const char* file, int line);

    /**
     * @brief Logs a warning message.
     *
     * @param msg Warning message text.
     * @param file Source file name.
     * @param line Source line number.
     */
    static void Warn(const std::string& msg, const char* file, int line);

    /**
     * @brief Logs an error message.
     *
     * @param msg Error message text.
     * @param file Source file name.
     * @param line Source line number.
     */
    static void Error(const std::string& msg, const char* file, int line);

private:
    /**
     * @brief Currently active global log level.
     */
    static LogLevel currentLevel;
};
