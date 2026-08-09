/**
 * @file CellularKernel.cu
 * @brief CUDA HPC Implementation for 70-Million Cell Bio-Simulation (Phase 72: Stream Sync Cleanup & Final Decoupling)
 * @note [SECURE SKELETON: EDUCATIONAL & RESEARCH BIO-SIMULATION ONLY]
 */

#include "CudaCore/CellularKernel.cuh"
#include <device_launch_parameters.h>
#include <stdexcept>
#include <iostream>

namespace ZeroCancerReactor {
    namespace CudaCore {

        CellularKernelEngine* CellularKernelEngine::instance_ = nullptr;

        __device__ inline float gpu_rand_float(uint64_t& state) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            return (float)((state * 0x2545F4914F6CDD1DULL) * (1.0 / 18446744073709551616.0));
        }

        __device__ inline float gpu_gaussian_noise_fast(uint64_t& state, float mean, float std_dev) {
            float u1 = gpu_rand_float(state);
            float u2 = gpu_rand_float(state);
            u1 = fmaxf(u1, 1e-7f); 
            float z0 = __fsqrt_rn(-2.0f * __logf(u1)) * __cosf(6.28318530718f * u2); 
            return mean + z0 * std_dev;
        }

        __global__ void InitCellsKernel(Cellular::Cell* d_cells, uint64_t* global_id_counter, size_t initial_pop) {
            size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
            if (idx < initial_pop) {
                uint64_t seed = 19937ULL + idx * 1009ULL;
                Cellular::Cell c;
                c.id = atomicAdd((unsigned long long*)global_id_counter, 1ULL);
                c.age = 0.0;
                c.telomere_length = Cellular::INITIAL_TELOMERE;
                c.mutation_load = 0.0;
                c.metabolic_exhaustion = (double)(gpu_rand_float(seed) * 20.0f);
                
                c.epigenetic_shield = 0.0f;
                c.z_tumor_activation_threshold = 0.0f;
                c.autophagy_active_level = 0.0f;
                c.cd59_shield = 1.0f;          
                c.hif1a_expression = 0.0f;     
                
                c.state = Cellular::CellState::HEALTHY;
                c.flags = Cellular::FLAG_P53_ACTIVE; 
                d_cells[idx] = c;
            }
        }

        __device__ inline float CalculateSurvivalProbabilityGPU(float cell_age, float mutation_load, float cd59_shield, float hif1a_expression, const AI_Director::BioEnvironment& env_snap, const AI_Director::ImmuneSystem& immune_snap, bool is_oncogenic, float tumor_stealth_factor, uint64_t& rng_state) {
            float normalized_age = cell_age / 50.0f;
            
            float resource_survival = ((float)env_snap.oxygen_level * 0.4f) + ((float)env_snap.glucose_level * 0.6f) + (hif1a_expression * 0.35f);

            float perf_attack = (float)immune_snap.effectors.perforin_granzyme_pathway;
            float fasl_attack = (float)immune_snap.effectors.fas_fasl_death_kiss;
            
            float mac_attack  = (float)immune_snap.effectors.mac_complement_system * (1.0f - cd59_shield);
            float ros_attack  = (float)immune_snap.effectors.phagocytosis_ros_storm * (1.0f - (cd59_shield * 0.4f));

            if (is_oncogenic) {
                float evasion = fmaxf(tumor_stealth_factor, (float)immune_snap.checkpoints.pdl1_binding_axis);
                perf_attack *= (1.0f - evasion);
                fasl_attack *= (1.0f - (float)immune_snap.checkpoints.treg_suppression_aura);
            }

            float immune_attack = 
                (mutation_load * perf_attack) + 
                (mutation_load * fasl_attack * 0.5f) +
                (mac_attack * 0.9f) + 
                (normalized_age * ros_attack * 0.4f);

            float base_survival = (resource_survival + (float)immune_snap.effectors.tissue_regeneration) - immune_attack;
            float survival_noise = gpu_gaussian_noise_fast(rng_state, 0.0f, 0.05f);
            
            return __saturatef(base_survival + survival_noise); 
        }

