const nx = require('@zality/nxmesh');

const fs = require('fs');
const path = require('path');
const mkdirp = require('mkdirp')
const pUrl=require('url');
const puppeteer = require("puppeteer");
const {sed} = require("sed-lite");


// we're using async/await - so we need an async function, that we can run
const capture = async (rss, width, height) => {
  // open the browser and prepare a page
  const browser = await puppeteer.launch()
  const page = await browser.newPage()

  width = NullToString(width, 2880);
  height = NullToString(height, 1920);

  // set the size of the viewport, so our screenshot will have the desired size
  await page.setViewport({
      width: parseInt(width),
      height: parseInt(height)
  })

  if (! IsArray(rss) )
    rss = [rss];    // if not array, make it so

// for each item
  for(let i = 0; i < rss.length; ++i) {
    const item = rss[i];
    const link = item.link;

    console.log('Capture: ' + link);

    const u = pUrl.parse(link, true);
    const p = u.path.substring(1); // without leading '/'
    const f = path.basename(p, '.html');

    const ssp  = 'images/' + f + '.png';
    const html = 'html/'   + f + '.html';
    mkdirp('images');
    mkdirp('html');

    if( ! fileExists(ssp) )
    try {
      await page.goto(link);
      await page.screenshot({
        path: ssp,
        fullPage: true
      });


// write the html
      const ws = fs.createWriteStream(html);
      ws.write(`<!DOCTYPE html>\n`);
      ws.write(`<html>\n`);
      ws.write(`<head>\n`);
      ws.write(`<title>${item.title}\n</title>\n`);
      ws.write(`<style>\nh1{text-align: left;}\n</style>\n`);
      ws.write(`</head>\n`);
      ws.write(`<body>\n`);
      ws.write(`<h1>${item.title}</h1>\n`);
      ws.write(`<a href="${item.link}">\n`);
      ws.write(`<img align="left" border="0" alt="Investments" src="../${ssp}">\n`);
      ws.write(`</a>\n`);
      ws.write(`</body>\n`);
      ws.write(`</html>\n`);
      ws.on('finish', () => { ws.end(); });
      // fs.writeFileSync(html, `<!DOCTYPE html>\n<html>\n<body>\n<p>\n${item.title}: <a href="${item.link}">\n<img align="left" border="0" alt="Investments" src="../${ssp}">\n</a>\n</p>\n</body>\n</html>\n`);
    } catch (err) {
      console.log(`capture(${link}) ` + err);
      try {
        fs.unlinkSync(ssp);
        fs.unlinkSync(html);
      } catch(err) {}
    }
  }

  // close the browser
  await browser.close();
};


const getRss = async (item) => {
  const rss = [];
  let Parser = require('rss-parser');
  let parser = new Parser();
  try {
    let feed = await parser.parseURL(item);
    feed.items.forEach(item => {
      rss.push(item);
    });
  } catch (err) {
    console.log(`getRss(${item}) ` + err);
  }
  return rss;
};


const argv = process.argv.slice(2);
if( argv[0] === '-b' ) {
} else {
  try {
    getRss('https://www.google.com/alerts/feeds/16767096407436880812/1781270674870871084').then(rss => {
      for(let r = 0; r < rss.length; ++r) {
        let link = rss[r].link;
        const r1 = sed('s/.*url=//g');
        const r2 = sed('s/.ct=.*//g');
        rss[r].link = r2(r1(link));
      }
      capture(rss, 640, 480).then(r => {console.log('done')});
    });
  } catch (err) {
    console.log('getRss ' + err);
  }
}
