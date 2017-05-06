#ifndef __VOOGGDECID_H_
#define __VOOGGDECID_H_

#ifndef _WIN32_WCE
#ifndef _DEBUG

#define RENAME(func)	_##func


#define AntiCosTable_16                               voQCELPEnc00000000 
#define Autocorr_s                                    voQCELPEnc00000001 
#define CosineTable_16                                voQCELPEnc00000006 
#define GA                                            voQCELPEnc00000010 
#define GPRED_COEFF                                   voQCELPEnc00000011 
#define G_QUANT_TYPE                                  voQCELPEnc00000012 
#define HAMMINGwindow                                 voQCELPEnc00000014 
//#define InitFrameBuffer                               voQCELPEnc00000015 
#define LOWEST_LEVEL                                  voQCELPEnc00000016 
#define LSPVQ0                                        voQCELPEnc00000017 
#define LSPVQ1                                        voQCELPEnc00000018 
#define LSPVQ2                                        voQCELPEnc00000019 
#define LSPVQ3                                        voQCELPEnc00000020 
#define LSPVQ4                                        voQCELPEnc00000021 
#define LSPVQSIZE                                     voQCELPEnc00000022 
#define LSP_DPCM_DECAY                                voQCELPEnc00000023 
#define LSP_QTYPE                                     voQCELPEnc00000024 
#define Lag_max                                       voQCELPEnc00000025 
#define MAXG                                          voQCELPEnc00000026 
#define MAX_DELTA_LSP                                 voQCELPEnc00000027 
#define MING                                          voQCELPEnc00000028 
#define MIN_DELTA_LSP                                 voQCELPEnc00000029 
#define NUMBER_OF_G_LEVELS                            voQCELPEnc00000030 
#define NUM_LSP_QLEVELS                               voQCELPEnc00000032 
#define QG8                                           voQCELPEnc00000034 
#define RELNUMBER_OF_G_LEVELS                         voQCELPEnc00000035 
#define SinineTable_16                                voQCELPEnc00000036 
#define THRESH_SNR                                    voQCELPEnc00000037 
#define adjust_rate_down                              voQCELPEnc00000039 
#define adjust_rate_up                                voQCELPEnc00000040 
#define anticosvalue                                  voQCELPEnc00000041 
#define band_energy_fcn                               voQCELPEnc00000042 
#define clear_packet_params                           voQCELPEnc00000043 
//#define comp_corr                                     voQCELPEnc00000052 
#define comp_corr10                                   voQCELPEnc00000053 
#define comp_corr40                                   voQCELPEnc00000054 
#define comp_corr40_pitch                             voQCELPEnc00000055 
#define compute_autocorr                              voQCELPEnc00000056 
#define compute_cb                                    voQCELPEnc00000057 
#define compute_cb_gain                               voQCELPEnc00000058 
#define compute_features                              voQCELPEnc00000059 
#define compute_lpc                                   voQCELPEnc00000060 
#define compute_pitch                                 voQCELPEnc00000061 
#define compute_sns                                   voQCELPEnc00000062 
#define compute_target_snr                            voQCELPEnc00000063 
#define create_target_speech                          voQCELPEnc00000064 
#define debug_do_pole_filter                          voQCELPEnc00000065 
#define decimate_filter                               voQCELPEnc00000066 
#define dis_coef                                      voQCELPEnc00000067 
//#define divide_s_cb                                   voQCELPEnc00000068 
#define do_fir_linear_filter                          voQCELPEnc00000069 
//#define do_fir_linear_filter_asm                      voQCELPEnc00000070 
#define do_ploe_filter_response                       voQCELPEnc00000071 
//#define do_ploe_filter_response1_asm                  voQCELPEnc00000072 
#define do_ploe_filter_response_1                     voQCELPEnc00000073 
//#define do_ploe_filter_response_asm                   voQCELPEnc00000074 
#define do_pole_filter                                voQCELPEnc00000075 
#define do_pole_filter_1_tap_interp                   voQCELPEnc00000076 
//#define do_pole_filter_asm                            voQCELPEnc00000077 
#define do_pole_filter_high                           voQCELPEnc00000078 
//#define do_pole_filter_high_asm                       voQCELPEnc00000079 
#define do_zero_filter                                voQCELPEnc00000080 
//#define do_zero_filter_asm                            voQCELPEnc00000081 
#define do_zero_filter_front                          voQCELPEnc00000082 
//#define do_zero_filter_front_asm                      voQCELPEnc00000083 
#define durbin                                        voQCELPEnc00000084 
#define encoder                                       voQCELPEnc00000085 
//#define filter_1_tap_interp_asm                       voQCELPEnc00000086 
#define free_encoder                                  voQCELPEnc00000087 
#define free_pole_filter                              voQCELPEnc00000088 
#define free_zero_filter                              voQCELPEnc00000089 
#define front_end_filter                              voQCELPEnc00000090 
#define gArray0                                       voQCELPEnc00000091 
//#define g_hQCELPEncInst                               voQCELPEnc00000092 
#define g_memOP                                       voQCELPEnc00000093 
#define get_impulse_response_pole                     voQCELPEnc00000094 
#define get_zero_input_response_pole                  voQCELPEnc00000095 
#define get_zero_input_response_pole_1_tap_interp     voQCELPEnc00000096 
#define getbit                                        voQCELPEnc00000097 
#define grid                                          voQCELPEnc00000098 
#define hangover                                      voQCELPEnc00000099 
#define initial_recursive_conv                        voQCELPEnc00000100 
//#define initial_recursive_conv_asm                    voQCELPEnc00000101 
#define initialize_encoder                            voQCELPEnc00000102 
#define initialize_pole_1_tap_filter                  voQCELPEnc00000103 
#define initialize_pole_filter                        voQCELPEnc00000104 
#define initialize_zero_filter                        voQCELPEnc00000105 
#define initlsp                                       voQCELPEnc00000106 
#define interp_lpcs                                   voQCELPEnc00000107 
#define lin_quant                                     voQCELPEnc00000108 
#define lin_unquant                                   voQCELPEnc00000109 
#define lpc2lsp                                       voQCELPEnc00000110 
#define lsp2lpc                                       voQCELPEnc00000111 
#define pack_cb                                       voQCELPEnc00000114 
#define pack_frame                                    voQCELPEnc00000115 
#define pack_lpc                                      voQCELPEnc00000116 
#define pack_pitch                                    voQCELPEnc00000117 
#define quantize_G                                    voQCELPEnc00000118 
#define quantize_G_8th                                voQCELPEnc00000119 
#define quantize_b                                    voQCELPEnc00000120 
#define quantize_i                                    voQCELPEnc00000121 
#define quantize_lag                                  voQCELPEnc00000122 
#define quantize_lpc                                  voQCELPEnc00000123 
#define quantize_min_lag                              voQCELPEnc00000124 
#define rate_filt                                     voQCELPEnc00000125 
#define recursive_conv_10                             voQCELPEnc00000126 
#define recursive_conv_10_Opt                         voQCELPEnc00000127 
//#define recursive_conv_40_Opt                         voQCELPEnc00000128 
#define reset_encoder                                 voQCELPEnc00000129 
#define run_decoder                                   voQCELPEnc00000130 
#define save_pitch                                    voQCELPEnc00000131 
#define save_target                                   voQCELPEnc00000132 
#define select_mode1                                  voQCELPEnc00000133 
#define select_mode2                                  voQCELPEnc00000134 
#define set_lag_range                                 voQCELPEnc00000135 
#define target_reduction                              voQCELPEnc00000136 
#define truefalse                                     voQCELPEnc00000137 
#define unquantize_G                                  voQCELPEnc00000138 
#define unquantize_G_8th                              voQCELPEnc00000139 
#define unquantize_b                                  voQCELPEnc00000140 
#define unquantize_i                                  voQCELPEnc00000141 
#define unquantize_lag                                voQCELPEnc00000142 
#define unquantize_lsp                                voQCELPEnc00000143 
#define unquantize_min_lag                            voQCELPEnc00000144 
#define unv_filter                                    voQCELPEnc00000145 
#define update_form_resid_mems                        voQCELPEnc00000146 
#define update_hist_cnt                               voQCELPEnc00000147 
#define update_target_cb                              voQCELPEnc00000148 
#define update_target_pitch                           voQCELPEnc00000149 
#define voQCELP_GetOutputData                         voQCELPEnc00000150 
#define voQCELP_GetParam                              voQCELPEnc00000151 
#define voQCELP_Init                                  voQCELPEnc00000152 
#define voQCELP_SetInputData                          voQCELPEnc00000153 
#define voQCELP_SetParam                              voQCELPEnc00000154 
#define voQCELP_Uninit                                voQCELPEnc00000155 
#define vo_comput_cb                                  voQCELPEnc00000156 

