(function () {
    "use strict";

    function returnWith(promise, data) {
        // invoke the promise and get the data yielded from it (if any),
        // and return that along with "data"
        return promise.then(function (moreData) {
            if (moreData) {
                data.result = moreData;
            }

            return data;
        });
    }

    WinJS.Namespace.define("Utils", {
        returnWith: returnWith
    });

})();
