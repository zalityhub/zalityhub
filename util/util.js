_nx = exports;
_nx.platform = require('os').platform();

const process = require('process');
const fs = require('fs');
const path = require('path');
const util = require('util');
const stringify = require('json-stringify-safe');
const v8 = require('v8');


// String Prototypes
String.prototype.indexOfNotAlphaNumeric = function () {
  for (let i = 0, ilen = this.length; i < ilen; ++i) {
    const ch = this.charAt(i);
    if (!(ch >= '0' && ch <= '9') && // numeric (0-9)
      !(ch >= 'A' && ch <= 'Z') && // upper alpha (A-Z)
      !(ch >= 'a' && ch <= 'z')) { // lower alpha (a-z)
      return i;
    }
  }
  return -1;		// is all alphanumeric
}

String.prototype.isAllDigits = function () {
  for (let c of this) {
    if (c != '.' && !((c >= '0' && c <= '9')))
      return false;
  }
  return true;
}

String.prototype.isAllUpper = function () {
  for (let c of this) {
    if (!((c >= 'A' && c <= 'Z')))
      return false;
  }
  return true;
}

String.prototype.isAllAlpha = function () {
  for (let i = 0; i < this.length; ++i) {
    chr = this.charAt(i).toLowerCase();
    if (chr < 'a' || chr > 'z')
      return false;
  }
  return true;
};

String.prototype.isAllAlphaNumeric = function () {
  return (this.indexOfNotAlphaNumeric < 0) ? true : false;
};

String.prototype.isNumeric = function () {
  return (!isNaN(parseFloat(this)) && isFinite(this));
};

String.prototype.startsWith = function (prefix) {
  return this.indexOf(prefix) === 0;
};

String.prototype.get_number = function () {
  return this.match(/[0-9]+/g);
};

String.prototype.endsWith = function (suffix) {
  return this.indexOf(suffix, this.length - suffix.length) !== -1;
};

String.prototype.escapeText = function (text) {
  if (!text) return text;     // when given null, return same
  return '"' + text.replaceAll('"', '""') + '"';
};

String.prototype.strip = function (remove) {
  return this.replace(new RegExp(remove, 'g'), '');
};


// Enum Prototypes
_nx.enum = function (vals) {
  this.def = vals;
  this.vals = {};
  this.add(vals);
  return this;
}

_nx.enum.prototype.add = function (vals) {
  vals = _nx.nullTo(vals, {});
  if (_nx.isArray(vals)) {
    // init via array, numbering enums from 1 -> n
    for (let i = 0, ilen = vals.length; i < ilen; ++i) {
      const key = (i + 1).toString();
      this.vals[key] = vals[i];   // add key -> value
      this.vals[vals[i]] = key;   // and value -> key
    }
  } else {
    for (let key in vals) {
      this.vals[key] = vals[key];   // add key -> value
      this.vals[vals[key]] = key;   // and value -> key
    }
  }
}

_nx.enum.prototype.contains = function (key_val) {
  if (this.vals.hasOwnProperty(key_val))
    return this.vals[key_val];
  return null;
}

_nx.enum.prototype.valueOf = function (key_val) {
  ret = null;
  if ((ret = this.contains(key_val)))
    return ret;
// Neither; return input
  return key_val;
}


_nx.replaceAll = function (content, from, to, caseless) {

  function replace(content, from, to) {
    if (!caseless) {
      while (content.indexOf(from) >= 0)
        content = content.replace(from, to);
      return content;
    }

    let lcc = content.toLowerCase();
    let lcf = from.toLowerCase();
    let i, h = 0, l = lcf.length;
    while ((i = lcc.slice(h).indexOf(lcf)) >= 0) {
      h += i;
      lcc = lcc.slice(0, h) + to + lcc.slice(h + l + 1);
      content = content.slice(0, h) + to + content.slice(h + l + 1);
      h += l;
    }
    return content;
  }

  if(!to)
    to = '';

  if(_nx.isArray(from) && ((!_nx.isArray(to)) || (from.length !== to.length)))
    throw new Error(`replaceAll: both from(${from.length}) and to(${to.length}) must be matching array sizes`);
  
  if(_nx.isArray(from)) {
    from = [].concat(from);
    to = [].concat(to);
    while(from.length) 
      content = replace(content, from.shift(), to.shift());

    return content;
  }
  
  return replace(content, from, to);
}

