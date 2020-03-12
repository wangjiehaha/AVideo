package com.cv.led.ffmpegmetadataretriever;


import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;

import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.TimeZone;

public class LFFmpegMediaMetadataRetriever {

    static {
        System.loadLibrary("ffmpeg-lib");
        nativeInit();
    }

    private long mNativeContext = -1L;

    public LFFmpegMediaMetadataRetriever() {
        nativeSetup();
    }

    public void setDataSource(String uri, Map<String, String> headers) {
        int i = 0;
        String[] keys = new String[headers.size()];
        String[] values = new String[headers.size()];
        for (Map.Entry<String, String> entry : headers.entrySet()) {
            keys[i] = entry.getKey();
            values[i] = entry.getKey();
            ++i;
        }
        _setDataSource(uri, keys, values);
    }

    public void setDataSource(FileDescriptor fd) throws IllegalArgumentException {
        __setDataSource(fd, 0, 0x7ffffffffffffffL);
    }

    public void setDataSource(Context context, Uri uri) throws IllegalArgumentException, SecurityException{
        if (uri == null) {
            throw new IllegalArgumentException();
        }
        String scheme = uri.getScheme();
        if (scheme == null || scheme.equals("file")) {
            setDataSource(uri.getPath());
            return;
        }
        AssetFileDescriptor fd = null;
        try {
            ContentResolver resolver = context.getContentResolver();
            try {
                fd = resolver.openAssetFileDescriptor(uri, "r");
            } catch (FileNotFoundException e) {
                throw new IllegalArgumentException();
            }
            if (fd == null) {
                throw new IllegalArgumentException();
            }
            FileDescriptor descriptor = fd.getFileDescriptor();
            if (!descriptor.valid()) {
                throw new IllegalArgumentException();
            }
            if (fd.getDeclaredLength() < 0) {
                __setDataSource(descriptor, fd.getStartOffset(), fd.getDeclaredLength());
            }
            return;
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } finally {
            try {
                if (fd != null) {
                    fd.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        setDataSource(uri.toString());
    }

    private native void _setDataSource(String uri, String[] keys, String[] values) throws IllegalArgumentException;

    public native void __setDataSource(FileDescriptor fd, long offset, long length) throws IllegalArgumentException;

    public native void setDataSource(String path) throws IllegalArgumentException;

    private native void nativeSetup();

    public native String extractMetadata(String key);

    public native String extractMetadataFromChapter(String key, int chapter);

    private native HashMap<String, String> nativeGetMetadata(boolean update_only, boolean apply_filter
            , HashMap<String, String> reply);

    public Metadata getMetadata() {
        boolean update_only = false;
        boolean apply_filter = false;

        Metadata data = new Metadata();
        HashMap<String, String> metadata = null;
        if ((metadata = nativeGetMetadata(update_only, apply_filter, metadata)) == null) {
            return null;
        }

        // Metadata takes over the parcel, don't recycle it unless
        // there is an error.
        if (!data.parse(metadata)) {
            return null;
        }
        return data;
    }

    public ImageInputStream getFrameAtTime(long timeUs, int option) throws IOException{
        if (option < OPTION_PREVIOUS_SYNC || option > OPTION_CLOSEST) {
            throw new IllegalArgumentException("Unsupported option: " + option);
        }
        return _getFrameAtTimeAndCopyToStream(timeUs, option);
    }

    public ImageInputStream getFirstPictureAndCopyToStream() throws IOException {

        return getEmbeddedPictureAndCopyToStream();
    }

    private native byte[] _getFrameAtTime(long timeUs, int option);

    private native ImageInputStream _getFrameAtTimeAndCopyToStream(long timeUs, int option);

    public native ImageInputStream getEmbeddedPictureAndCopyToStream();

    public Bitmap getFrameAtTime(long timeUs) {
        Bitmap b = null;

        BitmapFactory.Options bitmapOptionsCache = new BitmapFactory.Options();
        //bitmapOptionsCache.inPreferredConfig = getInPreferredConfig();
        bitmapOptionsCache.inDither = false;

        byte[] picture = _getFrameAtTime(timeUs, OPTION_CLOSEST_SYNC);

        if (picture != null) {
            b = BitmapFactory.decodeByteArray(picture, 0, picture.length, bitmapOptionsCache);
        }

        return b;
    }

    public Bitmap getScaledFrameAtTime(long timeUs, int option, int width, int height) {
        if (option < OPTION_PREVIOUS_SYNC ||
                option > OPTION_CLOSEST) {
            throw new IllegalArgumentException("Unsupported option: " + option);
        }

        Bitmap b = null;

        BitmapFactory.Options bitmapOptionsCache = new BitmapFactory.Options();
        //bitmapOptionsCache.inPreferredConfig = getInPreferredConfig();
        bitmapOptionsCache.inDither = false;

        byte[] picture = _getScaledFrameAtTime(timeUs, option, width, height);

        if (picture != null) {
            b = BitmapFactory.decodeByteArray(picture, 0, picture.length, bitmapOptionsCache);
        }

        return b;
    }

    private native byte[] _getScaledFrameAtTime(long timeUs, int option, int width, int height);

    public native byte[] getEmbeddedPicture();

    public native void release();

    private static native void nativeInit();

    private native void nativeFinalize();

    public native void nativeCancelRequest();

    public void cancel() {
        nativeCancelRequest();
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            nativeFinalize();
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            super.finalize();
        }
    }

    public native void setSurface(Object surface);

    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a sync (or key) frame associated with a data source that is located
     * right before or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    public static final int OPTION_PREVIOUS_SYNC = 0x00;
    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a sync (or key) frame associated with a data source that is located
     * right after or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    public static final int OPTION_NEXT_SYNC = 0x01;
    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a sync (or key) frame associated with a data source that is located
     * closest to (in time) or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    public static final int OPTION_CLOSEST_SYNC = 0x02;
    /**
     * This option is used with {@link #getFrameAtTime(long, int)} to retrieve
     * a frame (not necessarily a key frame) associated with a data source that
     * is located closest to or at the given time.
     *
     * @see #getFrameAtTime(long, int)
     */
    public static final int OPTION_CLOSEST = 0x03;

    /**
     * The metadata key to retrieve the name of the set this work belongs to.
     */
    public static final String METADATA_KEY_ALBUM = "album";
    /**
     * The metadata key to retrieve the main creator of the set/album, if different
     * from artist. e.g. "Various Artists" for compilation albums.
     */
    public static final String METADATA_KEY_ALBUM_ARTIST = "album_artist";
    /**
     * The metadata key to retrieve the main creator of the work.
     */
    public static final String METADATA_KEY_ARTIST = "artist";
    /**
     * The metadata key to retrieve the any additional description of the file.
     */
    public static final String METADATA_KEY_COMMENT = "comment";
    /**
     * The metadata key to retrieve the who composed the work, if different from artist.
     */
    public static final String METADATA_KEY_COMPOSER = "composer";
    /**
     * The metadata key to retrieve the name of copyright holder.
     */
    public static final String METADATA_KEY_COPYRIGHT = "copyright";
    /**
     * The metadata key to retrieve the date when the file was created, preferably in ISO 8601.
     */
    public static final String METADATA_KEY_CREATION_TIME = "creation_time";
    /**
     * The metadata key to retrieve the date when the work was created, preferably in ISO 8601.
     */
    public static final String METADATA_KEY_DATE = "date";
    /**
     * The metadata key to retrieve the number of a subset, e.g. disc in a multi-disc collection.
     */
    public static final String METADATA_KEY_DISC = "disc";
    /**
     * The metadata key to retrieve the name/settings of the software/hardware that produced the file.
     */
    public static final String METADATA_KEY_ENCODER = "encoder";
    /**
     * The metadata key to retrieve the person/group who created the file.
     */
    public static final String METADATA_KEY_ENCODED_BY = "encoded_by";
    /**
     * The metadata key to retrieve the original name of the file.
     */
    public static final String METADATA_KEY_FILENAME = "filename";
    /**
     * The metadata key to retrieve the genre of the work.
     */
    public static final String METADATA_KEY_GENRE = "genre";
    /**
     * The metadata key to retrieve the main language in which the work is performed, preferably
     * in ISO 639-2 format. Multiple languages can be specified by separating them with commas.
     */
    public static final String METADATA_KEY_LANGUAGE = "language";
    /**
     * The metadata key to retrieve the artist who performed the work, if different from artist.
     * E.g for "Also sprach Zarathustra", artist would be "Richard Strauss" and performer "London
     * Philharmonic Orchestra".
     */
    public static final String METADATA_KEY_PERFORMER = "performer";
    /**
     * The metadata key to retrieve the name of the label/publisher.
     */
    public static final String METADATA_KEY_PUBLISHER = "publisher";
    /**
     * The metadata key to retrieve the name of the service in broadcasting (channel name).
     */
    public static final String METADATA_KEY_SERVICE_NAME = "service_name";
    /**
     * The metadata key to retrieve the name of the service provider in broadcasting.
     */
    public static final String METADATA_KEY_SERVICE_PROVIDER = "service_provider";
    /**
     * The metadata key to retrieve the name of the work.
     */
    public static final String METADATA_KEY_TITLE = "title";
    /**
     * The metadata key to retrieve the number of this work in the set, can be in form current/total.
     */
    public static final String METADATA_KEY_TRACK = "track";
    /**
     * The metadata key to retrieve the total bitrate of the bitrate variant that the current stream
     * is part of.
     */
    public static final String METADATA_KEY_VARIANT_BITRATE = "bitrate";
    /**
     * The metadata key to retrieve the duration of the work in milliseconds.
     */
    public static final String METADATA_KEY_DURATION = "duration";
    /**
     * The metadata key to retrieve the audio codec of the work.
     */
    public static final String METADATA_KEY_AUDIO_CODEC = "audio_codec";
    /**
     * The metadata key to retrieve the video codec of the work.
     */
    public static final String METADATA_KEY_VIDEO_CODEC = "video_codec";
    /**
     * This key retrieves the video rotation angle in degrees, if available.
     * The video rotation angle may be 0, 90, 180, or 270 degrees.
     */
    public static final String METADATA_KEY_VIDEO_ROTATION = "rotate";
    /**
     * The metadata key to retrieve the main creator of the work.
     */
    public static final String METADATA_KEY_ICY_METADATA = "icy_metadata";

    /**
     * This metadata key retrieves the average framerate (in frames/sec), if available.
     */
    public static final String METADATA_KEY_FRAMERATE = "framerate";
    /**
     * The metadata key to retrieve the chapter start time in milliseconds.
     */
    public static final String METADATA_KEY_CHAPTER_START_TIME = "chapter_start_time";
    /**
     * The metadata key to retrieve the chapter end time in milliseconds.
     */
    public static final String METADATA_KEY_CHAPTER_END_TIME = "chapter_end_time";
    /**
     * The metadata key to retrieve the chapter count.
     */
    public static final String METADATA_CHAPTER_COUNT = "chapter_count";
    /**
     * The metadata key to retrieve the file size in bytes.
     */
    public static final String METADATA_KEY_FILESIZE = "filesize";
    /**
     * The metadata key to retrieve the video width.
     */
    public static final String METADATA_KEY_VIDEO_WIDTH = "video_width";
    /**
     * The metadata key to retrieve the video height.
     */
    public static final String METADATA_KEY_VIDEO_HEIGHT = "video_height";
    /**
     * Thumbnailurl
     */
    public static final String METADATA_KEY_VIDEO_THUMB_URL = "Thumbnailurl";


    public class Metadata {
        // The metadata are keyed using integers rather than more heavy
        // weight strings. We considered using Bundle to ship the metadata
        // between the native layer and the java layer but dropped that
        // option since keeping in sync a native implementation of Bundle
        // and the java one would be too burdensome. Besides Bundle uses
        // String for its keys.
        // The key range [0 8192) is reserved for the system.
        //
        // We manually serialize the data in Parcels. For large memory
        // blob (bitmaps, raw pictures) we use MemoryFile which allow the
        // client to make the data purge-able once it is done with it.
        //

        /**
         * {@hide}
         */
        public static final int STRING_VAL = 1;
        /**
         * {@hide}
         */
        public static final int INTEGER_VAL = 2;
        /**
         * {@hide}
         */
        public static final int BOOLEAN_VAL = 3;
        /**
         * {@hide}
         */
        public static final int LONG_VAL = 4;
        /**
         * {@hide}
         */
        public static final int DOUBLE_VAL = 5;
        /**
         * {@hide}
         */
        public static final int DATE_VAL = 6;
        /**
         * {@hide}
         */
        public static final int BYTE_ARRAY_VAL = 7;
        // FIXME: misses a type for shared heap is missing (MemoryFile).
        // FIXME: misses a type for bitmaps.
        //private static final int LAST_TYPE = 7;

        //private static final String TAG = "media.Metadata";

        // After a successful parsing, set the parcel with the serialized metadata.
        //private Parcel mParcel;
        private HashMap<String, String> mParcel;

        /**
         * Check a parcel containing metadata is well formed. The header
         * is checked as well as the individual records format. However, the
         * data inside the record is not checked because we do lazy access
         * (we check/unmarshall only data the user asks for.)
         * <p>
         * Format of a metadata parcel:
         * <pre>
         * 1                   2                   3
         * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |                     metadata total size                       |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |     'M'       |     'E'       |     'T'       |     'A'       |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * |                                                               |
         * |                .... metadata records ....                     |
         * |                                                               |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * </pre>
         *
         * @param metadata With the serialized data. Metadata keeps a
         *               reference on it to access it later on. The caller
         *               should not modify the parcel after this call (and
         *               not call recycle on it.)
         * @return false if an error occurred.
         * {@hide}
         */
        public boolean parse(HashMap<String, String> metadata) {
            if (metadata == null) {
                return false;
            } else {
                mParcel = metadata;
                return true;
            }
        }

        /**
         * @return true if a value is present for the given key.
         */
        public boolean has(final String metadataId) {
            if (!checkMetadataId(metadataId)) {
                throw new IllegalArgumentException("Invalid key: " + metadataId);
            }
            return mParcel.containsKey(metadataId);
        }

        // Accessors.
        // Caller must make sure the key is present using the {@code has}
        // method otherwise a RuntimeException will occur.

        /**
         * @return a map containing all of the available metadata.
         */
        public HashMap<String, String> getAll() {
            return mParcel;
        }

        /**
         * {@hide}
         */
        public String getString(final String key) {
            checkType(key, STRING_VAL);
            return String.valueOf(mParcel.get(key));
        }

        /**
         * {@hide}
         */
        public int getInt(final String key) {
            checkType(key, INTEGER_VAL);
            return Integer.valueOf(mParcel.get(key));
        }

        /**
         * Get the boolean value indicated by key
         */
        public boolean getBoolean(final String key) {
            checkType(key, BOOLEAN_VAL);
            return Integer.valueOf(mParcel.get(key)) == 1;
        }

        /**
         * {@hide}
         */
        public long getLong(final String key) {
            checkType(key, LONG_VAL);
            return Long.valueOf(mParcel.get(key));
        }

        /**
         * {@hide}
         */
        public double getDouble(final String key) {
            checkType(key, DOUBLE_VAL);
            return Double.valueOf(mParcel.get(key));
        }

        /**
         * {@hide}
         */
        public byte[] getByteArray(final String key) {
            checkType(key, BYTE_ARRAY_VAL);
            return mParcel.get(key).getBytes();
        }

        /**
         * {@hide}
         */
        public Date getDate(final String key) {
            checkType(key, DATE_VAL);
            final long timeSinceEpoch = Long.valueOf(mParcel.get(key));
            final String timeZone = mParcel.get(key);

            if (timeZone.length() == 0) {
                return new Date(timeSinceEpoch);
            } else {
                TimeZone tz = TimeZone.getTimeZone(timeZone);
                Calendar cal = Calendar.getInstance(tz);

                cal.setTimeInMillis(timeSinceEpoch);
                return cal.getTime();
            }
        }

        /**
         * Check val is either a system id or a custom one.
         *
         * @param val Metadata key to test.
         * @return true if it is in a valid range.
         **/
        private boolean checkMetadataId(final String val) {
        /*if (val <= ANY || (LAST_SYSTEM < val && val < FIRST_CUSTOM)) {
            Log.e(TAG, "Invalid metadata ID " + val);
            return false;
        }*/
            return true;
        }

        /**
         * Check the type of the data match what is expected.
         */
        private void checkType(final String key, final int expectedType) {
            String type = mParcel.get(key);

            if (type == null) {
                //if (type != expectedType) {
                throw new IllegalStateException("Wrong type " + expectedType + " but got " + type);
            }
        }
    }
}
