#pragma once
#include <vector>
#include <mutex>
#include "Cellular/Cell.h"
#include "Interface/BioTerminal.h"
#include "Core/ReactorEngine.h" 
#include "AI_Director/NatureDirector.h" 

namespace ZeroCancerReactor {
    namespace Shield {

        class SentinelGuard {
        private:
            SentinelGuard() = delete;
            ~SentinelGuard() = delete;
            SentinelGuard(const SentinelGuard&) = delete;
            SentinelGuard& operator=(const SentinelGuard&) = delete;

            static constexpr double BASE_ANOMALY_THRESHOLD = 0.85;
            static constexpr size_t MVG_THRESHOLD = 3;

        public:
            static void PerformSecurityScan(const std::vector<Cellular::Cell*>& target_pool, Interface::BioTerminal& terminal, const Core::BioTelemetry& telemetry);
        };

    }
}