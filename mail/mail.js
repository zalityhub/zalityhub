const fs = require('fs');
const stringify = require('json-stringify-safe');
const inspect = require('util').inspect;

const nx = require('../util/util.js');
const Config = nx.getEnv(['mail'], true);

const Mail = require('./nxmail.js');


// const Credentials = Config.zality;
const Credentials = Config.chat;


const LogHandle = {"enabled": true, "file": "mail.log", "options": {"flags": "a"}};
LogHandle.pipes = {
  console: (text) => {
    console.log(text.slice(0, -1));
  }
};


function Log(text) {
  nx.writeHandle(LogHandle, `${text}\n`);
}


function getEmails(config) {

  function save(emails) {
    for (let i = 0; i < emails.length; ++i)
      fs.writeFileSync(`j${i}.txt`, emails[i].body.toString());
  }


  function get(box, search) {

    imap.connect(config).then((config) => {
      Log(`Connected: ${config.auth.user}:${stringify(config.host)}`);

      const options = {box: box, readonly: true, search: [search]};
      imap.open(options).then((options) => {
        Log('box open');
        imap.search(options).then((messages) => {
          Log(`${messages.length} emails`);

          options.messages = messages;
          imap.fetch(options).then((emails) => {
            Log(`received ${emails.length} emails`);
            if (imap.trace)
              fs.writeFileSync('trace.log', `trace: ${stringify(imap.trace, null, 4)}`);

            imap.end();

            // save(emails);

            emails.forEach((email) => {
              fs.writeFileSync(`${email.seq}.txt`, email.body.toString());
              const parsed = imap.parse(email);
              fs.writeFileSync(`${email.seq}.json`, stringify(parsed, null, 4));
            });
          });
        });
      });
    });
  }

  const imap = new Mail.Imap('main');

  get('INBOX', 'UNSEEN');
}

getEmails(Credentials.imap);


function send(config, text) {
  const smtp = new Mail.Smtp('main');

  smtp.connect(config);

  const options = {
    from: smtp.options.auth.user,
    to: 'chat@zality.com',
    subject: 'Sending Email using Node.js'
  };

  smtp.send(options, text).then((info) => {
    Log(stringify(info));
  });
}

// send(Credentials.smtp, 'good evening');
