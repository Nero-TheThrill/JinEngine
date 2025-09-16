#pragma once
#include <iostream>
#include <sstream>
#pragma once

/**
 * @brief Macro that extracts only the filename from __FILE__.
 */
#define FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

 /**
  * @brief Logging macro for general information.
  *
  * @details
  * Wraps DebugLogger::Log() with filename and line number.
  * Usage:
  * @code
  * SNAKE_LOG("Player position: " << pos);
  * @endcode
  */
#define SNAKE_LOG(x)   do { std::ostringstream oss; oss << x; DebugLogger::Log(oss.str(), FILENAME, __LINE__); } while(0)

  /**
   * @brief Logging macro for warnings.
   *
   * @details
   * Wraps DebugLogger::Warn() with filename and line number.
   * Usage:
   * @code
   * SNAKE_WRN("Enemy count exceeded expected maximum!");
   * @endcode
   */
#define SNAKE_WRN(x)   do { std::ostringstream oss; oss << x; DebugLogger::Warn(oss.str(), FILENAME, __LINE__); } while(0)

   /**
    * @brief Logging macro for errors.
    *
    * @details
    * Wraps DebugLogger::Error() with filename and line number.
    * Usage:
    * @code
    * SNAKE_ERR("Failed to load texture: " << path);
    * @endcode
    */
#define SNAKE_ERR(x)   do { std::ostringstream oss; oss << x; DebugLogger::Error(oss.str(), FILENAME, __LINE__); } while(0)

    /**
     * @brief Logging severity levels.
     */
enum class LogLevel
{
    None,    ///< No logging output.
    Error,   ///< Only errors are logged.
    Warning, ///< Errors and warnings are logged.
    Log,     ///< Errors, warnings, and general logs are logged.
    All      ///< All log levels are enabled.
};

/**
 * @brief Static logger class providing logging functions with levels.
 *
 * @details
 * DebugLogger provides static functions for logging with severity levels
 * (Log, Warn, Error). It uses the currently set log level to filter output.
 *
 * Each log message includes the originating filename and line number.
 */
class DebugLogger
{
public:
    /**
     * @brief Sets the current logging level.
     * @param level Desired logging level.
     */
    static void SetLogLevel(LogLevel level);

    /**
     * @brief Returns the current logging level.
     * @return Current log level.
     */
    static LogLevel GetLogLevel();

    /**
     * @brief Logs a general information message.
     * @param msg Log message.
     * @param file Originating filename.
     * @param line Originating line number.
     */
    static void Log(const std::string& msg, const char* file, int line);

    /**
     * @brief Logs a warning message.
     * @param msg Log message.
     * @param file Originating filename.
     * @param line Originating line number.
     */
    static void Warn(const std::string& msg, const char* file, int line);

    /**
     * @brief Logs an error message.
     * @param msg Log message.
     * @param file Originating filename.
     * @param line Originating line number.
     */
    static void Error(const std::string& msg, const char* file, int line);

private:
    static LogLevel currentLevel; ///< Currently active logging level.
};
