/*--------------------------------------------------------------------
  This file is part of the TTN-Apeldoorn Sound Sensor.

  This code is free software:
  you can redistribute it and/or modify it under the terms of a Creative
  Commons Attribution-NonCommercial 4.0 International License
  (http://creativecommons.org/licenses/by-nc/4.0/) by
  TTN-Apeldoorn (https://www.thethingsnetwork.org/community/apeldoorn/) 

  The program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  --------------------------------------------------------------------*/

/*!
 * \file SoundSensor.cpp
 * \author Marcel Meek, Remko Welling (remko@rfsee.nl)
 */

#ifdef ESP32
 
#ifndef __SOUND_SENSOR_H_
#define __SOUND_SENSOR_H_

#include <Arduino.h>
#include <driver/i2s.h>
#include "arduinoFFT.h"
#include "i2s_noise_valid.h"

// size of noise sample
#define SAMPLES I2S_NOISE_SAMPLES  ///< 2048 @ 22,627 kHz ≈ 90 ms.
#define SAMPLE_FREQ I2S_NOISE_SAMPLE_FREQ  ///< bin bandwidth 22627 / 2048 = 11 Hz
#define OCTAVES 9

const int BLOCK_SIZE = SAMPLES;

class SoundSensor {
  public:
  
    /// \brief constructor
    SoundSensor();
    ~SoundSensor();

    /// \brief Initialize Sound sensor class and start.
    bool begin();
    bool start();
    void stop();
    void disable();
    bool running()  { return _i2s; }
    
    // Read one full block and calculate octave energy.
    // nullptr if the I2S read failed or was short — caller must not FFT/aggregate.
    float* readSamples();

  private:
    arduinoFFT   *_fft;               ///< FFT class
    // FFT buffers
    float         _real[SAMPLES];
    float         _imag[SAMPLES];
    float         _energy[OCTAVES];
    int32_t       _samples[BLOCK_SIZE];
    float         _runningDC = 0.0;   // compensate MEMS DC offset
    int           _runningN = 0;      // running DSC offset average count
    esp_err_t     _err;               ///< Variable to store errors from ESP32
    boolean       _i2s;
#if defined(ALTRUIST_BUILD_DEBUG)
    esp_err_t _debugStartError = ESP_OK;
#endif

    /// \brief Convert integer to float
    bool integerToFloat(int32_t *samples, float *vReal, float *vImag, uint16_t size);
    
    // calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
    void calculateEnergy(float *vReal, float *vImag, uint16_t samples);
    
    // sums up energy in whole octave bins
    void sumEnergy(const float *samples, float *energies);
    // sums up energy in terts bins
    void sumEnergy3(const float *samples, float *energies);
}; 

#endif // __SOUND_SENSOR_H_

#endif // ESP32
