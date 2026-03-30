#pragma once

// Xcode 26.3's macOS 26.2 SDK imports this legacy header from Foundation.h,
// but the header is missing from the SDK bundle. A forward declaration is
// sufficient for modern JUCE builds because no concrete GC API is used.
@class NSGarbageCollector;
