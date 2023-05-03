exports.stats = {
  arr1: null,
  arr2: null,
  v1: null,
  v2: null,
  commons: null,
  vAB: null,
  a: null,
  b: null,
  ratio: null
}

exports.compare = function (string1, string2) {
// Find the cosine similarity between two strings //
  let cosine = {
    calculateSimilarity: function (string1, string2) {
      stats.arr1 = cosineLib.getTokens(string1);
      stats.arr2 = cosineLib.getTokens(string2);

      // Define Commons Array //
      stats.commons = stats.arr1;

      // Word Frequency as Vectors //
      stats.v1 = cosineLib.computeFrequency(stats.arr1, stats.commons)
      stats.v2 = cosineLib.computeFrequency(stats.arr2, stats.commons)

      // Calculate Vector A.B //
      stats.vAB = cosineLib.computeVectorAB(stats.v1, stats.v2)

      // Abs Vector A and B //
      stats.a = cosineLib.absVector(stats.v1)
      stats.b = cosineLib.absVector(stats.v2)

      // cosine similarity //
      stats.ratio = cosineLib.similarity(stats.vAB, stats.a, stats.b)
      return stats.ratio
    }
  }

  let cosineLib = {
    // Convert strings into arrays (Tokenize, toLowerCase) //
    getTokens: function (str) {
      return tokensLib
        .tokenize(str)
        .filter(function (val) {
          if (!isNaN(val))
            return false;
          else
            return true;
        })
        .map((word) => {
          return word.toLowerCase();
        });
    },

    // Find Common words Frequency //
    computeFrequency: function (arr, commons) {
      return commons.map((word) => {
        return arr.reduce((f, element) => {
          if (element === word)
            return f += 1;
          else
            return f += 0;
        }, 0)
      })
    },

    // Calculate Vector A.B //
    computeVectorAB: function (v1, v2) {
      return v1.reduce((sum, f, index) => {
        return sum += (f * v2[index]);
      }, 0)
    },

    // Calculate ||a|| and ||b|| //
    absVector: function (v) {
      return Math.sqrt(v.reduce((sum, f) => {
        return sum += (f * f);
      }, 0));
    },

    // cosine similarity //
    similarity: function (vAB, a, b) {
      if ((a * b) === 0)
        return 0.0;
      return (vAB / (a * b));
    }
  }


  let tokensLib = {
    // Split the string that contains any character
    // other then alpha-numeric characters.
    tokenize: function (text) {
      return text.split(/[^A-Za-z0-9]+/);
    }
  }

  const stats = exports.stats;
  return cosine.calculateSimilarity(string1, string2)
}
