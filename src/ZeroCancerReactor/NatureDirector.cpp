/**
 * @file NatureDirector.cpp
 * @brief Implementation of the Adversarial Environment Engine (Phase 69: Mathematical Anchor & Volterra Physics)
 * @note [SECURE SKELETON: EDUCATIONAL & RESEARCH BIO-SIMULATION ONLY]
 */

#include "AI_Director/NatureDirector.h" 
#include <algorithm>
#include <mutex>
#include <chrono>
#include <thread>
#include <random>

namespace ZeroCancerReactor {
    namespace AI_Director {

        struct FastXoshiro256Nature {
            uint64_t s[4];
            static inline uint64_t rotl(const uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
            uint64_t next() {
                const uint64_t result = rotl(s[1] * 5, 7) * 9;
                const uint64_t t = s[1] << 17;
                s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3];
                s[2] ^= t; s[3] = rotl(s[3], 45);
                return result;
            }
            double next_double() { return (next() >> 11) * 0x1.0p-53; }
        };

        NatureDirector::NatureDirector() {
            env_ = { 1.0, 1.0, 0.05, 0.1, 0.95, 1.0 };
            immune_ = {
                0.0, 0.0,
                {0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0},
                {0,0,0,0,0}
            };
            organs_ = { 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
            vital_ = { 0.20, 1.0, 1.0, 1.0 };
            endocrine_ = { 0.0, 0.0, 0.0, 0.6, 0.5 };
            genomic_instability_ = 0.01;
            quorum_sensor_ = { {0.0, 0.0, 0.0}, 0.0, 0.0, 0.0 };
        }

        double NatureDirector::GenerateGaussianNoise(double mean, double std_dev) const {
            thread_local FastXoshiro256Nature fast_rng = []() {
                FastXoshiro256Nature r;
                std::random_device rd;
                r.s[0] = (static_cast<uint64_t>(rd()) << 32) | rd();
                r.s[1] = (static_cast<uint64_t>(rd()) << 32) | rd();
                r.s[2] = (static_cast<uint64_t>(rd()) << 32) | rd();
                r.s[3] = (static_cast<uint64_t>(rd()) << 32) | rd();
                return r;
                }();
            double u1 = fast_rng.next_double();
            double u2 = fast_rng.next_double();
            if (u1 < 1e-15) u1 = 1e-15;
            double z0 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * 3.14159265358979323846 * u2);
            return mean + z0 * std_dev;
        }

        double NatureDirector::MichaelisMentenKinetics(double ligand, double kd) const {
            if (ligand <= 0.0) return 0.0;
            return ligand / (kd + ligand);
        }

        double NatureDirector::SigmoidActivation(double x, double midpoint, double steepness) const {
            return 1.0 / (1.0 + std::exp(-steepness * (x - midpoint)));
        }

        void NatureDirector::ApplyEfferocytosis(double recycled_toxins) {
            std::scoped_lock<std::mutex> lock(mtx_);
            double glucose_yield = recycled_toxins * 3.14159;
            double ecm_yield = recycled_toxins * 2.71828;
            env_.glucose_level = std::clamp(env_.glucose_level + glucose_yield, 0.0, 1.0);
            env_.ecm_integrity = std::clamp(env_.ecm_integrity + ecm_yield, 0.0, 1.0);

            double efferocytosis_multiplier = 1.0 + (quorum_sensor_.symbiosis_bribe_factor * 10.0);

            immune_.cytokines.il_10 = std::clamp(immune_.cytokines.il_10 + (recycled_toxins * 5.0 * efferocytosis_multiplier), 0.0, 1.0);
            immune_.cells.macrophages_m2 = std::clamp(immune_.cells.macrophages_m2 + (recycled_toxins * 2.0 * efferocytosis_multiplier), 0.0, 1.0);

            immune_.systemic_inflammation = std::max(0.0, immune_.systemic_inflammation - (recycled_toxins * 0.85 * efferocytosis_multiplier));
            endocrine_.dopamine = std::clamp(endocrine_.dopamine + (recycled_toxins * 0.1), 0.0, 1.0);
        }

        void NatureDirector::InjectTumorFactors(double vegf, double wnt, double dialysis) {
            std::scoped_lock<std::mutex> lock(mtx_);
            organs_.circulating_vegf += vegf;
            organs_.circulating_wnt_factors += wnt;
            organs_.cybernetic_dialysis_capacity += dialysis;
        }