#define _AntiCosTable_16                               RENAME(voQCELPEnc00000000) 
#define _Autocorr_s                                    RENAME(voQCELPEnc00000001) 
#define _CosineTable_16                                RENAME(voQCELPEnc00000006) 
#define _GA                                            RENAME(voQCELPEnc00000010) 
#define _GPRED_COEFF                                   RENAME(voQCELPEnc00000011) 
#define _G_QUANT_TYPE                                  RENAME(voQCELPEnc00000012) 
#define _HAMMINGwindow                                 RENAME(voQCELPEnc00000014) 
//#define _InitFrameBuffer                               RENAME(voQCELPEnc00000015) 
#define _LOWEST_LEVEL                                  RENAME(voQCELPEnc00000016) 
#define _LSPVQ0                                        RENAME(voQCELPEnc00000017) 
#define _LSPVQ1                                        RENAME(voQCELPEnc00000018) 
#define _LSPVQ2                                        RENAME(voQCELPEnc00000019) 
#define _LSPVQ3                                        RENAME(voQCELPEnc00000020) 
#define _LSPVQ4                                        RENAME(voQCELPEnc00000021) 
#define _LSPVQSIZE                                     RENAME(voQCELPEnc00000022) 
#define _LSP_DPCM_DECAY                                RENAME(voQCELPEnc00000023) 
#define _LSP_QTYPE                                     RENAME(voQCELPEnc00000024) 
#define _Lag_max                                       RENAME(voQCELPEnc00000025) 
#define _MAXG                                          RENAME(voQCELPEnc00000026) 
#define _MAX_DELTA_LSP                                 RENAME(voQCELPEnc00000027) 
#define _MING                                          RENAME(voQCELPEnc00000028) 
#define _MIN_DELTA_LSP                                 RENAME(voQCELPEnc00000029) 
#define _NUMBER_OF_G_LEVELS                            RENAME(voQCELPEnc00000030) 
#define _NUM_LSP_QLEVELS                               RENAME(voQCELPEnc00000032) 
#define _QG8                                           RENAME(voQCELPEnc00000034) 
#define _RELNUMBER_OF_G_LEVELS                         RENAME(voQCELPEnc00000035) 
#define _SinineTable_16                                RENAME(voQCELPEnc00000036) 
#define _THRESH_SNR                                    RENAME(voQCELPEnc00000037) 
#define _adjust_rate_down                              RENAME(voQCELPEnc00000039) 
#define _adjust_rate_up                                RENAME(voQCELPEnc00000040) 
#define _anticosvalue                                  RENAME(voQCELPEnc00000041) 
#define _band_energy_fcn                               RENAME(voQCELPEnc00000042) 
#define _clear_packet_params                           RENAME(voQCELPEnc00000043) 
//#define _comp_corr                                     RENAME(voQCELPEnc00000052) 
#define _comp_corr10                                   RENAME(voQCELPEnc00000053) 
#define _comp_corr40                                   RENAME(voQCELPEnc00000054) 
#define _comp_corr40_pitch                             RENAME(voQCELPEnc00000055) 
#define _compute_autocorr                              RENAME(voQCELPEnc00000056) 
#define _compute_cb                                    RENAME(voQCELPEnc00000057) 
#define _compute_cb_gain                               RENAME(voQCELPEnc00000058) 
#define _compute_features                              RENAME(voQCELPEnc00000059) 
#define _compute_lpc                                   RENAME(voQCELPEnc00000060) 
#define _compute_pitch                                 RENAME(voQCELPEnc00000061) 
#define _compute_sns                                   RENAME(voQCELPEnc00000062) 
#define _compute_target_snr                            RENAME(voQCELPEnc00000063) 
#define _create_target_speech                          RENAME(voQCELPEnc00000064) 
#define _debug_do_pole_filter                          RENAME(voQCELPEnc00000065) 
#define _decimate_filter                               RENAME(voQCELPEnc00000066) 
#define _dis_coef                                      RENAME(voQCELPEnc00000067) 
//#define _divide_s_cb                                   RENAME(voQCELPEnc00000068) 
#define _do_fir_linear_filter                          RENAME(voQCELPEnc00000069) 
//#define _do_fir_linear_filter_asm                      RENAME(voQCELPEnc00000070) 
#define _do_ploe_filter_response                       RENAME(voQCELPEnc00000071) 
//#define _do_ploe_filter_response1_asm                  RENAME(voQCELPEnc00000072) 
#define _do_ploe_filter_response_1                     RENAME(voQCELPEnc00000073) 
//#define _do_ploe_filter_response_asm                   RENAME(voQCELPEnc00000074) 
#define _do_pole_filter                                RENAME(voQCELPEnc00000075) 
#define _do_pole_filter_1_tap_interp                   RENAME(voQCELPEnc00000076) 
//#define _do_pole_filter_asm                            RENAME(voQCELPEnc00000077) 
#define _do_pole_filter_high                           RENAME(voQCELPEnc00000078) 
//#define _do_pole_filter_high_asm                       RENAME(voQCELPEnc00000079) 
#define _do_zero_filter                                RENAME(voQCELPEnc00000080) 
//#define _do_zero_filter_asm                            RENAME(voQCELPEnc00000081) 
#define _do_zero_filter_front                          RENAME(voQCELPEnc00000082) 
//#define _do_zero_filter_front_asm                      RENAME(voQCELPEnc00000083) 
#define _durbin                                        RENAME(voQCELPEnc00000084) 
#define _encoder                                       RENAME(voQCELPEnc00000085) 
//#define _filter_1_tap_interp_asm                       RENAME(voQCELPEnc00000086) 
#define _free_encoder                                  RENAME(voQCELPEnc00000087) 
#define _free_pole_filter                              RENAME(voQCELPEnc00000088) 
#define _free_zero_filter                              RENAME(voQCELPEnc00000089) 
#define _front_end_filter                              RENAME(voQCELPEnc00000090) 
#define _gArray0                                       RENAME(voQCELPEnc00000091) 
//#define _g_hQCELPEncInst                               RENAME(voQCELPEnc00000092) 
#define _g_memOP                                       RENAME(voQCELPEnc00000093) 
#define _get_impulse_response_pole                     RENAME(voQCELPEnc00000094) 
#define _get_zero_input_response_pole                  RENAME(voQCELPEnc00000095) 
#define _get_zero_input_response_pole_1_tap_interp     RENAME(voQCELPEnc00000096) 
#define _getbit                                        RENAME(voQCELPEnc00000097) 
#define _grid                                          RENAME(voQCELPEnc00000098) 
#define _hangover                                      RENAME(voQCELPEnc00000099) 
#define _initial_recursive_conv                        RENAME(voQCELPEnc00000100) 
//#define _initial_recursive_conv_asm                    RENAME(voQCELPEnc00000101) 
#define _initialize_encoder                            RENAME(voQCELPEnc00000102) 
#define _initialize_pole_1_tap_filter                  RENAME(voQCELPEnc00000103) 
#define _initialize_pole_filter                        RENAME(voQCELPEnc00000104) 
#define _initialize_zero_filter                        RENAME(voQCELPEnc00000105) 
#define _initlsp                                       RENAME(voQCELPEnc00000106) 
#define _interp_lpcs                                   RENAME(voQCELPEnc00000107) 
#define _lin_quant                                     RENAME(voQCELPEnc00000108) 
#define _lin_unquant                                   RENAME(voQCELPEnc00000109) 
#define _lpc2lsp                                       RENAME(voQCELPEnc00000110) 
#define _lsp2lpc                                       RENAME(voQCELPEnc00000111) 
#define _pack_cb                                       RENAME(voQCELPEnc00000114) 
#define _pack_frame                                    RENAME(voQCELPEnc00000115) 
#define _pack_lpc                                      RENAME(voQCELPEnc00000116) 
#define _pack_pitch                                    RENAME(voQCELPEnc00000117) 
#define _quantize_G                                    RENAME(voQCELPEnc00000118) 
#define _quantize_G_8th                                RENAME(voQCELPEnc00000119) 
#define _quantize_b                                    RENAME(voQCELPEnc00000120) 
#define _quantize_i                                    RENAME(voQCELPEnc00000121) 
#define _quantize_lag                                  RENAME(voQCELPEnc00000122) 
#define _quantize_lpc                                  RENAME(voQCELPEnc00000123) 
#define _quantize_min_lag                              RENAME(voQCELPEnc00000124) 
#define _rate_filt                                     RENAME(voQCELPEnc00000125) 
#define _recursive_conv_10                             RENAME(voQCELPEnc00000126) 
#define _recursive_conv_10_Opt                         RENAME(voQCELPEnc00000127) 
//#define _recursive_conv_40_Opt                         RENAME(voQCELPEnc00000128) 
#define _reset_encoder                                 RENAME(voQCELPEnc00000129) 
#define _run_decoder                                   RENAME(voQCELPEnc00000130) 
#define _save_pitch                                    RENAME(voQCELPEnc00000131) 
#define _save_target                                   RENAME(voQCELPEnc00000132) 
#define _select_mode1                                  RENAME(voQCELPEnc00000133) 
#define _select_mode2                                  RENAME(voQCELPEnc00000134) 
#define _set_lag_range                                 RENAME(voQCELPEnc00000135) 
#define _target_reduction                              RENAME(voQCELPEnc00000136) 
#define _truefalse                                     RENAME(voQCELPEnc00000137) 
#define _unquantize_G                                  RENAME(voQCELPEnc00000138) 
#define _unquantize_G_8th                              RENAME(voQCELPEnc00000139) 
#define _unquantize_b                                  RENAME(voQCELPEnc00000140) 
#define _unquantize_i                                  RENAME(voQCELPEnc00000141) 
#define _unquantize_lag                                RENAME(voQCELPEnc00000142) 
#define _unquantize_lsp                                RENAME(voQCELPEnc00000143) 
#define _unquantize_min_lag                            RENAME(voQCELPEnc00000144) 
#define _unv_filter                                    RENAME(voQCELPEnc00000145) 
#define _update_form_resid_mems                        RENAME(voQCELPEnc00000146) 
#define _update_hist_cnt                               RENAME(voQCELPEnc00000147) 
#define _update_target_cb                              RENAME(voQCELPEnc00000148) 
#define _update_target_pitch                           RENAME(voQCELPEnc00000149) 
#define _voQCELP_GetOutputData                         RENAME(voQCELPEnc00000150) 
#define _voQCELP_GetParam                              RENAME(voQCELPEnc00000151) 
#define _voQCELP_Init                                  RENAME(voQCELPEnc00000152) 
#define _voQCELP_SetInputData                          RENAME(voQCELPEnc00000153) 
#define _voQCELP_SetParam                              RENAME(voQCELPEnc00000154) 
#define _voQCELP_Uninit                                RENAME(voQCELPEnc00000155) 
#define _vo_comput_cb                                  RENAME(voQCELPEnc00000156) 

#endif //#ifndef _DEBUG
#endif //#ifndef _WIN32_WCE
#endif //#ifndef __VOOGGDECID_H_