const inspect = require('util').inspect;
const stringify = require('json-stringify-safe');

const Imap = require('imap');
const {simpleParser} = require('mailparser');

const nx = require('../util/util.js');
const Config = nx.getEnv(['mail'], true);

const zality = Config.zality;
const auth = zality.auth;
const host = zality.host;


const imapConfig = {
  user: auth.user, password: auth.password, host: host.name, port: host.port, tls: host.tls
};

const imap = new Imap(imapConfig);

const emails = [];

imap.once('ready', () => {
  imap.openBox('INBOX', true, (err, box) => {
    if (err) throw err;
    imap.search(['UNSEEN'], (err, results) => {
      if (err) throw err;
      const f = imap.fetch(results, {bodies: ''});
      f.on('message', (msg, seqno) => {
        const headers = msg.headers;
        let buffer = '';
        msg.on('body', (stream, info) => {
          stream.on('data', (chunk) => {
            buffer += chunk.toString('utf8');
          });
          stream.once('end', () => {
            simpleParser(buffer, (err, parsed) => {
              if (err) throw err;
              const {text} = parsed;
              emails.push(parsed);
            });
          });
        });
      });
      f.once('error', (err) => {
        throw err;
      });
      f.once('end', () => {
        console.log('Done fetching all unseen emails!');
        console.log(stringify(emails.length, null, 4));
        // imap.end();
      });
    });
  });
});

imap.once('error', (err) => {
  console.error(err);
});

imap.once('end', () => {
  console.log('Connection ended');
  console.log(stringify(emails.length, null, 4));
});

imap.connect();