        double NatureDirector::ProcessOrganAxis(double raw_necrotic_input, double healthy_ratio, double systemic_lazarus_payload, double systemic_exosomes_concentration) {
            std::scoped_lock<std::mutex> lock(mtx_);

            // 🛑 PHASE 69: Population-Scaled Baseline Anchor
            // Prevents liver collapse at Tick 0. The host liver naturally scales with the size of the organism.
            double scale_factor = std::max<double>(1.0, static_cast<double>(cached_population_) / 1500.0);

            // Dynamic thresholds that scale beautifully with the total absolute cell count
            double dynamic_toxin_threshold = 0.02 * scale_factor;
            double dynamic_aki_threshold = 0.05 * scale_factor;

            organs_.cori_cycle_adaptation = std::clamp((systemic_exosomes_concentration * 8000.0) + (quorum_sensor_.symbiosis_bribe_factor * 0.5), 0.0, 0.98);

            double recycled_lactate = raw_necrotic_input * organs_.cori_cycle_adaptation;
            raw_necrotic_input -= recycled_lactate;

            organs_.circulating_metabolites += (recycled_lactate * 1.5);

            double neutralized_toxins = std::min(raw_necrotic_input, organs_.cybernetic_dialysis_capacity);
            raw_necrotic_input -= neutralized_toxins;
            organs_.cybernetic_dialysis_capacity = std::max(0.0, organs_.cybernetic_dialysis_capacity - neutralized_toxins);

            double exosome_organ_healing = systemic_exosomes_concentration * 2.5;
            organs_.aki_level = std::max(0.0, organs_.aki_level - exosome_organ_healing);
            organs_.hepatic_stress = std::max(0.0, organs_.hepatic_stress - exosome_organ_healing);

            if (organs_.renal_gfr < 0.2 && organs_.hepatic_stress > 0.8) organs_.endocrine_sos_signal = 1.0;
            else organs_.endocrine_sos_signal = std::max(0.0, organs_.endocrine_sos_signal - 0.02);

            if (organs_.renal_gfr < 0.85) organs_.raas_activation_level = std::clamp(organs_.raas_activation_level + 0.05, 0.0, 1.0);
            else if (organs_.renal_gfr > 0.90) organs_.raas_activation_level = std::max(0.0, organs_.raas_activation_level - 0.02);

            organs_.klotho_expression_level = std::clamp(systemic_exosomes_concentration * 8000.0, 0.0, 1.0);
            double klotho_shield = organs_.klotho_expression_level;

            double hypertrophy_signal = systemic_exosomes_concentration * 50.0;
            double hepatic_receptivity = std::clamp(1.0 - (organs_.hepatic_stress * 0.5), 0.1, 1.0);
            double renal_receptivity = std::clamp(1.0 - (organs_.aki_level * 0.5), 0.1, 1.0);

            if (systemic_lazarus_payload > 0.0 || hypertrophy_signal > 0.001) {
                organs_.aki_level = std::max(0.0, organs_.aki_level - (systemic_lazarus_payload * 0.1) - (hypertrophy_signal * 0.01));
                organs_.hepatic_stress = std::max(0.0, organs_.hepatic_stress - (systemic_lazarus_payload * 0.1) - (hypertrophy_signal * 0.01));
                organs_.hyper_hepatic_multiplier = std::clamp(organs_.hyper_hepatic_multiplier + ((systemic_lazarus_payload * 0.05) + (hypertrophy_signal * 0.1)) * hepatic_receptivity, 1.0, 500.0);
                organs_.hyper_renal_multiplier = std::clamp(organs_.hyper_renal_multiplier + ((systemic_lazarus_payload * 0.05) + (hypertrophy_signal * 0.1)) * renal_receptivity, 1.0, 500.0);
            }

            double biological_atrophy = 0.005 * std::clamp(1.0 - (systemic_exosomes_concentration * 1000.0), 0.0, 1.0);
            if (organs_.hyper_hepatic_multiplier > 1.0) organs_.hyper_hepatic_multiplier = std::max(1.0, organs_.hyper_hepatic_multiplier - biological_atrophy);
            if (organs_.hyper_renal_multiplier > 1.0) organs_.hyper_renal_multiplier = std::max(1.0, organs_.hyper_renal_multiplier - biological_atrophy);

            double wnt_boost = organs_.circulating_wnt_factors * 0.02;
            organs_.circulating_wnt_factors *= 0.5;
            if (wnt_boost > 0.0) {
                organs_.aki_level = std::max(0.0, organs_.aki_level - (organs_.aki_level * wnt_boost + 0.01));
                organs_.hepatic_stress = std::max(0.0, organs_.hepatic_stress - (organs_.hepatic_stress * wnt_boost + 0.01));
            }

            double vegf_boost = organs_.circulating_vegf * 0.005;
            organs_.circulating_vegf *= 0.5;
            organs_.aki_level = std::max(0.0, organs_.aki_level - ((0.002 + vegf_boost) * organs_.hyper_renal_multiplier));
            organs_.hepatic_stress = std::max(0.0, organs_.hepatic_stress - ((0.005 + vegf_boost) * organs_.hyper_hepatic_multiplier));

            // Scaled base hepatic capacity prevents 0-tick crash
            double base_hepatic_capacity = (0.005 * scale_factor + (healthy_ratio * 0.02)) * organs_.hyper_hepatic_multiplier;
            double effective_hepatic_capacity = base_hepatic_capacity * (1.0 - organs_.hepatic_stress) * (1.0 + (quorum_sensor_.symbiosis_bribe_factor * 8.0) + organs_.cori_cycle_adaptation * 10.0);

            organs_.unprocessed_toxins += raw_necrotic_input;
            double processed_amount = std::min(organs_.unprocessed_toxins, effective_hepatic_capacity * vital_.cardiac_perfusion_rate);
            organs_.unprocessed_toxins -= processed_amount;
            organs_.circulating_metabolites += processed_amount;

            // 🛑 PHASE 69: Normalized Toxin Damage Mathematics
            if (organs_.unprocessed_toxins > dynamic_toxin_threshold) {
                organs_.hepatic_stress += (organs_.unprocessed_toxins / scale_factor) * 0.5;
            }

            double max_stress_allowed = 1.0 - (organs_.cori_cycle_adaptation * 0.95);
            organs_.hepatic_stress = std::clamp(organs_.hepatic_stress, 0.0, max_stress_allowed);

            if (organs_.unprocessed_toxins > dynamic_aki_threshold) {
                double acute_aki_damage = ((organs_.unprocessed_toxins / scale_factor) * 0.2) * (1.0 - klotho_shield);
                organs_.aki_level += acute_aki_damage * (1.0 - (organs_.raas_activation_level * 0.7));
            }
            organs_.aki_level = std::clamp(organs_.aki_level, 0.0, 1.0);

            organs_.renal_gfr = std::clamp((organs_.hyper_renal_multiplier * (1.0 - organs_.aki_level)) + (organs_.raas_activation_level * 0.15) + (klotho_shield * 1.5), 0.0, 1.0);
            double filtration_amount = std::min(organs_.circulating_metabolites, 0.05 * organs_.renal_gfr * vital_.cardiac_perfusion_rate);
            organs_.circulating_metabolites -= filtration_amount;

            env_.glucose_level = std::clamp(env_.glucose_level + (filtration_amount * organs_.cori_cycle_adaptation * 2.0), 0.0, 1.0);

            return std::clamp((organs_.unprocessed_toxins * 0.6) + (organs_.circulating_metabolites * 0.1), 0.0, 1.0);
        }