        __global__ void SentinelGuardKernel(
            Cellular::Cell* d_cells, uint64_t current_pop, 
            double biological_attack_signal, bool pruning_mode, bool mvg_lockdown, double dynamic_threshold,
            uint64_t epoch_ticks, unsigned long long* counters) 
        {
            __shared__ unsigned long long s_counters[4];
            if (threadIdx.x == 0) {
                s_counters[0] = 0; s_counters[1] = 0; s_counters[2] = 0; s_counters[3] = 0;
            }
            __syncthreads();

            unsigned long long l_c0 = 0, l_c1 = 0, l_c2 = 0, l_c3 = 0;

            size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
            
            if (idx < current_pop) {
                Cellular::Cell c = d_cells[idx];
                bool modified = false;

                if (c.state == Cellular::CellState::HEALTHY || c.state == Cellular::CellState::SENESCENT) {
                    if (c.mutation_load > dynamic_threshold) {
                        float eff_damage = 1.0f;
                        float eff_inst = 1.0f;
                        if (c.epigenetic_shield > 0.0f) {
                            eff_damage *= 0.1f; eff_inst *= 0.1f;
                            c.epigenetic_shield = __saturatef(c.epigenetic_shield - 0.1f);
                        }
                        c.mutation_load = __saturatef((float)c.mutation_load + (eff_damage * eff_inst));
                        c.metabolic_exhaustion += (eff_damage * 1.2f) + 0.15f;

                        bool p53 = Cellular::HasFlag(c.flags, Cellular::FLAG_P53_ACTIVE);
                        bool tel = Cellular::HasFlag(c.flags, Cellular::FLAG_TELOMERASE_ACTIVE);
                        
                        if (!p53 && c.mutation_load > 0.9 && tel) {
                            c.state = Cellular::CellState::ONCOGENIC;
                            l_c1++;
                        } else if (c.mutation_load >= Cellular::APOPTOSIS_THRESHOLD) {
                            c.state = Cellular::CellState::APOPTOTIC;
                            Cellular::ClearFlag(c.flags, Cellular::FLAG_CLEAN_DEATH); 
                            l_c0++;
                        } else {
                            l_c1++;
                        }
                        modified = true;
                    }
                } else if (c.state == Cellular::CellState::ONCOGENIC && pruning_mode) {
                    float attack_prob = __saturatef((float)biological_attack_signal * 0.3f);

                    if (mvg_lockdown) {
                        attack_prob = 0.0f; 
                        l_c3++;
                    }

                    if (attack_prob > 0.0f) {
                        uint64_t rng_state = c.id ^ (epoch_ticks * 0x853C49E67F4A7C15ULL);
                        if (gpu_rand_float(rng_state) < attack_prob) {
                            c.state = Cellular::CellState::APOPTOTIC;
                            if (gpu_rand_float(rng_state) < 0.7f) Cellular::SetFlag(c.flags, Cellular::FLAG_CLEAN_DEATH);
                            else Cellular::ClearFlag(c.flags, Cellular::FLAG_CLEAN_DEATH);
                            
                            l_c2++;
                            modified = true;
                        }
                    }
                }
                if (modified) d_cells[idx] = c;
            }

            if (l_c0) atomicAdd(&s_counters[0], l_c0);
            if (l_c1) atomicAdd(&s_counters[1], l_c1);
            if (l_c2) atomicAdd(&s_counters[2], l_c2);
            if (l_c3) atomicAdd(&s_counters[3], l_c3);

            __syncthreads();

            if (threadIdx.x == 0) {
                if (s_counters[0]) atomicAdd(&counters[0], s_counters[0]);
                if (s_counters[1]) atomicAdd(&counters[1], s_counters[1]);
                if (s_counters[2]) atomicAdd(&counters[2], s_counters[2]);
                if (s_counters[3]) atomicAdd(&counters[3], s_counters[3]);
            }
        }

        __global__ void TelomeraseExploitKernel(
            Cellular::Cell* d_cells, uint64_t current_pop, size_t infection_count,
            double target_telomere_lock, uint64_t hardware_seed, 
            AI_Director::BioEnvironment env_snap, AI_Director::ImmuneSystem immune_snap,
            uint64_t epoch_ticks, unsigned long long* survived_count)
        {
            __shared__ unsigned long long s_success;
            if (threadIdx.x == 0) s_success = 0;
            __syncthreads();

            unsigned long long l_success = 0;

            size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
            if (idx < infection_count) {
                uint64_t rng_state = (idx + 1ULL) ^ hardware_seed ^ (epoch_ticks * 0x853C49E67F4A7C15ULL);
                
                size_t target_idx = (size_t)(gpu_rand_float(rng_state) * current_pop);
                if (target_idx >= current_pop) target_idx = current_pop - 1;

                Cellular::Cell c = d_cells[target_idx];
                
                if (c.state != Cellular::CellState::APOPTOTIC && c.state != Cellular::CellState::ONCOGENIC && !Cellular::HasFlag(c.flags, Cellular::FLAG_Z_TUMOR_MARKER)) {
                    
                    Cellular::SetFlag(c.flags, Cellular::FLAG_Z_TUMOR_MARKER);
                    c.z_tumor_activation_threshold = (float)target_telomere_lock;
                    
                    c.epigenetic_shield = 5.0f; 
                    c.cd59_shield = 1.0f; 
                    c.hif1a_expression = 1.0f; 
                    c.mutation_load = fmaxf(0.0f, (float)c.mutation_load - 0.2f); 
                    
                    float survival_prob = CalculateSurvivalProbabilityGPU((float)c.age, (float)c.mutation_load, c.cd59_shield, c.hif1a_expression, env_snap, immune_snap, true, 1.0f, rng_state);
                    
                    if (gpu_rand_float(rng_state) > survival_prob && c.epigenetic_shield <= 1.0f) {
                        c.state = Cellular::CellState::APOPTOTIC;
                        Cellular::ClearFlag(c.flags, Cellular::FLAG_CLEAN_DEATH);
                    } else {
                        if (c.telomere_length <= target_telomere_lock + 1.0) {
                            Cellular::ClearFlag(c.flags, Cellular::FLAG_P53_ACTIVE);
                            Cellular::SetFlag(c.flags, Cellular::FLAG_TELOMERASE_ACTIVE);
                            c.state = Cellular::CellState::ONCOGENIC;
                            c.mutation_load = __saturatef((float)c.mutation_load + 0.3f);
                        }
                        l_success++;
                    }
                    d_cells[target_idx] = c;
                }
            }

            if (l_success) atomicAdd(&s_success, l_success);
            __syncthreads();

            if (threadIdx.x == 0 && s_success > 0) {
                atomicAdd(survived_count, s_success);
            }
        }