_nx.date = function() { return `${new Date().toISOString().replace('T', ' ')}`; }


_nx.isNull = function (en, required) {
  if (required === undefined || required === null)
    required = false;
  if (en === undefined || en === null) {
    if (!required)
      return true;
    throw new Error(stringify(en) + ' must not be null');
  }
  return false;
}

_nx.isObject = function (en, required) {
  if (_nx.isNull(required))
    required = false;
  if (!_nx.isNull(en) && en.constructor === Object)
    return true;
  if (!required)
    return false;
  throw new Error(stringify(en) + ' must be an Object');
}


_nx.isArray = function (en, required) {
  if (_nx.isNull(required))
    required = false;
  if (en instanceof Array)
    return true;
  if (!required)
    return false;
  throw new Error(stringify(en) + ' must be an Array');
}


_nx.isString = function (en, required) {
  if (_nx.isNull(required))
    required = false;
  if (typeof (en) === 'string')
    return true;
  if (!required)
    return false;
  throw new Error(stringify(en) + ' must be a String');
}

_nx.isStringBuilder = function (en, required) {
  if (_nx.isNull(required))
    required = false;
  if (en._stringbuilder)
    return true;
  if (!required)
    return false;
  throw new Error(stringify(en) + ' must be a StringBuilder');
}

_nx.isFunction = function (it, isRequired) {
  if (_nx.isNull(isRequired))
    isRequired = false;

  if (typeof it === 'function')
    return true;
  if (!isRequired)
    return false;
  throw new Error(stringify(it) + ' must be a Function');
}

_nx.isNumber = function (en, required) {
  if (_nx.isNull(required))
    required = false;
  if (typeof (en) === 'number')
    return true;
  if (!required)
    return false;
  throw new Error(stringify(en) + ' must be a Number');
}

_nx.isBoolean = function (en, required) {
  if (_nx.isNull(required))
    required = false;
  if (typeof (en) === 'boolean')
    return true;
  if (!required)
    return false;
  throw new Error(stringify(en) + ' must be a Boolean');
}

_nx.isTrue = function (it, isRequired) {
  if (_nx.isNull(isRequired))
    isRequired = false;

  if (it === undefined && isRequired)
    throw new Error(stringify(it) + ' must be Present');

  if (it !== undefined && it)
    return true;
  return false;
}


_nx.isWhat = function (it) {
  const what = {};

  what.is = function (is) {
    if (_nx.isString(is))
      return this[is];
    if (_nx.isArray(is)) {
      for (let i = 0, ilen = is.length; i < ilen; ++i)
        if (this[is[i]])
          return this[is[i]];
    }
  }

  what.isNull = _nx.isNull(it);
  what.isObject = _nx.isObject(it);
  what.isArray = _nx.isArray(it);
  what.isString = _nx.isString(it);
  what.isStringBuilder = _nx.isStringBuilder(it);
  what.isFunction = _nx.isFunction(it);
  what.isBoolean = _nx.isBoolean(it);
  what.isNumber = _nx.isNumber(it);
  what.isTrue = _nx.isTrue(it);
  what.isAny = true;

  return what;
}

_nx.isCharDigit = function (c) {
  return c >= '0' && c <= '9';
}


_nx.getCpuInfo = function () {
  let content = '';
  try {
    content = fs.readFileSync('/proc/cpuinfo', 'utf8');
  } catch (err) {
    content = '';
  }
  return content;
}


_nx.nullTo = function (s, def) {
  if (s === undefined || s === null)
    s = def;
  return s;
};


_nx.nullToString = function (s, def) {
  return this.nullTo(s, def).toString();
};


_nx.getProcessOption = function (opt) {
  for (let i = 0; i < process.argv.length; ++i) {
    if (process.argv[i] === opt)
      return i;
  }
  return -1;
};

_nx.getProcessArg = function (i) {
  if (i >= 0 && i < process.argv.length)
    return process.argv[i];
  return null;
};


// Hash To Array
_nx.hashToArray = function (obj) {
  const _ar = [];
  Object.keys(obj).forEach(function (key) {
    const p = obj[key];
    if (p != null)
      _ar.push(obj[key]);
  });
  return _ar;
};


