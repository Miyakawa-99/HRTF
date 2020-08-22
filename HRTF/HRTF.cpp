#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <windows.h>
#include "wave.h"
#include <complex.h>
#include <fftw3.h>
#include <sndfile.h>

#define M_PI 3.14159265358979323846264338
#define BUFFER_COUNT 2 // マルチバッファ数
#define OVERLAP  2
#define FFTSIZE  2048
#define SAMPLE_RATE 44100 //sampling rate
#define AMPLITUDE 1.0// 16bit
#pragma warning(disable : 4996)

int AudioFileWriter(const char* filename, SF_INFO* sfinfo, float* data)
{
    SNDFILE* sfr;
    SF_INFO sfinfo_out = *sfinfo;

    //open file
    if (!(sfr = sf_open(filename, SFM_WRITE, &sfinfo_out))) {
        printf("Error : Could not open output file .\n");
        return 0;
    }

    if (!(sf_writef_float(sfr, data, (long long)(sfinfo->frames / FFTSIZE)*FFTSIZE))) {
        printf("Error : 0 data has been written.\n");
        return 0;
    }

    sf_close(sfr);
    return 1;
}

float* AudioFileLoader(const char* filename, SF_INFO* sfinfo, float* data)
{

    SNDFILE* sfr;

    //open file
    if (!(sfr = sf_open(filename, SFM_READ, sfinfo))) {
        printf("Error : Could not open output file .\n");
        return 0;
    }


    // format should be Wav or Aiff
    if (sfinfo->format == 0x010002 || sfinfo->format == 0x020002) {// 16bit

    }
    else {
        printf("Error : Could not open! file format is not WAV or AIFF\n");
        return 0;
    }

    if (sfinfo->samplerate != 44100) {
        printf("Error : Samplingrate is not 44.1kHz : %d\n", sfinfo->samplerate);
        return 0;
    }

    if (sfinfo->channels != 1) {
        printf("Error : Audio file is not MONO\n");
        return 0;
    }

    //allocate
    data = (float*)calloc(sfinfo->frames, sizeof(float));

    //read data
    sf_readf_float(sfr, data, sfinfo->frames);
    sf_close(sfr);
    return data; // success
}

int applyHRTF(MONO_PCM source, int deg, int elev)
{
    STEREO_PCM appliedSource; /* ステレオの音データ */
    SF_INFO sfinfo;
    const char* filename = "asano2.wav";
    const char* generatename = "result.wav";

    float* data = NULL;

    if (!(data = AudioFileLoader(filename, &sfinfo, data))) {
        printf("Could not open Audio file\n");
        return 0;
    }
    else std::cout << "OpenFile: " << filename << std::endl;

    long long frame_num = (long long)(sfinfo.frames / FFTSIZE);
    int		FREQ_OF_DATA = 1;

    // 入力、出力配列のメモリ確保
    fftwf_complex* src = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    fftwf_complex* dst = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    fftwf_complex* dst2 = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
    float* outdata = (float*)calloc(frame_num * FFTSIZE, sizeof(float));

    //for FFT analysis
    //scalling
    float scale = 1.0 / FFTSIZE;
    std::cout << "Scale: " << scale << std::endl;

    for (int i = 0; i < frame_num * OVERLAP - 1; i++) {

        //copy data
        for (int j = 0; j < FFTSIZE; j++) {
            src[j][0] = data[i * FFTSIZE / OVERLAP + j];
            src[j][1] = 0;
        }

        // プランの生成( 配列サイズ, 入力配列, 出力配列, 変換・逆変換フラグ, 最適化フラグ)
        fftwf_plan plan = fftwf_plan_dft_1d(FFTSIZE, src, dst, FFTW_FORWARD, FFTW_ESTIMATE);
        // フーリエ変換実行 
        fftwf_execute(plan);
        if (plan) fftwf_destroy_plan(plan);
        /*
         ここに処理を書く
       */
       // プランの生成( 配列サイズ, 入力配列, 出力配列, 変換・逆変換フラグ, 最適化フラグ)
        fftwf_plan plan2 = fftwf_plan_dft_1d(FFTSIZE, dst, dst2, FFTW_BACKWARD, FFTW_ESTIMATE);
        // フーリエ変換実行
        fftwf_execute(plan2);

        //add data
        for (int j = 0; j < FFTSIZE; j++)
            outdata[i * FFTSIZE / OVERLAP + j] += scale * dst2[j][0];
        if (plan2) fftwf_destroy_plan(plan2);
    }

    if (!AudioFileWriter("result.wav", &sfinfo, outdata)) {
        printf("Could not Write Audio file\n");
        return 0;
    }

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
        fs_dst << dst2[i][0] << "," << dst2[i][1] << std::endl;
        fs_out << outdata[i] << "," << dst2[i][1] << std::endl;
    }

    // 終了時、専用関数でメモリを開放する
    fftw_free(src);
    fftw_free(dst);
    fftw_free(dst2);
    free(data);

    int c;
    std::cin >> c;

    return 0;
}

int main(void)
{
    printf("START!!!\n");
    MONO_PCM pcm0; /* モノラルの音データ */
    STEREO_PCM pcm1; /* ステレオの音データ */
    int n;
    double a, depth, rate;

    mono_wave_read(&pcm0, (char*)"sample08.wav"); /* WAVEファイルからモノラルの音データを入力する */
    applyHRTF(pcm0, 10, 5);

    pcm1.fs = pcm0.fs; /* 標本化周波数 */
    pcm1.bits = pcm0.bits; /* 量子化精度 */
    pcm1.length = pcm0.length; /* 音データの長さ */
    pcm1.sL = (double*)calloc(pcm1.length, sizeof(double)); /* メモリの確保 */
    pcm1.sR = (double*)calloc(pcm1.length, sizeof(double)); /* メモリの確保 */

    depth = 1.0;
    rate = 0.2; /* 0.2Hz */

    /* オートパン */
    for (n = 0; n < pcm1.length; n++)
    {
        a = 1.0 + depth * sin(2.0 * M_PI * rate * n / pcm1.fs);
        pcm1.sL[n] = a * pcm0.s[n];

        a = 1.0 + depth * sin(2.0 * M_PI * rate * n / pcm1.fs + M_PI);
        pcm1.sR[n] = a * pcm0.s[n];
    }

    stereo_wave_write(&pcm1, (char*)"practice.wav"); /* WAVEファイルにステレオの音データを出力する */
    //std::cout << pcm0.fs << "," << pcm0.bits<< "," << pcm0.length << std::endl;
    free(pcm0.s); /* メモリの解放 */
    free(pcm1.sL); /* メモリの解放 */
    free(pcm1.sR); /* メモリの解放 */

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

