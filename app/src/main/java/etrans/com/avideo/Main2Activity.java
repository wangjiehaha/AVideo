package etrans.com.avideo;

import android.graphics.Bitmap;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.ImageView;

import com.cv.led.ffmpegmetadataretriever.LFFmpegMediaMetadataRetriever;

import static com.cv.led.ffmpegmetadataretriever.LFFmpegMediaMetadataRetriever.OPTION_CLOSEST_SYNC;

public class Main2Activity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main2);

        ImageView imageView = findViewById(R.id.testImage);

        LFFmpegMediaMetadataRetriever retriever = new LFFmpegMediaMetadataRetriever();
        try {
            retriever.setDataSource("/storage/emulated/0/test.mp4");
            Bitmap bitmap;
            bitmap = retriever.getScaledFrameAtTime(100, OPTION_CLOSEST_SYNC, 1920, 1080);
            imageView.setImageBitmap(bitmap);
        } catch (RuntimeException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
