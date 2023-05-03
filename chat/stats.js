exports.Stats = class Stats {

  nx = require('../util/util.js');

  _vmethod = function (it) {
    return it;
  }

  constructor(vmethod) {
    const self = this;
    if (vmethod)
      self._vmethod = vmethod;
  }

  median(array) {
    const self = this;
    return self.calc(array, 0.50);
  }

  calc(array, q) {
    const self = this;
    const sorted = self.nx.CloneObj(self.sort(array));
    const pos = ((sorted.length) - 1) * q;
    if (pos < 0)
      return 0.0;
    const base = Math.floor(pos);
    const rest = pos - base;
    if ((sorted[base + 1] !== undefined)) {
      return self._vmethod(sorted[base]) + rest * (self._vmethod(sorted[base + 1]) - self._vmethod(sorted[base]));
    } else {
      return self._vmethod(sorted[base]);
    }
  }

  sort(array) {
    const self = this;
    return self.nx.CloneObj(array.sort(function (a, b) {
      return self._vmethod(a) - self._vmethod(b);
    }));
  }

  sum(array) {
    const self = this;
    return array.reduce(function (a, b) {
      return a + self._vmethod(b);
    }, 0);
  }

  average(array) {
    const self = this;
    return self.sum(array) / (array.length ? array.length : 1);
  }

  stdev(array) {
    const self = this;
    let i, j, total = 0, mean = 0, diffSqredArr = [];
    for (i = 0; i < array.length; i += 1) {
      total += self._vmethod(array[i]);
    }
    mean = total / array.length;
    for (j = 0; j < array.length; j += 1) {
      diffSqredArr.push(Math.pow((self._vmethod(array[j]) - mean), 2));
    }
    return (Math.sqrt(diffSqredArr.reduce(function (firstEl, nextEl) {
      return firstEl + nextEl;
    }) / array.length));
  }
}
