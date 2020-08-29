#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <wave.h>
#include <complex>
#include <fftw3.h>
#include <sndfile.h>
#include <portaudio.h>

#define PA_SAMPLE_TYPE paFloat32
#define OVERLAP  2
#define FFTSIZE 1024
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER FFTSIZE

#pragma warning(disable : 4996)

int applyHRTF(MONO_PCM source, int deg, int elev, PaStream* streamObj)
{
    STEREO_PCM appliedSource; // ステレオの音データ
    SF_INFO sfinfo;
    SF_INFO Lsfinfo;
    SF_INFO Rsfinfo;

    const char* filename = "../SoundData/input/asano.wav";
    const char* generatename = "../SoundData/output/result.wav";
    const char* Lhrtf = "../SoundData/HRTFfull/elev40/L40e032a.wav";
    const char* Rhrtf = "../SoundData/HRTFfull/elev40/R40e032a.wav";

    float* data = NULL;
    float* Ldata = NULL;
    float* Rdata = NULL;

    //Audio,インパルス応答の読み込み
    if (!(data = AudioFileLoader(filename, &sfinfo, data))) return 0;
    std::cout << "FORMAT: " << sfinfo.format << std::endl;

    if (!(Ldata = AudioFileLoader(Lhrtf, &Lsfinfo, Ldata))) return 0;
    else std::cout << "OpenFile: " << Lhrtf << std::endl;

    if (!(Rdata = AudioFileLoader(Rhrtf, &Rsfinfo, Rdata))) return 0;
    else std::cout << "OpenFile: " << Rhrtf << std::endl;


    long long frame_num = (long long)(sfinfo.frames / FFTSIZE);


    // 入力配列のメモリ確保
    fftwf_complex* src= (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    // 入力配列FFT後メモリ確保
    fftwf_complex* Ldst = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    // 入力配列FFT後メモリ確保
    fftwf_complex* Rdst = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);

    // LeftHRTF配列のメモリ確保
    fftwf_complex* left = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    // LeftHRTF配列FFT後メモリ確保
    fftwf_complex* FFTleft = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);

    // RightHRTF配列のメモリ確保
    fftwf_complex* right = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    // RightHRTF配列FFT後メモリ確保
    fftwf_complex* FFTright = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);

    // 最終出力配列
    fftwf_complex* Ldst2 = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    fftwf_complex* Rdst2 = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);

    float* Loutdata = (float*)calloc(frame_num * FFTSIZE, sizeof(float));
    float* Routdata = (float*)calloc(frame_num * FFTSIZE, sizeof(float));
    float* LBuffer = (float*)calloc(FFTSIZE/2, sizeof(float));
    float* RBuffer = (float*)calloc(FFTSIZE/2, sizeof(float));
    float* Buffer = (float*)calloc(FFTSIZE, sizeof(float));

    //for FFT analysis
    //scalling
    float scale = 1.0 / FFTSIZE*OVERLAP;
    std::cout << "Scale: " << scale << std::endl;

    //LeftHRTF
    for (int j = 0; j < FFTSIZE; j++) {
        if (j < FFTSIZE / 2) {
            left[j][0] = 0;
            left[j][1] = 0;
            right[j][0] = 0;
            right[j][1] = 0;
        }
        else {
            left[j][0] = Ldata[j- FFTSIZE / 2];
            left[j][1] = 0;
            right[j][0] = Rdata[j - FFTSIZE / 2];
            right[j][1] = 0;
        }
    }
    // プランの生成( 配列サイズ, 入力配列, 出力配列, 変換・逆変換フラグ, 最適化フラグ)
    fftwf_plan leftplan = fftwf_plan_dft_1d(FFTSIZE, left, FFTleft, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_plan rightplan = fftwf_plan_dft_1d(FFTSIZE, right, FFTright, FFTW_FORWARD, FFTW_ESTIMATE);

    // フーリエ変換実行 
    fftwf_execute(leftplan);
    if (leftplan) fftwf_destroy_plan(leftplan);
    fftwf_execute(rightplan);
    if (rightplan) fftwf_destroy_plan(rightplan);


    for (int i = 0; i < frame_num * OVERLAP - 1; i++) {

        //copy data
        for (int j = 0; j < FFTSIZE; j++) {
            src[j][0] = data[i * FFTSIZE / OVERLAP + j];
            src[j][1] = 0;
        }

        // プランの生成( 配列サイズ, 入力配列, 出力配列, 変換・逆変換フラグ, 最適化フラグ)
        fftwf_plan plan = fftwf_plan_dft_1d(FFTSIZE, src, Ldst, FFTW_FORWARD, FFTW_ESTIMATE);
        // フーリエ変換実行 
        fftwf_execute(plan);
        if (plan) fftwf_destroy_plan(plan);

        // ここに処理を書く 
        for (int j = 0; j < FFTSIZE; j++) {
            std::complex<float> s(Ldst[j][0], Ldst[j][1]);
            std::complex<float> l(FFTleft[j][0], FFTleft[j][1]);
            std::complex<float> sl = s*l;
            Ldst[j][0] = sl.real();
            Ldst[j][1] = sl.imag();
            // RIGHT
            std::complex<float> r(FFTright[j][0], FFTright[j][1]);
            std::complex<float> sr = s * r;
            Rdst[j][0] = sr.real();
            Rdst[j][1] = sr.imag();
        }

       // プランの生成( 配列サイズ, 入力配列, 出力配列, 変換・逆変換フラグ, 最適化フラグ)
        fftwf_plan leftplan2 = fftwf_plan_dft_1d(FFTSIZE, Ldst, Ldst2, FFTW_BACKWARD, FFTW_ESTIMATE);
        fftwf_plan rightplan2 = fftwf_plan_dft_1d(FFTSIZE, Rdst, Rdst2, FFTW_BACKWARD, FFTW_ESTIMATE);

        // フーリエ変換実行
        fftwf_execute(leftplan2);
        fftwf_execute(rightplan2);

        //add data
        for (int j = 0; j < FFTSIZE / 2; j++) {
            Loutdata[i * FFTSIZE / OVERLAP + j] = scale * Ldst2[j][0];
            Routdata[i * FFTSIZE / OVERLAP + j] = scale * Rdst2[j][0];
            LBuffer[j] = scale * Ldst2[j][0];
            RBuffer[j] = scale * Rdst2[j][0];
        }
        if (leftplan2) fftwf_destroy_plan(leftplan2);
        if (rightplan2) fftwf_destroy_plan(rightplan2);

        for (int j = 0; j < FFTSIZE; j++) {
            if (j == 0 || j % 2 == 0)Buffer[j] = LBuffer[j/2];
            else Buffer[j] = RBuffer[j / 2];
        }
        //
        PaError err;
        err = Pa_WriteStream(streamObj, Buffer, FFTSIZE/2);
        if (err != paNoError) {
            fprintf(stderr, "error writing audio_buffer %s (rc=%d)\n", Pa_GetErrorText(err), err);
        }
    }

    appliedSource.fs = SAMPLE_RATE; // 標本化周波数
    appliedSource.bits = 16; // 量子化精度
    appliedSource.length = sfinfo.frames; // 音データの長さ 
    appliedSource.sL = (double*)calloc(appliedSource.length, sizeof(double)); 
    appliedSource.sR = (double*)calloc(appliedSource.length, sizeof(double));
    for (int n = 0; n < appliedSource.length; n++)
    {
        appliedSource.sL[n] = Loutdata[n];
        appliedSource.sR[n] = Routdata[n];
    }

    stereo_wave_write(&appliedSource, (char*)generatename); // WAVEファイルにステレオの音データを出力する 
    free(appliedSource.sL); // メモリの解放 
    free(appliedSource.sR); // メモリの解放 

    // 終了時、専用関数でメモリを開放する
    fftw_free(src);
    fftw_free(left);
    fftw_free(right);
    fftw_free(FFTleft);
    fftw_free(FFTright);
    fftw_free(Ldst);
    fftw_free(Rdst);
    fftw_free(Ldst2);
    fftw_free(Rdst2);
    free(data);
    free(Ldata);
    free(Rdata);
    free(Loutdata);
    free(Routdata);

    return 0;
}


int main(void)
{
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;

    err = Pa_Initialize();
    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* デフォルトアウトプットデバイス */
    if (outputParameters.device == paNoDevice) {
        printf("Error: No default output device.\n");
        Pa_Terminate();
        return 1;
    }
    outputParameters.channelCount = 2; /* ステレオアウトプット */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER/2,
        0, /* paClipOff, */ /* we won't output out of range samples so don't bother clipping them */
        NULL,
        NULL);

    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }
    
    MONO_PCM pcm0; // モノラルの音データ
    mono_wave_read(&pcm0, (char*)"C:\\Users\\haru1\\source\\repos\\HRTF\\SoundData\\input\\asanoFloat32.wav");
    applyHRTF(pcm0, 10, 5, stream);

    printf("Hit ENTER to stop program.\n");
    getchar();
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        Pa_Terminate();
        return 1;
    }

    printf("Finished\n");
    Pa_Terminate();
    return 0;
}
