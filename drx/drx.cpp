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
#include <errno.h>
#include <openssl/md5.h>

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>




#include "json/json.h"


using namespace std;


char *Me = (char*)"<unknown>";


void
Usage(string err)
{
    cerr << err << endl;
	cerr << "Usage: " << Me << " sample dir | verify dir control_file" << endl;
    exit(1);
}

void
Fatal(string err)
{
    cerr << err << endl;
	exit(1);
}


typedef int (*IterateDirCallBack_t)(const char *, const char *, const struct stat *, void *);

int
IterateDirPath(const char *dirPath, IterateDirCallBack_t callback, void *cbarg)
{
    char *dirp = strdup(dirPath);
    struct stat st;
    int ret = 0;

    while (dirp[strlen(dirp) - 1] == '/')
        dirp[strlen(dirp) - 1] = '\0';        // remove trailing slash chars

    if ((ret = lstat(dirp, &st)) || !S_ISDIR(st.st_mode)) {
        char err[1024];
        if (ret)
            snprintf(err, sizeof(err), "lstat of %s failed; error %s", dirp, strerror(errno));
        else
            snprintf(err, sizeof(err), "%s must be a directory name", dirp);
        if (callback)
            (void) (*callback)(err, dirp, NULL, cbarg);
        free(dirp);
        return -1;
    }

    DIR *dir = opendir(dirp);
    if (NULL != dir) {
        struct dirent *entry;

        for (; NULL != (entry = readdir(dir));) {
            // Skip dots
            if ('.' == entry->d_name[0] &&
                ('\0' == entry->d_name[1] || ('.' == entry->d_name[1] && '\0' == entry->d_name[2])))
                continue;        // do nothing

            int size = strlen(dirp) + strlen(entry->d_name) + 10;
            char *node = (char *) malloc(size);
            snprintf(node, size, "%s/%s", dirp, entry->d_name);

            if (0 == lstat(node, &st)) {
                if (callback) {
                    if ((ret = (*callback)(NULL, node, &st, cbarg)) != 0)
                        break;            // returned an error; skip out...
                }
            } else {
                char err[1024];
                snprintf(err, sizeof(err), "lstat of %s failed; error %s", node, strerror(errno));
                if (callback) {
                    if ((ret = (*callback)(err, node, NULL, cbarg)) != 0)
                        break;            // returned an error; skip out...
                }
            }
            free(node);
        }

        closedir(dir);
        dir = NULL;        // done
    } else {
        char err[1024];
        snprintf(err, sizeof(err), "opendir of %s failed; error %s", dirp, strerror(errno));
        if (callback)
            (void) (*callback)(err, dirp, NULL, cbarg);
        ret = -1;
    }

    free(dirp);
    return ret;
}


// buf needs to store 128 characters
char*
timespec2str(char *buf, uint len, const struct timespec *ts)
{
    static char tmp[128];
    int ret;
    struct tm t;

    if (buf == 0 || len == 0) {
        buf = tmp;
        len = sizeof(tmp);
    }

    tzset();
    if (localtime_r(&(ts->tv_sec), &t) == NULL)
        return strcpy(tmp, "!!localtime_r failed");

    ret = strftime(buf, len, "%F %T", &t);
    if (ret == 0)
        return strcpy(tmp, "!!strftime failed");
    len -= ret - 1;

    ret = snprintf(&buf[strlen(buf)], len, ".%09ld", ts->tv_nsec);
    if (ret >= len)
        return strcpy(tmp, "!!snprintf failed");

    return buf;
}


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
writeJson(Json::Value root, string fname)
{
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";

    std::unique_ptr <Json::StreamWriter> writer(builder.newStreamWriter());
    std::ofstream outputFileStream(fname);
    writer->write(root, &outputFileStream);
}


Json::Value
GetContent(string file, int size)
{
    Json::Value ret;

    int fd = open(file.c_str(), O_RDONLY);

    if (fd < 0) {
        char err[1024];
        snprintf(err, sizeof(err), "open of %s failed; error %s", file.c_str(), strerror(errno));
		ret["error"] = err;
        return ret;
    }

    char *content = (char *) calloc(1, size + 1);
    char *out = content;

    // read the content
    int rlen = read(fd, out, size);    // one big read
    if (rlen < 0) {
        char err[1024];
        snprintf(err, sizeof(err), "read of %s failed; error %s", file.c_str(), strerror(errno));
		ret["error"] = err;
        close(fd);
        return ret;
    }
    out += rlen;
    *out = '\0';

    close(fd);
	ret["content"] = (int64_t)content;
    return ret;
}


