/*
 *  pv.h
 *  Spectral Template
 *
 *  Created by Johannes Bochmann on 20.04.09.
 *  Copyright 2009 Grafikdesign. All rights reserved.
 *
 */

#ifndef __pv_h__
#define __pv_h__


int stft(float *input, float *window, float *output, int input_size, int fftsize, int hopsize);

int istft(float* input, float* window, float* output, int input_size, int fftsize, int hopsize);

void streamStft(float *input, float *window, float *output, int fftsize, int hopsize);

void streamIstft(float *input, float *window, float *output, int fftsize, int hopsize);

void initializeStft();

void fft_test(float *in, float *out, int N);

void fft_kiss(float *in, float *out, int N);

void ifft_kiss(float *in, float *out, int N);


#endif //__pv_h__