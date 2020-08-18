#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wave.h"
#define M_PI 3.141592
#pragma warning(disable : 4996)

int main(void)
{
    MONO_PCM pcm0; /* モノラルの音データ */
    STEREO_PCM pcm1; /* ステレオの音データ */
    int n;
    double a, depth, rate;

    mono_wave_read(&pcm0, (char*)"asano.wav"); /* WAVEファイルからモノラルの音データを入力する */

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

    free(pcm0.s); /* メモリの解放 */
    free(pcm1.sL); /* メモリの解放 */
    free(pcm1.sR); /* メモリの解放 */

    return 0;
}
