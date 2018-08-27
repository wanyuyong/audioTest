package com.chaoyi.myapplication;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    String input = new File(Environment.getExternalStorageDirectory(), "input.mp3").getAbsolutePath();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final MusicPlay musicPlay = new MusicPlay();
        findViewById(R.id.btn_play).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.v("magic", "file patch : " + input);
                File file = new File(input);
                if (file.exists()) {
                    Log.v("magic", "file is exists : " + file.getAbsolutePath());
                }
                musicPlay.playSound(input);
            }
        });
    }
}