        __global__ void BiologicalTickKernel(
            Cellular::Cell* d_read, Cellular::Cell* d_write, uint64_t current_pop, size_t max_capacity,
            uint64_t* new_pop_counter, uint64_t* global_id_counter, GpuTelemetry* d_telemetry,
            AI_Director::BioEnvironment env_snap, AI_Director::ImmuneSystem immune_snap,
            double genomic_instability, double stromal_rigidity, double accumulated_toxicity,
            size_t base_carrying_capacity, size_t dynamic_carrying_capacity, 
            double systemic_exosomes_concentration, double systemic_z_tumor_saturation, 
            double dynamic_tumor_stealth_factor, bool integration_phase_active, uint64_t epoch_ticks) 
        {
            __shared__ unsigned long long s_total_healthy;
            __shared__ unsigned long long s_total_senescent;
            __shared__ unsigned long long s_total_oncogenic;
            __shared__ double s_total_telomere;
            __shared__ double s_total_mutation;
            __shared__ double s_total_age;
            __shared__ double s_total_exhaustion;
            __shared__ unsigned long long s_necrotic_deaths;
            __shared__ unsigned long long s_clean_deaths;
            __shared__ double s_toxins_cleared;
            __shared__ double s_local_tumor_exosomes;
            __shared__ unsigned long long s_exosomes_absorbed;

            if (threadIdx.x == 0) {
                s_total_healthy = 0; s_total_senescent = 0; s_total_oncogenic = 0;
                s_total_telomere = 0.0; s_total_mutation = 0.0; s_total_age = 0.0; s_total_exhaustion = 0.0;
                s_necrotic_deaths = 0; s_clean_deaths = 0; s_toxins_cleared = 0.0;
                s_local_tumor_exosomes = 0.0; s_exosomes_absorbed = 0;
            }
            __syncthreads();

            double l_local_tumor_exosomes = 0.0;
            unsigned long long l_exosomes_absorbed = 0;
            unsigned long long l_clean_deaths = 0;
            unsigned long long l_necrotic_deaths = 0;
            double l_toxins_cleared = 0.0;
            double l_total_telomere = 0.0;
            double l_total_mutation = 0.0;
            double l_total_age = 0.0;
            double l_total_exhaustion = 0.0;
            unsigned long long l_total_healthy = 0;
            unsigned long long l_total_senescent = 0;
            unsigned long long l_total_oncogenic = 0;

            size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
            bool is_active = (idx < current_pop && idx < max_capacity);
            
            Cellular::Cell c;
            
            if (is_active) {
                c = d_read[idx];
                if (c.state == Cellular::CellState::APOPTOTIC) {
                    if (Cellular::HasFlag(c.flags, Cellular::FLAG_CLEAN_DEATH)) l_clean_deaths++;
                    else l_necrotic_deaths++;
                    is_active = false; 
                }
            }

            float eaten_toxins = 0.0f;
            bool divided = false;
            Cellular::Cell daughter;

            if (is_active) {
                uint64_t rng_state = c.id ^ (epoch_ticks * 0x9E3779B97F4A7C15ULL);
                
                float cancer_ratio = (float)(systemic_z_tumor_saturation * 0.28); 
                float tumor_stealth_factor = (float)dynamic_tumor_stealth_factor;

                bool p53_active = Cellular::HasFlag(c.flags, Cellular::FLAG_P53_ACTIVE);
                bool telomerase_active = Cellular::HasFlag(c.flags, Cellular::FLAG_TELOMERASE_ACTIVE);
                bool z_tumor_marker = Cellular::HasFlag(c.flags, Cellular::FLAG_Z_TUMOR_MARKER);
                bool is_onco = (c.state == Cellular::CellState::ONCOGENIC);

                float cd59_drop = ((float)c.mutation_load * 0.01f) + ((float)c.metabolic_exhaustion / (float)Cellular::EXHAUSTION_LIMIT * 0.02f);
                c.cd59_shield -= cd59_drop;

                if (env_snap.oxygen_level < 0.6) {
                    c.hif1a_expression += 0.05f;
                } else {
                    c.hif1a_expression -= 0.1f;
                }

                if (is_onco) {
                    c.cd59_shield = fmaxf(c.cd59_shield, tumor_stealth_factor);
                    c.hif1a_expression = 1.0f;
                }

                c.cd59_shield = __saturatef(c.cd59_shield);
                c.hif1a_expression = __saturatef(c.hif1a_expression);

                if (c.hif1a_expression > 0.7f && !is_onco) {
                    l_local_tumor_exosomes += 0.00005;
                }

                float telomere_vuln = (c.telomere_length < 20.0) ? (float)((20.0 - c.telomere_length) / 20.0) : 0.0f;
                telomere_vuln = __saturatef(telomere_vuln);

                float raw_damage = ((float)env_snap.ros_level * 1.5f) + ((float)env_snap.cosmic_radiation * 2.0f) + (telomere_vuln * 3.0f);
                float mitigated_damage = raw_damage * (1.0f - (float)env_snap.dna_repair_efficacy);
                float dna_damage = fmaxf(0.0f, mitigated_damage + gpu_gaussian_noise_fast(rng_state, 0.0f, 0.1f));

                float total_stress = (float)genomic_instability + ((float)accumulated_toxicity * 0.1f);

                if (c.state == Cellular::CellState::HEALTHY || c.state == Cellular::CellState::SENESCENT) {
                    if (immune_snap.effectors.phagocytosis_ros_storm > 0.5 && gpu_rand_float(rng_state) < (immune_snap.effectors.phagocytosis_ros_storm * 0.05)) {
                        c.mutation_load = __saturatef((float)c.mutation_load + 0.02f);
                        c.metabolic_exhaustion += 2.0;
                    }
                    if (immune_snap.effectors.tissue_regeneration > 0.1) {
                        c.metabolic_exhaustion = fmaxf(0.0f, (float)c.metabolic_exhaustion - ((float)immune_snap.effectors.tissue_regeneration * 0.5f));
                    }
                }

                if (is_onco && telomerase_active) {
                    c.mutation_load = __saturatef((float)c.mutation_load - (dna_damage * total_stress * 0.5f));
                    c.metabolic_exhaustion = 0.0;

                    double secretion_rate = (env_snap.oxygen_level * 0.6 + env_snap.glucose_level * 0.4) * 0.15;
                    l_local_tumor_exosomes += secretion_rate;
                } else {
                    float eff_damage = dna_damage;
                    float eff_inst = total_stress;
                    if (c.epigenetic_shield > 0.0f) {
                        eff_damage *= 0.1f; eff_inst *= 0.1f;
                        c.epigenetic_shield = __saturatef(c.epigenetic_shield - 0.1f);
                    }
                    if (p53_active) {
                        float healing = (float)env_snap.dna_repair_efficacy * 0.02f;
                        c.mutation_load = fmaxf(0.0f, (float)c.mutation_load - healing);
                    }
                    c.mutation_load = __saturatef((float)c.mutation_load + (eff_damage * eff_inst));
                    c.metabolic_exhaustion += (eff_damage * 1.2f) + 0.15f;
                }

                if (c.state == Cellular::CellState::HEALTHY || c.state == Cellular::CellState::SENESCENT) {
                    if (systemic_exosomes_concentration > 0.0001) {
                        float base_affinity = ((float)c.age / 100.0f) + ((float)c.mutation_load * 0.8f);
                        if (c.state == Cellular::CellState::SENESCENT) base_affinity += 1.0f; 

                        float panic_multiplier = 1.0f;
                        if (c.mutation_load > 0.5 || c.metabolic_exhaustion > (Cellular::EXHAUSTION_LIMIT * 0.5) || c.state == Cellular::CellState::SENESCENT) {
                            panic_multiplier = 100.0f * ((float)c.mutation_load + ((float)c.metabolic_exhaustion / (float)Cellular::EXHAUSTION_LIMIT));
                        }

                        float absorption_prob = __saturatef((float)systemic_exosomes_concentration * base_affinity * panic_multiplier * 0.05f);

                        if (gpu_rand_float(rng_state) < absorption_prob) {
                            c.telomere_length += 20.0; 
                            if (c.telomere_length > Cellular::INITIAL_TELOMERE) c.telomere_length = Cellular::INITIAL_TELOMERE;
                            c.autophagy_active_level = 1.0f; 
                            c.mutation_load *= 0.2; 
                            if (c.state == Cellular::CellState::SENESCENT && c.telomere_length > Cellular::CRITICAL_TELOMERE_LENGTH + 5.0) {
                                c.state = Cellular::CellState::HEALTHY;
                            }
                            c.metabolic_exhaustion *= 0.3; 
                            c.epigenetic_shield = 1.0f; 
                            l_exosomes_absorbed++;
                        }
                    }
                }

                if (z_tumor_marker && !telomerase_active && !is_onco) {
                    if (c.telomere_length <= c.z_tumor_activation_threshold + 0.5f) {
                        float vegf_proxy = (float)immune_snap.checkpoints.treg_suppression_aura;
                        float dialysis_proxy = 1.0f - (float)accumulated_toxicity;
                        float awakening_prob = 0.001f + (vegf_proxy * 0.03f) + (dialysis_proxy * 0.02f);
                        
                        if (gpu_rand_float(rng_state) <= awakening_prob) {
                            Cellular::ClearFlag(c.flags, Cellular::FLAG_P53_ACTIVE);
                            p53_active = false;
                            Cellular::SetFlag(c.flags, Cellular::FLAG_TELOMERASE_ACTIVE);
                            telomerase_active = true;
                            c.state = Cellular::CellState::ONCOGENIC;
                            c.mutation_load = __saturatef((float)c.mutation_load + 0.3f); 
                            c.epigenetic_shield = 1.0f; 
                            is_onco = true;
                        }
                    }
                }

                if (!p53_active) {
                    if (c.mutation_load > 0.9 && telomerase_active && !is_onco) {
                        c.state = Cellular::CellState::ONCOGENIC;
                        c.epigenetic_shield = 1.0f; 
                        is_onco = true;
                    }
                } else {
                    if (stromal_rigidity > 0.5 && !is_onco) {
                        if (c.state == Cellular::CellState::SENESCENT || (c.age > 30.0 && c.metabolic_exhaustion > Cellular::EXHAUSTION_LIMIT * 0.4)) {
                            c.state = Cellular::CellState::APOPTOTIC;
                            Cellular::SetFlag(c.flags, Cellular::FLAG_CLEAN_DEATH);
                        }
                    }
                    if (c.state == Cellular::CellState::SENESCENT && c.metabolic_exhaustion > Cellular::EXHAUSTION_LIMIT * 0.75) {
                        c.state = Cellular::CellState::APOPTOTIC;
                        Cellular::SetFlag(c.flags, Cellular::FLAG_CLEAN_DEATH);
                    }
                    if (c.metabolic_exhaustion >= Cellular::EXHAUSTION_LIMIT && !is_onco) {
                        c.state = Cellular::CellState::APOPTOTIC;
                        Cellular::SetFlag(c.flags, Cellular::FLAG_CLEAN_DEATH);
                    }
                    if (c.mutation_load >= Cellular::APOPTOSIS_THRESHOLD) {
                        c.state = Cellular::CellState::APOPTOTIC;
                        Cellular::ClearFlag(c.flags, Cellular::FLAG_CLEAN_DEATH); 
                    }
                }

                float matrix_shield = (is_onco && integration_phase_active) ? 0.98f : 0.0f;
                float network_collapse = 1.0f / (1.0f + __expf(-80.0f * (cancer_ratio - 0.28f)));

                if (is_onco) {
                    float simulated_depth = gpu_rand_float(rng_state); 
                    float core_threshold = 1.0f - cancer_ratio; 
                    
                    if (simulated_depth > core_threshold || network_collapse > 0.5f) {
                        c.hif1a_expression = __saturatef(c.hif1a_expression + (0.1f * network_collapse + 0.05f) * (1.0f - matrix_shield * 0.8f));
                    }
                }

                float cell_stealth = is_onco ? __saturatef(tumor_stealth_factor + (c.hif1a_expression * 0.15f)) : 0.0f;
                float survival_prob = CalculateSurvivalProbabilityGPU((float)c.age, (float)c.mutation_load, c.cd59_shield, c.hif1a_expression, env_snap, immune_snap, is_onco, cell_stealth, rng_state);
                
                if (is_onco) {
                    float core_ischemia_prob = __powf(c.hif1a_expression, 3.0f) * network_collapse;
                    core_ischemia_prob *= (1.0f - matrix_shield); 
                    survival_prob *= (1.0f - core_ischemia_prob);
                }
                
                float crushing_prob = 1.0f / (1.0f + __expf(-40.0f * ((float)stromal_rigidity - 0.85f)));
                crushing_prob *= (1.0f - matrix_shield); 
                survival_prob *= (1.0f - crushing_prob * 0.5f); 

                if (survival_prob < 0.1f) {
                    c.state = Cellular::CellState::APOPTOTIC;
                    if (gpu_rand_float(rng_state) < 0.6f) Cellular::SetFlag(c.flags, Cellular::FLAG_CLEAN_DEATH); 
                    else Cellular::ClearFlag(c.flags, Cellular::FLAG_CLEAN_DEATH); 
                }

                if (c.state == Cellular::CellState::HEALTHY || c.state == Cellular::CellState::SENESCENT) {
                    if (accumulated_toxicity > 0.6 && env_snap.glucose_level < 0.4) {
                        c.autophagy_active_level = __saturatef(c.autophagy_active_level + 0.1f);
                    } else if (c.autophagy_active_level > 0.0f) {
                        c.autophagy_active_level = __saturatef(c.autophagy_active_level - 0.05f);
                    }

                    if (c.autophagy_active_level > 0.0f) {
                        eaten_toxins = 0.00005f * c.autophagy_active_level;
                        c.metabolic_exhaustion = fmaxf(0.0f, (float)c.metabolic_exhaustion - (0.5f * c.autophagy_active_level));
                    }
                }

                if (c.state == Cellular::CellState::APOPTOTIC) {
                    if (Cellular::HasFlag(c.flags, Cellular::FLAG_CLEAN_DEATH)) l_clean_deaths++;
                    else l_necrotic_deaths++;
                    is_active = false; 
                }
            }

            if (is_active) {
                l_total_telomere += c.telomere_length;
                l_total_mutation += c.mutation_load;
                l_total_age += c.age;
                l_total_exhaustion += c.metabolic_exhaustion;
                
                if (c.state == Cellular::CellState::HEALTHY) l_total_healthy++;
                else if (c.state == Cellular::CellState::SENESCENT) l_total_senescent++;
                else if (c.state == Cellular::CellState::ONCOGENIC) l_total_oncogenic++;

                float pop_ratio = (float)current_pop / (float)dynamic_carrying_capacity;
                float vascularization_limit = (float)dynamic_carrying_capacity / (float)(current_pop > 0 ? current_pop : 1);
                float angiogenesis_factor = fminf(vascularization_limit, 2.0f); 
                
                bool is_onco = (c.state == Cellular::CellState::ONCOGENIC);
                float tumor_contact_inhibition = 1.0f;
                if (is_onco) {
                    float cancer_ratio = (float)(systemic_z_tumor_saturation * 0.28);
                    tumor_contact_inhibition = 1.0f - (1.0f / (1.0f + __expf(-300.0f * (cancer_ratio - 0.275f)))); 
                }
                float final_angiogenesis_factor = angiogenesis_factor * tumor_contact_inhibition;

                float satiety_signal = __saturatef((float)systemic_exosomes_concentration * 800.0f);
                float resting_phase_prob = satiety_signal * 0.95f; 

                if (pop_ratio < 1.0f || is_onco) {
                    float hippo = fmaxf(0.0f, 1.0f - pop_ratio);
                    
                    bool can_divide = true;
                    bool telomerase_active = Cellular::HasFlag(c.flags, Cellular::FLAG_TELOMERASE_ACTIVE);
                    if (c.state == Cellular::CellState::SENESCENT && !telomerase_active) can_divide = false;
                    if (hippo <= 0.0f && !is_onco) can_divide = false;
                    if (pop_ratio >= 0.95f && !is_onco) can_divide = false;
                    
                    float req_oxy = is_onco ? 0.02f : 0.1f;
                    float req_glu = is_onco ? 0.05f : 0.2f;
                    if (env_snap.glucose_level < req_glu || env_snap.oxygen_level < req_oxy) can_divide = false;

                    if (can_divide) {
                        uint64_t rng_state = c.id ^ (epoch_ticks * 0x9E3779B97F4A7C15ULL);
                        float roll = gpu_rand_float(rng_state);
                        if (is_onco) {
                            float onco_div_chance = 0.25f * (float)env_snap.oxygen_level * (float)env_snap.glucose_level * final_angiogenesis_factor;
                            onco_div_chance *= (1.0f - resting_phase_prob); 
                            if (roll <= onco_div_chance) divided = true;
                        } else {
                            float final_chance = 0.05f * (1.0f - pop_ratio) * hippo;
                            if (roll <= final_chance) divided = true;
                        }
                    }
                }

                if (divided) {
                    c.age += 1.0;
                    c.metabolic_exhaustion += 4.5;
                    bool telomerase_active = Cellular::HasFlag(c.flags, Cellular::FLAG_TELOMERASE_ACTIVE);
                    if (!telomerase_active) {
                        c.telomere_length -= Cellular::TELOMERE_LOSS_PER_DIVISION;
                    } else {
                        c.telomere_length += (Cellular::TELOMERE_LOSS_PER_DIVISION * 1.5);
                        if (c.telomere_length > Cellular::INITIAL_TELOMERE) c.telomere_length = Cellular::INITIAL_TELOMERE;
                    }
                    if (c.telomere_length <= Cellular::CRITICAL_TELOMERE_LENGTH && !is_onco) c.state = Cellular::CellState::SENESCENT;

                    daughter = c;
                    daughter.id = atomicAdd((unsigned long long*)global_id_counter, 1ULL);
                    daughter.age = 0.0;
                    daughter.metabolic_exhaustion = 0.0;
                    daughter.epigenetic_shield = 1.0f;
                    daughter.cd59_shield = c.cd59_shield; 
                    daughter.hif1a_expression = 0.0f; 
                }

                unsigned long long write_idx = atomicAdd((unsigned long long*)new_pop_counter, 1ULL);
                if (write_idx < max_capacity) {
                    d_write[write_idx] = c;
                } else {
                    atomicMin((unsigned long long*)new_pop_counter, (unsigned long long)max_capacity);
                }

                if (divided) {
                    unsigned long long d_write_idx = atomicAdd((unsigned long long*)new_pop_counter, 1ULL);
                    if (d_write_idx < max_capacity) {
                        d_write[d_write_idx] = daughter;
                    } else {
                        atomicMin((unsigned long long*)new_pop_counter, (unsigned long long)max_capacity);
                    }
                }
            }

            if (eaten_toxins > 0.0f) l_toxins_cleared += eaten_toxins;

            if (l_total_healthy) atomicAdd(&s_total_healthy, l_total_healthy);
            if (l_total_senescent) atomicAdd(&s_total_senescent, l_total_senescent);
            if (l_total_oncogenic) atomicAdd(&s_total_oncogenic, l_total_oncogenic);
            if (l_total_telomere > 0.0) atomicAdd(&s_total_telomere, l_total_telomere);
            if (l_total_mutation > 0.0) atomicAdd(&s_total_mutation, l_total_mutation);
            if (l_total_age > 0.0) atomicAdd(&s_total_age, l_total_age);
            if (l_total_exhaustion > 0.0) atomicAdd(&s_total_exhaustion, l_total_exhaustion);
            if (l_necrotic_deaths) atomicAdd(&s_necrotic_deaths, l_necrotic_deaths);
            if (l_clean_deaths) atomicAdd(&s_clean_deaths, l_clean_deaths);
            if (l_toxins_cleared > 0.0) atomicAdd(&s_toxins_cleared, l_toxins_cleared);
            if (l_local_tumor_exosomes > 0.0) atomicAdd(&s_local_tumor_exosomes, l_local_tumor_exosomes);
            if (l_exosomes_absorbed) atomicAdd(&s_exosomes_absorbed, l_exosomes_absorbed);

            __syncthreads();

            if (threadIdx.x == 0) {
                if (s_total_healthy) atomicAdd(&(d_telemetry->total_healthy), s_total_healthy);
                if (s_total_senescent) atomicAdd(&(d_telemetry->total_senescent), s_total_senescent);
                if (s_total_oncogenic) atomicAdd(&(d_telemetry->total_oncogenic), s_total_oncogenic);
                if (s_total_telomere > 0.0) atomicAdd(&(d_telemetry->total_telomere), s_total_telomere);
                if (s_total_mutation > 0.0) atomicAdd(&(d_telemetry->total_mutation), s_total_mutation);
                if (s_total_age > 0.0) atomicAdd(&(d_telemetry->total_age), s_total_age);
                if (s_total_exhaustion > 0.0) atomicAdd(&(d_telemetry->total_exhaustion), s_total_exhaustion);
                if (s_necrotic_deaths) atomicAdd(&(d_telemetry->necrotic_deaths_this_tick), s_necrotic_deaths);
                if (s_clean_deaths) atomicAdd(&(d_telemetry->clean_deaths_this_tick), s_clean_deaths);
                if (s_toxins_cleared > 0.0) atomicAdd(&(d_telemetry->toxins_cleared_by_autophagy), s_toxins_cleared);
                if (s_local_tumor_exosomes > 0.0) atomicAdd(&(d_telemetry->local_tumor_exosomes_produced), s_local_tumor_exosomes);
                if (s_exosomes_absorbed) atomicAdd(&(d_telemetry->exosomes_absorbed_this_tick), s_exosomes_absorbed);
            }
        }

