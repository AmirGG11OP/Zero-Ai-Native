#define NOMINMAX
#include "Shield/SentinelGuard.h"
#include "CudaCore/CellularKernel.cuh"
#include <random>

namespace ZeroCancerReactor {
    namespace Shield {

        void SentinelGuard::PerformSecurityScan(const std::vector<Cellular::Cell*>& target_pool, Interface::BioTerminal& terminal, const Core::BioTelemetry& telemetry) {
            auto* gpu_engine = CudaCore::CellularKernelEngine::GetInstance();
            if (!gpu_engine || gpu_engine->GetCurrentPopulation() == 0) return;

            double perforin_pressure = telemetry.effector_perforin_granzyme;
            double mac_pressure = telemetry.effector_mac_complement;
            double pd1_pdl1_evasion = telemetry.checkpoint_pd1_pdl1_axis;
            double treg_aura = telemetry.checkpoint_treg_aura;

            // 🛑 PHASE 69: Lotka-Volterra Predator-Prey Logic
            // Massive pruning occurs only during a complete system crash
            bool massive_pruning = (perforin_pressure > 0.65 || mac_pressure > 0.75);

            // The magic oscillation layer. When tumor hits 80%, PD-L1 drops, perforin rises to ~0.35.
            // This gracefully skims the weakest 5% without triggering a TLS cytokine storm.
            bool lotka_volterra_pruning = (perforin_pressure > 0.25 && perforin_pressure <= 0.65 && treg_aura <= 0.85 && telemetry.oncogenic_cells > 0);

            // Absolute stealth phase, keeping the rest of the body clean
            bool micro_pruning = (perforin_pressure > 0.05 && perforin_pressure <= 0.25 && treg_aura > 0.80);

            bool suppressed_mode = (pd1_pdl1_evasion > 0.85 || treg_aura > 0.90);
            bool mvg_lockdown = (telemetry.oncogenic_cells > 0 && telemetry.oncogenic_cells <= MVG_THRESHOLD);

            double dynamic_threshold = BASE_ANOMALY_THRESHOLD;
            double biological_attack_signal = 0.0;

            if (massive_pruning && !mvg_lockdown) {
                dynamic_threshold = std::max(0.20, 0.85 - (perforin_pressure * 0.5));
                biological_attack_signal = perforin_pressure + (mac_pressure * 0.5);
            }
            else if (lotka_volterra_pruning && !mvg_lockdown) {
                // The 75-80% Equilibrium Bounce! Low threshold to prune senescent/weak tumor cells gently.
                dynamic_threshold = 0.50;
                biological_attack_signal = perforin_pressure * 0.4;
            }
            else if (micro_pruning && !mvg_lockdown && !suppressed_mode) {
                dynamic_threshold = 0.90;
                biological_attack_signal = perforin_pressure * 0.15;
            }
            else if (suppressed_mode || mvg_lockdown) {
                dynamic_threshold = 0.98;
                biological_attack_signal = -1.0 * std::max(pd1_pdl1_evasion, treg_aura);
            }

            bool host_grace_period = (telemetry.total_population < 35000000);
            if (host_grace_period) {
                massive_pruning = false;
                lotka_volterra_pruning = false;
                micro_pruning = false;
                suppressed_mode = false;
                dynamic_threshold = 0.99;
                biological_attack_signal = 0.0;
            }

            CudaCore::SentinelTelemetry out_tel = { 0, 0, 0, 0 };
            gpu_engine->RunSentinelGuard(biological_attack_signal, massive_pruning || lotka_volterra_pruning || micro_pruning, suppressed_mode, mvg_lockdown, dynamic_threshold, telemetry.epoch_ticks, out_tel);

            std::mt19937_64 rng(telemetry.epoch_ticks);
            std::uniform_real_distribution<double> dist(0.0, 1.0);

            if (massive_pruning && out_tel.pruned_tumor_cells > 0 && !host_grace_period) {
                terminal.AddLog(Interface::LogLevel::WARNING,
                    "[ACUTE NECROSIS WARNING] Massive CD8+ execution. Pruned %llu cells. High risk of Tumor Lysis Syndrome.",
                    out_tel.pruned_tumor_cells);
            }

            // 🛑 PHASE 69: Visualizing the Equilibrium Pulse
            if (lotka_volterra_pruning && out_tel.pruned_tumor_cells > 0 && dist(rng) < 0.02) {
                terminal.AddLog(Interface::LogLevel::INFO,
                    "[LOTKA-VOLTERRA EQUILIBRIUM] Nutrient stress detected at ~80%% mass. Soft 5%% pruning engaged. Pruned %llu weak cells.", out_tel.pruned_tumor_cells);
            }

            if (micro_pruning && out_tel.pruned_tumor_cells > 0 && dist(rng) < 0.015) {
                terminal.AddLog(Interface::LogLevel::INFO,
                    "[MICRO-TOLEROGENIC SYMBIOSIS] Continuous invisible immune maintenance. Pruned %llu outlier cells. Host is painless.", out_tel.pruned_tumor_cells);
            }

            if (telemetry.hepatic_stress < 0.05 && telemetry.oncogenic_cells > 10000000 && dist(rng) < 0.02) {
                terminal.AddLog(Interface::LogLevel::INFO,
                    "[HEPATIC SYMBIOSIS] Liver Kupffer cells adapted via Cori Cycle. Tumor lactate converted to Glucose. Hepatic Stress completely mitigated.");
            }

            if (telemetry.cytokine_il6 > 0.8 && dist(rng) < 0.05) {
                terminal.AddLog(Interface::LogLevel::CRITICAL,
                    "[CYTOKINE STORM] IL-6 and TNF-a at critical levels! Acute systemic inflammation in progress.");
            }

            if (pd1_pdl1_evasion > 0.5 && dist(rng) < 0.05 && telemetry.oncogenic_cells > 0 && !lotka_volterra_pruning) {
                terminal.AddLog(Interface::LogLevel::CRITICAL,
                    "[IMMUNE CHECKPOINT HACKED] Tumor PD-L1 engaged CD8+ PD-1. Evasion Axis: %.0f%%. Immune response mathematically nullified.", pd1_pdl1_evasion * 100.0);
            }

            if (telemetry.wbc_macrophage_m2 > 0.7 && dist(rng) < 0.03) {
                terminal.AddLog(Interface::LogLevel::INFO,
                    "[TISSUE REGENERATION] M2 Macrophages secreting Wnt/VEGF. Efferocytosis in progress to prevent collateral organ failure.");
            }

            if (mvg_lockdown && (massive_pruning || lotka_volterra_pruning || micro_pruning) && out_tel.mvg_saved_cells > 0 && dist(rng) < 0.1 && !host_grace_period) {
                terminal.AddLog(Interface::LogLevel::CRITICAL,
                    "[TREG RIOT POLICE] Regulatory T-Cells deployed! TGF-beta aura active. Tumor at critical mass (%llu). Immune attack blocked.", telemetry.oncogenic_cells);
            }

            if (out_tel.purged_count > 0 && !massive_pruning && !lotka_volterra_pruning && !micro_pruning && !mvg_lockdown && !host_grace_period) {
                terminal.AddLog(Interface::LogLevel::INFO,
                    "[NK CELLS PATROL] Natural Killers cleansed %llu mutated anomalies independent of MHC-I restriction.", out_tel.purged_count);
            }

            if (out_tel.bypassed_count > 0 && telemetry.oncogenic_cells == 0 && !host_grace_period) {
                terminal.AddLog(Interface::LogLevel::CRITICAL,
                    "[SENTINEL FAILURE] %llu anomalies bypassed T-Cell surveillance! ONCOGENESIS IMMINENT.", out_tel.bypassed_count);
            }

            if ((telemetry.checkpoint_t_cell_exhaustion_tim3 > 0.8 || telemetry.checkpoint_t_cell_exhaustion_lag3 > 0.8) && dist(rng) < 0.02) {
                terminal.AddLog(Interface::LogLevel::WARNING,
                    "[T-CELL EXHAUSTION] Chronic antigen exposure detected. TIM-3/LAG-3 receptors expressed. Lymphocytes metabolically depleted. TIM-3 Level: %.1f%%", telemetry.checkpoint_t_cell_exhaustion_tim3 * 100.0);
            }

            if (host_grace_period && telemetry.epoch_ticks % 1000 == 0) {
                terminal.AddLog(Interface::LogLevel::INFO, "[SENTINEL] Host incubation active. Aggressive immunity suspended to ensure tissue maturity.");
            }
        }

    }
}