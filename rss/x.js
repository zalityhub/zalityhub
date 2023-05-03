const stringify = require('json-stringify-safe');
const Parser = require('rss-parser');
const parser = new Parser();

(async () => {

  const url = 'https://www.google.com/alerts/feeds/16767096407436880812/1781270674870871084';
  let feed = await parser.parseURL(url);
  console.log(feed.title);

  feed.items.forEach(item => {
    console.log(stringify(item));
  });

})();
