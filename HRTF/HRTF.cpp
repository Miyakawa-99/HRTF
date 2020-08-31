#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <wave.h>
#include <complex>
#include <fftw3.h>
#include <sndfile.h>
#include <portaudio.h>
#include <vector>
#include <chrono>
#include <thread>

#define M_PI  3.1415926535897932384626433
#define PA_SAMPLE_TYPE paFloat32
#define OVERLAP  2
#define FFTSIZE 1024
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER FFTSIZE/2

#pragma warning(disable : 4996)

class StereoGenerate {
public:
    const int   INTERFACE_UPDATETIME = 50;      // 50ms update for interface
    const float DISTANCEFACTOR = 1.0f;          // Units per meter.  I.e feet would = 3.28.  centimeters would = 100
    std::vector<float> ListenerPos = {0,0,0};
    float degree=0.0;
    PaError err;
    PaStream* stream;

    int LoadHRTF(int elev, int deg, fftwf_complex* Left, fftwf_complex* Right) {
        SF_INFO Lsfinfo, Rsfinfo;

        std::string Lhrtf = "../SoundData/HRTFfull/elev" + std::to_string(elev) + "/L" + std::to_string(elev) + "e" + std::to_string(deg) + "a.wav";
        std::string Rhrtf = "../SoundData/HRTFfull/elev" + std::to_string(elev) + "/L" + std::to_string(elev) + "e" + std::to_string(360 - deg) + "a.wav";

        float* Ldata = NULL;
        float* Rdata = NULL;

        if (!(Ldata = AudioFileLoader(Lhrtf.c_str(), &Lsfinfo, Ldata))) return 0;
        else std::cout << "OpenFile: " << Lhrtf << std::endl;

        if (!(Rdata = AudioFileLoader(Rhrtf.c_str(), &Rsfinfo, Rdata))) return 0;
        else std::cout << "OpenFile: " << Rhrtf << std::endl;

        // LeftHRTF配列のメモリ確保
        fftwf_complex* left = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        // RightHRTF配列のメモリ確保
        fftwf_complex* right = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        //LeftHRTF
        for (int j = 0; j < FFTSIZE; j++) {
            if (j < FFTSIZE / 2) {
                left[j][0] = 0;
                left[j][1] = 0;
                right[j][0] = 0;
                right[j][1] = 0;
            }
            else {
                left[j][0] = Ldata[j - FFTSIZE / 2];
                left[j][1] = 0;
                right[j][0] = Rdata[j - FFTSIZE / 2];
                right[j][1] = 0;
            }
        }
        // プランの生成( 配列サイズ, 入力配列, 出力配列, 変換・逆変換フラグ, 最適化フラグ)
        fftwf_plan leftplan = fftwf_plan_dft_1d(FFTSIZE, left, Left, FFTW_FORWARD, FFTW_ESTIMATE);
        fftwf_plan rightplan = fftwf_plan_dft_1d(FFTSIZE, right, Right, FFTW_FORWARD, FFTW_ESTIMATE);

        // フーリエ変換実行 
        fftwf_execute(leftplan);
        if (leftplan) fftwf_destroy_plan(leftplan);
        fftwf_execute(rightplan);
        if (rightplan) fftwf_destroy_plan(rightplan);

        fftw_free(left);
        fftw_free(right);
        return 0;
    }

    int applyHRTF(char* input, int elev, int deg)
    {
        SF_INFO sfinfo;
        float* data = NULL;

        //Audioの読み込み
        if (!(data = AudioFileLoader(input, &sfinfo, data))) return 0;
        else std::cout << "FORMAT: " << sfinfo.format << std::endl;

        long long frame_num = (long long)(sfinfo.frames / FFTSIZE);
        //scalling
        float scale = 1.0 / FFTSIZE * OVERLAP;

        // 入力配列のメモリ確保
        fftwf_complex* src = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        // 入力配列FFT後メモリ確保
        fftwf_complex* Ldst = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        // 入力配列FFT後メモリ確保
        fftwf_complex* Rdst = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        // LeftHRTF配列FFT後メモリ確保
        fftwf_complex* FFTleft = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        // LeftHRTF配列FFT後メモリ確保
        fftwf_complex* FFTright = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        // 最終出力配列
        fftwf_complex* Ldst2 = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);
        fftwf_complex* Rdst2 = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * FFTSIZE);

        float* LBuffer = (float*)calloc(FFTSIZE / 2, sizeof(float));
        float* RBuffer = (float*)calloc(FFTSIZE / 2, sizeof(float));
        float* Buffer = (float*)calloc(FFTSIZE, sizeof(float));

        //for FFT analysis
        LoadHRTF(elev, deg, FFTleft, FFTright);

        int k = 1;
        do {
            for (int i = 0; i < frame_num * OVERLAP - 1; i++) {
                UpdateListener(k,0.0,0.0);
                if(k%5==0) LoadHRTF(0, degree, FFTleft, FFTright);
                std::cout << i << "\n"; //0,1,2,3,4が出力される
                k = k ++;
                if (k == 361)k = 1;
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
                    std::complex<float> sl = s * l;
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
                    LBuffer[j] = scale * Ldst2[j][0];
                    RBuffer[j] = scale * Rdst2[j][0];
                }
                if (leftplan2) fftwf_destroy_plan(leftplan2);
                if (rightplan2) fftwf_destroy_plan(rightplan2);

                for (int j = 0; j < FFTSIZE; j++) {
                    if (j == 0 || j % 2 == 0)Buffer[j] = LBuffer[j / 2];
                    else Buffer[j] = RBuffer[j / 2];
                }
                //
                PaError err;
                err = Pa_WriteStream(stream, Buffer, FRAMES_PER_BUFFER);
                if (err != paNoError) {
                    fprintf(stderr, "error writing audio_buffer %s (rc=%d)\n", Pa_GetErrorText(err), err);
                }
            }
        } while (k < 300);
        // 終了時、専用関数でメモリを開放する
        fftw_free(src);
        fftw_free(FFTleft);
        fftw_free(FFTright);
        fftw_free(Ldst);
        fftw_free(Rdst);
        fftw_free(Ldst2);
        fftw_free(Rdst2);
        free(data);

        return 0;
    }

    int SoundPlayInitialize() {
        PaStreamParameters outputParameters;
      
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
            FRAMES_PER_BUFFER,
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
        return 0;
    }

    int AudioInitialize() {
        ListenerPos[0] = 1.0;
        ListenerPos[1] = 0.0;
        ListenerPos[2] = 0.0;

        return 0;
    }

    int CloseStream() {
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

    void UpdateListener(float x, float y, float z){
        ListenerPos[0] = x;
        ListenerPos[1] = y;
        ListenerPos[2] = z;
        degree = x;
    }
};

int main(void)
{
    StereoGenerate sound;
    sound.SoundPlayInitialize();
    sound.AudioInitialize();
    char filename[] = "../SoundData/input/asano.wav";
    sound.applyHRTF(filename, 0, 40);

    printf("Hit ENTER to stop program.\n");
    getchar();
    sound.CloseStream();
}
