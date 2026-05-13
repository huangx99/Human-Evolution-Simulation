#pragma once

class SystemContext;
class HistoryModule;

// IHistoryDetector: detects significant historical events from world state.
//
// Implementations live in rules/ (e.g. FirstStableFireUsageDetector, MassDeathDetector).
// The engine runs detectors via HistoryDetectionSystem; it does not know about specific events.

class IHistoryDetector
{
public:
    virtual ~IHistoryDetector() = default;

    // Scan world state and emit HistoryEvents into the module.
    virtual void Scan(SystemContext& ctx, HistoryModule& history) = 0;
};