_nx.resolveProperty = function (obj, path, default_value) {
  function resolve(obj, path) {
    if (!obj)
      return obj;
    return path.reduce(function (val, seg) {
      return val && typeof val === 'object' && val[seg];
    }, obj);
  }

  obj = resolve(obj, path);
  if (_nx.isNull(obj))
    return default_value;
  return obj;
}

_nx.modProperty = function (obj, path, val) {
  const keys = [];
  let mods = 0;

  function mapRecursive(o, callback) {
    for (let key in o) {
      keys.push(key);
      if (o.hasOwnProperty(key)) {
        if (typeof o[key] === "object") {
          mods += callback(obj, keys, o, key);
          mods += mapRecursive(o[key], callback);
        } else {
          mods += callback(obj, keys, o, key);
        }
      }
      keys.pop();
    }
    return mods;
  }

  return mapRecursive(obj, function (obj, keys, o, key) {
    const kpath = keys.join('.');

    if (kpath === path) {
      o[key] = val;
      return 1;
    }
    return 0;
  });
}


_nx.parseJsonFile = function (file) {
  try {
    const text = fs.readFileSync(file).toString();
    return JSON.parse(text);
  } catch (e) {
    _nx.logError(e.toString());
    throw e;
    return null;
  }
}

_nx.getHomeDir = function () {
  if (_nx.platform === 'win32')
    return `c:/cygwin64/home/hbray/`;
  else if (_nx.platform === 'darwin')
    return `/Users/hbray/`;
  else
    return `/home/hbray/`;
}

_nx.getEnv = function (section, isRequired, file) {
  if (_nx.isNull(isRequired))
    isRequired = false;
  if (_nx.isNull(file))
    file = `${_nx.getHomeDir()}etc/env.json`;
  let env = _nx.parseJsonFile(file);
  if (isRequired && _nx.isNull(env))
    _nx.fatal(`unable to continue without a ${file}`);
  if (!section)
    return env;

// want's a specific section of the env...
  return _nx.resolveProperty(env, section);
}

_nx.putEnv = function (section, env, file) {
  if (_nx.isNull(file))
    file = `${_nx.getHomeDir()}etc/env.json`;

  let current = _nx.getEnv(null, true, file);

  if (section)
    _nx.modProperty(current, section, env);
  else
    current = env;

  try {
    const text = stringify(current, null, 4);
    fs.writeFileSync(file, text.toString().trim());
  } catch (e) {
    _nx.logError(e.toString());
    throw e;
  }
}


_nx.logger = function (settings) {
  this.set(settings);
}

_nx.logger.prototype.set = function (settings) {
  if (_nx.isNull(settings))
    settings = {};

  delete settings['stream'];    // remove inapproriate value
// add missing members
  this.settings = {...{timeStamp: false, lineSpacing: 1, file: '', mask: {}}, ...settings};

  if (this.settings.stream) {
    this.settings.stream.end();
    delete this.settings['stream'];
  }

  if (this.settings.file.length > 0)
    this.settings.stream = fs.createWriteStream(this.settings.file);
}

_nx.logger.prototype.setMask = function (mask) {
  this.settings.mask = {...this.settings.mask, ...mask};
  return this.settings.mask;
}

_nx.logger.prototype.logInitMask = function (mask) {
  this.settings.mask = mask;
  return this.settings.mask;
}

_nx.logger.prototype.logOutput = function (con, text, settings) {
  function out(stream, text, settings) {
    let eol = '';
    if (!settings)
      settings = this.settings;
    if (settings.lineSpacing)
      eol = Buffer.alloc(settings.lineSpacing).fill('\n');

    let ts = '';
    if (settings.timeStamp)
      ts = `${_nx.date()}:`;
    stream.write(`${ts}${text}${eol}`);
  }

  if (!settings)
    settings = this.settings;
  out(con, text, settings);
  if (this.settings.stream)
    out(this.settings.stream, text, settings);
  return text;
}

_nx.logger.prototype.write = function (text) {
  return this.logOutput(process.stdout, util.format.apply(this, arguments), {lineSpacing: 0});
}

_nx.logger.prototype.log = function (text) {
  return this.logOutput(process.stdout, util.format.apply(this, arguments));
}

_nx.logger.prototype.mLog = function (mask, text) {
  if (_nx.isNull(this.settings.mask[mask]) || !this.settings.mask[mask])
    return '';    // mask is off, don't log
  return this.logOutput(process.stdout, util.format.apply(this, arguments));
}

