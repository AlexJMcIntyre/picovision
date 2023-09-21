#pragma once

#include "hardware/pio.h"
#include "common/pimoroni_common.hpp"
#include "../pico_synth/pico_synth.hpp"

namespace pimoroni {

  class PicoSynth_I2S {
  public:
    uint i2s_data               =  9;
    uint i2s_bclk               = 10;
    uint i2s_lrclk              = 11;

    PIO audio_pio;
    uint audio_sm;
    uint audio_sm_offset        = 0;
    uint audio_dma_channel;

  private:
    static const uint SYSTEM_FREQ = 22050;

    uint16_t volume = 127;

    static void dma_complete();

    static const uint NUM_TONE_BUFFERS = 2;
    static const uint TONE_BUFFER_SIZE = 4;
    int16_t tone_buffers[NUM_TONE_BUFFERS][TONE_BUFFER_SIZE] = {0};
    uint current_buffer = 0;

    PicoSynth synth;

    enum PlayMode {
      PLAYING_BUFFER,
      //PLAYING_TONE,
      PLAYING_SYNTH,
      NOT_PLAYING
    };

    PlayMode play_mode = NOT_PLAYING;

  public:
    PicoSynth_I2S(uint i2s_data, uint i2s_bclk, uint i2s_lrclk, uint audio_pio, uint audio_sm) : i2s_data(i2s_data), i2s_bclk(i2s_bclk), i2s_lrclk(i2s_lrclk), audio_pio(audio_pio == 0 ? pio0 : pio1), audio_sm(audio_sm) {
        // Nothing to see here...
    }

    PicoSynth_I2S(uint i2s_data, uint i2s_bclk, uint i2s_lrclk) : PicoSynth_I2S(i2s_data, i2s_bclk, i2s_lrclk, 0, 0) {};

    ~PicoSynth_I2S();

    void init();
    static inline void pio_program_init(PIO pio, uint sm, uint offset);

    void set_volume(float value);
    float get_volume();
    void adjust_volume(float delta);

    void play_sample(uint8_t *data, uint32_t length);
    void play_synth();
    void stop_playing();
    AudioChannel& synth_channel(uint channel);

    static PicoSynth_I2S* picosynth;

  private:
    void partial_teardown();
    void dma_safe_abort(uint channel);
    void next_audio_sequence();
    void populate_next_synth();
  };

}