Json::Value
getNodeSignature(string node, const struct stat *sb)
{

    Json::Value signature;
    signature["name"] = node;

    signature["ignored"] = false;

    // set specs
    Json::Value specs;
    specs["dev"] = sb->st_dev;         // ID of device containing file
    specs["ino"] = sb->st_ino;         // inode number
    specs["mode"] = sb->st_mode;        // file type and mode
    specs["nlink"] = sb->st_nlink;       // number of hard links
    specs["uid"] = sb->st_uid;         // user ID of owner
    specs["gid"] = sb->st_gid;         // group ID of owner
    specs["rdev"] = sb->st_rdev;        // device ID (if special file)
    specs["size"] = sb->st_size;        // total size, in bytes
    specs["blksize"] = sb->st_blksize;     // blocksize for filesystem I/O
    specs["blocks"] = sb->st_blocks;      // number of 512B blocks allocated
    signature["specs"] = specs;

    // set node type
    Json::Value type;
    type["mode"] = sb->st_mode;
    type["isFile"] = S_ISREG(sb->st_mode);
    type["isDirectory"] = S_ISDIR(sb->st_mode);
    type["isBlockDevice"] = S_ISBLK(sb->st_mode);
    type["isCharacterDevice"] = S_ISCHR(sb->st_mode);
    type["isFIFO"] = S_ISFIFO(sb->st_mode);
    type["isSocket"] = S_ISSOCK(sb->st_mode);
    type["isSymbolicLink"] = S_ISLNK(sb->st_mode);
    signature["type"] = type;

    // set times
    Json::Value times;
    times["atime"] = timespec2str(NULL, 0, &sb->st_atim);  // time of last access
    times["mtime"] = timespec2str(NULL, 0, &sb->st_mtim);  // time of last modification
    times["ctime"] = timespec2str(NULL, 0, &sb->st_ctim);  // time of last status change
    signature["times"] = times;

    // set permissions
    Json::Value perms;

    Json::Value spec;
    spec["setuid"] = (sb->st_mode & S_ISUID) != 0;    // set-user-ID bit
    spec["setgid"] = (sb->st_mode & S_ISGID) != 0;    // set-group-ID bit
    spec["sticky"] = (sb->st_mode & S_ISVTX) != 0;    // sticky bit
    perms["special"] = spec;

    Json::Value owner;
    owner["r"] = (sb->st_mode & S_IRUSR) != 0;    // owner has read permission
    owner["w"] = (sb->st_mode & S_IWUSR) != 0;    // owner has write permission
    owner["x"] = (sb->st_mode & S_IXUSR) != 0;    // owner has execute permission
    perms["owner"] = owner;

    Json::Value group;
    group["r"] = (sb->st_mode & S_IRGRP) != 0;    // group has read permission
    group["w"] = (sb->st_mode & S_IWGRP) != 0;    // group has write permission
    group["x"] = (sb->st_mode & S_IXGRP) != 0;    // group has execute permission
    perms["group"] = group;

    Json::Value other;
    other["r"] = (sb->st_mode & S_IROTH) != 0;    // others have read permission
    other["w"] = (sb->st_mode & S_IWOTH) != 0;    // others have write permission
    other["x"] = (sb->st_mode & S_IXOTH) != 0;    // others have execute permission
    perms["other"] = other;

    signature["perms"] = perms;

    if (S_ISLNK(sb->st_mode) || S_ISREG(sb->st_mode)) {
        // get the hash
        Json::Value ret = GetContent(node, sb->st_size);
		if( ret.isMember("error") ) {		// error in read
			Fatal(ret["error"].asString());
		}
		char *content = (char*)ret["content"].asUInt64();
        if (content != NULL) {
            unsigned char *md5 = MD5((const unsigned char *) content, sb->st_size, NULL);
            char hash[(MD5_DIGEST_LENGTH * 2) + 2];
            char *h = hash;
            for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
                char c1, c2;
                if ((c1 = (md5[i] >> 4) + '0') > '9')
                    c1 += 7;
                if ((c2 = (md5[i] & 0xF) + '0') > '9')
                    c2 += 7;
                *h++ = c1;
                *h++ = c2;
                *h = '\0';        // terminate the output
            }
            signature["hash"] = hash;
            free(content);
        }
    }

    return signature;
}


static int
iterateNode(const char *err, const char *name, const struct stat *st, void *arg)
{
    Json::Value *files = (Json::Value *) arg;

    if (err != NULL) {    // this is an error
        Json::Value file;
        file["name"] = name;
        file["error"] = err;
        file["ignored"] = true;
        (*files)[name] = file;
        return -1;
    }

    (*files)[name] = getNodeSignature(name, st);

    if (S_ISDIR(st->st_mode))
        return IterateDirPath(name, iterateNode, arg);

    return 0;
}


Json::Value
Sample(char *argv[])
{
	char *dir = argv[0];

	if( dir == NULL )
		Usage("sample needs a deployment directory name");

    char *dirp = strdup(dir);
    while (dirp[strlen(dirp) - 1] == '/')
        dirp[strlen(dirp) - 1] = '\0';        // remove trailing slash chars

// build the info header
    Json::Value info;
    info["deployment_dir"] = dirp;
    info["version"] = "1.2";

    // current time
    char tim[128];
    time_t t = time(NULL);
    (void) strftime(tim, sizeof(tim), "%F %T", localtime(&t));
    info["time"] = tim;

    // get nodename...
    struct utsname utsname;
    uname(&utsname);
    char *nodename = (char *) utsname.nodename;
    for (char *s = nodename; *s; ++s)
        *s = tolower(*s);
    info["uname"] = nodename;

// iterate the directory adding all content to the sample
    Json::Value files;
    IterateDirPath(dirp, iterateNode, &files);
    info["file_count"] = files.size();

    Json::Value sample;
    sample["info"] = info;
    sample["files"] = files;

	free(dirp);
    return sample;
}