        void NatureDirector::UpdateEcosystem(size_t current_population, size_t carrying_capacity, double avg_cell_age, double cell_death_rate, double tissue_toxicity, double healthy_ratio, double immortality_control, double systemic_exosomes_concentration, double cancer_ratio) {
            std::scoped_lock<std::mutex> lock(mtx_);

            cached_population_ = current_population;

            size_t safe_capacity = std::max<size_t>(1, carrying_capacity);
            double population_ratio = static_cast<double>(current_population) / static_cast<double>(safe_capacity);

            double overgrowth_penalty = SigmoidActivation(cancer_ratio, 0.28, 100.0);

            env_.oxygen_level = std::clamp(1.0 - (population_ratio * 1.2) - (overgrowth_penalty * 0.5), 0.01, 1.0);
            env_.glucose_level = std::clamp(1.0 - (population_ratio * 1.5) - (overgrowth_penalty * 0.6), 0.01, 1.0);

            if (cancer_ratio > 0.0) {
                double hypoxia = std::clamp(1.0 - env_.oxygen_level, 0.0, 1.0);
                organs_.circulating_vegf = std::clamp(organs_.circulating_vegf + (cancer_ratio * hypoxia * 0.05), 0.0, 1.0);
            }

            double cortisol_trigger = 0.0;
            if (env_.glucose_level < vital_.brain_glucose_demand) {
                double deficit = vital_.brain_glucose_demand - env_.glucose_level;
                endocrine_.adrenaline = std::clamp(endocrine_.adrenaline + (deficit * 2.0), 0.0, 1.0);
                cortisol_trigger += (deficit * 1.5);
            }
            else { endocrine_.adrenaline = std::max(0.0, endocrine_.adrenaline - 0.05); }

            if (immune_.systemic_inflammation > 0.8 && organs_.renal_gfr < 0.2) cortisol_trigger += 0.05;
            endocrine_.cortisol = (cortisol_trigger > 0.0) ? std::clamp(endocrine_.cortisol + cortisol_trigger, 0.0, 1.0) : std::max(0.0, endocrine_.cortisol - 0.01);
            organs_.cortisol_shock_level = endocrine_.cortisol;

            if (endocrine_.cortisol > 0.1) {
                double catabolism = endocrine_.cortisol * 0.08;
                env_.ecm_integrity = std::max(0.0, env_.ecm_integrity - catabolism);
                env_.glucose_level = std::clamp(env_.glucose_level + (catabolism * 1.5), 0.0, 1.0);
            }

            endocrine_.dopamine = std::clamp(immortality_control * healthy_ratio, 0.0, 1.0);
            double inotropic_overdrive = std::clamp(systemic_exosomes_concentration * 1000.0, 0.0, 2.0);
            vital_.cardiac_perfusion_rate += (std::clamp(1.0 + (endocrine_.adrenaline * 1.2) - (immune_.systemic_inflammation * 0.4) + (endocrine_.dopamine * 0.2) + inotropic_overdrive, 0.1, 3.0) - vital_.cardiac_perfusion_rate) * 0.05;
            vital_.pulmonary_stress_resistance = std::clamp(0.5 + (endocrine_.dopamine * 0.5) - (endocrine_.cortisol * 0.3), 0.1, 1.0);
            vital_.pulmonary_o2_capacity = std::clamp(1.0 - (immune_.systemic_inflammation * (1.0 - vital_.pulmonary_stress_resistance)), 0.1, 1.0);
            env_.oxygen_level = std::min(env_.oxygen_level * vital_.cardiac_perfusion_rate, vital_.pulmonary_o2_capacity);

            double klotho_systemic_anti_aging = organs_.klotho_expression_level * 0.6;
            env_.dna_repair_efficacy = std::clamp(env_.glucose_level * 0.9 - (tissue_toxicity * 0.2) + (endocrine_.estrogen * 0.05) + klotho_systemic_anti_aging, 0.1, 1.0);
            env_.ecm_integrity = std::clamp(env_.ecm_integrity - (tissue_toxicity * 0.08) + ((env_.glucose_level * healthy_ratio * 0.03) + (endocrine_.testosterone * 0.01)), 0.0, 1.0);

            double effective_toxicity = tissue_toxicity * (1.0 - env_.ecm_integrity);
            env_.ros_level = std::clamp(0.1 + (1.0 - env_.oxygen_level) * 0.8 + (effective_toxicity * 0.5) - klotho_systemic_anti_aging + GenerateGaussianNoise(0.0, 0.05), 0.0, 1.0);
            genomic_instability_ = std::clamp((env_.ros_level * 0.6) + (env_.cosmic_radiation * 0.2) + (1.0 - env_.dna_repair_efficacy) + (endocrine_.testosterone * 0.015), 0.0, 1.0);

            auto& cyto = immune_.cytokines;
            auto& cells = immune_.cells;
            auto& check = immune_.checkpoints;
            auto& eff = immune_.effectors;

            // 🛑 PHASE 69: Lotka-Volterra Nutrient Stress Modulator
            // When cancer approaches 78% mass, spatial/nutrient stress forces the tumor to drop its IL-10/TGF-b shield.
            double tumor_nutrient_stress = SigmoidActivation(cancer_ratio, 0.78, 40.0);

            double symbiotic_ceasefire = SigmoidActivation(immortality_control, 0.50, 15.0);
            double base_bribe = SigmoidActivation(cancer_ratio, 0.10, 20.0);

            quorum_sensor_.symbiosis_bribe_factor = std::clamp(base_bribe + symbiotic_ceasefire - (tumor_nutrient_stress * 0.15), 0.0, 1.0);

            cyto.cxcl8 = SigmoidActivation(cell_death_rate + (1.0 - env_.oxygen_level), 0.25, 8.0);
            cyto.ccl2 = SigmoidActivation(cancer_ratio + effective_toxicity, 0.30, 8.0);

            cyto.g_csf = std::clamp(cyto.cxcl8 * 1.5 - (endocrine_.cortisol * 0.5), 0.0, 1.0);
            cyto.m_csf = std::clamp(cyto.ccl2 * 1.2 - (endocrine_.cortisol * 0.3), 0.0, 1.0);

            cells.neutrophils = std::clamp(cyto.g_csf * cyto.cxcl8 * 2.0, 0.0, 1.0);

            cyto.il_12 = SigmoidActivation(genomic_instability_ + cancer_ratio, 0.2, 12.0);
            cells.macrophages_m1 = std::clamp((cyto.ccl2 + cyto.m_csf) * (cyto.il_12 + cyto.ifn_gamma), 0.0, 1.0);

            cyto.il_6 = std::clamp((cells.macrophages_m1 * 0.8) + (cells.neutrophils * 0.6) + (effective_toxicity * 0.5), 0.0, 1.0);
            cyto.tnf_alpha = std::clamp(cells.macrophages_m1 * 0.9 + (cancer_ratio * 0.4), 0.0, 1.0);

            immune_.immune_memory = std::clamp(immune_.immune_memory + (cancer_ratio * 0.01), 0.0, 1.0);
            cells.cd4_helper_t = std::clamp(immune_.immune_memory * (1.0 - check.treg_suppression_aura), 0.0, 1.0);

            cyto.il_2 = std::clamp(cells.cd4_helper_t * 1.5 * (1.0 - endocrine_.cortisol), 0.0, 1.0);

            cells.nk_cells = std::clamp(cyto.il_12 * 0.8 + (1.0 - env_.oxygen_level) * 0.2, 0.0, 1.0) * (1.0 - endocrine_.cortisol * 0.8);

            double nascent_tcf1 = immune_.immune_memory * cyto.il_12 * (1.0 - endocrine_.cortisol);
            cells.tcf1_stem_like_cd8 = std::clamp(cells.tcf1_stem_like_cd8 + (nascent_tcf1 * 0.02) - (cells.tcf1_stem_like_cd8 * 0.005), 0.0, 1.0);

            double killing_stress = cells.cd8_cytotoxic_t * cell_death_rate * 50.0;
            double lag3_increase = 0.03 * std::log10(1.0 + killing_stress * 10.0);
            double lag3_decrease = 0.002 * (1.0 - cancer_ratio);

            check.t_cell_exhaustion_lag3 = std::clamp(check.t_cell_exhaustion_lag3 + lag3_increase - lag3_decrease, 0.0, 1.0);
            check.t_cell_exhaustion_tim3 = SigmoidActivation(check.t_cell_exhaustion_lag3 + (cyto.il_6 * 0.8), 0.6, 15.0);

            double exhaustion_pressure = std::max(check.t_cell_exhaustion_tim3, check.t_cell_exhaustion_lag3);
            double tcf1_drip_feed = cells.tcf1_stem_like_cd8 * exhaustion_pressure * 0.15;
            cells.tcf1_stem_like_cd8 = std::max(0.0, cells.tcf1_stem_like_cd8 - tcf1_drip_feed);

            double base_cd8 = std::clamp(cyto.il_2 * 2.0 + immune_.immune_memory, 0.0, 1.0);

            cells.cd8_cytotoxic_t = std::clamp(
                (base_cd8 * (1.0 - check.t_cell_exhaustion_tim3) * (1.0 - check.treg_suppression_aura)) + tcf1_drip_feed,
                0.0, 1.0
            );

            cyto.ifn_gamma = std::clamp((cells.cd8_cytotoxic_t * 1.2) + (cells.nk_cells * 1.5), 0.0, 1.0);

            cyto.tgf_beta = std::clamp((cancer_ratio * 2.5) + (organs_.circulating_vegf * 1.5) + (quorum_sensor_.symbiosis_bribe_factor * 2.0), 0.0, 1.0);
            double nascent_tregs = std::clamp(cyto.tgf_beta * 1.5 + (cyto.il_10 * 0.8), 0.0, 1.0);

            if (nascent_tregs > 0.8) {
                check.treg_exhaustion_aicd += 0.025;
            }
            else {
                check.treg_exhaustion_aicd -= 0.015;
            }
            check.treg_exhaustion_aicd = std::clamp(check.treg_exhaustion_aicd, 0.0, 1.0);

            double treg_survival_factor = 1.0 - SigmoidActivation(check.treg_exhaustion_aicd, 0.85, 20.0);
            cells.regulatory_t_cells = nascent_tregs * treg_survival_factor;

            // 🛑 PHASE 69: Absolute Maximum Tolerogenic Clamps (Preventing 100% Blindness)
            check.treg_suppression_aura = std::clamp(cells.regulatory_t_cells * 2.0 - (tumor_nutrient_stress * 0.20), 0.0, 0.85);

            quorum_sensor_.saturation_history[0] = quorum_sensor_.saturation_history[1];
            quorum_sensor_.saturation_history[1] = quorum_sensor_.saturation_history[2];
            quorum_sensor_.saturation_history[2] = cancer_ratio;

            double reactive_pdl1 = SigmoidActivation(cyto.ifn_gamma, 0.3, 15.0);
            double baseline_pdl1 = cancer_ratio * 0.5;

            if (cancer_ratio >= 0.25) {
                quorum_sensor_.epigenetic_silence_pressure += 0.01;
            }
            else {
                quorum_sensor_.epigenetic_silence_pressure = std::max(0.0, quorum_sensor_.epigenetic_silence_pressure - 0.005);
            }

            quorum_sensor_.pd_l1_expression = std::clamp(reactive_pdl1 + baseline_pdl1 - quorum_sensor_.epigenetic_silence_pressure - (tumor_nutrient_stress * 0.20), 0.0, 0.85);

            check.pd1_expression = std::clamp(cells.cd8_cytotoxic_t * 1.5, 0.0, 1.0);

            // Hard clamp at 0.85 guarantees CD8+ cells can always see at least 15% of the tumor
            check.pdl1_binding_axis = std::clamp(MichaelisMentenKinetics(quorum_sensor_.pd_l1_expression * check.pd1_expression, 0.2), 0.0, 0.85);

            cyto.il_4 = std::clamp(cell_death_rate * 5.0 + cyto.tgf_beta, 0.0, 1.0);
            cyto.il_13 = cyto.il_4;

            cells.macrophages_m2 = std::clamp((cyto.il_4 + cyto.il_13) * cyto.m_csf + (quorum_sensor_.symbiosis_bribe_factor * 1.5), 0.0, 1.0);

            double vagus_nerve_tone = SigmoidActivation(cyto.il_6 + cyto.tnf_alpha, 1.4, 15.0);

            cyto.il_10 = std::clamp((cells.macrophages_m2 * 1.5) + (cells.regulatory_t_cells * 2.0) + (vagus_nerve_tone * 3.0) + (quorum_sensor_.symbiosis_bribe_factor * 3.0) - (tumor_nutrient_stress * 0.20), 0.0, 1.0);

            double tls_suppressor = std::max(0.0, 1.0 - (quorum_sensor_.symbiosis_bribe_factor * 0.9));

            cyto.il_6 = std::clamp((cyto.il_6 - (cyto.il_10 * 0.8)) * tls_suppressor, 0.0, 1.0);
            cyto.tnf_alpha = std::clamp((cyto.tnf_alpha - (cyto.il_10 * 0.6)) * tls_suppressor, 0.0, 1.0);

            cells.b_cells_antibodies = std::clamp(systemic_exosomes_concentration * 100.0, 0.0, 1.0);

            // Due to the 0.85 pdl1 clamp, perforin_base will naturally rise to ~0.35 during tumor_nutrient_stress, triggering Lotka-Volterra pruning
            double perforin_base = (cells.cd8_cytotoxic_t * cyto.ifn_gamma + cells.nk_cells) * (1.0 - check.pdl1_binding_axis);
            eff.perforin_granzyme_pathway = std::clamp(perforin_base, 0.0, 1.0);

            eff.fas_fasl_death_kiss = std::clamp(cells.cd8_cytotoxic_t * 0.8 * (1.0 - check.treg_suppression_aura), 0.0, 1.0);

            eff.mac_complement_system = SigmoidActivation(cyto.il_6 + cyto.tnf_alpha, 1.5, 6.0) * tls_suppressor;
            eff.phagocytosis_ros_storm = std::clamp(cells.neutrophils + cells.macrophages_m1, 0.0, 1.0) * tls_suppressor;
            eff.tissue_regeneration = std::clamp((cells.macrophages_m2 * 1.5) + (cyto.tgf_beta * 0.5) + organs_.cori_cycle_adaptation, 0.0, 1.0);

            double pain_suppression = 1.0 - immortality_control;

            immune_.systemic_inflammation = std::clamp(cyto.il_6 + cyto.tnf_alpha - cyto.il_10, 0.0, 1.0);
            immune_.systemic_inflammation *= pain_suppression;

            endocrine_.cortisol *= pain_suppression;
            env_.ros_level *= pain_suppression;
            organs_.aki_level *= pain_suppression;
            organs_.renal_gfr = std::clamp(organs_.renal_gfr + (immortality_control * 0.5), 0.0, 1.0);
            env_.ecm_integrity = std::clamp(env_.ecm_integrity + (immortality_control * 0.5), 0.0, 1.0);
        }

