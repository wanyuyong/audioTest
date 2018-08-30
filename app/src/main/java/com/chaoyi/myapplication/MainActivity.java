package com.chaoyi.myapplication;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    String input = new File(Environment.getExternalStorageDirectory(), "input.mp3").getAbsolutePath();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final AudioTrackPlay audioTrackPlay = new AudioTrackPlay();
        findViewById(R.id.btn_audio_track_play).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.v("magic", "file patch : " + input);
                File file = new File(input);
                if (file.exists()) {
                    Log.v("magic", "file is exists : " + file.getAbsolutePath());
                }
                audioTrackPlay.playSound(input);
            }
        });


        final OpenSLPlay openSLPlay = new OpenSLPlay();
        findViewById(R.id.btn_opensl_play).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.v("magic", "file patch : " + input);
                File file = new File(input);
                if (file.exists()) {
                    Log.v("magic", "file is exists : " + file.getAbsolutePath());
                }
                openSLPlay.playSound(input);
            }
        });


    }

}
