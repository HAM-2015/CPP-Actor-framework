#ifndef __XML_CONFIG_H
#define __XML_CONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <string>
#include <map>

using namespace std;

#define ATTR_NODE	"<xmlattr>"

/*!
@brief xmlÅäÖÃÎÄ¼þ²Ù×÷
*/
class xml_config
{
public:
	struct vals_string
	{
		typedef boost::shared_ptr<vals_string> ptr;

		static ptr make_ptr()
		{
			return ptr(new vals_string);
		}

		string name;
		map<string, string> vals;

		const string& castToString(const string& name, const string& def);
		int castToInt(const string& name, int def);
		double castToDouble(const string& name, double def);
	};

	struct vals_int
	{
		typedef boost::shared_ptr<vals_int> ptr;

		static ptr make_ptr()
		{
			return ptr(new vals_int);
		}

		string name;
		map<string, int> vals;
	};

	struct vals_double
	{
		typedef boost::shared_ptr<vals_double> ptr;

		static ptr make_ptr()
		{
			return ptr(new vals_double);
		}

		string name;
		map<string, double> vals;
	};
public:
	xml_config();
	~xml_config();
public:
	bool load(const char* file, const char* pas = NULL);
	bool load(const wchar_t* file, const char* pas = NULL);
	bool save();
	bool saveAs(const char* file, const char* pas = NULL);
	bool saveAs(const wchar_t* file, const char* pas = NULL);
	void erase(const char* path, const string& name);
	void clear(const char* path, bool clearAtt = true);

	bool getAttr(const char* path, const string& key, string& res);
	bool getAttr(const char* path, const string& key, int& res);
	bool getAttr(const char* path, const string& key, double& res);

	bool setAttr(const char* path, const string& key, const string& att);
	bool setAttr(const char* path, const string& key, int att);
	bool setAttr(const char* path, const string& key, double att, int fmt = 3);

	bool getAttr(const char* path, map<string, string>& res);
	bool getAttr(const char* path, map<string, int>& res);
	bool getAttr(const char* path, map<string, double>& res);

	bool setAttr(const char* path, const map<string, string>& att);

	bool getGroup(const char* path, list<vals_string>& res);
	bool getGroup(const char* path, list<vals_int>& res);
	bool getGroup(const char* path, list<vals_double>& res);

	bool appendAttr(const char* path, const string& key, const string& p);
	bool appendAttr(const char* path, const string& key, int p);
	bool appendAttr(const char* path, const string& key, double p, int fmt = 3);

	bool appendAttr(const char* path, const map<string, string>& p, bool newGroup = true);
	bool appendAttr(const char* path, const map<string, int>& p, bool newGroup = true);
	bool appendAttr(const char* path, const map<string, double>& p, bool newGroup = true, int fmt = 3);

	bool appendGroup(const char* path, const list<vals_string>& p, bool newGroup = true);
	bool appendGroup(const char* path, const list<vals_string::ptr>& p, bool newGroup = true);
	bool appendGroup(const char* path, const list<vals_int>& p, bool newGroup = true);
	bool appendGroup(const char* path, const list<vals_int::ptr>& p, bool newGroup = true);
	bool appendGroup(const char* path, const list<vals_double>& p, bool newGroup = true, int fmt = 3);
	bool appendGroup(const char* path, const list<vals_double::ptr>& p, bool newGroup = true, int fmt = 3);
private:
	void clearData();
	bool putValue(boost::property_tree::ptree& exml, const char* path, const string& key, const string& p);
	boost::property_tree::ptree& getChild(const char* path, bool newGroup = true);
	bool decrypt(string& data);
	void encrypt(string& data, std::ostringstream& inData, const char* pas);
private:
	wstring _filePath;
	string _password;
	boost::property_tree::ptree _xml;
};

#endif