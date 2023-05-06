const fs = require('fs');
const stringify = require('json-stringify-safe');
const inspect = require('util').inspect;

const nx = require('../util/util.js');
const Mail = require('./nxmail.js');


const Env = nx.getEnv(['mail'], true);


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

  return new Promise(function (resolve, reject) {
    try {
      const imap = new Mail.Imap('main');

      const box = 'INBOX';
      const search = 'UNSEEN';

      imap.connect(config).then((config) => {
        Log(`Connected: ${config.auth.user}:${stringify(config.host)}`);

        const options = {box: box, readonly: true, search: [search]};
        options.readonly = false;
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
              resolve(emails);
            });
          });
        });
      });
    } catch (err) {
      reject(err);
    }
  });
}


function send(config, dest, subject, text) {
  const smtp = new Mail.Smtp('main');

  smtp.connect(config);

  const options = {
    from: smtp.options.auth.user,
    to: dest,
    subject: subject
  };

  smtp.send(options, text).then((info) => {
    Log(stringify(info));
  });
}


// Start Here
//

const config = Env.chat;
// const config = Env.zality;

getEmails(config.imap).then((emails) => {
  emails.forEach((email) => {
    const simpleParser = require('mailparser').simpleParser;
    simpleParser(email.body, (err, mail) => {
      let from = mail.headers.get('from');
      from = from.value[0].address.toString().trim();
      const text = mail.text.trim();

      console.log(from);
      console.log(mail.subject);
      console.log(mail.text);

      if (config.options.save) {
        fs.writeFileSync(`${email.seq}.txt`, email.body.toString());
        fs.writeFileSync(`${email.seq}.json`, stringify(mail, null, 4));
      }

      if (config.options.isChat) {
        const chat = require('../chat/qchat.js');
        const qchat = new chat.Qchat('main');
        qchat.Query(text).then((result) => {
          console.log(`Question: ${text}\n\n\nResponse: ${result.text}\n`);
          return send(config.smtp, from, `From ${config.smtp.auth.user}`,
            `Question: ${text}\n\n\nResponse: ${result.text}\n`);
        }).catch((err) => {
          console.error(err.toString());
        });
      }
    });
  });
});

// const dest = 'chat@zality.com';
// send(config.smtp, dest, `From ${config.smtp.auth.user}`, 'good evening');
