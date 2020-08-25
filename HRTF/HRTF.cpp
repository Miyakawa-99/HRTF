﻿#include <stdio.h>
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

int applyHRTF(MONO_PCM source, int deg, int elev)
{
    STEREO_PCM appliedSource; /* ステレオの音データ */
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

        /* ここに処理を書く */
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

    appliedSource.fs = SAMPLE_RATE; /* 標本化周波数 */
    appliedSource.bits = 16; /* 量子化精度 */
    appliedSource.length = sfinfo.frames; /* 音データの長さ */
    appliedSource.sL = (double*)calloc(appliedSource.length, sizeof(double)); /* メモリの確保 */
    appliedSource.sR = (double*)calloc(appliedSource.length, sizeof(double)); /* メモリの確保 */
    for (int n = 0; n < appliedSource.length; n++)
    {
        appliedSource.sL[n] = Loutdata[n];
        appliedSource.sR[n] = Routdata[n];
    }
    //ここで音声出力bufferに流せばよい？
    stereo_wave_write(&appliedSource, (char*)generatename); /* WAVEファイルにステレオの音データを出力する */
    free(appliedSource.sL); /* メモリの解放 */
    free(appliedSource.sR); /* メモリの解放 */


    /*// 入出力配列の値をcsvファイルに保存
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
    }*/

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
    printf("START!!!\n");
    MONO_PCM pcm0; /* モノラルの音データ */

    mono_wave_read(&pcm0, (char*)"sample08.wav"); /* WAVEファイルからモノラルの音データを入力する */
    applyHRTF(pcm0, 10, 5);

    free(pcm0.s); /* メモリの解放 */

    return 0;
}