        double NatureDirector::GetTumorStealthFactor() const {
            std::scoped_lock<std::mutex> lock(mtx_);
            return quorum_sensor_.pd_l1_expression;
        }

        double NatureDirector::CalculateSurvivalProbability(double cell_age, double mutation_load, double cd59_shield, double hif1a_expression, const BioEnvironment& env_snap, const ImmuneSystem& immune_snap, bool is_oncogenic, double tumor_stealth_factor) const {
            double normalized_age = cell_age / REFERENCE_AGE;

            double resource_survival = (env_snap.oxygen_level * 0.4) + (env_snap.glucose_level * 0.6) + (hif1a_expression * 0.35);

            double perf_attack = immune_snap.effectors.perforin_granzyme_pathway;
            double fasl_attack = immune_snap.effectors.fas_fasl_death_kiss;

            double mac_attack = immune_snap.effectors.mac_complement_system * (1.0 - cd59_shield);
            double ros_attack = immune_snap.effectors.phagocytosis_ros_storm * (1.0 - (cd59_shield * 0.4));

            if (is_oncogenic) {
                double evasion = std::max(tumor_stealth_factor, immune_snap.checkpoints.pdl1_binding_axis);
                perf_attack *= (1.0 - evasion);
                fasl_attack *= (1.0 - immune_snap.checkpoints.treg_suppression_aura);
            }

            double immune_attack =
                (mutation_load * perf_attack) +
                (mutation_load * fasl_attack * 0.5) +
                (mac_attack * 0.9) +
                (normalized_age * ros_attack * 0.4);

            double base_survival = (resource_survival + immune_snap.effectors.tissue_regeneration) - immune_attack;
            double survival_noise = GenerateGaussianNoise(0.0, 0.05);

            return std::clamp(base_survival + survival_noise, 0.0, 1.0);
        }

