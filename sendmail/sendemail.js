const fs = require('fs');
const stringify = require('json-stringify-safe');

const nx = require('../util/util.js');
const Config = nx.getEnv(['mail'], true);

const nodeoutlook = require('nodejs-nodemailer-outlook');

const mail = {};

mail.auth = Config.auth;
mail.from = Config.from;

mail.to = 'hbray@zality.com';
mail.subject = 'Hey you';
mail.html = '<b>This is bold text</b>';
mail.text = 'This is text version!';
mail.replyTo = Config.from;
mail.attachments = [];

mail.attachments.push({
  filename: 'text1.txt',
  content: 'content of text1'
}); // poe

mail.attachments.push({
  filename: 'text2.txt',
  content: Buffer.from('binary string')
});   // binary buffer as an attachment

mail.attachments.push({
  filename: 'text3.txt',
  path: './sfile.txt' // stream this file
});   // file on disk as an attachment

mail.attachments.push({
  path: './pfile.txt'
});   // filename and content type is derived from path

mail.attachments.push({
  filename: 'text4.txt',
  content: fs.createReadStream('./afile.txt')
});   // stream as an attachment

mail.attachments.push({
  filename: 'text.bin',
  content: 'text.bin',
  contentType: 'text/plain'
});   // define custom content type for the attachment

mail.attachments.push({
  filename: 'license.txt',
  path: 'https://raw.github.com/nodemailer/nodemailer/master/LICENSE'
});   // use URL as an attachment

mail.attachments.push({
  filename: 'base64.txt',
  content: 'aGVsbG8gd29ybGQh',
  encoding: 'base64'
});   // encoded string as an attachment

mail.attachments.push({
  path: 'data:text/plain;base64,aGVsbG8gd29ybGQ='
});   // data uri as an attachment

mail.attachments.push({
  raw: 'Content-Type: text/plain\r\n' +
    'Content-Disposition: attachment;\r\n' +
    '\r\n' +
    'use pregenerated MIME node'
}); // use pregenerated MIME node


mail.onError = function (e) {
  console.log(e)
};

mail.onSuccess = function (i) {
  console.log(i)
};

nodeoutlook.sendEmail(mail);
