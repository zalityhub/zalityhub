const fs = require('fs');
const stringify = require('json-stringify-safe');
const http = require('http');
const pUrl = require('url');
const nx = require('@zality/nxmesh');


const CallControl = {};
CallControl.apiKey = 'KEY0185D52CEEF26CBA888211D708D87BE3_mVine39rFf32gIzvOr1uz1';
CallControl.activeCalls = {};

CallControl.dialers = [];
CallControl.dialers.push({nbr: 0, connection_id: '2086382349288736608', from: '8084375810'});
CallControl.dialers.push({nbr: 1, connection_id: '2086383982257112140', from: '8085150110'});

Telnyx = {api: [], calls: []};
Telnyx.api.push(require('telnyx')(CallControl.apiKey));
Telnyx.api.push(require('telnyx')(CallControl.apiKey));


function SecretDialEvent(ev) {
  const callId = ev.payload.call_leg_id;
  const ccmd = CallControl.activeCalls[callId];

  if (!nx.isObject(ccmd)) {
    nx.logError(`No call command for ${callId}`);
    return false;   // must have a call cmd
  }

  if (ev.event_type === 'call.initiated') {
    if (ccmd.state === 'dialing') {
      nx.log(`First sdial complete\n    Placing second sdial: ${ccmd.opts.to}`);
      ccmd.opts.type = 'sdial';
      dial({...ccmd.opts, ...{channel: 1}}, function (err, call) {
        if (err)
          return nx.logError(err.toString());
        call.ccmd.state = 'second';
        call.ccmd.f_callId = callId;
        nx.log(`Placed second sdial: ${call.ccmd.callId}`);
      });
      return true;
    }
    return false;
  }

  if (ev.event_type === 'call.answered') {
    if (ccmd.state === 'second') {
      nx.log(`Initiate hangup of first sdial: ${ccmd.f_callId}`);
      hangup({callId: ccmd.f_callId}, function (err, rsp) {   // hangup first
        if (err)
          return nx.logError(err.toString());
        nx.log(`Hangup of first sdial ${stringify(rsp, null, 2)}`);
      });
      return true;
    }
    return false;
  }

  return false;
}


function CallProgressEvent(ev) {
  ev = ev.data;
  const callId = ev.payload.call_leg_id;
  const ccmd = CallControl.activeCalls[callId];

  if (nx.isObject(ccmd)) {
    ev.ccmd = ccmd;   // add ccmd, if available
  }

  nx.log(`${ev.event_type.toString()}\n${stringify(ev, null, 2)}`);

  if (nx.isObject(ccmd) && ccmd.type === 'sdial') { // secret dial
    if (SecretDialEvent(ev))
      return;     // was handled
  }

  switch (ev.event_type) {
    default:
      break;

    case 'call.initiated':
      break;

    case 'call.answered':
      break;

    case 'call.playback.started':
      break;

    case 'call.playback.ended':
      nx.log(`Playback ended; Initiate hangup: ${callId}`);
      hangup({callId: callId}, function (err, rsp) {
        if (err)
          return nx.logError(err.toString());
        nx.log(stringify(rsp, null, 2));
      });
      break;

    case 'call.hangup':
      if (nx.isObject(ccmd)) {
        nx.log(`removing callId: ${ccmd.callId}`);
        Telnyx.calls[ccmd.opts.channel] = null;
        delete CallControl.activeCalls[callId];
      }
      break;
  }
}


function getFile(opts, cb) {
  if (!opts.file) {
    const etext = nx.logError(`missing file`);
    if (cb)
      cb(etext);
    return etext;
  }

  try {
    fs.readFile(opts.file, function (err, content) {
      if (err) {
        const etext = nx.logError(err.toString());
        if (cb)
          cb(etext);
        return etext;
      }
      if (cb)
        cb(null, content);
      return;
    });
  } catch (err) {
    nx.logError(err.toString());
    if (cb)
      cb(err);
    return;
  }
}


function hangup(opts, cb) {
  let call = null;

  if (opts.channel)
    call = Telnyx[opts.channel];

  if (opts.callId) {
    const ccmd = CallControl.activeCalls[opts.callId];
    if (!nx.isObject(ccmd)) {
      const etext = nx.logError(`no such call: ${opts.callId}`);
      if (cb)
        cb(etext);
      return etext;
    }
    call = Telnyx.calls[ccmd.opts.channel];
  }

  if (!call) {
    const etext = nx.logError(`must have callId or channel`);
    if (cb)
      cb(etext);
    return etext;
  }

  call.hangup({call_control_id: call.ccmd.callId}, function (err, rsp) {
    if (cb) {
      nx.log(`hangup ${call.ccmd.callId} complete`);
      cb(err, rsp);
    }
  });
}


function fixNbr(nbr) {
  nbr = nbr.toString();
  if (!nbr.startsWith('+'))
    return `+1${nbr}`;
  return nbr;
}


function dial(opts, cb) {
  if (!opts.to) {
    const etext = nx.logError(`missing to`);
    if (cb)
      cb(etext);
    return etext;
  }
  if (!opts.channel)
    opts.channel = 0;

  const dialer = CallControl.dialers[opts.channel];
  if (!nx.isObject(dialer)) {
    const etext = nx.logError(`no dialer ${dialer}`);
    if (cb)
      cb(etext);
    return etext;
  }

  const ccmd = {};
  ccmd.type = NullToString(opts.type, 'dial');
  ccmd.dialer = dialer;
  ccmd.opts = opts;
  ccmd.state = 'dialing';

  const dopts = {connection_id: dialer.connection_id, to: fixNbr(opts.to), from: fixNbr(dialer.from)};
  if (opts.audio)
    dopts.audio_url = `https://hbray.me/wav?file=${opts.audio}`;

  nx.log(`dial begin: ${stringify(dopts, null, 2)}`);
  Telnyx.api[opts.channel].calls.create(dopts, function (err, response) {
    if (err) {
      nx.logError(`dial error: ${err.toString()}`);
      if (cb)
        cb(err);
      return;
    }

    if (!response)
      return;

    const call = response.data;

    ccmd.callId = call.call_leg_id;
    nx.log(`dial end: ${stringify(ccmd, null, 2)}`);
    call.ccmd = ccmd;
    CallControl.activeCalls[ccmd.callId] = ccmd;
    Telnyx.calls[opts.channel] = call;
    nx.log(`added callId: ${ccmd.callId}`);
    if (cb)
      cb(null, call);
  });
}