        CellularKernelEngine::CellularKernelEngine() : d_cells_read_(nullptr), d_cells_write_(nullptr), max_capacity_(0),
            d_heatmap_r_(nullptr), d_heatmap_g_(nullptr), d_heatmap_b_(nullptr),
            compute_stream_(nullptr), h_telemetry_pinned_(nullptr), h_counters_pinned_(nullptr), h_success_pinned_(nullptr), h_current_pop_pinned_(nullptr) {
            instance_ = this;
        }

        CellularKernelEngine::~CellularKernelEngine() { FreeVRAM(); }

        CellularKernelEngine* CellularKernelEngine::GetInstance() { return instance_; }

        void CellularKernelEngine::AllocateVRAM(size_t max_capacity, size_t initial_population) {
            FreeVRAM();
            max_capacity_ = max_capacity;

            cudaDeviceSetLimit(cudaLimitPrintfFifoSize, 0);
            cudaDeviceSetLimit(cudaLimitMallocHeapSize, 0);
            cudaDeviceSetLimit(cudaLimitDevRuntimePendingLaunchCount, 0);
            cudaDeviceSetLimit(cudaLimitStackSize, 1024); 

            // 🛑 PHASE 71/72: Initialize Async Stream with LOW PRIORITY and Pinned Memory (WDDM Decoupling)
            int leastPriority = 0, greatestPriority = 0;
            cudaDeviceGetStreamPriorityRange(&leastPriority, &greatestPriority);
            cudaStreamCreateWithPriority(&compute_stream_, cudaStreamNonBlocking, leastPriority);

            cudaMallocHost(&h_telemetry_pinned_, sizeof(GpuTelemetry));
            cudaMallocHost(&h_counters_pinned_, 4 * sizeof(unsigned long long));
            cudaMallocHost(&h_success_pinned_, sizeof(unsigned long long));
            cudaMallocHost(&h_current_pop_pinned_, sizeof(uint64_t));

            cudaMalloc(&d_cells_read_, max_capacity_ * sizeof(Cellular::Cell));
            cudaMalloc(&d_cells_write_, max_capacity_ * sizeof(Cellular::Cell));
            cudaMalloc(&d_current_population_, sizeof(uint64_t));
            cudaMalloc(&d_global_id_counter_, sizeof(uint64_t));
            cudaMalloc(&d_telemetry_, sizeof(GpuTelemetry));

            int map_size = 1024 * 1024 * sizeof(float);
            cudaMalloc(&d_heatmap_r_, map_size);
            cudaMalloc(&d_heatmap_g_, map_size);
            cudaMalloc(&d_heatmap_b_, map_size);

            uint64_t zero = 0;
            cudaMemcpy(d_global_id_counter_, &zero, sizeof(uint64_t), cudaMemcpyHostToDevice);

            unsigned int blockSize = 256;
            unsigned int numBlocks = static_cast<unsigned int>((initial_population + blockSize - 1) / blockSize);
            InitCellsKernel<<<numBlocks, blockSize, 0, compute_stream_>>>(d_cells_read_, d_global_id_counter_, initial_population);
            
            // 🛑 PHASE 72: Necessary sync to guarantee initial population memory setup
            cudaStreamSynchronize(compute_stream_);

            uint64_t init_pop = initial_population;
            cudaMemcpy(d_current_population_, &init_pop, sizeof(uint64_t), cudaMemcpyHostToDevice);
        }

