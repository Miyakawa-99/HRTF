#HRTF
HRTFたたみこみとスピーカー位置補正用

0829時点
指定した音源とHRTFwavファイルが畳み込まれ，スピーカー出力される

ライブラリ
・FFTW: FFT用
・portaudio: 再生用
・linsndfile: 音源読み込み用


/SoundData: 使う音源(ignore)
/HRTFfull: HRTFをたたみこむ際に必要なインパルス応答のwavData
/input: 入力用音源(WAV,Mono,16bit,44.1kHz)
/output: 畳み込みを終えた音源保存用
