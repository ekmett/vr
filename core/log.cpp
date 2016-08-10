#include "log.h"

using namespace spdlog;

static inline level::level_enum gl_log_severity(GLenum severity) {
  using namespace level;
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH: return critical;
  case GL_DEBUG_SEVERITY_MEDIUM: return warn;
  default: return info;
  }
}

static void APIENTRY callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
  ((logger*)userParam)->log(gl_log_severity(severity), "{} {}: {} ({})", show_debug_source(source), show_debug_message_type(type), message, id);
}

gl_logger::gl_logger(std::shared_ptr<spdlog::logger> & logger): logger(logger) {
  // initialize callback 
  glDebugMessageCallback((GLDEBUGPROC)callback, (void*)logger._Get());
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 5, "start");
}

gl_logger::~gl_logger() {
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_LOW, 4, "stop");
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
  glDebugMessageCallback(nullptr, nullptr);
  logger->flush();
}