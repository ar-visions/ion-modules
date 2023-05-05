#include <core/core.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fft/gsl_fft_real.h>
#include <opus/opus.h>

///
#define FRAME_SIZE           960
#define SAMPLE_RATE          48000
#define CHANNELS             2

///
int main(int argc, cstr argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.opus\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;
    OpusDecoder *decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
    if (err < 0) {
        fprintf(stderr, "Failed to create decoder: %s\n", opus_strerror(err));
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    int16_t pcm[FRAME_SIZE * CHANNELS];
    float fft_in[FRAME_SIZE];
    double fft_out[FRAME_SIZE / 2 + 1];

    gsl_fft_real_wavetable *real = gsl_fft_real_wavetable_alloc(FRAME_SIZE);
    gsl_fft_real_workspace *work = gsl_fft_real_workspace_alloc(FRAME_SIZE);

    while (1) {
        unsigned char packet[1275];
        int packet_size = fread(packet, 1, sizeof(packet), fp);
        if (packet_size == 0) {
            break;
        }

        int frames_decoded = opus_decode(decoder, packet, packet_size, pcm, FRAME_SIZE, 0);
        if (frames_decoded < 0) {
            fprintf(stderr, "Failed to decode frame: %s\n", opus_strerror(frames_decoded));
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FRAME_SIZE; i++) {
            fft_in[i] = pcm[i * CHANNELS] / 32768.0;
        }

        gsl_fft_real_transform(fft_in, 1, FRAME_SIZE, real, work);
        for (int i = 0; i < FRAME_SIZE / 2 + 1; i++) {
            fft_out[i] = sqrt(fft_in[i * 2] * fft_in[i * 2] + fft_in[i * 2 + 1] * fft_in[i * 2 + 1]);
        }

        // Do something with the FFT output...
    }

    fclose(fp);
    opus_decoder_destroy(decoder);
    gsl_fft_real_wavetable_free(real);
    gsl_fft_real_workspace_free(work);

    return 0;
}