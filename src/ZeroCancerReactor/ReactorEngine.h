#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <deque>
#include <queue>
#include <shared_mutex>
#include <fstream>
#include <thread>
#include <condition_variable>
#include <string>
#include <chrono>

#include "Cellular/Cell.h"
#include "AI_Director/NatureDirector.h"
#include "CudaCore/CellularKernel.cuh"

namespace ZeroCancerReactor {

    // 🛑 PHASE 72: Forward Declaration to avoid circular dependencies
    namespace Interface {
        class BioTerminal;
    }

    namespace Core {

        struct BioTelemetry {
            size_t total_population = 0;
            size_t healthy_cells = 0;
            size_t senescent_cells = 0;
            size_t apoptotic_cells = 0;
            size_t oncogenic_cells = 0;

            double avg_telomere_length = 0.0;
            double avg_mutation_load = 0.0;
            double avg_cell_age = 0.0;
            double mortality_rate = 0.0;

            double tissue_toxicity = 0.0;
            double vascularization = 0.0;
            double systemic_inflammation = 0.0;
            double resource_drain = 0.0;
            double immortality_control = 0.0;
            double tumor_probability = 0.0;
            uint64_t epoch_ticks = 0;

            size_t active_exosomes_in_circulation = 0;
            size_t total_exosomes_absorbed = 0;

            double pid_control_signal = 0.0;
            double pid_p = 0.0;
            double pid_i = 0.0;
            double pid_d = 0.0;

            double avg_metabolic_exhaustion = 0.0;
            double hepatic_stress = 0.0;
            double renal_gfr = 0.0;
            double endocrine_sos_signal = 0.0;
            double hyper_hepatic_multiplier = 0.0;
            double hyper_renal_multiplier = 0.0;

            double brain_glucose_demand = 0.0;
            double cardiac_perfusion_rate = 0.0;
            double pulmonary_o2_capacity = 0.0;

            double endocrine_adrenaline = 0.0;
            double endocrine_cortisol = 0.0;
            double endocrine_dopamine = 0.0;
            double endocrine_testosterone = 0.0;
            double endocrine_estrogen = 0.0;

            double systemic_z_tumor_saturation = 0.0;

            // --- CYTOKINE NETWORK (The Blood Comms) ---
            double cytokine_il2 = 0.0;
            double cytokine_il4 = 0.0;
            double cytokine_il6 = 0.0;
            double cytokine_il10 = 0.0;
            double cytokine_il12 = 0.0;
            double cytokine_il13 = 0.0;
            double cytokine_il35 = 0.0;
            double cytokine_ifn_gamma = 0.0;
            double cytokine_tnf_alpha = 0.0;
            double cytokine_tgf_beta = 0.0;
            double cytokine_cxcl8 = 0.0;
            double cytokine_ccl2 = 0.0;
            double cytokine_g_csf = 0.0;
            double cytokine_m_csf = 0.0;

            // --- WBC LINEAGES (The Army) ---
            double wbc_neutrophils = 0.0;
            double wbc_macrophage_m1 = 0.0;
            double wbc_macrophage_m2 = 0.0;
            double wbc_nk_cells = 0.0;
            double wbc_cd8_t_cells = 0.0;
            double wbc_cd4_t_cells = 0.0;
            double wbc_b_cells = 0.0;
            double wbc_tregs = 0.0;
            double wbc_tcf1_stem_like_cd8 = 0.0;

            // --- EFFECTOR MECHANISMS (Weapons & Repair) ---
            double effector_perforin_granzyme = 0.0;
            double effector_fas_fasl = 0.0;
            double effector_mac_complement = 0.0;
            double effector_phagocytosis_ros = 0.0;
            double effector_tissue_regen = 0.0;

            // --- TOLERANCE & CHECKPOINTS ---
            double checkpoint_pd1_pdl1_axis = 0.0;
            double checkpoint_ctla4_clash = 0.0;
            double checkpoint_treg_aura = 0.0;
            double checkpoint_t_cell_exhaustion_tim3 = 0.0;
            double checkpoint_t_cell_exhaustion_lag3 = 0.0;
        };

