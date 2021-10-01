# copyright (c) 2016 Zhang Rui <bbcallen@gmail.com>
#
# This file is part of ijkPlayer.
#
# ijkPlayer is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# ijkPlayer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public 
# License along with ijkPlayer; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS += -std=c99

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common_audio/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common_audio/resampler/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common_audio/signal_processing/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/common_audio/vad/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/modules/audio_processing/aec/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/modules/audio_processing/aecm/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/modules/audio_processing/agc/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/modules/audio_processing/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/modules/audio_processing/ns/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/system_wrappers/interface
LOCAL_C_INCLUDES += $(realpath $(LOCAL_PATH))

LOCAL_SRC_FILES += common_audio/signal_processing/complex_bit_reverse.c
LOCAL_SRC_FILES += common_audio/signal_processing/complex_fft.c
LOCAL_SRC_FILES += common_audio/signal_processing/copy_set_operations.c
LOCAL_SRC_FILES += common_audio/signal_processing/cross_correlation.c
LOCAL_SRC_FILES += common_audio/signal_processing/division_operations.c
LOCAL_SRC_FILES += common_audio/signal_processing/dot_product_with_scale.c
LOCAL_SRC_FILES += common_audio/signal_processing/downsample_fast.c
LOCAL_SRC_FILES += common_audio/signal_processing/energy.c
LOCAL_SRC_FILES += common_audio/signal_processing/get_scaling_square.c
LOCAL_SRC_FILES += common_audio/signal_processing/randomization_functions.c
LOCAL_SRC_FILES += common_audio/signal_processing/real_fft.c
LOCAL_SRC_FILES += common_audio/signal_processing/refl_coef_to_lpc.c
LOCAL_SRC_FILES += common_audio/signal_processing/resample_48khz.c
LOCAL_SRC_FILES += common_audio/signal_processing/resample_by_2_internal.c
LOCAL_SRC_FILES += common_audio/signal_processing/resample_by_2.c
LOCAL_SRC_FILES += common_audio/signal_processing/resample_fractional.c
LOCAL_SRC_FILES += common_audio/signal_processing/resample.c
LOCAL_SRC_FILES += common_audio/signal_processing/spl_init.c
LOCAL_SRC_FILES += common_audio/signal_processing/spl_sqrt_floor.c
LOCAL_SRC_FILES += common_audio/signal_processing/spl_sqrt.c
LOCAL_SRC_FILES += common_audio/signal_processing/splitting_filter.c
LOCAL_SRC_FILES += common_audio/signal_processing/vector_scaling_operations.c

LOCAL_SRC_FILES += common_audio/vad/vad_core.c
LOCAL_SRC_FILES += common_audio/vad/vad_filterbank.c
LOCAL_SRC_FILES += common_audio/vad/vad_gmm.c
LOCAL_SRC_FILES += common_audio/vad/vad_sp.c
LOCAL_SRC_FILES += common_audio/vad/webrtc_vad.c

LOCAL_SRC_FILES += modules/audio_processing/aec/aec_core.c
LOCAL_SRC_FILES += modules/audio_processing/aec/aec_rdft.c
LOCAL_SRC_FILES += modules/audio_processing/aec/aec_resampler.c
LOCAL_SRC_FILES += modules/audio_processing/aec/echo_cancellation.c

LOCAL_SRC_FILES += modules/audio_processing/aecm/aecm_core_c.c
LOCAL_SRC_FILES += modules/audio_processing/aecm/aecm_core.c
LOCAL_SRC_FILES += modules/audio_processing/aecm/echo_control_mobile.c

LOCAL_SRC_FILES += modules/audio_processing/agc/analog_agc.c
LOCAL_SRC_FILES += modules/audio_processing/agc/digital_agc.c

LOCAL_SRC_FILES += modules/audio_processing/ns/noise_suppression.c
LOCAL_SRC_FILES += modules/audio_processing/ns/noise_suppression_x.c
LOCAL_SRC_FILES += modules/audio_processing/ns/ns_core.c
LOCAL_SRC_FILES += modules/audio_processing/ns/nsx_core.c

LOCAL_SRC_FILES += modules/audio_processing/utility/delay_estimator_wrapper.c
LOCAL_SRC_FILES += modules/audio_processing/utility/delay_estimator.c
LOCAL_SRC_FILES += modules/audio_processing/utility/fft4g.c
LOCAL_SRC_FILES += modules/audio_processing/utility/ring_buffer.c

LOCAL_SRC_FILES += system_wrappers/source/cpu_features.cc

LOCAL_MODULE := audio_processing
include $(BUILD_STATIC_LIBRARY)

$(call import-module,android/cpufeatures)
