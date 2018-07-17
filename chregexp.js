const addon = require('./build/Release/hyperscan');

class CHRegExp {
    constructor(pattern, flags) {
        pattern += '';
        flags += '';
        this.source = pattern;
        this.flags = flags;
        this.global = false;
        this.ignoreCase = false;
        this.multiline = false;
        this.sticky = false;
        this.unicode = false;
        this.lastIndex = 0;

        if (flags) {
            for (let i = 0, iMax = flags.length; i < iMax; ++i) {
                if (flags[i] === 'g') {
                    this.global = true;
                } else if (flags[i] === 'i') {
                    this.ignoreCase = true;
                } else if (flags[i] === 'm') {
                    this.multiline = true;
                } else if (flags[i] === 'u') {
                    this.unicode = true;
                } else if (flags[i] === 'y') {
                    this.sticky = true;
                }
            }
        }

        this._database = new addon.ChimeraDatabase([pattern], [{caseless: this.ignoreCase, multiline: this.multiline, ucp: this.unicode}]);
        this._globalSaveData = {
            str: ''
            , data: new Uint32Array(0)
            , pos: 0
        };
    }

    exec(str) {
        str += '';
        let data = this._getScanData(str);

        let result = null;
        let index = this.global === true ? this.lastIndex : 0;
        let pos = this.global === true ? this._globalSaveData.pos : 0;
        while (pos < data.length && data[pos+1] < index) {
            pos += 4 + (2 * data[pos+3]);
        }

        if (pos < data.length) {
            result = [];
            result.index = data[pos+1];
            result.input = str;
            if (this.global === true) {
                this.lastIndex = data[pos+2];
            }

            result[0] = str.slice(data[pos+1], data[pos+2]);
            pos += 4;
            for (let i = 0, iMax = data[pos-1]; i < iMax; ++i) {
                result.push(str.slice(data[pos], data[pos+1]));
                pos += 2;
            }
            this._globalSaveData.pos = pos;
        }
        return result;
    }

    test(str) {
        str += '';
        let data = this._getScanData(str);

        let result = false;
        let index = this.global === true ? this.lastIndex : 0;
        let pos = this.global === true ? this._globalSaveData.pos : 0;
        while (pos < data.length && data[pos+1] < index) {
            pos += 4 + (2 * data[pos+3]);
        }

        if (pos < data.length) {
            if (this.global === true) {
                this.lastIndex = data[pos+2];
            }

            pos +=4;
            for (let i = 0, iMax = data[pos-1]; i < iMax; ++i) {
                pos += 2;
            }
            this._globalSaveData.pos = pos;

            result = true;
        }
        return result;
    }

    [Symbol.match](str) {
        str += '';
        let data = this._database.scan(str);
        let results = null;
        if (this.global === true) {
            let pos = 0;
            results = [];
            while (pos < data.length) {
                results.push(str.slice(data[pos+1], data[pos+2]));

                pos += 4;
                for (let i = 0, iMax = data[pos-1]; i < iMax; ++i) {
                    pos += 2;
                }
            }
        } else {
            let pos = 0;
            if (pos < data.length) {
                results = [];
                results.push(str.slice(data[pos+1], data[pos+2]));
                results.index = data[pos+1];
                results.input = str;

                pos += 4;
                for (let i = 0, iMax = data[pos-1]; i < iMax; ++i) {
                    results.push(str.slice(data[pos], data[pos+1]));
                    pos += 2;
                }
            }
        }

        return results;
    }

    [Symbol.replace](str, replaceValue) {
    }

    _getScanData(str) {
        // Depending on the global flag we get the scan data for the string
        if (this.global === true) {
            if (this._globalSaveData.str !== str) {
                // Scanning a different string than before so we need to reset our scan data
                this._globalSaveData.str = str;
                this._globalSaveData.data = this._database.scan(str);
                this._globalSaveData.pos = 0;
            }
            return this._globalSaveData.data;
        } else {
            // Not doing global so always scan the string
            return this._database.scan(str);
        }
    }
}

module.exports = CHRegExp;
