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

#include "soundsensor.h"
#include "arduinoFFT.h"
#include "../../../defines.h"

const i2s_port_t I2S_PORT = I2S_NUM_0;

// The I2S config as per the example
const i2s_config_t i2s_config = {
  .mode                 = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),  // Receive, not transfer
  .sample_rate          = SAMPLE_FREQ,
  .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,                  // only 24 bits are used
  .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,                 // For ARduino V2.2 changed From LEFT to RIGHT !!!!!! 
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,                       // Interrupt level 1
  .dma_buf_count        = 8,                                          // number of buffers
  .dma_buf_len          = 1024,                                       //BLOCK_SIZE, samples per buffer
  .use_apll             = true
};

const i2s_pin_config_t pin_config = {
  .bck_io_num   = I2S_PIN_BCLK,
  .ws_io_num    = I2S_PIN_WS,
  .data_out_num = I2S_PIN_DOUT,
  .data_in_num  = I2S_PIN_DIN
};

SoundSensor::SoundSensor() {
  _fft = new arduinoFFT(_real, _imag, SAMPLES, SAMPLES);
  _runningDC = 0.0;
  _runningN = 0;
  _i2s = false;
}

SoundSensor::~SoundSensor(){
  i2s_driver_uninstall(  I2S_PORT);
  delete _fft;
}

bool SoundSensor::begin(){
  
  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  _err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
 // printf("_err=%d\n", _err);
//  printf("%d, %d, %d, %d\n", pin_config.bck_io_num, pin_config.ws_io_num, pin_config.data_out_num, pin_config.data_in_num);
  if (_err != ESP_OK) {
    Serial.printf("Failed installing I2S driver: %d\r\n", _err);
    return false;
  }
  
  _err = i2s_set_pin(I2S_PORT, &pin_config);
  if (_err != ESP_OK) {
    Serial.printf("Failed setting pin: %d\r\n", _err);
    return false;
  }
  // Serial.printf("I2S driver installed.\r\n");
  stop(); 
  return true;
}

void SoundSensor::start() {
  // Serial.printf("i2s_start\r\n");
  i2s_start( I2S_PORT);
  _i2s = true;
  // Serial.printf("i2s_started\r\n");
}

void SoundSensor::stop() {
  // Serial.printf("i2s_stop\r\n");
  _i2s = false;
  i2s_stop( I2S_PORT);
}

void SoundSensor::disable() {
  i2s_driver_uninstall(  I2S_PORT);
}

float* SoundSensor::readSamples(){
  // Read multiple samples at once and calculate the sound pressure
   
  size_t num_bytes_read;
  _err = i2s_read(
    I2S_PORT,
    (char *) _samples,
    BLOCK_SIZE * 4,        // 4 bytes per sample
    &num_bytes_read,
    portMAX_DELAY
  );    // no timeout

   if(_err != ESP_OK){
    Serial.printf("%d err\r\n",_err);
  }
 
  integerToFloat(_samples, _real, _imag, SAMPLES);

  // apply HANN window, optimal for energy calculations
  _fft->Windowing(FFT_WIN_TYP_HANN, FFT_FORWARD);     // changed was FFT_WIN_TYP_FLT_TOP
 
  // do FFT processing
  _fft->Compute(FFT_FORWARD);

  // calculate energy in each bin
  calculateEnergy(_real, _imag, SAMPLES);

  // sum up energy in bin for each octave
  sumEnergy(_real, _energy);

  // Absolute SPL needs mean-square in ±1.0 FS units (REF_ENERGY in Measurement).
  // arduinoFFT forward transform is unnormalized: sum |X[k]|^2 = N * sum x[n]^2,
  // so mean-square = sum|X|^2 / N^2. Octave bins already cover the positive half
  // used by this pipeline; divide by N^2 here so quiet-room levels land near the
  // ICS-43434 expectation (~36-38 dBA).
  // Hann window mean power is 0.375 of true power → compensate by 1/0.375.
  const float FFT_ENERGY_TO_MS = 1.0f / ((float)SAMPLES * (float)SAMPLES);
  const float HANN_ENERGY_COMPENSATION = 2.6666667f;
  for (uint8_t i = 0; i < OCTAVES; i++) {
    _energy[i] *= FFT_ENERGY_TO_MS * HANN_ENERGY_COMPENSATION;
  }

  return _energy;
}

// Convert 24-bit I2S samples to float normalized to Full Scale ±1.0 (ICS-43434).
// DC removal kept for MEMS offset; absolute SPL uses FS = 2^23.
void SoundSensor::integerToFloat(int32_t * samples, float *vReal, float *vImag, uint16_t size) {
  float sum = 0.0;
  // calculate offset
   for (uint16_t i = 0; i < size; i++) {
    int32_t val = (samples[i] >> 8);            // move 24 value bits on the correct place in a long
    vReal[i] = (float)val;
    sum += vReal[i];
  }
  float offs = sum / (float)size;   //dc component
  if( _runningN < 100)
    _runningN++;
  float newDC = _runningDC + (offs - _runningDC)/_runningN;
  _runningDC = newDC;

  const float FS = 8388608.0f;  // ±2^23 LSB full scale
  for (uint16_t i = 0; i < size; i++) {
    vReal[i] = (vReal[i] - newDC) / FS;
    vImag[i] = 0.0f;
  }
}

// calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
void SoundSensor::calculateEnergy(float *vReal, float *vImag, uint16_t samples)
{
  for (uint16_t i = 0; i < samples; i++) {
    vReal[i] = sq(vReal[i]) + sq(vImag[i]);
    vImag[i] = 0.0;
  }
}

// sums up energy in whole octave bins
void SoundSensor::sumEnergy(const float *samples, float *energies) {

  // skip the first two bins
  int bin_size = 2;
  int bin = bin_size;
  for (int octave = 0; octave < OCTAVES; octave++){
    float sum = 0.0;
    for (int i = 0; i < bin_size; i++){
      sum += samples[bin++];
    }
    energies[octave] = sum;
    bin_size *= 2;
    //printf("octaaf=%d, bin=%d, sum=%f\n", octave, bin-1, sum);
  }
}

/*
// generate test sinus
static void generateSineWave( int32_t* samples, float amplitude, float freq) {
  float c = round( freq / (SAMPLE_FREQ / (float)SAMPLES)) / SAMPLES;     // put a multipe of complete sinewaves in buffer
  for ( int i = 0; i < BLOCK_SIZE; i++) {
    int32_t temp = 256 * amplitude * sin((float)i * c * twoPi );        // sine wave
    samples[i] = temp & 0xFFFFFF00;  // convert to WAV integers
  }
}
*/

#endif // ESP32