        void CellularKernelEngine::FreeVRAM() {
            if (d_cells_read_) cudaFree(d_cells_read_);
            if (d_cells_write_) cudaFree(d_cells_write_);
            if (d_current_population_) cudaFree(d_current_population_);
            if (d_global_id_counter_) cudaFree(d_global_id_counter_);
            if (d_telemetry_) cudaFree(d_telemetry_);
            if (d_heatmap_r_) cudaFree(d_heatmap_r_);
            if (d_heatmap_g_) cudaFree(d_heatmap_g_);
            if (d_heatmap_b_) cudaFree(d_heatmap_b_);

            if (h_telemetry_pinned_) cudaFreeHost(h_telemetry_pinned_);
            if (h_counters_pinned_) cudaFreeHost(h_counters_pinned_);
            if (h_success_pinned_) cudaFreeHost(h_success_pinned_);
            if (h_current_pop_pinned_) cudaFreeHost(h_current_pop_pinned_);
            if (compute_stream_) cudaStreamDestroy(compute_stream_);
            
            d_cells_read_ = nullptr;
            compute_stream_ = nullptr;
            h_telemetry_pinned_ = nullptr;
            h_counters_pinned_ = nullptr;
            h_success_pinned_ = nullptr;
            h_current_pop_pinned_ = nullptr;
        }

