#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <windows.h>
#include "wave.h"
#include <complex>
#include <fftw3.h>
#include <sndfile.h>

#define M_PI 3.14159265358979323846264338
#define BUFFER_COUNT 2 // マルチバッファ数
#define OVERLAP  2
#define FFTSIZE 1024
#define SAMPLE_RATE 44100 
#define AMPLITUDE 1.0// 16bit
#pragma warning(disable : 4996)

/*int applyHRTF(MONO_PCM source, int deg, int elev)
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
    else std::cout << "OpenFile: " << filename << std::endl;

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
        for (int j = 0; j < FFTSIZE / 2; j++) {//? 
            Loutdata[i * FFTSIZE / OVERLAP + j] = scale * Ldst2[j][0];//scale...?
            Routdata[i * FFTSIZE / OVERLAP + j] = scale * Rdst2[j][0];//scale...?
        }
        if (leftplan2) fftwf_destroy_plan(leftplan2);
        if (rightplan2) fftwf_destroy_plan(rightplan2);

    }

    appliedSource.fs = SAMPLE_RATE; // 標本化周波数
    appliedSource.bits = 16; // 量子化精度
    appliedSource.length = sfinfo.frames; // 音データの長さ 
    appliedSource.sL = (double*)calloc(appliedSource.length, sizeof(double));  // メモリの解放 
    appliedSource.sR = (double*)calloc(appliedSource.length, sizeof(double));  // メモリの解放 
    for (int n = 0; n < appliedSource.length; n++)
    {
        appliedSource.sL[n] = Loutdata[n];
        appliedSource.sR[n] = Routdata[n];
    }
    //ここで音声出力bufferに流せばよい？
    stereo_wave_write(&appliedSource, (char*)generatename); // WAVEファイルにステレオの音データを出力する 
    free(appliedSource.sL); // メモリの解放 
    free(appliedSource.sR); // メモリの解放 


    // 入出力配列の値をcsvファイルに保存
    std::fstream fs_src("src.csv", std::ios::out);
    std::fstream fs_dst("dst.csv", std::ios::out);
    std::fstream fs_out("output.csv", std::ios::out);

    fs_src << "実部,虚部" << std::endl;
    fs_dst << "実部,虚部" << std::endl;
    fs_out << "実部,虚部" << std::endl;

    for (int i = 0; i < FFTSIZE; i++)
    {
        fs_src << src[i][0] << "," << src[i][1] << std::endl;
        fs_dst << FFTleft[i][0] << "," << FFTleft[i][1] << std::endl;
        fs_out << Loutdata[i] << "," << Ldst2[i][1] << std::endl;
    }

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
}*/

/*int main(void)
{
    printf("START!!!\n");
    MONO_PCM pcm0; // モノラルの音データ

    mono_wave_read(&pcm0, (char*)"sample08.wav");
    applyHRTF(pcm0, 10, 5);

    free(pcm0.s); // メモリの解放

    return 0;
}*/



#include<stdio.h>
#include<math.h>
#include"portaudio.h"
#define Fs 44100 //サンプリング周波数
#define FRAMES_PER_BUFFER 128 //バッファサイズ
#define pi 3.14159265358979323

/*ユーザ定義データ*/
typedef struct {
    float freq; //正弦波の周波数
    float index;
}padata;

/* オーディオ処理コールバック関数*/
static int dsp(const void* inputBuffer, //入力
    void* outputBuffer, //出力
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData //ユーザ定義データ 
) {
    padata* data = (padata*)userData;
    float* out = (float*)outputBuffer;
    long i;

    for (i = 0; i < framesPerBuffer; i++) {
        *out++ = 0.7 * sin(2 * pi * data->freq * data->index / Fs); //チャンネル1（左）
        *out++ = 0.7 * sin(2 * pi * data->freq * data->index / Fs); //チャンネル2（右）
        data->index += 1.f;
    }
    return 0;
}
int main(void) {
    PaStreamParameters outParam; //出力の定義
    PaStream* stream;
    PaError err;
    padata data; //ユーザ定義データ
    data.freq = 800.f;
    data.index = 0.f;

    //PortAudio初期化
    Pa_Initialize();

    //出力の設定
    outParam.device = Pa_GetDefaultOutputDevice(); //デフォルトのオーディオデバイス
    outParam.channelCount = 2;
    outParam.sampleFormat = paFloat32; //32bit floatで処理
    outParam.suggestedLatency = Pa_GetDeviceInfo(outParam.device)->defaultLowOutputLatency;
    outParam.hostApiSpecificStreamInfo = NULL;

    //PortAudioオープン
    Pa_OpenStream(
        &stream,
        NULL,
        &outParam,
        Fs,
        FRAMES_PER_BUFFER,
        paClipOff,
        dsp,
        &data);

    //PortAudioスタート
    Pa_StartStream(stream);

    //エンターキーが押されるまで待機
    getchar();

    //PortAudio終了
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}