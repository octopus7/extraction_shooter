#include "Modules/ModuleManager.h"

// Default materials and profiles are authored assets checked into plugin Content.
// Keep the module identity stable without generating or resaving them on startup.
IMPLEMENT_MODULE(FDefaultModuleImpl, TunaWarpTransitionEditor)
