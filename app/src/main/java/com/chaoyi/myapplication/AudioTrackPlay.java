package com.chaoyi.myapplication;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;


public class AudioTrackPlay {
    static {
        System.loadLibrary("audiotrackplay");
    }

    public native void playSound(String input);

    private AudioTrack audioTrack;

    /**
     * 此方法，是C进行调用
     * @param sampleRateInHz 采样率
     * @param nb_channals  声道数
     */
    public void createTrack(int sampleRateInHz, int nb_channals) {
        int channaleConfig;
        if (nb_channals == 1) {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else if (nb_channals == 2) {
            channaleConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        }
        int buffersize = AudioTrack.getMinBufferSize(sampleRateInHz,
                channaleConfig, AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channaleConfig,
                AudioFormat.ENCODING_PCM_16BIT, buffersize, AudioTrack.MODE_STREAM);
        audioTrack.play();
    }

    //C传入音频数据
    public void playTrack(byte[] buffer, int lenth) {
        if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(buffer, 0, lenth);
        }
    }


}