/*#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "wavlib.h"
#define BUFFER_COUNT 2 // マルチバッファ数

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT Message,
    WPARAM wParam, LPARAM lParam);

static const char* lpClassName = "My Class";
static const char* lpClassTitle = "First WindowsApplication";
static const char* lpMenuName = "My Menu";

wav_head bufhead;
static char InFileN[] = { "C:\\Users\\haru1\\source\\repos\\HRTF\\x64\\Debug\\fot.bak.441.wav" };
int bufferSize;    // waveバッファサイズ(byte)
int bufferLength;  // waveバッファ長(データ数/s)
int bufferSelect;  // バッファ選択番号
static short* (wWave[BUFFER_COUNT]); // waveバッファ(マルチ)

static HWAVEOUT hWave;
static WAVEHDR whdr[BUFFER_COUNT];
int waveDataSize;  // 再生予定のWAVEデータサイズ
int readDataSize;  // 再生済のWAVEデータサイズ
int initWave(HWND hWnd);
void closeWave(void);
LRESULT wmWomDone(void);




/* -----------
   Main Program
    ------------ */
    /*int main(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpszCmdLine, INT nCmdShow)
    {
        HWND hWndMain;
        MSG msg;
        WNDCLASSEX   wndclass;

        wndclass.cbSize = sizeof(WNDCLASSEX);
        wndclass.hInstance = hInstance;
        wndclass.lpszClassName = lpClassName;
        wndclass.lpfnWndProc = MainWndProc;
        wndclass.style = 0;
        wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.lpszMenuName = NULL;
        wndclass.cbClsExtra = wndclass.cbWndExtra = 0;
        wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        if (!RegisterClassEx(&wndclass)) {
            MessageBox(0, "エラー：ウィンドウ未登録", "MyApp", MB_OK);
            return FALSE;
        }
        if (!(hWndMain =
            CreateWindow(lpClassName, lpClassTitle, WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                HWND_DESKTOP, NULL, hInstance, NULL)))
            return FALSE;

        ShowWindow(hWndMain, nCmdShow);
        UpdateWindow(hWndMain);
        if (initWave(hWndMain) == FALSE)return FALSE; // wave初期化
        while (GetMessage(&msg, NULL, 0, 0)) {
            DispatchMessage(&msg);
        }
        closeWave(); // 後処理
        return msg.wParam;
    }
    /* ------------------
        Window Procedure
       ------------------*/
       /*LRESULT CALLBACK MainWndProc(HWND hwnd, UINT Message,
           WPARAM wParam, LPARAM lParam)
       {
           switch (Message) {

           case MM_WOM_DONE:// 1つのバッファのデータ再生終了時に呼び出される
               return wmWomDone();
           case WM_DESTROY:
               PostQuitMessage(0);
               break;
           default:
               return DefWindowProc(hwnd, Message, wParam, lParam);
           }
           return FALSE;
       }
       /* ----------------------------------------
        Initialization of Wave relevant procedure
        ------------------------------------------ */
        /*int initWave(HWND hWnd)
        {
            int i;
            WAVEFORMATEX wfe;

            wavread(&bufhead, InFileN);
            bufferLength =
                bufhead.fs * bufhead.Nch;      // fs * channelデータ数/s
            bufferSize =
                bufferLength * sizeof(short);  // バッファサイズ〔byte〕
            for (i = 0; i < BUFFER_COUNT; i++) {
                wWave[i] =
                    (short*)malloc(bufferSize); // wWaveに領域割り当て
            }

            // 出力デバイスの設定
            wfe.wBitsPerSample = bits;  // bits is in wavlib.h
            wfe.nChannels = bufhead.Nch;
            wfe.wFormatTag = WAVE_FORMAT_PCM;
            wfe.nSamplesPerSec =
                (int)bufhead.fs;          // fs　サンプリング周波数〔Hz〕
            wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample / 8;
            wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;
            wfe.cbSize = waveDataSize = bufhead.Nbyte;

            waveOutOpen(&hWave, WAVE_MAPPER, &wfe,   // 出力デバイスを開く
                (DWORD)hWnd, 0, CALLBACK_WINDOW);

            for (i = 0; i < BUFFER_COUNT; i++) {
                // 再生バッファにデータを送るための設定
                whdr[i].lpData = (LPSTR)wWave[i];
                whdr[i].dwBufferLength = 0;
                whdr[i].dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
                whdr[i].dwLoops = 1;

                waveOutPrepareHeader(hWave, &(whdr[i]), sizeof(WAVEHDR));
            }

            readDataSize = 0;
            // ダブルバッファリングをするために空データを書き込み
            waveOutWrite(hWave, &(whdr[0]), sizeof(WAVEHDR));
            waveOutWrite(hWave, &(whdr[1]), sizeof(WAVEHDR));

            // バッファ番号の設定
            bufferSelect = 2 % BUFFER_COUNT;
            return TRUE;
        }
        /* ------------
           Waveの後処理
           ------------*/
           /*void closeWave(void)
           {
               int i;
               waveOutReset((HWAVEOUT)hWave);
               for (i = 0; i < BUFFER_COUNT; i++) {
                   waveOutUnprepareHeader((HWAVEOUT)hWave, &(whdr[i]),
                       sizeof(WAVEHDR));
                   free(wWave[i]);
               }
               free(bufhead.data);
           }
           /* -----------------------
           バッファの終了時に呼ばれる
             ---------------------- */
             /*LRESULT wmWomDone(void)
             {
                 int readsize, i, idlast;
                 // 最後まで再生した場合は最初にメモリのポインタを戻す
                 if (waveDataSize - readDataSize == 0) {
                     readDataSize = 0;
                     // MM_WOM_DONEをおこすためにwaveOutWriteを呼ぶ
                     whdr[bufferSelect].dwBufferLength = 0;
                     waveOutWrite((HWAVEOUT)hWave, &(whdr[bufferSelect]),
                         sizeof(WAVEHDR));

                     bufferSelect =
                         (bufferSelect + 1) % BUFFER_COUNT; // バッファのチャネル選択
                     return 0;
                 }

                 readsize = bufferSize;  // 読み込みサイズ
                 if (readsize > waveDataSize - readDataSize) {
                     readsize = waveDataSize - readDataSize;
                     idlast = 1;
                 }
                 else {
                     idlast = 0;
                 }
                 /* -------------------------------------------------------
                  読み込み bufhead.data に書き込むか，wWave[bufferSelect]に
                  直接データを書き込む
                   ------------------------------------------------------- */
                   /*for (i = 0; i < readsize / 2; i++) {
                       wWave[bufferSelect][i] = bufhead.data[readDataSize / 2 + i];
                   }
                   if (idlast == 1) readDataSize = waveDataSize;
                   else          readDataSize += readsize;
                   // 再生バッファに書き込み
                   whdr[bufferSelect].dwBufferLength = readsize;
                   waveOutWrite((HWAVEOUT)hWave, &(whdr[bufferSelect]),
                       sizeof(WAVEHDR));

                   bufferSelect =
                       (bufferSelect + 1) % BUFFER_COUNT;   // バッファ番号の更新
                   return 0;
               }*/