        void CellularKernelEngine::PerformBiologicalTick(
            const AI_Director::BioEnvironment& env_snap,
            const AI_Director::ImmuneSystem& immune_snap,
            double genomic_instability,
            double stromal_rigidity,
            double accumulated_toxicity,
            size_t base_carrying_capacity,
            size_t dynamic_carrying_capacity,
            double systemic_exosomes_concentration,
            double systemic_z_tumor_saturation,
            double dynamic_tumor_stealth_factor, 
            bool integration_phase_active,
            GpuTelemetry& out_telemetry) 
        {
            cudaMemcpyAsync(h_current_pop_pinned_, d_current_population_, sizeof(uint64_t), cudaMemcpyDeviceToHost, compute_stream_);
            cudaStreamSynchronize(compute_stream_);
            uint64_t current_pop = *h_current_pop_pinned_;

            if (current_pop > max_capacity_) current_pop = max_capacity_;

            if (current_pop == 0) return;

            cudaMemsetAsync(d_telemetry_, 0, sizeof(GpuTelemetry), compute_stream_);
            uint64_t zero = 0;
            cudaMemcpyAsync(d_current_population_, &zero, sizeof(uint64_t), cudaMemcpyHostToDevice, compute_stream_);

            static uint64_t epoch = 0;
            epoch++;

            unsigned int blockSize = 256;
            unsigned int numBlocks = static_cast<unsigned int>((current_pop + blockSize - 1) / blockSize);

            BiologicalTickKernel<<<numBlocks, blockSize, 0, compute_stream_>>>(
                d_cells_read_, d_cells_write_, current_pop, max_capacity_,
                d_current_population_, d_global_id_counter_, d_telemetry_,
                env_snap, immune_snap, genomic_instability, stromal_rigidity,
                accumulated_toxicity, base_carrying_capacity, dynamic_carrying_capacity, 
                systemic_exosomes_concentration, systemic_z_tumor_saturation, 
                dynamic_tumor_stealth_factor, integration_phase_active, epoch
            );

            cudaMemcpyAsync(h_telemetry_pinned_, d_telemetry_, sizeof(GpuTelemetry), cudaMemcpyDeviceToHost, compute_stream_);
            
            // 🛑 PHASE 72: Mandatory engine thread synchronization for exact biological telemetry logic processing in CPU
            // Note: This no longer stalls the UI because SentinelGuard and this function run solely in the decoupled ReactorEngine thread!
            cudaStreamSynchronize(compute_stream_); 

            out_telemetry = *h_telemetry_pinned_;

            Cellular::Cell* temp = d_cells_read_;
            d_cells_read_ = d_cells_write_;
            d_cells_write_ = temp;
        }

