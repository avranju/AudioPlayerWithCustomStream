(function () {
    "use strict";

    // MP3 Framesize and length for Layer II and Layer III
    var SAMPLE_SIZE = 1152;
    var SAMPLE_DURATION_MS = 70;

    // namespace aliases
    var Storage = Windows.Storage;
    var Streams = Windows.Storage.Streams;

    var XORMediaSource = WinJS.Class.define(function (url, key) {
        this.url = url;
        this.key = key;
        this.streamInfo = null;
    }, {
        initAsync: function () {
            var self = this;
            
            // load the URL
            var p1 = this._loadStream(this.url);
            
            // extract display name from url
            var displayName = self.url.substr(self.url.lastIndexOf("/") + 1);

            // created streamed file
            var p2 = Storage.StorageFile.createStreamedFileAsync(
                displayName, self._streamedDataProvider.bind(self), null);

            return WinJS.Promise.join([p1, p2]).then(function (results) {
                self.streamInfo = results[0];
                var file = results[1];

                return file.openAsync(Storage.FileAccessMode.read).then(function (stream) {
                    return {
                        file: file,
                        stream: stream
                    };
                });
            }).then(function (data) {
                // get encoding properties of the mp3 file
                var props = [
                    "System.Audio.SampleRate",
                    "System.Audio.ChannelCount",
                    "System.Audio.EncodingBitrate"
                ];

                return data.file.properties.retrievePropertiesAsync(props);
            }).then(function (props) {

                var b = props.hasKey("System.Audio.SampleRate");

                //var sampleRate = props["System.Audio.SampleRate"];
                //var channelCount = props["System.Audio.ChannelCount"];
                //var encodingBitRate = props["System.Audio.EncodingBitRate"];

            });
        },

        _streamedDataProvider: function (request) {
            // xor-decrypt the stream and load into a memory stream
            var reader = new Streams.DataReader(this.streamInfo.inputStream);

            // write the contents of "stream" into "request"
            var writer = new Streams.DataWriter(request);

            // copy the streams
            return this._copyStreamAsync(
                reader, writer, this._xorTransform.bind(self)).then(function () {
                    writer.close();
            });
        },

        _xorTransform: function (buffer) {
            for (var i = 0; i < buffer.length; i++) {
                buffer[i] = buffer[i] ^ this.key;
            }
        },

        _copyStreamAsync: function (dataReader, dataWriter, transform) {
            var BUFFER_SIZE = 1024;
            var buffer = new Uint8Array(BUFFER_SIZE);

            var loop = function (bytesLoaded) {
                // load the data into buffer
                if (bytesLoaded < BUFFER_SIZE) {
                    buffer = new Uint8Array(bytesLoaded);
                }
                dataReader.readBytes(buffer);

                // transform data
                transform(buffer);

                // write the data into dataWriter
                dataWriter.writeBytes(buffer);

                // if bytesLoaded is less than BUFFER_SIZE then we are done
                if (bytesLoaded < BUFFER_SIZE) {
                    dataReader.close();
                } else {
                    // Create another promise to asynchronously execute the next iteration. 
                    // Because loadAsync is permitted to complete synchronously, we avoid using a simple 
                    // nested call to loop.  Such a nested call could lead to deep recursion, and, if enough 
                    // loadAsync calls complete synchronously, eventual stack overflow. 
                    return WinJS.Promise.timeout(0).then(function () {
                        return dataReader.loadAsync(BUFFER_SIZE);
                    }).then(loop);
                }
            };

            return dataReader.loadAsync(BUFFER_SIZE).then(loop);
        },

        _loadStream: function(url) {
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