_nx.logger.prototype.logError = function (text) {
  return this.logOutput(process.stderr, util.format.apply(this, arguments));
}

_nx.logger.prototype.fatal = function (message) {
  this.logError(new Error(message.toString()));
  process.exit(1);
}

_nx.log = function (text) {
  if (_nx.isNull(_nx._log))
    _nx._log = new _nx.logger();
  return _nx._log.log(text);
}

_nx.mLog = function (mask, text) {
  if (_nx.isNull(_nx._log))
    _nx._log = new _nx.logger();
  return _nx._log.mLog(text);
}

_nx.logError = function (text) {
  if (_nx.isNull(_nx._log))
    _nx._log = new _nx.logger();
  return _nx._log.logError(text);
}

_nx.fatal = function (message) {
  if (_nx.isNull(_nx._log))
    _nx._log = new _nx.logger();
  return _nx._log.fatal(message);
}


_nx.doCmd = function (cmd, args) {
  try {
    _nx.log(cmd);
    _nx.log(args);
    const proc = require('child_process').spawnSync(cmd, args);
    if (proc.error)
      throw proc.error;
    return [proc.status, proc.stdout.toString(), proc.stderr.toString()];
  } catch (err) {
    _nx.logError('DoCmd failed ' + err);
    return [-1, '', _nx.logError('DoCmd failed ' + err)];
  }
}

_nx.statSafeSync = function (path) {
  try {
    const stat = fs.statSync(path);
    stat.rpath = path;
    return stat;
  } catch (err) {
    _nx.logError(err.toString());
    throw err;
  }
}


_nx.fileExists = function (path) {
  const stat = StatSafeSync(path);
  return !_nx.isNull(stat.dev);
}


_nx.getBody = function (req, cb) {
  cb = cb ? cb : function () {
  };
  try {
    const body = [];
    req.on('data', (chunk) => {
      body.push(chunk);
    }).on('end', () => {
      req.body = Buffer.concat(body).toString();
      if (req.body.startsWith('{'))
        req.body = JSON.parse(req.body); // convert to a json object
      cb(null, req);
    });
  } catch (err) {
    _nx.logError(err.toString());
    cb(err);
  }
}


_nx.fsIterate = function (dir, cb) {
  cb = cb ? cb : function () {
  };
  fs.readdir(dir, function (err, objects) {
    if (err) {
      _nx.logError(`Could not list ${objects}`, err);
      process.exit(1);
    }

    objects.forEach(function (object, index) {
      const fpath = path.join(dir, object);

      fs.stat(fpath, function (err, stat) {
        if (err)
          return _nx.logError(`Error stating ${fpath}: ${err.toString()}`);
        stat.fpath = fpath;
        cb(null, stat);
      });
    })
  });
}


_nx.fsGet = function (dir, exp, recursive) {
  let stats = [];
  let re = new RegExp('.*');    // anything
  if (exp && _nx.isString(exp))
    re = new RegExp(exp);
  const ar = fs.readdirSync(dir);
  for (let i = 0, ilen = ar.length; i < ilen; ++i) {
    const fpath = path.join(dir, ar[i]);
    const stat = fs.statSync(fpath);
    stat.fpath = fpath;
    stat.name = ar[i];
    if (recursive && stat.isDirectory())
      stats = stats.concat(FsGet(fpath, recursive));
    else if (re.exec(stat.name))
      stats.push(stat);
  }
  return stats;
}


_nx.countObjectProperties = function (obj) {
  return Object.keys(obj).length;
}


_nx.getMemoryStats = function () {
  return Object.entries(process.memoryUsage()).reduce((carry, [key, value]) => {
    return `${carry}${key}:${Math.round(value / 1024 / 1024 * 100) / 100}MB;`;
  }, '');
}


_nx.garbageCollect = function () {
  try {
    if (global.gc) {
      global.gc();
    }
  } catch (err) {
    _nx.logError(`Error: ${err.message}`);
    process.exit();
  }
}


_nx.serialize = function (queue, handler) {
  this.queue = queue;
  this.handler = handler;
  this.next();
}