Json::Value
CheckFilesExist(Json::Value& local, Json::Value& control, string title)
{
	Json::Value report;
	Json::Value files;
	int exceptions = 0;

// Create Files missing in local vs deployment -- or -- vice versa
	report["title"] = title;

// iterate each member of control files
	Json::Value::Members members = control["files"].getMemberNames();
    for (auto it = members.begin(); it != members.end(); ++it) {
      const string& key = *it;
	  if( local["files"][key].empty() )
	  	files.append(key);
    }
	if( files.empty() )
		files.append("NONE");
	else
		exceptions += files.size();
	report["files"] = files;
	report["exceptions"] = exceptions;

	return report;
}


Json::Value
CheckFilesHash(Json::Value& local, Json::Value& control, string title)
{
	Json::Value report;
	Json::Value files;
	int exceptions = 0;

// Create Files with incorrect hash as compared to control
	report["title"] = title;

// iterate each member of local files
	Json::Value::Members members = local["files"].getMemberNames();
    for (auto it = members.begin(); it != members.end(); ++it) {
      const string& key = *it;
	  if( ! control["files"][key].empty() &&
	  	local["files"][key]["hash"].asString() != control["files"][key]["hash"].asString() )
	  	files.append(key);
    }
	if( files.empty() )
		files.append("NONE");
	else
		exceptions += files.size();
	report["files"] = files;
	report["exceptions"] = exceptions;

	return report;
}


vector<string> Bld(Json::Value o, string ckey)
{
	vector<string> keys;

	Json::Value::Members members = o.getMemberNames();
    for (auto it = members.begin(); it != members.end(); ++it) {
      const string& key = *it;
	  Json::Value v = o[key];
	  Json::ValueType vt = v.type();
	  if( vt == Json::arrayValue || vt == Json::objectValue )
	  	ckey += Bld(v, ckey + '.' + key);
	else
		ckey += key;
    }

	return ckey;
}


Json::Value
CheckFilesAttributes(Json::Value& local, Json::Value& control, string title)
{
	Json::Value report;
	Json::Value files;
	int exceptions = 0;

	report["title"] = title;

	Json::Value attributes = readJson("at.json");
	if( attributes.isMember("error") ) {		// error in read
		Fatal(attributes["error"].asString());
	}

	vector<string> keys;
	cout << Bld(attributes, "") << endl;

	return report;
}



Json::Value
Verify(char *argv[])
{

	char *sfile = argv[1];
	if( sfile == NULL )
		Usage("verify needs a sample control file name");

	Json::Value control = readJson(sfile);
	if( control.isMember("error") ) {		// error in read
		Fatal(control["error"].asString());
	}

	control = control["root"];
    if( control.empty() ) {
		Fatal(string("Sample control file '") + sfile + "' is empty");
	}

// sample current state
	char *dirp = argv[0];
	Json::Value local = Sample(&dirp);		// take a sample

	int exceptions = 0;
	Json::Value report;
    Json::Value section;


// Write title of the report
	section["title"] = string("") + "Comparing " + local["info"]["uname"].asString() + " to " + control["info"]["uname"].asString();

	section["local"] = local["info"];
	section["control"] = control["info"];
	report.append(section);


	section = CheckFilesExist(local, control, string("") + "Files missing in local deployment that exist in control " + control["info"]["uname"].asString());
	report.append(section);
	exceptions += section["exceptions"].asInt64();

	
	section = CheckFilesExist(control, local, string("") + "Files extra in local deployment that do not exist in control " + control["info"]["uname"].asString());
	report.append(section);
	exceptions += section["exceptions"].asInt64();


	section = CheckFilesHash(local, control, string("") + "Files with incorrect hash as compared to control " + control["info"]["uname"].asString());
	report.append(section);
	exceptions += section["exceptions"].asInt64();


	section = CheckFilesAttributes(local, control, string("") + "Files with incorrect attributes as compared to control " + control["info"]["uname"].asString());
	report.append(section);
	exceptions += section["exceptions"].asInt64();


// Create Summary of verification
	section["title"] = string("") + "Summary of verification of " + local["info"]["uname"].asString() + " compared to " + control["info"]["uname"].asString();

	section["exceptions"] = exceptions;
	report.append(section);

    return report;
}


int
main(int argc, char *argv[])
{
	Me = argv[0];

    // Json::Value root = readJson("fec.json");
    // writeJson(root, "/dev/stdout");

    if (argc < 2)
        Usage("Tell me what to do");

	Json::Value details;

	if( strcmp(argv[1], "sample") == 0 )
    	details = Sample(&argv[2]);
	else if( strcmp(argv[1], "verify") == 0 )
    	details = Verify(&argv[2]);
	else
		Usage(string("") + argv[1] + " is not an option");

    // cout << details << endl;

    return 0;
}