function sdial(opts, cb) {
  if (!opts.to) {
    const etext = nx.logError(`missing to`);
    if (cb)
      cb(etext);
    return etext;
  }

  // first dial with limited opts
  fopts = {};
  fopts.channel = 0;    // first channel
  fopts.to = opts.to;
  fopts.type = 'sdial';
  nx.log(`Placing first sdial: ${fopts.to}`);
  dial(fopts, function (err, call) {
    if (err) {
      nx.logError(err.toString());
      if (cb)
        cb(err);
      return;
    }
    const ccmd = call.ccmd;
    ccmd.opts = opts;       // save full opts object
    nx.log(`Placed first sdial: ${ccmd.callId}`);
    if (cb)
      cb(null, call);
  });
}

function lookup(opts, cb) {
  if (!opts.nbr) {
    const etext = nx.logError('missing number');
    if (cb)
      cb(etext);
    return etext;
  }

  Telnyx.api[0].numberLookup.retrieve(`+1${opts.nbr.toString()}`, function (err, res) {
    if (err) {
      const etext = nx.logError(err.toString());
      if (cb)
        cb(etext);
      return etext;
    }
    if (cb)
      return cb(0, res);
    nx.log(stringify(res, null, 4));
  });
  return;

  try {
    const q = JSON.parse(CloneObj(res[1]));
    let text = q.data.phone_number + ':';
    text += '\nCountry:           ' + q.data.country_code;
    text += '\nType:              ' + q.data.carrier.type;
    text += '\nCarrier:           ' + q.data.carrier.name;
    text += '\nName:              ' + q.data.caller_name.caller_name;
    text += '\nCity:              ' + q.data.portability.city;
    text += '\nState:             ' + q.data.portability.state;
    return res.end(text);
  } catch (e) {
    console.log(e.toString());
    return res.end('error');
  }
}


const server = http.createServer((req, res) => {

  function processReq(req, res, body) {

    const url = pUrl.parse(req.url, true);
    const opts = url.query;
    switch (url.pathname) {
      default:
        nx.log(url.pathname.toString());
        console.log(`${stringify(body, null, 2)}\n`);
        res.end('\n');
        break;

      case '/hook':
      case '/hook.0':
      case '/hook.1':
        CallProgressEvent(body);
        res.end('\n');
        break;

      case '/texml':
        getFile(opts, function (err, content) {
          if (err) {
            nx.logError(err.toString());
            return res.end('error\n');
          }
          res.setHeader('Access-Control-Allow-Origin', '*');
          res.writeHead(200, {'Content-type': 'text/plain'});
          res.write(content.toString());
          res.end();
        });
        break;

      case '/mp3':
        getFile(opts, function (err, content) {
          if (err) {
            nx.logError(err.toString());
            return res.end('error\n');
          }
          res.setHeader('Access-Control-Allow-Origin', '*');
          res.writeHead(200, {'Content-type': 'audio/mpeg'});
          res.write(content);
          res.end();
        });
        break;

      case '/wav':
        getFile(opts, function (err, content) {
          if (err) {
            nx.logError(err.toString());
            return res.end('error\n');
          }
          setTimeout(() => {
            res.setHeader('Access-Control-Allow-Origin', '*');
            res.writeHead(200, {'Content-type': 'audio/wave'});
            res.write(content);
            res.end();
          }, 3000);
        });
        break;

      case '/hangup':
        hangup(opts, function (err, call) {
          if (err) {
            nx.logError(err.toString());
            return res.end('error\n');
          }
          res.end(`hangup ${opts}\n`);
        });
        break;

      case '/dial':
        dial(opts, function (err, call) {
          if (err) {
            nx.logError(err.toString());
            return res.end('error\n');
          }
          res.end(`dialed ${opts.to}\n`);
        });
        break;

      case '/sdial':
        sdial(opts, function (err, call) {
          if (err) {
            nx.logError(err.toString());
            return res.end('error\n');
          }
          res.end(`dialed ${opts.to}\n`);
        });
        break;

      case '/sms':
        sms(opts);
        break;

      case '/nodejs/lookup':
      case '/lookup':
        lookup(opts, function (err, look) {
          if (err) {
            nx.logError(err.toString());
            return res.end('error\n');
          }
          res.end(`${stringify(look.data,null,4)}\n`);
        });
        break;
    }
  }

  GetBody(req, function (err, req) {
    if (err) {
      nx.logError(err.toString());
      return res.end('error\n');
    }
    processReq(req, res, req.body);
  });
});


const argv = process.argv.slice(2);    // arg list

switch (argv.length) {
  case 0:
    server.listen(3000, 'localhost', () => {
      nx.log(`Listening on http://localhost:3000`);
    });
    break;

  default:
    for (let i = 0, ilen = argv.length; i < ilen; ++i) {
      if (argv[0].toString().allDigits()) {
        Telnyx.api[0].numberLookup.retrieve(`+1${argv[i].toString()}`, function (err, res) {
          if (err)
            return nx.logError(err.toString());
          console.log(stringify(res, null, 4));
        });
      }
    }
}