_nx.serialize.prototype.next = function () {
  if (this.queue.length > 0)
    this.handler(this, this.queue.pop());   // do the next one
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::                                                                         :*/
/*::  This routine calculates the distance between two points (given the     :*/
/*::  latitude/longitude of those points). It is being used to calculate     :*/
/*::  the distance between two locations using GeoDataSource (TM) prodducts  :*/
/*::                                                                         :*/
/*::  Definitions:                                                           :*/
/*::    South latitudes are negative, east longitudes are positive           :*/
/*::                                                                         :*/
/*::  Passed to function:                                                    :*/
/*::    lat1, lon1 = Latitude and Longitude of point 1 (in decimal degrees)  :*/
/*::    lat2, lon2 = Latitude and Longitude of point 2 (in decimal degrees)  :*/
/*::    unit = the unit you desire for results                               :*/
/*::           where: 'M' is statute miles (default)                         :*/
/*::                  'K' is kilometers                                      :*/
/*::                  'N' is nautical miles                                  :*/
/*::  Worldwide cities and other features databases with latitude longitude  :*/
/*::  are available at http://www.geodatasource.com                          :*/
/*::                                                                         :*/
/*::  For enquiries, please contact sales@geodatasource.com                  :*/
/*::                                                                         :*/
/*::  Official Web site: http://www.geodatasource.com                        :*/
/*::                                                                         :*/
/*::           GeoDataSource.com (C) All Rights Reserved 2015                :*/
/*::                                                                         :*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


_nx.distanceLatLng = function (lat1, lon1, lat2, lon2, unit) {
  const theta = lon1 - lon2;
  let dist = Math.sin(_nx.deg2rad(lat1)) * Math.sin(_nx.deg2rad(lat2)) + Math.cos(_nx.deg2rad(lat1)) * Math.cos(_nx.deg2rad(lat2)) * Math.cos(_nx.deg2rad(theta));
  dist = Math.acos(dist);
  dist = _nx.rad2deg(dist);
  dist = dist * 60 * 1.1515;

  if (unit == 'K') {
    dist = dist * 1.609344;
  } else if (unit == 'N') {
    dist = dist * 0.8684;
  } else if (unit == 'F') {
    dist *= 5280;
  }

  return (dist);
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

_nx.deg2rad = function (deg) {
  return (deg * Math.PI / 180.0);
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

_nx.rad2deg = function (rad) {
  return (rad * 180 / Math.PI);
}

/*
  // System.out.printf("%8.10f, %8.10f\n",
  // (33.59307044158479 - 33.59309786854634),
  // (-84.71678148950154 - (-84.71674283218286)));

  console.log(DistanceLatLng(
    33.773178, -79.920094,
    32.781666666666666,-79.916666666666671,
    'N') + " Feet\n");

  // System.out.println(l.distance(32.9697, -96.80322, 29.46786, -98.53506, 'M') + " Miles\n");
  // System.out.println(l.distance(32.9697, -96.80322, 29.46786, -98.53506, 'K') + " Kilometers\n");
  // System.out.println(l.distance(32.9697, -96.80322, 29.46786, -98.53506, 'N') + " Nautical Miles\n");
*/


_nx.launchBrowser = function (url) {
  const {exec} = require('child_process');

  if (url === undefined) {
    console.error('Please enter a URL, e.g. "http://google.com"');
    process.exit(0);
  }

  let command;
  let opts = {};

  if (_nx.platform === 'win32') {
    command = `"c:\\Program Files\\Mozilla Firefox\\firefox.exe" ${url}`;
  } else if (_nx.platform === 'darwin') {
    command = `open -a "Google Chrome" ${url}`;
  } else {
    command = `google-chrome --no-sandbox ${url}`;
  }

  exec(command, opts);
}


_nx.getchar = function (cb) {
  if (!cb) {   // no callback, do readSync
    const buffer = Buffer.alloc(1);
    fs.readSync(0, buffer, 0, 1);
    return buffer.toString('utf8');
  }
  const stdin = process.stdin;
  stdin.setRawMode(true);
  stdin.resume();
  stdin.setEncoding('utf8');
  stdin.on('data', function (key) {
    key = key.toString();
    cb(null, key.length, key);
  });
}

_nx.putchar = function (ch) {
  return _nx.puts(ch);
}

_nx.putAscii = function (string) {
  string = string ? string : '';
  for (let i = 0, ilen = string.length; i < ilen; i++) {
    const ascii = string.charCodeAt(i);
    _nx.puts(ascii);
  }
}

_nx.puts = function (string, cb) {
  cb = cb ? cb : function () {
  };
  string = string ? string : '';
  return fs.write(1, string.toString(), cb);
}


_nx.StringBuilder = function (initial) {
  this._array = [];
  this._stringbuilder = true;
  if (_nx.isString(initial))
    this.push(initial);
  else if (_nx.isArray(initial))
    for (let i = 0, ilen = initial.length; i < ilen; ++i)
      this.push(initial[i]);
  else if (_nx.isObject(initial) && initial._stringbuilder)
    for (let i = 0, ilen = initial._array.length; i < ilen; ++i)
      this.push(initial._array[i]);
  return this;
}

_nx.StringBuilder.prototype = {
  get length() {
    return this._array.length;
  }
}

_nx.StringBuilder.prototype = {
  get array() {
    return this._array;
  }
}

_nx.StringBuilder.prototype.push = function (text) {
  if (!text)
    return;   // nothing
  this._array.push(text);
  return this;
}

_nx.StringBuilder.prototype.push_line = function (text) {
  return this.push(text.toString() + '\n');
}

_nx.StringBuilder.prototype.write = function (text) {
  return this.push(text);
}

_nx.StringBuilder.prototype.write_line = function (text) {
  return this.push_line(text);
}

_nx.StringBuilder.prototype.unshift = function (n) {
  return this._array.unshift(n);
}

_nx.StringBuilder.prototype.shift = function (n) {
  return this._array.shift(n);
}

_nx.StringBuilder.prototype.pop = function (n) {
  return this._array.pop(n);
}

_nx.StringBuilder.prototype.slice = function (n) {
  this._array.slice(n);
  return this;
}

_nx.StringBuilder.prototype.insert = function (index, ...items) {
  this._array.splice(index, 0, ...items);
  return this;
}

_nx.StringBuilder.prototype.replace = function (index, text) {
  if (index >= this._array.length)
    return this.push(index, text);
  this._array[index] = text;
  return this;
}

_nx.StringBuilder.prototype.sort = function () {
  this._array.sort();
  return this;
}

_nx.StringBuilder.prototype.toString = function (opts) {
  if (!opts || _nx.isString(opts))
    return this._array.join(opts ? opts : '');

  opts.pre = opts.pre ? opts.pre : '';
  opts.post = opts.post ? opts.post : '';

  const sb = new _nx.StringBuilder();
  for (let i = 0, ilen = this._array.length; i < ilen; ++i)
    sb.push(`${opts.pre}${this._array[i].toString()}${opts.post}`);
  return sb.toString();
}


_nx.CloneObj = function (obj, force) {
  if (_nx.isArray(obj))
    return obj.slice();
  else if (_nx.isString(obj))
    return (' ' + obj).slice(1);    // force a copy
  else if (typeof obj !== "object")
    return obj;

  if(!force && obj._do_not_clone_me)
    _nx.fatal(`do_not_clone: ${stringify(obj)}`);
    
  return Object.fromEntries(Object.entries(obj).map(
    ([k, v]) => ([k, v ? _nx.CloneObj(v, force) : v])
  ));
}

_nx.stopWords = [
  "a",
  "about",
  "above",
  "after",
  "again",
  "against",
  "all",
  "am",
  "an",
  "and",
  "any",
  "are",
  "aren't",
  "as",
  "at",
  "be",
  "because",
  "been",
  "before",
  "being",
  "below",
  "between",
  "both",
  "but",
  "by",
  "can't",
  "cannot",
  "could",
  "couldn't",
  "did",
  "didn't",
  "do",
  "does",
  "doesn't",
  "doing",
  "don't",
  "down",
  "during",
  "each",
  "few",
  "for",
  "from",
  "further",
  "had",
  "hadn't",
  "has",
  "hasn't",
  "have",
  "haven't",
  "having",
  "he",
  "he'd",
  "he'll",
  "he's",
  "her",
  "here",
  "here's",
  "hers",
  "herself",
  "him",
  "himself",
  "his",
  "how",
  "how's",
  "i",
  "i'd",
  "i'll",
  "i'm",
  "i've",
  "if",
  "in",
  "into",
  "is",
  "isn't",
  "it",
  "it's",
  "its",
  "itself",
  "let's",
  "me",
  "more",
  "most",
  "mustn't",
  "my",
  "myself",
  "no",
  "nor",
  "not",
  "of",
  "off",
  "on",
  "once",
  "only",
  "or",
  "other",
  "ought",
  "our",
  "ours",
  "ourselves",
  "out",
  "over",
  "own",
  "same",
  "shan't",
  "she",
  "she'd",
  "she'll",
  "she's",
  "should",
  "shouldn't",
  "so",
  "some",
  "such",
  "than",
  "that",
  "that's",
  "the",
  "their",
  "theirs",
  "them",
  "themselves",
  "then",
  "there",
  "there's",
  "these",
  "they",
  "they'd",
  "they'll",
  "they're",
  "they've",
  "this",
  "those",
  "through",
  "to",
  "too",
  "under",
  "until",
  "up",
  "very",
  "was",
  "wasn't",
  "we",
  "we'd",
  "we'll",
  "we're",
  "we've",
  "were",
  "weren't",
  "what",
  "what's",
  "when",
  "when's",
  "where",
  "where's",
  "which",
  "while",
  "who",
  "who's",
  "whom",
  "why",
  "why's",
  "with",
  "won't",
  "would",
  "wouldn't",
  "you",
  "you'd",
  "you'll",
  "you're",
  "you've",
  "your",
  "yours",
  "yourself",
  "yourselves"
];

_nx.remove_stopwords = function (text, stopwords) {
  if (!stopwords)
    stopwords = _nx.stopWords;
  res = []
  words = text.trim().toLowerCase().split(/[^A-Za-z0-9]+/);
  for (i = 0; i < words.length; ++i) {
    word_clean = words[i].split('.').join('')
    if (!stopwords.includes(word_clean))
      res.push(word_clean)
  }
  return (res.join(' '))
}


_nx.Iterate = function (name, el, cb) {

  function _iterate(base, el, path) {
    const _path = [].concat(path);

    if (_nx.isNull(el))
      return;

    if (_nx.isObject(el)) {
      for (const key of Object.keys(el)) {
        if (el.hasOwnProperty(key)) 
          _iterate(base, el[key], [].concat(_path).concat([key]));
      }
    }

    else if (_nx.isArray(el)) {
      for (const key of Object.keys(el)) {
        _iterate(base, el[key], [].concat(_path).concat([key]));
      }
    }

    else if (_nx.isFunction(el)) {
      return;
    }

    cb(base, [].concat(_path), el);
  }

  const path = [];
  if(name && name.toString().length)
    path.push(name);
  _iterate(el, el, path);
}


_nx.pipe = function(pipes, text) {
  if (pipes) {
    text = text.toString();
    for (const [key, value] of Object.entries(pipes))
      value(text);    // call the pipe function
  }
}



_nx.writeHandle = function (handle, text) {

  if (!handle.enabled)
    return;       // nothing to do

  let file;
  if(!_nx.fileHandles)
    _nx.fileHandles = {};
  if (!(file = _nx.fileHandles[handle.file])) {
    file = {stream: fs.openSync(handle.file, handle.options.flags)};
    process.on('beforeExit', (code) => {
      if(file.stream) {
        fs.fdatasyncSync(file.stream);
        fs.closeSync(file.stream);
        delete file.stream;
      }
    });
    file.handle = _nx.CloneObj(handle);
    _nx.fileHandles[handle.file] = file;
    _nx.writeHandle(handle, `\nOpened: ${_nx.date()}\n`);
  }
  fs.writeSync(file.stream, text.toString());
  _nx.pipe(handle.pipes, text);
}


Object.defineProperty(global, '__stack', {
  get: function () {
    const orig = Error.prepareStackTrace;
    Error.prepareStackTrace = function (_, stack) {
      return stack;
    };
    const err = new Error;
    Error.captureStackTrace(err, arguments.callee);
    const stack = err.stack;
    Error.prepareStackTrace = orig;
    return stack;
  }
});

Object.defineProperty(global, '__line', {
  get: function () {
    let ln = __stack[1].getLineNumber();
    return ln;
  }
});

Object.defineProperty(global, '__function', {
  get: function () {
    let fn = __stack[1].getFunctionName();
    if( _nx.isNull(fn))
      fn = __stack[1].getFileName();
    return fn;
  }
});