        void CellularKernelEngine::RunSentinelGuard(double pid_signal, bool pruning_mode, bool suppressed_mode, bool mvg_lockdown, double dynamic_threshold, uint64_t epoch_ticks, SentinelTelemetry& out_sentinel_tel) {
            cudaMemcpyAsync(h_current_pop_pinned_, d_current_population_, sizeof(uint64_t), cudaMemcpyDeviceToHost, compute_stream_);
            cudaStreamSynchronize(compute_stream_);
            uint64_t current_pop = *h_current_pop_pinned_;
            
            if (current_pop > max_capacity_) current_pop = max_capacity_;
            if (current_pop == 0) return;
            
            unsigned long long* d_counters; 
            cudaMallocAsync(&d_counters, 4 * sizeof(unsigned long long), compute_stream_); 
            cudaMemsetAsync(d_counters, 0, 4 * sizeof(unsigned long long), compute_stream_);
            
            unsigned int blockSize = 256; 
            unsigned int numBlocks = static_cast<unsigned int>((current_pop + blockSize - 1) / blockSize);
            
            SentinelGuardKernel<<<numBlocks, blockSize, 0, compute_stream_>>>(d_cells_read_, current_pop, pid_signal, pruning_mode, mvg_lockdown, dynamic_threshold, epoch_ticks, d_counters);
            
            cudaMemcpyAsync(h_counters_pinned_, d_counters, 4 * sizeof(unsigned long long), cudaMemcpyDeviceToHost, compute_stream_);
            
            // 🛑 PHASE 72: Safely contained sync within the decoupled Engine thread.
            cudaStreamSynchronize(compute_stream_);
            
            out_sentinel_tel.purged_count = h_counters_pinned_[0]; 
            out_sentinel_tel.bypassed_count = h_counters_pinned_[1]; 
            out_sentinel_tel.pruned_tumor_cells = h_counters_pinned_[2]; 
            out_sentinel_tel.mvg_saved_cells = h_counters_pinned_[3];
            
            cudaFreeAsync(d_counters, compute_stream_);
        }

