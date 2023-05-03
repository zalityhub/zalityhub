const inspect = require('util').inspect;
const stringify = require('json-stringify-safe');

const Imap = require('imap');

const nx = require('../util/util.js');
const Config = nx.getEnv(['mail'], true);

const zality = Config.zality;
const auth = zality.auth;
const host = zality.host;


const imap = new Imap({
user: auth.user, password: auth.password, host: host.name, port: host.port, tls: host.tls
});


const emails = [];

function openInbox(cb) {
  imap.openBox('INBOX', true, cb);
}
 
imap.once('ready', function() {
  openInbox(function(err, box) {
    if (err) throw err;
    const f = imap.seq.fetch('1:3', {
      bodies: 'HEADER.FIELDS (FROM TO SUBJECT DATE)',
      struct: true
    });
    f.on('message', function(msg, seqno) {
      // console.log('Message #%d', seqno);
      const prefix = '(#' + seqno + ') ';
      msg.on('body', function(stream, info) {
        let buffer = '';
        stream.on('data', function(chunk) {
          buffer += chunk.toString('utf8');
        });
        stream.once('end', function() {
          // console.log(prefix + 'Parsed header: %s', inspect(Imap.parseHeader(buffer)));
        });
      });
      msg.once('attributes', function(attrs) {
        // console.log(prefix + 'Attributes: %s', inspect(attrs, false, 8));
      });
      msg.once('end', function() {
        console.log(prefix + 'Finished');
        emails.push(msg);
      });
    });
    f.once('error', function(err) {
      console.log('Fetch error: ' + err);
    });
    f.once('end', function() {
      console.log('Done fetching all messages!');
      imap.end();
    });
  });
});
 
imap.once('error', function(err) {
  console.log(err);
});
 
imap.once('end', function() {
  console.log('Connection ended');
  console.log(stringify(emails.length, null, 4));
});
 
imap.connect();
