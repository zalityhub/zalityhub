const fs = require('fs');
const path = require('path');
const process = require('process');
const util = require('util');


const stringify = require('json-stringify-safe');
const ExifImage = require('exif').ExifImage;
const nx = require('@zality/hub/util');

const Config = nx.getEnv(['google'], true);
const googleMapsClient = require('@google/maps').createClient({
  key: ''
});


function ProcessDups(targets) {

  function dirHandler(dir) {
    const stats = nx.fsGet(dir, true);
    for (let i = 0, ilen = stats.length; i < ilen; ++i) {
      const stat = stats[i];
      if (stat.isFile()) {
        const fname = path.basename(stat.fpath);
        if (fname.length > 0 && isCharDigit(fname[0])) {
          if (!nx.isArray(files[fname]))
            files[fname] = [];   // new file
          files[fname].push(stat.fpath);   // add this one
        }
      }
    }
  }

// 3034860064 871559 pictures/CameraRoll/041218_115837.jpg
  function sumHandler(file) {
    const lines = fs.readFileSync(file, 'utf-8').split('\n').filter(Boolean);

    for (let i = 0, ilen = lines.length; i < ilen; ++i) {
      const line = lines[i];
      const items = line.split(' ');
      if (items.length === 3) {
        const sum = items[0];
        if (!nx.isArray(files[sum]))
          files[sum] = [];   // new file
        files[sum].push(items[2]);   // add this one
      }
    }
  }

  if (!nx.isArray(targets))
    targets = [targets];    // make it so

  let files = {};

  for (let i = 0, ilen = targets.length; i < ilen; ++i) {
    const target = targets[i];
    const stat = fs.statSync(target);
    if (stat.isDirectory())
      dirHandler(target);
    else
      sumHandler(target);
  }

  files = HashToArray(files);
  let cnt = 0;
  for (let i = 0, ilen = files.length; i < ilen; ++i) {
    const dup = files[i];
    if (dup.length > 1) {
      console.log('');
      for (let j = 0, jlen = dup.length; j < jlen; ++j) {
        console.log(`${j}: ${fs.realpathSync(dup[j]).toString()}`);
        ++cnt;
      }
    }
  }

  console.log(`${cnt} dups`);
}


function ConvertLatLng(ll, ref) {
  let r = 0.0;
  let d = 1.0;

  for (let i = 0, ilen = ll.length; i < ilen; ++i) {
    r = r + (parseFloat(ll[i]) / d);
    d = d * 60.0;
  }

  if (ref.toLowerCase() === 'w')
    r = 0.0 - r;
  if (ref.toLowerCase() === 's')
    r = 0.0 - r;
  return r;
}


function QueueJpgs(targets) {

  function enqueue(targets) {
    let queue = [];

    for (let i = 0, ilen = targets.length; i < ilen; ++i) {
      const target = targets[i];
      const stat = nx.statSafeSync(target);
      if (!nx.isNull(stat.isDirectory) && stat.isDirectory()) {
        const stats = nx.fsGet(target, true);
        for (let i = 0, ilen = stats.length; i < ilen; ++i)
          queue = queue.concat(enqueue([stats[i].fpath]));   // recursive queue for sub directories
      } else if (!nx.isNull(stat.isFile) && stat.isFile()) {
        if (path.extname(target).toString().toLowerCase() === '.jpg')
          queue.push(target);
      }
    }
    return queue;
  }


  if (!nx.isArray(targets))
    targets = [targets];    // make it so

  return enqueue(argv);
}