        struct BiologicalPID {
            double base_kp = 3.5;
            double kp = 3.5;
            double ki = 0.05;
            double kd = 12.0;
            double treg_multiplier = 2.5;
            double integral_error = 0.0;
            double previous_error = 0.0;
            double base_target_ratio = 0.25;
            double dynamic_target_ratio = 0.25;
            double immune_exhaustion = 0.0;

            void Calculate(double current_ratio, double dt, double systemic_exosome_concentration, double treg_aura, double& out_p, double& out_i, double& out_d, double& out_signal);
            void Reset();
        };

        class ExosomeNetwork {
        private:
            std::deque<Cellular::Exosome> message_queue_;
            mutable std::shared_mutex rw_mtx_;
            std::atomic<size_t> total_absorbed_{ 0 };
        public:
            ExosomeNetwork() = default;
            ~ExosomeNetwork() = default;
            void Transmit(const Cellular::Exosome& packet);
            bool Receive(Cellular::Exosome& out_packet);
            bool HasPackets() const;
            void FlushDegraded();
            size_t GetActiveCount() const;
            void RecordAbsorption();
            size_t GetTotalAbsorbed() const;
            void Reset();
        };

        class AsyncDataLogger {
        private:
            std::string filepath_;
            std::ofstream file_;
            std::vector<BioTelemetry> front_buffer_;
            std::vector<BioTelemetry> back_buffer_;
            std::mutex mtx_;
            std::condition_variable cv_;
            std::thread worker_;
            std::atomic<bool> stop_{ false };
            void WorkerLoop();
        public:
            AsyncDataLogger();
            ~AsyncDataLogger();
            AsyncDataLogger(const AsyncDataLogger&) = delete;
            AsyncDataLogger& operator=(const AsyncDataLogger&) = delete;
            void Start(const std::string& prefix = "reactor_telemetry");
            void Stop();
            void LogTick(const BioTelemetry& data);
            std::string GetFilePath() const;
        };

        struct ReactorSnapshot {
            BioTelemetry telemetry;
            std::vector<Cellular::Cell> cells_buffer;
        };

        class ReactorEngine {
        private:
            CudaCore::CellularKernelEngine gpu_engine_;

            ReactorSnapshot snapshots_[3];
            std::atomic<int> ui_reading_idx_{ 0 };
            std::atomic<int> latest_written_idx_{ -1 };
            int engine_writing_idx_ = 1;

            AI_Director::NatureDirector nature_director_;
            ExosomeNetwork exosome_network_;
            BiologicalPID pid_controller_;
            AsyncDataLogger data_logger_;

            // 🛑 PHASE 72: Pointer to BioTerminal for internal SentinelGuard execution
            Interface::BioTerminal* terminal_ = nullptr;

            size_t base_carrying_capacity_;
            size_t dynamic_carrying_capacity_;
            uint64_t tick_counter_;
            double accumulated_toxicity_;

            std::thread engine_worker_;
            std::atomic<bool> engine_running_{ false };

            bool tumor_incubation_phase_;
            double max_historical_cancer_ratio_;

            void UpdateTelemetryFromGPU(BioTelemetry& tel, const CudaCore::GpuTelemetry& gpu_tel, size_t current_pop, double current_concentration);
            void ProcessExosomeBroadcast(std::vector<Cellular::Cell>& target_buffer);
            void EngineLoop();
            void PerformBiologicalTickInternal();

        public:
            ReactorEngine();
            ~ReactorEngine();
            ReactorEngine(const ReactorEngine&) = delete;
            ReactorEngine& operator=(const ReactorEngine&) = delete;

            void InitializeReactor(size_t initial_population, size_t carrying_capacity);
            void StartEngine();
            void StopEngine();

            AI_Director::BioEnvironment GetEnvironmentState() const;
            AI_Director::ImmuneSystem GetImmuneState() const;

            void SyncAndFetchUIState(BioTelemetry& out_tel, std::vector<Cellular::Cell*>& out_cell_refs);

            // 🛑 PHASE 72: Attach the terminal to the engine for decoupled processing
            void AttachTerminal(Interface::BioTerminal* terminal) { terminal_ = terminal; }

            std::string GetTelemetryFilePath() const;
        };
    }
}