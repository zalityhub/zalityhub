#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>


#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <string>
#include <errno.h>


#include "json/json.h"


using namespace std;


Json::Value
readJson(string fname)
{
    Json::Value ret;
    Json::Reader reader;

    std::ifstream test(fname, std::ifstream::binary);
    Json::Value root;
    bool parsingSuccessful = reader.parse(test, root, false);
    if( ! parsingSuccessful )
		ret["error"] = reader.getFormatedErrorMessages();
	else
		ret["root"] = root;
    return ret;
}


void
Drill(Json::Value json, string pfx, string key)
{
	Json::Value::Members members = json[key].getMemberNames();
    for (auto fit = members.begin(); fit != members.end(); ++fit) {
		const string& key = *fit;
		cout << pfx << key << endl;
		Drill(json, pfx + "." + key, key);
	}
}


int
main()
{

	Json::Value control = readJson("junk");
	control = control["root"];

// iterate each member of control files
	Json::Value::Members files = control["files"].getMemberNames();
    for (auto fit = files.begin(); fit != files.end(); ++fit) {
      const string& fkey = *fit;
	  Drill(control["files"], "", fkey);
	}

	return 0;
}