function ProcessJpg(kick, file) {

  function geoResponse(rsp) {
    function isWeirdPrefix(str) {
      if (str.length <= 2)
        return false;
      if (str.allDigits())
        return false;
      for (let c of str.toString()) {
        if (!((c >= 'A' && c <= 'Z') || c === '+' || (c >= '0' && c <= '9')))
          return false;
      }
      return true;
    }

    function fixupAddress(addr) {
      addr = addr.toString();
      let ar = addr.split(' ');
      if (isWeirdPrefix(ar[0])) {
        ar.splice(0, 1);
        addr = ar.join(' ');
      }
      addr = addr.split(',');

      for (let i = 0, ilen = addr.length; i < ilen; ++i) {
        ar = addr[i].toString().allTrim().toString();
        if ((ar === 'USA')) {
          addr.splice(i, 1);    // remove it
          i = -1;    // restart the loop
          --ilen;
        } else {
          if ((ar.length > 5 && ar[ar.length - 6] === ' ' && ar.slice(ar.length - 5)).toString().allDigits())
            ar = ar.slice(0, ar.length - 6);    // looks like a zip code
          addr[i] = ar;
        }
      }
      return addr.join(', ').toString();
    }

    function normalizeAddress(addr) {
      addr = addr.toString();

      let seenNonDigit = false;
      addr = addr.split('');

      for (let i = 0, ilen = addr.length; i < ilen; ++i) {
        const c = addr[i];
        const isDigit = ((c >= '0' && c <= '9') || c === '-');
        if ((!seenNonDigit && isDigit) || c === '#' || c === '&' || c === '\\' || c === '+')
          addr[i] = '';
        if (c === ',' || c === '.')
          addr[i] = '_';
        seenNonDigit = (seenNonDigit || (!isDigit));
      }
      addr = addr.join('').toString().allTrim().split(' ');
      addr = addr.join('_').toString().allTrim().toString().replaceAll('__', '_').toString();
      return addr.replaceAll('__', '_').toString();
    }


    let locations = rsp.json.results;
    if (!nx.isArray(locations))
      locations = [];
    for (let i = 0, ilen = locations.length; i < ilen; ++i) {
      let addr = locations[i].formatted_address;
      if (addr) {
        addr = fixupAddress(addr.toString()).toString();
        locations[i].fixedAddress = addr;
        locations[i].normalizedAddress = normalizeAddress(addr);
      }
    }

    // if (locations.length >= 2)
    // locations = locations.slice(locations.length - 2); // keep only two
    console.log(stringify({file: file, locations: locations}));
  }


  function geoExif(exif) {
    const gps = {};

    if (nx.isNull(exif.gps) && !nx.isNull(exif.exif))   // look for gps data
      exif = exif.exif;

    if (!nx.isNull(exif.gps)) {
      if (nx.isArray(exif.gps.GPSLatitude))
        gps.lat = ConvertLatLng(exif.gps.GPSLatitude, exif.gps.GPSLatitudeRef);
      if (nx.isArray(exif.gps.GPSLongitude))
        gps.lng = ConvertLatLng(exif.gps.GPSLongitude, exif.gps.GPSLongitudeRef);

      if (!nx.isNull(gps) && !(nx.isNull(gps.lat) && nx.isNull(gps.lng))) {
        const geoPromise = util.promisify(googleMapsClient.reverseGeocode);
        geoPromise({latlng: `${gps.lat}, ${gps.lng}`}).then(response => {
          geoResponse(response)
          kick.next();
        }).catch(err => {
          if(nx.isNull(err.message))
            err = new Error(stringify(err.json));
          nx.logError(`Error: ${err.message} in ${file}`);
          kick.next();
        });
        return;
      }
    }

    const err = new Error(`No gps data in ${file}`);
    nx.logError('Error: ' + err.message);
    kick.next();
  }

  console.log(`do ${file}`);

  const exifPromise = util.promisify(ExifImage);
  exifPromise({image: file}).then(exif => {
    geoExif(exif)
  }).catch(err => {
    nx.logError(`Error: ${err.message} in ${file}`);
    kick.next();
  });
}



const argv = process.argv.slice(2);    // arg list

nx.serialize.start(QueueJpgs(argv), ProcessJpg);

// ProcessDups(argv);
