const addon = require('./build/Release/hyperscan');

module.exports.ChimeraDatabase = addon.ChimeraDatabase;

let db = new addon.ChimeraDatabase(['testing'], [{CH_FLAG_SINGLEMATCH: false}]);

let string = 'testing123 testing45623';

console.log(db.scan(string));
