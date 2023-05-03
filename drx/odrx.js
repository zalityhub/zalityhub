function IfNull(en, deflt) {
  return IsNull(en) ? deflt : en;
}


function IsNull(en, required) {
  if (required === undefined || required === null)
    required = false;
  if (en === undefined || en === null) {
    if (!required)
      return true;
    Fatal(stringify(en) + ' must not be null');
  }
  return false;
}

function getSignature(file) {
  var signature = {};
  signature.name = file;

  try {
    var lstat = fs.lstatSync(file);
    signature.size = lstat.size;
    signature.isFile = lstat.isFile();
    signature.isDirectory = lstat.isDirectory();
    signature.isBlockDevice = lstat.isBlockDevice();
    signature.isCharacterDevice = lstat.isCharacterDevice();
    signature.isSymbolicLink = lstat.isSymbolicLink();
    signature.isFIFO = lstat.isFIFO();
    signature.isSocket = lstat.isSocket();
  } catch (e) {
    signature.error = e.toString();
    signature.ignored = true;
    signature.isFile = false;
    signature.isDirectory = false;
    signature.isBlockDevice = false;
    signature.isCharacterDevice = false;
    signature.isSymbolicLink = false;
    signature.isFIFO = false;
    signature.isSocket = false;
  }

  return signature
}


function sampleFiles(dirs) {
  dirs = [].concat(dirs);       // insure input is an array

  var files = {};

  for (var d = 0; d < dirs.length; ++d) {
    var dir = dirs[d];

    var signature = getSignature(dir);
    if (!signature.isDirectory) {
      signature.ignored = true;
      files[dir] = signature;
      continue;		// not a directory...
    }

    files[dir] = signature;
    var fl = fs.readdirSync(dir);
    for (var f = 0; f < fl.length; ++f) {
      var fn = fl[f];
      fn = (dir + '/' + fn).toString().replaceAll('//', '/');

      signature = getSignature(fn);
      if (signature.isDirectory) {
        if (fn.endsWith('perl')
          //|| fn.endsWith('autosys')
          || fn.endsWith('CVS')
          || fn.endsWith('data')
          || fn.endsWith('jpegs')
          || fn.endsWith('logs')
          || fn.endsWith('log')
          || fn.endsWith('sockets')
          || fn.endsWith('tmp')
          || fn.endsWith('backup')
          || fn.endsWith('archive')
          || fn.startsWith('.')
          || fn.indexOf('/.') != -1) {
          signature.ignored = true;
          files[fn] = signature;
        } else {
          var f2 = sampleFiles(fn);
          Object.keys(f2).forEach(function (key) {
            files[key] = f2[key];
          });
        }
      } else if (signature.ignored
        || signature.isSymbolicLink
        || (signature.size > (1024 * 1024 * 20))
        || fn.indexOf('.harvest.sig') != -1
        || fn.indexOf('core') != -1
        || fn.indexOf('junk') != -1
        || fn.indexOf('.lock') != -1
        || fn.indexOf('.log') != -1
        || fn.indexOf('.txt') != -1
        || fn.indexOf('.ack') != -1
        || fn.indexOf('.cnf') != -1
        || fn.indexOf('.out') != -1
        || fn.indexOf('.gz') != -1
        || fn.indexOf('.tbz2') != -1
        || fn.indexOf('.tgz') != -1
        || fn.indexOf('.tar') != -1
        || fn.indexOf('.zip') != -1) {
        signature.ignored = true;
        files[fn] = signature;
      } else {
        if (signature.isFile) {
          signature.hash = hash(fs.readFileSync(fn, 'utf8'));
        }
        files[fn] = signature;
      }
    }
  }

  return files;
}


function getEcho(cmd) {
  var proc = cproc.spawnSync('sh', ['-c', 'echo ' + cmd]);
  var sout = proc.stdout.toString();
  return sout.split('\n');
}


