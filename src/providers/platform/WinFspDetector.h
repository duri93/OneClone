#pragma once

#include "src/providers/platform/MountBackendDetector.h"

// ---------------------------------------------------------------------------
// WinFspDetector
// Detects whether WinFsp is installed, via the registry and, as a fallback,
// its default install locations.
// ---------------------------------------------------------------------------
class WinFspDetector : public MountBackendDetector
{
public:
    bool isAvailable() const override;
};
