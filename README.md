# HRTF
音源にHRTF適用とスピーカー位置補正用

##master
0829時点
指定したHRTFwavFileと，音源(WAV,Mono,16bit,44.1kHz)を畳み込んでストリーミング再生が可能

##PlaySoundwithStreaming
0829時点
HRTF動的な畳み込みを実現できるように実装中

###ライブラリ
・FFTW: FFTのため
・portaudio: 音再生用
・libsndfile: ファイル読み込み
