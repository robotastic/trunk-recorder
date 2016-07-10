/*
 * EdUrlParser.h
 *
 *  Created on: Nov 25, 2014
 *      Author: netmind
 */

#ifndef EDURLPARSER_H_
#define EDURLPARSER_H_

#include <unordered_map>
#include <tuple>
#include <vector>
#include <string>

using namespace std;

typedef struct {
	string key;
	string val;
} query_kv_t;

typedef int (*__kv_callback)(void* list, string k, string v);

class EdUrlParser {
private:
	EdUrlParser();
public:
	virtual ~EdUrlParser();
	static EdUrlParser* parseUrl(string urlstr);
	static int parsePath(vector<string> *pdirlist, string pathstr);
	static string urlDecode(string str);
	static char toChar(const char* hex);
	static string urlEncode(string s);
	static void toHex(char *desthex, char c);
	static size_t parseKeyValueMap(unordered_map<string, string> *kvmap, string str, bool strict=true);
	static size_t parseKeyValueList(vector< query_kv_t > *kvmap, string rawstr, bool strict=true);
	static size_t parseKeyValue(string rawstr, __kv_callback kvcb, void* obj, bool strict);

private:
	void parse();

	string mRawUrl;
public:
	string scheme;
	string hostName;
	string port;
	string path;
	string query;
	string fragment;
};

#endif /* EDURLPARSER_H_ */
