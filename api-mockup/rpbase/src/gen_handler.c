/**
* $Id: $
*
* @brief Red Pitaya library Generate handler interface
*
* @Author Red Pitaya
*
* (c) Red Pitaya  http://www.redpitaya.com
*
* This part of code is written in C programming language.
* Please visit http://en.wikipedia.org/wiki/C_(programming_language)
* for more details on the language used herein.
*/

#include <sys/socket.h>
#include <float.h>
#include "math.h"
#include "common.h"
#include "generate.h"
#include "gen_handler.h"

double chA_amplitude = 1, chB_amplitude = 1;
double chA_offset = 0, chB_offset = 0;
double chA_dutyCycle, chB_dutyCycle;
double chA_frequency, chB_frequency;
rp_waveform_t chA_waveform, chB_waveform;
float chA_arbitraryData[BUFFER_LENGTH];
float chB_arbitraryData[BUFFER_LENGTH];
uint32_t chA_size = BUFFER_LENGTH, chB_size = BUFFER_LENGTH;


int gen_SetDefaultValues() {
    ECHECK(gen_Disable(RP_CH_1));
    ECHECK(gen_Disable(RP_CH_2));
    ECHECK(gen_Frequency(RP_CH_1, 1000));
    ECHECK(gen_Frequency(RP_CH_2, 1000));
    ECHECK(gen_Waveform(RP_CH_1, RP_WAVEFORM_SINE));
    ECHECK(gen_Waveform(RP_CH_2, RP_WAVEFORM_SINE));
    ECHECK(gen_setAmplitude(RP_CH_1, 1));
    ECHECK(gen_setAmplitude(RP_CH_2, 1));
    ECHECK(generate_setDCOffset(RP_CH_1, 0));
    ECHECK(generate_setDCOffset(RP_CH_2, 0));
    ECHECK(gen_Phase(RP_CH_1, 0));
    ECHECK(gen_Phase(RP_CH_2, 0));
    ECHECK(gen_DutyCycle(RP_CH_1, 0.5));
    ECHECK(gen_DutyCycle(RP_CH_2, 0.5));
    ECHECK(gen_GenMode(RP_CH_1, RP_GEN_MODE_CONTINUOUS));
    ECHECK(gen_GenMode(RP_CH_2, RP_GEN_MODE_CONTINUOUS));
    ECHECK(gen_BurstCount(RP_CH_1, 1));
    ECHECK(gen_BurstCount(RP_CH_2, 1));
    ECHECK(gen_TriggerSource(RP_CH_1, RP_TRIG_SRC_INTERNAL));
    ECHECK(gen_TriggerSource(RP_CH_2, RP_TRIG_SRC_INTERNAL));
    return RP_OK;
}

int gen_Disable(rp_channel_t channel) {
    return generate_setOutputDisable(channel, true);
}

int gen_Enable(rp_channel_t channel) {
    return generate_setOutputDisable(channel, false);
}

int gen_checkAmplitudeAndOffset(double amplitude, double offset) {
    if (fabs(amplitude) + fabs(offset) > LEVEL_MAX) {
        return RP_EOOR;
    }
    return RP_OK;
}

int gen_setAmplitude(rp_channel_t channel, double amplitude) {
    double offset;
    CHECK_OUTPUT(offset = chA_offset,
                 offset = chB_offset)
    ECHECK(gen_checkAmplitudeAndOffset(amplitude, offset));

    CHECK_OUTPUT(chA_amplitude = amplitude,
                 chB_amplitude = amplitude)
    return generate_setAmplitude(channel, (float) amplitude);
}

int gen_Offset(rp_channel_t channel, double offset) {
    double amplitude;
    CHECK_OUTPUT(amplitude = chA_amplitude,
                 amplitude = chB_amplitude)
    ECHECK(gen_checkAmplitudeAndOffset(amplitude, offset));

    CHECK_OUTPUT(chA_offset = offset,
                 chB_offset = offset)
    return generate_setDCOffset(channel, (float)offset);
}

int gen_Frequency(rp_channel_t channel, double frequency) {
    if (frequency < FREQUENCY_MIN || frequency > FREQUENCY_MAX) {
        return RP_EOOR;
    }
    CHECK_OUTPUT(chA_frequency = frequency,
                 chB_frequency = frequency)
    ECHECK(generate_setFrequency(channel, (float)frequency));
    return synthesize_signal(channel);
}