        double NatureDirector::CalculateDNADamage(double telomere_length, const BioEnvironment& env_snap) const {
            double telomere_vulnerability = (telomere_length < 20.0) ? std::clamp((20.0 - telomere_length) / 20.0, 0.0, 1.0) : 0.0;
            double raw_damage = (env_snap.ros_level * 1.5) + (env_snap.cosmic_radiation * 2.0) + (telomere_vulnerability * 3.0);
            return std::max(0.0, (raw_damage * (1.0 - env_snap.dna_repair_efficacy)) + GenerateGaussianNoise(0.0, 0.1));
        }

        double NatureDirector::GetGenomicInstability() const { std::scoped_lock<std::mutex> lock(mtx_); return genomic_instability_; }
        BioEnvironment NatureDirector::GetEnvironmentState() const { std::scoped_lock<std::mutex> lock(mtx_); return env_; }
        ImmuneSystem NatureDirector::GetImmuneState() const { std::scoped_lock<std::mutex> lock(mtx_); return immune_; }
        HostOrgans NatureDirector::GetHostOrgansState() const { std::scoped_lock<std::mutex> lock(mtx_); return organs_; }
        VitalOrgans NatureDirector::GetVitalOrgansState() const { std::scoped_lock<std::mutex> lock(mtx_); return vital_; }
        EndocrineSystem NatureDirector::GetEndocrineState() const { std::scoped_lock<std::mutex> lock(mtx_); return endocrine_; }

    }
}