        size_t CellularKernelEngine::RunTelomeraseExploit(size_t infection_count, double target_telomere_lock, uint64_t hardware_seed, const AI_Director::BioEnvironment& env_snap, const AI_Director::ImmuneSystem& immune_snap, uint64_t epoch_ticks) {
            cudaMemcpyAsync(h_current_pop_pinned_, d_current_population_, sizeof(uint64_t), cudaMemcpyDeviceToHost, compute_stream_);
            cudaStreamSynchronize(compute_stream_);
            uint64_t current_pop = *h_current_pop_pinned_;

            if (current_pop > max_capacity_) current_pop = max_capacity_;
            if (current_pop == 0) return 0;
            
            unsigned long long* d_success_count; 
            cudaMallocAsync(&d_success_count, sizeof(unsigned long long), compute_stream_); 
            cudaMemsetAsync(d_success_count, 0, sizeof(unsigned long long), compute_stream_);
            
            unsigned int blockSize = 256;
            unsigned int numBlocks = static_cast<unsigned int>((infection_count + blockSize - 1) / blockSize);

            TelomeraseExploitKernel<<<numBlocks, blockSize, 0, compute_stream_>>>(d_cells_read_, current_pop, infection_count, target_telomere_lock, hardware_seed, env_snap, immune_snap, epoch_ticks, d_success_count);
            
            cudaMemcpyAsync(h_success_pinned_, d_success_count, sizeof(unsigned long long), cudaMemcpyDeviceToHost, compute_stream_);
            cudaStreamSynchronize(compute_stream_);

            unsigned long long h_success_count = *h_success_pinned_;
            cudaFreeAsync(d_success_count, compute_stream_);
            
            return static_cast<size_t>(h_success_count);
        }

        void CellularKernelEngine::FetchSubsetToHost(Cellular::Cell* host_buffer, size_t max_count, size_t& actual_count) {
            cudaMemcpyAsync(h_current_pop_pinned_, d_current_population_, sizeof(uint64_t), cudaMemcpyDeviceToHost, compute_stream_);
            cudaStreamSynchronize(compute_stream_);
            uint64_t current_pop = *h_current_pop_pinned_;

            if (current_pop > max_capacity_) current_pop = max_capacity_;
            actual_count = (current_pop < max_count) ? static_cast<size_t>(current_pop) : max_count;
            if (actual_count > 0) {
                cudaMemcpyAsync(host_buffer, d_cells_read_, actual_count * sizeof(Cellular::Cell), cudaMemcpyDeviceToHost, compute_stream_);
                cudaStreamSynchronize(compute_stream_);
            }
        }

        Cellular::Cell* CellularKernelEngine::GetDevicePointer() { return d_cells_read_; }
        
        size_t CellularKernelEngine::GetCurrentPopulation() {
            uint64_t current_pop = 0;
            if (d_current_population_) {
                cudaMemcpyAsync(h_current_pop_pinned_, d_current_population_, sizeof(uint64_t), cudaMemcpyDeviceToHost, compute_stream_);
                cudaStreamSynchronize(compute_stream_);
                current_pop = *h_current_pop_pinned_;
            }
            if (current_pop > max_capacity_) current_pop = max_capacity_; 
            return static_cast<size_t>(current_pop);
        }
    }
}