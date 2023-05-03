#include <iostream>
#include <fstream>
#include <json/json.h> // or jsoncpp/json.h , or json/json.h etc.

using namespace std;

Json::Value signature;
Json::Value specs;

int main()
{

    signature["name"] = "signature";
	specs["name"] = "specs";
	signature["specs"] = specs;

	Json::StreamWriterBuilder builder;
	builder["commentStyle"] = "None";
	builder["indentation"] = "   ";

	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	std::ofstream outputFileStream("/dev/stdout");
	writer -> write(signature, &outputFileStream);
	puts("");
	return 0;
}
