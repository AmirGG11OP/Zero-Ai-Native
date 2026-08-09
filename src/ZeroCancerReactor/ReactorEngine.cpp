/**
 * @file ReactorEngine.cpp
 * @brief Core execution and internal Bio-Logistics for the simulation (Phase 72: Core Migration & UI Thread Liberation)
 * @note [SECURE SKELETON: EDUCATIONAL & RESEARCH BIO-SIMULATION ONLY]
 */

#define NOMINMAX 
#include <windows.h> 
#include "Core/ReactorEngine.h"
#include "Shield/SentinelGuard.h" // 🛑 PHASE 72: Included for Engine-Level Security Scan
#include <algorithm>
#include <execution> 
#include <numeric>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ZeroCancerReactor {
    namespace Core {

        AsyncDataLogger::AsyncDataLogger() : stop_(false) {
            front_buffer_.reserve(10000);
            back_buffer_.reserve(10000);
        }

        AsyncDataLogger::~AsyncDataLogger() { Stop(); }

        void AsyncDataLogger::Start(const std::string& prefix) {
            Stop();
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            struct tm time_info;
            localtime_s(&time_info, &in_time_t);
            char exe_path_buf[MAX_PATH];
            GetModuleFileNameA(NULL, exe_path_buf, MAX_PATH);
            std::string full_exe_path(exe_path_buf);
            size_t last_slash_idx = full_exe_path.find_last_of("\\/");
            std::string exe_dir = (last_slash_idx != std::string::npos) ? full_exe_path.substr(0, last_slash_idx + 1) : "";
            std::stringstream ss;
            ss << exe_dir << prefix << "_" << std::put_time(&time_info, "%Y%m%d_%H%M%S") << ".csv";
            filepath_ = ss.str();
            file_.open(filepath_, std::ios::out | std::ios::trunc);

            if (file_.is_open()) {
                file_ << "EpochTick,TotalPopulation,HealthyCells,SenescentCells,ApoptoticCells,OncogenicCells,"
                    << "AvgTelomere,AvgMutationLoad,AvgAge,MortalityRate,TissueToxicity,SystemicInflammation,"
                    << "ResourceDrain,ImmortalityControl,TumorProbability,ActiveExosomes,TotalExosomesAbsorbed,"
                    << "PID_Signal,PID_P,PID_I,PID_D,AvgExhaustion,HepaticStress,RenalGFR,"
                    << "EndocrineSOS,HyperHepatic,HyperRenal,"
                    << "BrainGlucose,CardiacPerfusion,PulmonaryO2,Adrenaline,Cortisol,Dopamine,Testosterone,Estrogen,ZTumorSaturation,"
                    << "IL2,IL4,IL6,IL10,IL12,IL13,IL35,IFNg,TNFa,TGFb,CXCL8,CCL2,G_CSF,M_CSF,"
                    << "Neutrophils,M1,M2,NK,CD8,CD4,BCells,Tregs,Tcf1_CD8,"
                    << "PerfGranzyme,FasL,MAC,ROS_Storm,TissueRegen,"
                    << "PDL1_Axis,CTLA4,TregAura,TIM3,LAG3\n";
            }
            stop_.store(false, std::memory_order_release);
            worker_ = std::thread(&AsyncDataLogger::WorkerLoop, this);
        }

        void AsyncDataLogger::Stop() {
            if (stop_.load(std::memory_order_acquire)) return;
            stop_.store(true, std::memory_order_release);
            cv_.notify_all();
            if (worker_.joinable()) worker_.join();
            if (file_.is_open()) file_.close();
            front_buffer_.clear();
            back_buffer_.clear();
        }

        void AsyncDataLogger::LogTick(const BioTelemetry& data) {
            if (stop_.load(std::memory_order_acquire)) return;
            bool should_notify = false;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                front_buffer_.push_back(data);
                if (front_buffer_.size() >= 1000) {
                    should_notify = true;
                }
            }
            if (should_notify) cv_.notify_one();
        }

        void AsyncDataLogger::WorkerLoop() {
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    cv_.wait_for(lock, std::chrono::milliseconds(500), [this]() {
                        return front_buffer_.size() >= 1000 || stop_.load(std::memory_order_acquire);
                        });

                    if (front_buffer_.empty() && stop_.load(std::memory_order_acquire)) break;
                    std::swap(front_buffer_, back_buffer_);
                }

                if (file_.is_open() && !back_buffer_.empty()) {
                    std::stringstream ss;
                    for (const auto& t : back_buffer_) {
                        ss << t.epoch_ticks << "," << t.total_population << ","
                            << t.healthy_cells << "," << t.senescent_cells << ","
                            << t.apoptotic_cells << "," << t.oncogenic_cells << ","
                            << t.avg_telomere_length << "," << t.avg_mutation_load << ","
                            << t.avg_cell_age << "," << t.mortality_rate << ","
                            << t.tissue_toxicity << "," << t.systemic_inflammation << ","
                            << t.resource_drain << "," << t.immortality_control << ","
                            << t.tumor_probability << "," << t.active_exosomes_in_circulation << ","
                            << t.total_exosomes_absorbed << "," << t.pid_control_signal << ","
                            << t.pid_p << "," << t.pid_i << "," << t.pid_d << ","
                            << t.avg_metabolic_exhaustion << "," << t.hepatic_stress << "," << t.renal_gfr << ","
                            << t.endocrine_sos_signal << "," << t.hyper_hepatic_multiplier << "," << t.hyper_renal_multiplier << ","
                            << t.brain_glucose_demand << "," << t.cardiac_perfusion_rate << "," << t.pulmonary_o2_capacity << ","
                            << t.endocrine_adrenaline << "," << t.endocrine_cortisol << "," << t.endocrine_dopamine << ","
                            << t.endocrine_testosterone << "," << t.endocrine_estrogen << "," << t.systemic_z_tumor_saturation << ","
                            << t.cytokine_il2 << "," << t.cytokine_il4 << "," << t.cytokine_il6 << "," << t.cytokine_il10 << ","
                            << t.cytokine_il12 << "," << t.cytokine_il13 << "," << t.cytokine_il35 << "," << t.cytokine_ifn_gamma << ","
                            << t.cytokine_tnf_alpha << "," << t.cytokine_tgf_beta << "," << t.cytokine_cxcl8 << "," << t.cytokine_ccl2 << ","
                            << t.cytokine_g_csf << "," << t.cytokine_m_csf << ","
                            << t.wbc_neutrophils << "," << t.wbc_macrophage_m1 << "," << t.wbc_macrophage_m2 << "," << t.wbc_nk_cells << ","
                            << t.wbc_cd8_t_cells << "," << t.wbc_cd4_t_cells << "," << t.wbc_b_cells << "," << t.wbc_tregs << "," << t.wbc_tcf1_stem_like_cd8 << ","
                            << t.effector_perforin_granzyme << "," << t.effector_fas_fasl << "," << t.effector_mac_complement << ","
                            << t.effector_phagocytosis_ros << "," << t.effector_tissue_regen << ","
                            << t.checkpoint_pd1_pdl1_axis << "," << t.checkpoint_ctla4_clash << "," << t.checkpoint_treg_aura << ","
                            << t.checkpoint_t_cell_exhaustion_tim3 << "," << t.checkpoint_t_cell_exhaustion_lag3 << "\n";
                    }
                    file_.write(ss.str().c_str(), ss.str().size());
                    file_.flush();
                    back_buffer_.clear();
                }
            }
            if (file_.is_open()) file_.flush();
        }

        std::string AsyncDataLogger::GetFilePath() const { return filepath_; }

        void BiologicalPID::Calculate(double current_ratio, double dt, double systemic_exosome_concentration, double treg_aura, double& out_p, double& out_i, double& out_d, double& out_signal) {
            dynamic_target_ratio = base_target_ratio + std::clamp(systemic_exosome_concentration * 10.0, 0.0, 0.25);
            double error = current_ratio - dynamic_target_ratio;

            if (error > 0.0 && systemic_exosome_concentration > 0.0001) {
                immune_exhaustion += (error * systemic_exosome_concentration * dt * 0.1);
            }
            else {
                immune_exhaustion -= (dt * 0.005);
            }
            immune_exhaustion = std::clamp(immune_exhaustion, 0.0, 0.95);

            kp = base_kp * (1.0 - immune_exhaustion);

            double current_kp = kp;
            if (error < 0.0) current_kp *= treg_multiplier;

            if (!(error > 0.0 && treg_aura > 0.85)) {
                integral_error += error * dt;
                integral_error = std::clamp(integral_error, -1.0, 1.0);
            }

            double derivative = (error - previous_error) / dt;
            previous_error = error;

            out_p = current_kp * error;
            out_i = ki * integral_error;
            out_d = kd * derivative;
            out_signal = std::clamp(out_p + out_i + out_d, -1.0, 1.0);
        }

        void BiologicalPID::Reset() {
            integral_error = 0.0; previous_error = 0.0; dynamic_target_ratio = base_target_ratio;
            immune_exhaustion = 0.0; kp = base_kp;
        }

        void ExosomeNetwork::Transmit(const Cellular::Exosome& packet) {
            std::unique_lock<std::shared_mutex> lock(rw_mtx_);
            message_queue_.push_back(packet);
        }
        bool ExosomeNetwork::Receive(Cellular::Exosome& out_packet) {
            std::unique_lock<std::shared_mutex> lock(rw_mtx_);
            if (message_queue_.empty()) return false;
            out_packet = message_queue_.front();
            message_queue_.pop_front();
            return true;
        }
        bool ExosomeNetwork::HasPackets() const {
            std::shared_lock<std::shared_mutex> lock(rw_mtx_);
            return !message_queue_.empty();
        }
        void ExosomeNetwork::FlushDegraded() {
            std::unique_lock<std::shared_mutex> lock(rw_mtx_);
            size_t keep_count = message_queue_.size() / 2;
            while (message_queue_.size() > keep_count) message_queue_.pop_front();
        }
        size_t ExosomeNetwork::GetActiveCount() const {
            std::shared_lock<std::shared_mutex> lock(rw_mtx_);
            return message_queue_.size();
        }
        void ExosomeNetwork::RecordAbsorption() { total_absorbed_.fetch_add(1, std::memory_order_relaxed); }
        size_t ExosomeNetwork::GetTotalAbsorbed() const { return total_absorbed_.load(std::memory_order_relaxed); }
        void ExosomeNetwork::Reset() {
            std::unique_lock<std::shared_mutex> lock(rw_mtx_);
            message_queue_.clear();
            total_absorbed_.store(0);
        }

        ReactorEngine::~ReactorEngine() {
            StopEngine();
            data_logger_.Stop();
        }

        ReactorEngine::ReactorEngine()
            : base_carrying_capacity_(10000), dynamic_carrying_capacity_(10000),
            tick_counter_(0), accumulated_toxicity_(0.0),
            tumor_incubation_phase_(false), max_historical_cancer_ratio_(0.0) {
        }

        void ReactorEngine::InitializeReactor(size_t initial_population, size_t carrying_capacity) {
            StopEngine();
            base_carrying_capacity_ = carrying_capacity;
            dynamic_carrying_capacity_ = carrying_capacity;
            tick_counter_ = 0;
            accumulated_toxicity_ = 0.0;
            tumor_incubation_phase_ = false;
            max_historical_cancer_ratio_ = 0.0;

            exosome_network_.Reset();
            pid_controller_.Reset();
            data_logger_.Start("ZeroCancerReactor_Log");

            size_t MAX_70M_LIMIT = 70000000;
            gpu_engine_.AllocateVRAM(MAX_70M_LIMIT, initial_population);

            snapshots_[0].cells_buffer.resize(1500);
            snapshots_[1].cells_buffer.resize(1500);
            snapshots_[2].cells_buffer.resize(1500);

            ui_reading_idx_.store(0);
            latest_written_idx_.store(-1);
            engine_writing_idx_ = 1;
        }

        void ReactorEngine::StartEngine() {
            if (engine_running_.load()) return;
            engine_running_ = true;
            engine_worker_ = std::thread(&ReactorEngine::EngineLoop, this);
        }

        void ReactorEngine::StopEngine() {
            if (!engine_running_.load()) return;
            engine_running_ = false;
            if (engine_worker_.joinable()) engine_worker_.join();
        }

        void ReactorEngine::EngineLoop() {
            const double TARGET_TICK_MS = 1000.0 / 60.0;

            while (engine_running_.load(std::memory_order_relaxed)) {
                auto start_time = std::chrono::high_resolution_clock::now();

                PerformBiologicalTickInternal();

                auto end_time = std::chrono::high_resolution_clock::now();
                double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

                if (elapsed_ms < TARGET_TICK_MS) {
                    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(TARGET_TICK_MS - elapsed_ms));
                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        }

        void ReactorEngine::UpdateTelemetryFromGPU(BioTelemetry& current_telemetry, const CudaCore::GpuTelemetry& gpu_tel, size_t current_pop, double current_concentration) {
            current_telemetry.total_population = current_pop;
            current_telemetry.healthy_cells = gpu_tel.total_healthy;
            current_telemetry.senescent_cells = gpu_tel.total_senescent;
            current_telemetry.apoptotic_cells = gpu_tel.total_apoptotic;
            current_telemetry.oncogenic_cells = gpu_tel.total_oncogenic;

            double cancer_ratio = current_pop > 0 ? static_cast<double>(current_telemetry.oncogenic_cells) / current_pop : 0.0;

            if (current_pop > 0) {
                current_telemetry.avg_telomere_length = gpu_tel.total_telomere / current_pop;
                current_telemetry.avg_mutation_load = gpu_tel.total_mutation / current_pop;
                current_telemetry.avg_cell_age = gpu_tel.total_age / current_pop;
                current_telemetry.avg_metabolic_exhaustion = gpu_tel.total_exhaustion / current_pop;
                current_telemetry.systemic_z_tumor_saturation = std::max(0.0, cancer_ratio / 0.28);
            }
            else {
                current_telemetry.avg_telomere_length = 0.0;
                current_telemetry.avg_mutation_load = 0.0;
                current_telemetry.avg_cell_age = 0.0;
                current_telemetry.avg_metabolic_exhaustion = 0.0;
                current_telemetry.systemic_z_tumor_saturation = 0.0;
            }

            auto organs = nature_director_.GetHostOrgansState();
            current_telemetry.hepatic_stress = organs.hepatic_stress;
            current_telemetry.renal_gfr = organs.renal_gfr;
            current_telemetry.endocrine_sos_signal = organs.endocrine_sos_signal;
            current_telemetry.hyper_hepatic_multiplier = organs.hyper_hepatic_multiplier;
            current_telemetry.hyper_renal_multiplier = organs.hyper_renal_multiplier;

            auto vital = nature_director_.GetVitalOrgansState();
            auto endo = nature_director_.GetEndocrineState();

            current_telemetry.brain_glucose_demand = vital.brain_glucose_demand;
            current_telemetry.cardiac_perfusion_rate = vital.cardiac_perfusion_rate;
            current_telemetry.pulmonary_o2_capacity = vital.pulmonary_o2_capacity;

            current_telemetry.endocrine_adrenaline = endo.adrenaline;
            current_telemetry.endocrine_cortisol = endo.cortisol;
            current_telemetry.endocrine_dopamine = endo.dopamine;
            current_telemetry.endocrine_testosterone = endo.testosterone;
            current_telemetry.endocrine_estrogen = endo.estrogen;

            auto immune_snap = nature_director_.GetImmuneState();

            current_telemetry.cytokine_il2 = immune_snap.cytokines.il_2;
            current_telemetry.cytokine_il4 = immune_snap.cytokines.il_4;
            current_telemetry.cytokine_il6 = immune_snap.cytokines.il_6;
            current_telemetry.cytokine_il10 = immune_snap.cytokines.il_10;
            current_telemetry.cytokine_il12 = immune_snap.cytokines.il_12;
            current_telemetry.cytokine_il13 = immune_snap.cytokines.il_13;
            current_telemetry.cytokine_il35 = immune_snap.cytokines.il_35;
            current_telemetry.cytokine_ifn_gamma = immune_snap.cytokines.ifn_gamma;
            current_telemetry.cytokine_tnf_alpha = immune_snap.cytokines.tnf_alpha;
            current_telemetry.cytokine_tgf_beta = immune_snap.cytokines.tgf_beta;
            current_telemetry.cytokine_cxcl8 = immune_snap.cytokines.cxcl8;
            current_telemetry.cytokine_ccl2 = immune_snap.cytokines.ccl2;
            current_telemetry.cytokine_g_csf = immune_snap.cytokines.g_csf;
            current_telemetry.cytokine_m_csf = immune_snap.cytokines.m_csf;

            current_telemetry.wbc_neutrophils = immune_snap.cells.neutrophils;
            current_telemetry.wbc_macrophage_m1 = immune_snap.cells.macrophages_m1;
            current_telemetry.wbc_macrophage_m2 = immune_snap.cells.macrophages_m2;
            current_telemetry.wbc_nk_cells = immune_snap.cells.nk_cells;
            current_telemetry.wbc_cd8_t_cells = immune_snap.cells.cd8_cytotoxic_t;
            current_telemetry.wbc_cd4_t_cells = immune_snap.cells.cd4_helper_t;
            current_telemetry.wbc_b_cells = immune_snap.cells.b_cells_antibodies;
            current_telemetry.wbc_tregs = immune_snap.cells.regulatory_t_cells;
            current_telemetry.wbc_tcf1_stem_like_cd8 = immune_snap.cells.tcf1_stem_like_cd8;

            current_telemetry.effector_perforin_granzyme = immune_snap.effectors.perforin_granzyme_pathway;
            current_telemetry.effector_fas_fasl = immune_snap.effectors.fas_fasl_death_kiss;
            current_telemetry.effector_mac_complement = immune_snap.effectors.mac_complement_system;
            current_telemetry.effector_phagocytosis_ros = immune_snap.effectors.phagocytosis_ros_storm;
            current_telemetry.effector_tissue_regen = immune_snap.effectors.tissue_regeneration;

            current_telemetry.checkpoint_pd1_pdl1_axis = immune_snap.checkpoints.pdl1_binding_axis;
            current_telemetry.checkpoint_ctla4_clash = immune_snap.checkpoints.ctla4_cd28_clash;
            current_telemetry.checkpoint_treg_aura = immune_snap.checkpoints.treg_suppression_aura;
            current_telemetry.checkpoint_t_cell_exhaustion_tim3 = immune_snap.checkpoints.t_cell_exhaustion_tim3;
            current_telemetry.checkpoint_t_cell_exhaustion_lag3 = immune_snap.checkpoints.t_cell_exhaustion_lag3;

            current_telemetry.systemic_inflammation = immune_snap.systemic_inflammation;

            current_telemetry.tissue_toxicity = accumulated_toxicity_;
            current_telemetry.vascularization = static_cast<double>(dynamic_carrying_capacity_) / static_cast<double>(base_carrying_capacity_);

            double pop_ratio = current_pop > 0 ? static_cast<double>(current_pop) / base_carrying_capacity_ : 0.0;
            size_t deaths_this_tick = gpu_tel.clean_deaths_this_tick + gpu_tel.necrotic_deaths_this_tick;
            current_telemetry.mortality_rate = current_pop > 0 ? static_cast<double>(deaths_this_tick) / current_pop : 0.0;

            current_telemetry.tumor_probability = std::clamp(current_telemetry.avg_mutation_load + current_telemetry.tissue_toxicity, 0.0, 1.0);

            if (current_pop > 0) {
                current_telemetry.immortality_control = std::clamp(1.0 - std::abs(0.265 - cancer_ratio) * 12.0, 0.0, 1.0);
                current_telemetry.resource_drain = std::clamp(pop_ratio + (cancer_ratio * 3.0), 0.0, 1.0);

                pid_controller_.Calculate(
                    cancer_ratio, 1.0, current_concentration, immune_snap.checkpoints.treg_suppression_aura,
                    current_telemetry.pid_p, current_telemetry.pid_i, current_telemetry.pid_d, current_telemetry.pid_control_signal
                );
            }
            else {
                current_telemetry.immortality_control = 0.0;
                current_telemetry.resource_drain = 0.0;
                current_telemetry.pid_control_signal = 0.0;
            }

            current_telemetry.epoch_ticks = tick_counter_;

            double angiogenesis_efficiency = std::clamp(organs.circulating_vegf * 8.0 + 0.2, 0.0, 1.0);

            double max_tumor_vessels = base_carrying_capacity_ * 1.5;

            double overgrowth_crush = 1.0 / (1.0 + std::exp(-80.0 * (cancer_ratio - 0.90)));

            double supported_tumor_capacity = max_tumor_vessels * angiogenesis_efficiency * (1.0 - overgrowth_crush);

            dynamic_carrying_capacity_ = base_carrying_capacity_ + static_cast<size_t>(supported_tumor_capacity);

            double raw_tox_generated = 0.0;
            if (current_telemetry.oncogenic_cells > 0) {
                raw_tox_generated += (current_telemetry.oncogenic_cells * 0.00005);
            }

            raw_tox_generated += (gpu_tel.necrotic_deaths_this_tick * 0.0001);

            double healthy_ratio = current_pop > 0 ? static_cast<double>(current_telemetry.healthy_cells) / current_pop : 0.0;
            double systemic_lazarus_payload = (current_telemetry.oncogenic_cells > 0) ? static_cast<double>(current_telemetry.oncogenic_cells) * 0.0001 : 0.0;
            double systemic_backflow = nature_director_.ProcessOrganAxis(raw_tox_generated, healthy_ratio, systemic_lazarus_payload, current_concentration);

            auto env_state = nature_director_.GetEnvironmentState();
            double base_drainage = env_state.oxygen_level * healthy_ratio * 0.00001 * current_telemetry.healthy_cells;
            double cytokine_multiplier = (current_telemetry.immortality_control >= 0.85) ? 3.0 : 1.0;
            double vegf_highway_multiplier = 1.0 + (organs.circulating_vegf * 9.0);

            double lymphatic_clearance = base_drainage * cytokine_multiplier * vegf_highway_multiplier * vital.cardiac_perfusion_rate;

            double m2_detox_effect = immune_snap.cells.macrophages_m2 * 0.005 * current_pop;
            double ecm_detox_effect = (systemic_lazarus_payload * 0.5) + gpu_tel.toxins_cleared_by_autophagy + m2_detox_effect;

            nature_director_.ApplyEfferocytosis(ecm_detox_effect);

            accumulated_toxicity_ = std::max(0.0, accumulated_toxicity_ + raw_tox_generated + systemic_backflow - lymphatic_clearance - ecm_detox_effect);
            accumulated_toxicity_ *= (1.0 - current_telemetry.immortality_control);
            accumulated_toxicity_ = std::min(1.0, accumulated_toxicity_);
        }

        void ReactorEngine::ProcessExosomeBroadcast(std::vector<Cellular::Cell>& target_buffer) {
            size_t actual_count = 0;
            target_buffer.resize(1500);
            gpu_engine_.FetchSubsetToHost(reinterpret_cast<Cellular::Cell*>(target_buffer.data()), 1500, actual_count);
            target_buffer.resize(actual_count);
        }

        void ReactorEngine::PerformBiologicalTickInternal() {
            ReactorSnapshot& next_snap = snapshots_[engine_writing_idx_];

            size_t current_pop = gpu_engine_.GetCurrentPopulation();
            double cancer_ratio = current_pop > 0 ? static_cast<double>(next_snap.telemetry.oncogenic_cells) / current_pop : 0.0;
            double healthy_ratio = current_pop > 0 ? static_cast<double>(next_snap.telemetry.healthy_cells) / current_pop : 0.0;

            static int injection_refractory_timer_ = 0;
            static size_t last_oncogenic_cells_ = 0;

            size_t current_onco = next_snap.telemetry.oncogenic_cells;

            if (last_oncogenic_cells_ == 0 && current_onco > 0) {
                injection_refractory_timer_ = 1500;
            }

            if (injection_refractory_timer_ > 0) {
                injection_refractory_timer_--;
                tumor_incubation_phase_ = true;
            }
            else if (cancer_ratio > 0.0) {
                max_historical_cancer_ratio_ = std::max(max_historical_cancer_ratio_, cancer_ratio);
                tumor_incubation_phase_ = (max_historical_cancer_ratio_ < 0.245);
            }
            else {
                tumor_incubation_phase_ = false;
                max_historical_cancer_ratio_ = 0.0;
            }

            last_oncogenic_cells_ = current_onco;

            static double s_systemic_exosome_pool = 0.0;
            static size_t s_total_exosomes_absorbed = 0;
            double current_concentration = current_pop > 0 ? s_systemic_exosome_pool / static_cast<double>(current_pop) : 0.0;

            nature_director_.UpdateEcosystem(current_pop, dynamic_carrying_capacity_, next_snap.telemetry.avg_cell_age, next_snap.telemetry.mortality_rate, accumulated_toxicity_, healthy_ratio, next_snap.telemetry.immortality_control, current_concentration, cancer_ratio);

            double stromal_rigidity = 0.0;
            if (current_pop > base_carrying_capacity_) {
                stromal_rigidity = std::clamp(static_cast<double>(current_pop - base_carrying_capacity_) / (base_carrying_capacity_ * 0.05), 0.0, 1.0);
            }

            double cardiac_pump = next_snap.telemetry.cardiac_perfusion_rate > 0.0 ? next_snap.telemetry.cardiac_perfusion_rate : 0.1;

            CudaCore::GpuTelemetry gpu_tel;
            double dynamic_tumor_stealth = nature_director_.GetTumorStealthFactor();

            if (tumor_incubation_phase_) dynamic_tumor_stealth = 1.0;

            gpu_engine_.PerformBiologicalTick(
                nature_director_.GetEnvironmentState(),
                nature_director_.GetImmuneState(),
                nature_director_.GetGenomicInstability(),
                stromal_rigidity,
                accumulated_toxicity_,
                base_carrying_capacity_,
                dynamic_carrying_capacity_,
                current_concentration,
                next_snap.telemetry.systemic_z_tumor_saturation,
                dynamic_tumor_stealth,
                tumor_incubation_phase_,
                gpu_tel
            );

            UpdateTelemetryFromGPU(next_snap.telemetry, gpu_tel, gpu_engine_.GetCurrentPopulation(), current_concentration);

            double newly_produced = gpu_tel.local_tumor_exosomes_produced;
            double pumped_into_blood = newly_produced * std::clamp(cardiac_pump, 0.1, 3.0);

            s_systemic_exosome_pool += pumped_into_blood;
            s_systemic_exosome_pool -= gpu_tel.exosomes_absorbed_this_tick;
            if (s_systemic_exosome_pool < 0.0) s_systemic_exosome_pool = 0.0;
            s_systemic_exosome_pool *= 0.95;

            s_total_exosomes_absorbed += gpu_tel.exosomes_absorbed_this_tick;

            next_snap.telemetry.active_exosomes_in_circulation = static_cast<size_t>(s_systemic_exosome_pool);
            next_snap.telemetry.total_exosomes_absorbed = s_total_exosomes_absorbed;

            ProcessExosomeBroadcast(next_snap.cells_buffer);

            // 🛑 PHASE 72: Core Migration - SentinelGuard executes securely in the Engine Thread, totally decoupling the UI!
            if (terminal_ != nullptr) {
                std::vector<Cellular::Cell*> scan_refs;
                scan_refs.reserve(next_snap.cells_buffer.size());
                for (auto& c : next_snap.cells_buffer) {
                    scan_refs.push_back(&c);
                }
                Shield::SentinelGuard::PerformSecurityScan(scan_refs, *terminal_, next_snap.telemetry);
            }

            data_logger_.LogTick(next_snap.telemetry);
            tick_counter_++;

            int prev_latest = latest_written_idx_.exchange(engine_writing_idx_, std::memory_order_acq_rel);
            int ui_reading = ui_reading_idx_.load(std::memory_order_acquire);

            for (int i = 0; i < 3; ++i) {
                if (i != ui_reading && i != engine_writing_idx_) {
                    engine_writing_idx_ = i;
                    break;
                }
            }
        }

        void ReactorEngine::SyncAndFetchUIState(BioTelemetry& out_tel, std::vector<Cellular::Cell*>& out_cell_refs) {
            int latest = latest_written_idx_.exchange(-1, std::memory_order_acq_rel);
            if (latest != -1) {
                ui_reading_idx_.store(latest, std::memory_order_release);
            }
            int current_active = ui_reading_idx_.load(std::memory_order_acquire);

            out_tel = snapshots_[current_active].telemetry;

            auto& buffer = snapshots_[current_active].cells_buffer;
            out_cell_refs.clear();
            out_cell_refs.reserve(buffer.size());
            for (auto& c : buffer) {
                out_cell_refs.push_back(&c);
            }
        }

        AI_Director::BioEnvironment ReactorEngine::GetEnvironmentState() const { return nature_director_.GetEnvironmentState(); }
        AI_Director::ImmuneSystem ReactorEngine::GetImmuneState() const { return nature_director_.GetImmuneState(); }

        std::string ReactorEngine::GetTelemetryFilePath() const {
            return data_logger_.GetFilePath();
        }

    }
}