function getTitle() {
  var title = {};
  title.uname = getEcho('$(uname -n|tr "A-Z" "a-z")')[0];
  // get current date/time
  var args = (getEcho('$(date +"%G/%m/%d %H:%M:%S")')[0]).split(' ');
  title.date = args[0];
  if (args.length > 1)
    title.time = args[1];
  title.version = '1.2';
  return title
}


function Usage(err) {
  console.error(err);
  console.error('Usage: ' + require('path').basename(process.argv[1]) + ' sample deployment | verify deployment');
  process.exit(1);
}


function sample(argv) {
  argv = [].concat(argv);       // insure input is an array

  if (IsNull(argv[0])) {
    Usage('Deployment directory name is missing');
  }

  var details = {};
  details.title = getTitle();
  details.title.deployment_dir = argv.shift();

  details.files = sampleFiles(details.title.deployment_dir);

  details.title.file_count = Object.keys(details.files).length;
  return details;
}


function verify(argv) {
  argv = [].concat(argv);       // insure input is an array

  var controlFile = argv.shift();
  if (IsNull(controlFile)) {
    Usage('Control file name is missing');
  }

// get the control file input
  var control = null;
  try {
    control = JSON.parse(fs.readFileSync(controlFile));
  } catch (e) {
    console.error(e.toString());
    console.error('Control input "' + controlFile + '" has invalid content');
    process.exit(1);
  }

//   see if the deployment directory has an over-ride
  var deploymentDir = control.title.deployment_dir;
  if (argv.length > 0)
    deploymentDir = argv.shift();

// sample current state
  var details = sample(deploymentDir);

  var report = [];
  var section = null;

  var exceptions = 0;

  report.push(section = {section: 0, title: 'Comparing ' + details.title.uname + ' to ' + control.title.uname});
  section.local = details.title;
  section.control = control.title;

  report.push(section = {
    section: 1,
    title: 'Files missing in local deployment that exist in control ' + control.title.uname
  });
  section.lines = [];
  Object.keys(control.files).forEach(function (key, index) {
    var ctl = control.files[key];
    var dtl = details.files[key];
    if (IsNull(dtl)) {
      section.lines.push(key);
      ++exceptions;
    }
  });
  if (section.lines.length <= 0)
    section.lines.push('NONE');

  report.push(section = {
    section: 2,
    title: 'Files extra in local deployment that do not exist in control ' + control.title.uname
  });
  section.lines = [];
  Object.keys(details.files).forEach(function (key, index) {
    var dtl = details.files[key];
    var ctl = control.files[key];
    if (IsNull(ctl)) {
      section.lines.push(key);
      ++exceptions;
    }
  });
  if (section.lines.length <= 0)
    section.lines.push('NONE');

  report.push(section = {section: 3, title: 'Files with incorrect hash as compared to control ' + control.title.uname});
  section.lines = [];
  Object.keys(details.files).forEach(function (key, index) {
    var dtl = details.files[key];
    var ctl = control.files[key];
    if (!IsNull(dtl) && !IsNull(ctl)) {
      if (!IsNull(ctl.hash)) {
        if (ctl.hash !== dtl.hash) {
          section.lines.push(key);
          ++exceptions;
        }
      }
    }
  });
  if (section.lines.length <= 0)
    section.lines.push('NONE');

  report.push(section = {
    section: 9,
    title: 'Summary of verification of ' + details.title.uname + ' to ' + control.title.uname
  });
  section.exceptions = exceptions.toString();

  return report;
}


function Main() {
  var argv = process.argv.slice(2);               // actual arg-list

  var cmd = argv.shift();
  switch (cmd) {
    default:
      Usage(cmd + ' is an unknown command');
      break;

    case 'sample':
      var details = sample(argv);
      console.log(JSON.stringify(details, null, 2));
      break;

    case 'verify':
      var details = verify(argv);
      console.log(JSON.stringify(details, null, 2));
      break;
  }
}


const fs = require('fs');
const nx = require('../util/util.js');
const path = require('path');
const readline = require('readline');
const cproc = require('child_process');
const hash = require('md5');

Main();


// flush output then exit
process.stdout.once('drain', function () {
  process.exit(0);
});
process.stdout.write('');