int gen_Phase(rp_channel_t channel, double phase) {
    if (phase < PHASE_MIN || phase > PHASE_MAX) {
        return RP_EOOR;
    }
    return generate_setPhase(channel, (float)phase);
}

int gen_Waveform(rp_channel_t channel, rp_waveform_t type) {
    CHECK_OUTPUT(chA_waveform = type,
                 chB_waveform = type)
    if (type != RP_WAVEFORM_ARBITRARY) {
        CHECK_OUTPUT(chA_size = BUFFER_LENGTH,
                     chB_size = BUFFER_LENGTH)
    }
    return synthesize_signal(channel);
}

int gen_ArbWaveform(rp_channel_t channel, float *data, uint32_t length) {
    // Check if data is normalized
    float min = FLT_MAX, max = FLT_MIN; // initial values
    int i;
    for(i = 0; i < length; i++) {
        if (data[i] < min)
            min = data[i];
        if (data[i] > max)
            max = data[i];
    }
    if (min < ARBITRARY_MIN || max > ARBITRARY_MAX) {
        return RP_ENN;
    }

    // Save data
    float *pointer;
    CHECK_OUTPUT(pointer = chA_arbitraryData,
                 pointer = chB_arbitraryData)
    for(i = 0; i < length; i++) {
        pointer[i] = data[i];
    }
    for(i = length; i < BUFFER_LENGTH; i++) { // clear the rest of the buffer
        pointer[i] = 0;
    }

    if (channel == RP_CH_1) {
        chA_waveform = RP_WAVEFORM_ARBITRARY;
        chA_size = length;
    }
    else if (channel == RP_CH_2) {
        chB_waveform = RP_WAVEFORM_ARBITRARY;
        chB_size = length;
    }
    else {
        return RP_EPN;
    }
    return synthesize_signal(channel);
}

int gen_DutyCycle(rp_channel_t channel, double ratio) {
    if (ratio < DUTY_CYCLE_MIN || ratio > DUTY_CYCLE_MAX) {
        return RP_EOOR;
    }
    CHECK_OUTPUT(chA_dutyCycle = ratio,
                 chB_dutyCycle = ratio)
    return synthesize_signal(channel);
}

int gen_GenMode(rp_channel_t channel, rp_gen_mode_t mode) {
    if (mode == RP_GEN_MODE_CONTINUOUS) {
        generate_setOneTimeTrigger(channel, 0);
        return RP_OK;
    }
    else if (mode == RP_GEN_MODE_BURST) {
        CHECK_OUTPUT(return generate_GenTrigger(RP_CH_1),
                     return generate_GenTrigger(RP_CH_2))
    }
    else if (mode == RP_GEN_MODE_STREAM) {
        //TODO
        return RP_OK;
    }
    else {
        return RP_EIPV;
    }
}

int gen_BurstCount(rp_channel_t channel, int num) {
    if (num != 1) {
        return RP_EUF;
    }
    return RP_OK;
}

int gen_TriggerSource(rp_channel_t channel, rp_trig_src_t src) {
    if (src == RP_TRIG_SRC_INTERNAL) {
        return generate_setTriggerSource(channel, 1);
    }
    else if(src == RP_TRIG_SRC_EXTERNAL) {
        return generate_setTriggerSource(channel, 2);
    }
    else {
        return RP_EIPV;
    }
}

int gen_Trigger(int mask) {
    switch (mask) {
        case 1:
            return generate_GenTrigger(RP_CH_1);
        case 2:
            return generate_GenTrigger(RP_CH_2);
        case 3:
            ECHECK(generate_GenTrigger(RP_CH_1));
            ECHECK(generate_GenTrigger(RP_CH_2));
            return RP_OK;
        default:
            return RP_EOOR;
    }
}

