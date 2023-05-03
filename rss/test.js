let Parser = require('rss-parser');
let parser = new Parser();

console.log('<!DOCTYPE html>\n<html data-theme="light" lang="en"><head>');

(async () => {
  let feed = await parser.parseURL('https://www.google.com/alerts/feeds/16767096407436880812/1781270674870871084');
  console.log('<title>'+feed.title+'</title>');

  feed.items.forEach(item => {
    console.log('<a href="'+item.link+'">'+item.title+'</a></br>');
  });

})();
