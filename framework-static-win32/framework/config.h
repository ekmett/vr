#pragma once

// windows supports precompiled headers
#define FRAMEWORK_USE_STDAFX

#define FRAMEWORK_PLATFORM_WIN32

// notably not set on ARM
#define FRAMEWORK_MEMORYMODEL_TSO

// Simple Direct Media Layer 2
#define FRAMEWORK_SUPPORTS_SDL2

#define FRAMEWORK_SUPPORTS_OPENAL_SOFT

// includes GLEW for now
#define FRAMEWORK_SUPPORTS_OPENGL

#define FRAMEWORK_SUPPORTS_OPENVR

// Oculus VR
#define FRAMEWORK_SUPPORTS_OCULUS

// Stock Concurrent Data Structures
#define FRAMEWORK_SUPPORTS_CDS

