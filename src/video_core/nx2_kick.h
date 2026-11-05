// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>

#include "common/thread.h"

namespace VideoCommon {

/// Wakes the GPU thread while it polls its suspended channels: set when a command is queued,
/// when the fencing thread has run released operations and when a GPU write meets a waited
/// semaphore. It slept 1 ms between polls.
inline Common::Event& Nx2GpuKick() {
    static Common::Event event;
    return event;
}

/// Parts of the GPU-to-CPU synchronisation of High accuracy kept while the accuracy setting is
/// Low (which the split GPU thread needs). Low's shortcuts let Ocarina of Time read results
/// early: white flashes, its terrain culled for a frame.
enum class Nx2Sync : unsigned {
    Fences = 1,       ///< fences run their operations once the host GPU passed them
    FenceReports = 2, ///< fence reports wait for the host GPU instead of being written at once
    Payloads = 4,     ///< payload releases wait for the commands before them
    CpuReads = 8,     ///< CPU reads of memory the GPU wrote flush it first
};

/// EDEN_NX2_ACCURATE_SYNC=<mask of Nx2Sync parts>, default Payloads: 0 is Low's own behaviour,
/// 15 everything. In Ocarina of Time's Kokiri Forest walk, Payloads halves the white flashes for
/// no measurable cost; Fences remove the rest but cost a third of the frame rate (the guest CPU
/// then waits for the host GPU, as with EDEN_NX2_SPLIT=0). FenceReports needs Fences (its reports
/// are written by fence operations, which only wait with Fences).
inline bool Nx2AccurateSync(Nx2Sync part) {
    static const unsigned mask = [] {
        const char* value = std::getenv("EDEN_NX2_ACCURATE_SYNC");
        unsigned parts = value == nullptr ? static_cast<unsigned>(Nx2Sync::Payloads)
                                          : static_cast<unsigned>(std::strtoul(value, nullptr, 0));
        if ((parts & static_cast<unsigned>(Nx2Sync::Fences)) == 0) {
            parts &= ~static_cast<unsigned>(Nx2Sync::FenceReports);
        }
        return parts;
    }();
    return (mask & static_cast<unsigned>(part)) != 0;
}

} // namespace VideoCommon
