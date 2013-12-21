(function () {
    "use strict";

    // MP3 Framesize and length for Layer II and Layer III
    var SAMPLE_SIZE = 1152;
    var SAMPLE_DURATION_MS = 48;
    var NANO_SECONDS_PER_MS = 1000000;

    // namespace aliases
    var Storage = Windows.Storage;
    var Streams = Windows.Storage.Streams;
    var MediaProperties = Windows.Media.MediaProperties;
    var MediaCore = Windows.Media.Core;

    var XORMediaSource = WinJS.Class.define(function (mediaPlayer, url, key) {
        this.mediaPlayer = mediaPlayer;
        this.url = url;
        this.title = url.substr(url.lastIndexOf("/") + 1);
        this.key = key;
        this.streamInfo = null;

        this.memoryStream = null;
        this.duration = 0;
        this.sampleRate = 0;
        this.bitRate = 0;
        this.channelCount = 0;
        this.mss = 0;
        this.timeOffset = 0;
        this.byteOffset = 0;
        this.sampleSize = SAMPLE_SIZE;
        this.sampleDuration = SAMPLE_DURATION_MS;
    }, {
        initAsync: function () {
            var self = this;
            
            // load the URL
            return this._loadStreamAsync(this.url).then(function myfunction(streamInfo) {
                // xor transform audio stream into memory
                self.memoryStream = new Streams.InMemoryRandomAccessStream();
                var reader = new Streams.DataReader(streamInfo.inputStream);
                var writer = new Streams.DataWriter(self.memoryStream);

                return Utils.returnWith(
                    self._copyStreamAsync(reader, writer, self._xorTransform.bind(self)), {
                        reader: reader,
                        writer: writer,
                        mimeType: streamInfo.mimeType
                    });
            }).then(function (data) {
                // close the reader
                data.reader.close();
                data.writer.detachStream();

                // seek to start
                self.memoryStream.seek(0);

                // get the metadata we need
                var helper = MFUtils.MFAttributesHelper.create(self.memoryStream, data.mimeType);

                // save the props
                self.sampleRate = helper.sampleRate;
                self.bitRate = helper.bitRate;
                // helper.duration is provided in 100 nanosecond units; MSS.duration
                // expects this to be specified in milliseconds, so we convert
                self.duration = (helper.duration / (NANO_SECONDS_PER_MS / 100)) >> 0;
                self.channelCount = helper.channelCount;

                var audioProps = MediaProperties.AudioEncodingProperties.createMp3(
                    helper.sampleRate,
                    helper.channelCount,
                    helper.bitRate
                );
                var audioDescriptor = new MediaCore.AudioStreamDescriptor(audioProps);
                var mss = new MediaCore.MediaStreamSource(audioDescriptor);
                mss.canSeek = true;
                mss.musicProperties.title = self.title;
                mss.duration = self.duration;

                self.mss = mss;

                self._playbackStartingHandler = self._playbackStarting.bind(self);
                mss.addEventListener("starting", self._playbackStartingHandler, false);

                self._sampleRequestedHandler = self._sampleRequested.bind(self);
                mss.addEventListener("samplerequested", self._sampleRequestedHandler, false);

                self._playbackClosedHandler = self._playbackClosed.bind(self);
                mss.addEventListener("closed", self._playbackClosedHandler, false);

                self.mediaPlayer.src = URL.createObjectURL(mss, { oneTimeOnly: true });
                self.mediaPlayer.play();
            });
        },

        _playbackStarting: function (e) {
            // initialize the stream reader
            this.memoryStream.seek(0);

            var request = e.request;
            if ((request.startPosition !== null) && request.startPosition <= this.duration) {
                var sampleOffset = Math.floor(request.startPosition / this.sampleDuration);
                this.timeOffset = sampleOffset * this.sampleDuration;
                this.byteOffset = sampleOffset * this.sampleSize;
            }

            request.setActualStartPosition(this.timeOffset);
        },

        _sampleRequested: function (e) {
            var request = e.request;

            // check if the sample requested byte offset is within the file size
            if (this.byteOffset + this.sampleSize <= this.memoryStream.size) {
                var deferal = request.getDeferral();
                var inputStream = this.memoryStream.getInputStreamAt(this.byteOffset);

                // create the MediaStreamSample and assign to the request object. 
                // You could also create the MediaStreamSample using createFromBuffer(...)
                MediaCore.MediaStreamSample.createFromStreamAsync(
                    inputStream, this.sampleSize, this.timeOffset).then(function (sample) {
                        sample.duration = this.sampleDuration;
                        sample.keyFrame = true;

                        // increment the time and byte offset
                        this.byteOffset = this.byteOffset + this.sampleSize;
                        this.timeOffset = this.timeOffset + this.sampleDuration;
                        request.sample = sample;
                        deferal.complete();
                    }.bind(this));
            }
        },

        _playbackClosed: function (e) {
            // close the MediaStreamSource
            this.memoryStream.close();
            this.memoryStream = null;

            // remove the MediaStreamSource event handlers
            e.target.removeEventListener("samplerequested", self._sampleRequestedHandler, false);
            e.target.removeEventListener("starting", self._playbackStartingHandler, false);
            e.target.removeEventListener("closed", self._playbackClosedHandler, false);

            if (e.target === this.mss) {
                this.mss = null;
            }
        },

        _xorTransform: function (buffer) {
            for (var i = 0; i < buffer.length; i++) {
                buffer[i] = buffer[i] ^ this.key;
            }
        },

        _copyStreamAsync: function (dataReader, dataWriter, transform) {
            var BUFFER_SIZE = 1024 * 1024;
            var buffer = new Uint8Array(BUFFER_SIZE);

            var loop = function (bytesLoaded) {
                // load the data into buffer
                if (bytesLoaded < BUFFER_SIZE) {
                    buffer = new Uint8Array(bytesLoaded);
                }
                dataReader.readBytes(buffer);

                // transform data
                if (transform) {
                    transform(buffer);
                }

                // write the data into dataWriter
                dataWriter.writeBytes(buffer);

                // if bytesLoaded is less than BUFFER_SIZE then we are done
                if (bytesLoaded < BUFFER_SIZE) {
                    return dataWriter.storeAsync();
                }

                // Create another promise to asynchronously execute the next iteration. 
                // Because loadAsync is permitted to complete synchronously, we avoid using a simple 
                // nested call to loop.  Such a nested call could lead to deep recursion, and, if enough 
                // loadAsync calls complete synchronously, eventual stack overflow. 
                return WinJS.Promise.timeout(0).then(function () {
                    return dataReader.loadAsync(BUFFER_SIZE);
                }).then(loop);
            };

            return dataReader.loadAsync(BUFFER_SIZE).then(loop);
        },

        _loadStreamAsync: function(url) {
            return new WinJS.Promise(function (success, error, progress) {
                var streamProcessed = false;
                WinJS.xhr({
                    url: url,
                    responseType: "ms-stream"
                }).done(null,function (xhr) {
                    error(xhr);
                }, function (xhr) {
                    if (xhr.readyState === 3 && xhr.status === 200 && streamProcessed === false) {
                        streamProcessed = true;

                        var msstream = xhr.response; // this is an MSStream object
                        var inputStream = msstream.msDetachStream();
                        success({
                            inputStream: inputStream,
                            mimeType: msstream.type
                        });
                    }
                });
        });
}
    });

    // add XORMediaSource to namespace
    WinJS.Namespace.define("StreamUtils", {
        XORMediaSource: XORMediaSource
    });

})();