int synthesize_signal(rp_channel_t channel) {
    float data[BUFFER_LENGTH];
    rp_waveform_t waveform;
    double dutyCycle, frequency;
    uint32_t size;

    if (channel == RP_CH_1) {
        waveform = chA_waveform;
        dutyCycle = chA_dutyCycle;
        frequency = chA_frequency;
        size = chA_size;
    }
    else if (channel == RP_CH_2) {
        waveform = chB_waveform;
        dutyCycle = chB_dutyCycle;
        frequency = chB_frequency;
        size = chB_size;
    }
    else{
        return RP_EPN;
    }

    switch (waveform) {
        case RP_WAVEFORM_SINE:
            synthesis_sin(data);
            break;
        case RP_WAVEFORM_TRIANGLE:
            synthesis_triangle(data);
            break;
        case RP_WAVEFORM_SQUARE:
            synthesis_square(frequency, data);
            break;
        case RP_WAVEFORM_RAMP_UP:
            synthesis_rampUp(data);
            break;
        case RP_WAVEFORM_RAMP_DOWN:
            synthesis_rampDown(data);
            break;
        case RP_WAVEFORM_DC:
            synthesis_DC(data);
            break;
        case RP_WAVEFORM_PWM:
            synthesis_PWM(dutyCycle, data);
            break;
        case RP_WAVEFORM_ARBITRARY:
            synthesis_arbitrary(channel, data);
            break;
        default:
            return RP_EIPV;
    }
    return generate_writeData(channel, data, size);
}

int synthesis_sin(float *data_out) {
    int i;
    for(i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = (float) (sin(2 * M_PI * (double) i / (double) BUFFER_LENGTH));
    }
    return RP_OK;
}

int synthesis_triangle(float *data_out) {
    int i;
    for(i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = (float) ((asin(sin(2 * M_PI * (double) i / (double) BUFFER_LENGTH)) / M_PI * 2));
    }
    return RP_OK;
}

int synthesis_rampUp(float *data_out) {
    int i;
    data_out[BUFFER_LENGTH -1] = 0;
    for(i = 0; i < BUFFER_LENGTH-1; i++) {
        data_out[BUFFER_LENGTH - i-2] = (float) (-1.0 * (acos(cos(M_PI * (double) i / (double) BUFFER_LENGTH)) / M_PI - 1));
    }
    return RP_OK;
}

int synthesis_rampDown(float *data_out) {
    int i;
    for(i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = (float) (-1.0 * (acos(cos(M_PI * (double) i / (double) BUFFER_LENGTH)) / M_PI - 1));
    }
    return RP_OK;
}

int synthesis_DC(float *data_out) {
    int i;
    for(i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = 1.0;
    }
    return RP_OK;
}

int synthesis_PWM(double ratio, float *data_out) {
    int i;

    // calculate number of samples that need to be high
    int h = (int) (BUFFER_LENGTH/2 * ratio);

    for(i = 0; i < BUFFER_LENGTH; i++) {
        if (i < h || i >= BUFFER_LENGTH - h) {
            data_out[i] = 1.0;
        }
        else {
            data_out[i] = (float) -1.0;
        }
    }
    return RP_OK;
}

int synthesis_arbitrary(rp_channel_t channel, float *data_out) {
    float *pointer;
    CHECK_OUTPUT(pointer = chA_arbitraryData,
                 pointer = chB_arbitraryData)
    for (int i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = pointer[i];
    }
    return RP_OK;
}

int synthesis_square(double frequency, float *data_out) {
    uint32_t i;

    // Various locally used constants - HW specific parameters
    const int trans0 = 30;
    const int trans1 = 300;
    const double tt2 = 0.249;

    int trans = (int) (frequency / 1e6 * trans1); // 300 samples at 1 MHz

    if (trans <= 10) {
        trans = trans0;
    }

    for(i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = (float) (sin(2 * M_PI * (double) i / (double) BUFFER_LENGTH));
        if (data_out[i] > 0)
            data_out[i] = 1.0;
        else
            data_out[i] = (float) -1.0;

        // Soft linear transitions
        double mm, qq, xx, xm;
        double x1, x2, y1, y2;

        xx = i;
        xm = BUFFER_LENGTH;

        x1 = xm * tt2;
        x2 = xm * tt2 + (double) trans;

        if ((xx > x1) && (xx <= x2)) {

            y1 = 1.0;
            y2 = -1.0;

            mm = (y2 - y1) / (x2 - x1);
            qq = y1 - mm * x1;

            data_out[i] = (int32_t) round(mm * xx + qq);
        }

        x1 = xm * 0.75;
        x2 = xm * 0.75 + trans;

        if ((xx > x1) && (xx <= x2)) {

            y1 = -1.0;
            y2 = 1.0;

            mm = (y2 - y1) / (x2 - x1);
            qq = y1 - mm * x1;

            data_out[i] = (float) (mm * xx + qq);
        }
    }
    return RP_OK;
}