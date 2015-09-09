#include "xml_config.h"
#include "float_format.h"
#include <boost/lexical_cast.hpp>
#include <io.h>
#include <codecvt>
#include <windows.h>

#ifndef DISABLE_XML_ENC
#pragma comment(lib, "libeay32.lib")
#include <openssl/aes.h>
#include <openssl/md5.h>
#endif

const string& xml_config::vals_string::castToString( const string& name, const string& def )
{
	auto it = vals.find(name);
	if (it != vals.end())
	{
		return it->second;
	}
	return def;
}

int xml_config::vals_string::castToInt( const string& name, int def )
{
	auto it = vals.find(name);
	if (it != vals.end())
	{
		try
		{
			return boost::lexical_cast<int>(it->second);
		}
		catch (...)
		{
			return def;
		}
	}
	return def;
}

double xml_config::vals_string::castToDouble( const string& name, double def )
{
	auto it = vals.find(name);
	if (it != vals.end())
	{
		try
		{
			return boost::lexical_cast<double>(it->second);
		}
		catch (...)
		{
			return def;
		}
	}
	return def;
}
//////////////////////////////////////////////////////////////////////////

struct enc_head 
{
	unsigned long long rand;
	int length;
	unsigned char md5[16];
};

xml_config::xml_config()
{

}

xml_config::~xml_config()
{

}

bool xml_config::decrypt(string& data)
{
#ifndef DISABLE_XML_ENC
	if (data.size() > 0 && data.size() % 16 == 0)
	{
		{
			unsigned char tBuf[16];
			memset(tBuf, 0, sizeof(tBuf));
			memcpy(tBuf, _password.c_str(), _password.size() < 16? _password.size(): 16);
			AES_KEY key;
			AES_set_decrypt_key(tBuf, 128, &key);
			int blocks = (int)data.size() / 16;
			unsigned char ivec[16];
			memset(ivec, 0, sizeof(ivec));
			for (int i = 0; i < blocks; i++)
			{
				AES_cbc_encrypt((unsigned char*)data.data()+i*16, tBuf, 16, &key, ivec, AES_DECRYPT);
				memcpy((unsigned char*)data.data()+i*16, tBuf, 16);
			}
		}

		{
			enc_head head = *(enc_head*)data.data();
			if (head.length <= (int)(data.size()-sizeof(enc_head)-16) || head.length > (int)(data.size()-sizeof(enc_head)))
			{
				return false;
			}
			memcpy((unsigned char*)data.data(), data.data()+sizeof(enc_head), head.length);
			int blocks = head.length / 16;
			data.resize(head.length);
			MD5_CTX ctx;
			MD5_Init(&ctx);
			for (int i = 0; i < blocks; i++)
			{
				MD5_Update(&ctx, (unsigned char*)data.data()+i*16, 16);
			}
			if (head.length % 16)
			{
				MD5_Update(&ctx, (unsigned char*)data.data()+blocks*16, head.length % 16);
			}
			unsigned char md5[16];
			MD5_Final(md5, &ctx);
			for (int i = 0; i < 16; i++)
			{
				if (md5[i] != head.md5[i])
				{
					return false;
				}
			}
		}
		return true;
	}
	return false;
#else
	return true;
#endif
}

void xml_config::encrypt(string& data, std::ostringstream& inData, const char* pas)
{
#ifndef DISABLE_XML_ENC
	{
		string& src = inData.str();
		data.resize((sizeof(enc_head) + src.size() + 15) & -16);
		enc_head* head = (enc_head*)data.data();
		head->rand = GetTickCount64();
		head->length = (int)src.size();
		MD5_CTX ctx;
		MD5_Init(&ctx);
		int blocks = (int)src.size() / 16;
		for (int i = 0 ; i < blocks; i++)
		{
			MD5_Update(&ctx, src.data()+i*16, 16);
		}
		if (src.size() % 16)
		{
			MD5_Update(&ctx, src.data()+blocks*16, src.size() % 16);
		}
		memcpy((unsigned char*)data.data()+sizeof(enc_head), src.data(), src.size());
		MD5_Final(head->md5, &ctx);
	}

	{
		AES_KEY key;
		unsigned char tBuf[16];
		memset(tBuf, 0, sizeof(tBuf));
		int sl = (int)strlen(pas);
		memcpy(tBuf, pas, sl < 16? sl: 16);
		AES_set_encrypt_key(tBuf, 128, &key);
		int blocks = (int)data.size() / 16;
		assert(data.size() % 16 == 0);
		unsigned char ivec[16];
		memset(ivec, 0, sizeof(ivec));
		for (int i = 0 ; i < blocks; i++)
		{
			AES_cbc_encrypt((unsigned char*)data.data()+i*16, tBuf, 16, &key, ivec, AES_ENCRYPT);
			memcpy((unsigned char*)data.data()+i*16, tBuf, 16);
		}
	}
#else
	data = inData.str();
#endif
}

bool xml_config::load(const char* file, const char* pas /* = NULL */)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	wstring unicode = conv.from_bytes(file);
	return load(unicode.c_str(), pas);
}

bool xml_config::load(const wchar_t* file, const char* pas /* = NULL */)
{
	FILE* fh = _wfsopen(file, L"rb", _SH_DENYNO);
	if (fh)
	{
		long long size = _filelengthi64(_fileno(fh));
		if (size && size <= 1*1024*1024)
		{
			std::string sxml;
			sxml.resize((size_t)size);
			fread((void*)sxml.data(), 1, sxml.size(), fh);
			fclose(fh);
			if (pas)
			{
				_password = pas;
				if (!decrypt(sxml))
				{
					return false;
				}
			}
			std::istringstream ixml(sxml);
			try
			{
				boost::property_tree::xml_parser::read_xml(ixml, _xml);
				clearData();
				_filePath = file;
				return true;
			}
			catch (...)
			{
				
			}
		}
	}
	return false;
}

void xml_config::clearData()
{
	struct _node 
	{
		_node(boost::property_tree::ptree* p1, boost::property_tree::ptree::iterator& p2)
		{
			_xml = p1;
			_it = p2;
		}

		boost::property_tree::ptree* _xml;
		boost::property_tree::ptree::iterator _it;
	};
	list<_node> treeStack;

	boost::property_tree::ptree* pxml = &_xml;
	auto it = pxml->begin();
	pxml->data().clear();
	while (true)
	{
		if (it == pxml->end())
		{
			if (treeStack.empty())
			{
				break;
			}
			pxml = treeStack.front()._xml;
			it = treeStack.front()._it;
			treeStack.pop_front();
			it++;
		} 
		else if (it->second.begin() != it->second.end())
		{
			treeStack.push_front(_node(pxml, it));
			pxml = &it->second;
			it = pxml->begin();
			pxml->data().clear();
		} 
		else
		{
			it++;
		}
	}
}

bool xml_config::save()
{
	if (_password.empty())
	{
		return saveAs(_filePath.c_str(), NULL);
	}
	else
	{
		return saveAs(_filePath.c_str(), _password.c_str());
	}
}

bool xml_config::saveAs(const char* file, const char* pas /* = NULL */)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	wstring unicode = conv.from_bytes(file);
	return saveAs(unicode.c_str(), pas);
}

bool xml_config::saveAs(const wchar_t* file, const char* pas /* = NULL */)
{
	try
	{
		std::ostringstream oxml;
		boost::property_tree::xml_writer_settings<boost::property_tree::ptree::key_type> settings('\t', 1);
		boost::property_tree::xml_parser::write_xml(oxml, _xml, settings);
		FILE* fh = NULL;
		_wfopen_s(&fh, file, pas? L"wb": L"w");
		if (fh)
		{
			if (pas)
			{
				string sxml;
				encrypt(sxml, oxml, pas);
				fwrite(sxml.data(), 1, sxml.size(), fh);
			}
			else
			{
				string& sxml = oxml.str();
				fwrite(sxml.data(), 1, sxml.size(), fh);
			}
			fclose(fh);
			return true;
		}
	}
	catch (...)
	{

	}
	return false;
}

void xml_config::erase(const char* path, const string& name)
{
	try
	{
		auto* cd = &_xml;
		if (path)
		{
			cd = &_xml.get_child(path);
		}
		for (auto it = cd->begin(); it != cd->end();)
		{
			if (name == it->first)
			{
				auto t = it++;
				cd->erase(t);
			}
			else
			{
				it++;
			}
		}
	}
	catch (...)
	{
		
	}
}

void xml_config::clear(const char* path, bool clearAtt /* = true */)
{
	try
	{
		if (path)
		{
			if (clearAtt)
			{
				_xml.get_child(path).clear();
			}
			else
			{
				auto cdatt = _xml.get_child(string(path) + "." + ATTR_NODE);
				auto& cd = _xml.get_child(path);
				cd.clear();
				cd.add_child(ATTR_NODE, cdatt);
			}
		}
		else
		{
			_xml.clear();
		}
	}
	catch (...)
	{
		
	}
}

bool xml_config::getAttr(const char* path, const string& key, string& res)
{
	try
	{
		res = _xml.get<string>(string(path)+"."+ATTR_NODE+"."+key);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::getAttr(const char* path, const string& key, int& res)
{
	try
	{
		res = _xml.get<int>(string(path)+"."+ATTR_NODE+"."+key);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::getAttr(const char* path, const string& key, double& res)
{
	try
	{
		res = _xml.get<double>(string(path)+"."+ATTR_NODE+"."+key);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::getAttr(const char* path, map<string, string>& res)
{
	try
	{
		res.clear();
		auto& cd = _xml.get_child(string(path)+"."+ATTR_NODE);
		for (auto it = cd.begin(); it != cd.end(); it++)
		{
			res[it->first] = it->second.data();
		}
	}
	catch (...)
	{
		res.clear();
		return false;
	}
	return true;
}

bool xml_config::getAttr(const char* path, map<string, int>& res)
{
	try
	{
		res.clear();
		auto& cd = _xml.get_child(string(path)+"."+ATTR_NODE);
		for (auto it = cd.begin(); it != cd.end(); it++)
		{
			res[it->first] = boost::lexical_cast<int>(it->second.data());
		}
	}
	catch (...)
	{
		res.clear();
		return false;
	}
	return true;
}

bool xml_config::getAttr(const char* path, map<string, double>& res)
{
	try
	{
		res.clear();
		auto& cd = _xml.get_child(string(path)+"."+ATTR_NODE);
		for (auto it = cd.begin(); it != cd.end(); it++)
		{
			res[it->first] = boost::lexical_cast<double>(it->second.data());
		}
	}
	catch (...)
	{
		res.clear();
		return false;
	}
	return true;
}

bool xml_config::setAttr(const char* path, const string& key, const string& att)
{
	try
	{
		auto& cd = getChild((string(path) + "." + ATTR_NODE).c_str(), false);
		auto it = cd.find(key);
		if (it != cd.not_found())
		{
			it->second.put_value(att);
		}
		else
		{
			cd.put(key, att);
		}
		return true;
	}
	catch (...) {}
	return false;
}

bool xml_config::setAttr(const char* path, const map<string, string>& att)
{
	try
	{
		auto& cd = getChild((string(path) + "." + ATTR_NODE).c_str(), false);
		for (auto it = att.begin(); it != att.begin(); it++)
		{
			auto sit = cd.find(it->first);
			if (sit != cd.not_found())
			{
				sit->second.put_value(it->second);
			}
			else
			{
				cd.put(it->first, it->second);
			}
		}
		return true;
	}
	catch (...) {}
	return false;
}

bool xml_config::setAttr(const char* path, const string& key, int att)
{
	string satt = boost::lexical_cast<string>(att);
	return setAttr(path, key, satt);
}

bool xml_config::setAttr(const char* path, const string& key, double att, int fmt)
{
	string satt = floatFormat(att, fmt);
	return setAttr(path, key, satt);
}

bool xml_config::getGroup( const char* path, list<vals_string>& res )
{
	try
	{
		res.clear();
		auto& cd = _xml.get_child(path);
		for (auto it = cd.begin(); it != cd.end(); it++)
		{
			res.push_back(vals_string());
			vals_string& tn = res.back();
			tn.name = it->first;
			auto& tcd = it->second.get_child(ATTR_NODE);
			for (auto tit = tcd.begin(); tit != tcd.end(); tit++)
			{
				tn.vals[tit->first] = tit->second.data();
			}
		}
	}
	catch (...)
	{
		res.clear();
		return false;
	}
	return true;
}

bool xml_config::getGroup( const char* path, list<vals_int>& res )
{
	try
	{
		res.clear();
		auto& cd = _xml.get_child(path);
		for (auto it = cd.begin(); it != cd.end(); it++)
		{
			res.push_back(vals_int());
			vals_int& tn = res.back();
			tn.name = it->first;
			auto& tcd = it->second.get_child(ATTR_NODE);
			for (auto tit = tcd.begin(); tit != tcd.end(); tit++)
			{
				tn.vals[tit->first] = boost::lexical_cast<int>(tit->second.data());
			}
		}
	}
	catch (...)
	{
		res.clear();
		return false;
	}
	return true;
}

bool xml_config::getGroup( const char* path, list<vals_double>& res )
{
	try
	{
		res.clear();
		auto& cd = _xml.get_child(path);
		for (auto it = cd.begin(); it != cd.end(); it++)
		{
			res.push_back(vals_double());
			vals_double& tn = res.back();
			tn.name = it->first;
			auto& tcd = it->second.get_child(ATTR_NODE);
			for (auto tit = tcd.begin(); tit != tcd.end(); tit++)
			{
				tn.vals[tit->first] = boost::lexical_cast<double>(tit->second.data());
			}
		}
	}
	catch (...)
	{
		res.clear();
		return false;
	}
	return true;
}

bool xml_config::putValue(boost::property_tree::ptree& exml, const char* path, const string& key, const string& p)
{
	try
	{
		boost::property_tree::ptree tTree;
		tTree.put_value(p);
		if (path)
		{
			exml.add_child(string(path)+"."+ATTR_NODE+"."+key, tTree);
		}
		else
		{
			exml.add_child(string(ATTR_NODE)+"."+key, tTree);
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

boost::property_tree::ptree& xml_config::getChild(const char* path, bool newGroup /* = true*/)
{
	boost::property_tree::ptree* tTree = NULL;
	if (!newGroup)
	{
		try
		{
			tTree = &_xml.get_child(path);
		}
		catch (...)
		{
			tTree = &_xml.add_child(path, boost::property_tree::ptree());
		}
	}
	else
	{
		tTree = &_xml.add_child(path, boost::property_tree::ptree());
	}
	return *tTree;
}

bool xml_config::appendAttr( const char* path, const string& key, const string& p )
{
	return putValue(_xml, path, key, p);
}

bool xml_config::appendAttr( const char* path, const string& key, int p )
{
	return putValue(_xml, path, key, boost::lexical_cast<string>(p));
}

bool xml_config::appendAttr( const char* path, const string& key, double p, int fmt /*= 3*/ )
{
	if (fmt > 0)
	{
		return putValue(_xml, path, key, floatFormat(p, fmt));
	} 
	else
	{
		return putValue(_xml, path, key, boost::lexical_cast<string>(p));
	}
}

bool xml_config::appendAttr(const char* path, const map<string, string>& p, bool newGroup /* = true */)
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto it = p.begin(); it != p.end(); it++)
		{
			putValue(tTree, NULL, it->first, it->second);
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendAttr( const char* path, const map<string, int>& p, bool newGroup /* = true */ )
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto it = p.begin(); it != p.end(); it++)
		{
			putValue(tTree, NULL, it->first, boost::lexical_cast<string>(it->second));
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendAttr( const char* path, const map<string, double>& p, bool newGroup /* = true */, int fmt /*= 3*/ )
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto it = p.begin(); it != p.end(); it++)
		{
			if (fmt > 0)
			{
				putValue(tTree, NULL, it->first, floatFormat(it->second, fmt));
			}
			else
			{
				putValue(tTree, NULL, it->first, boost::lexical_cast<string>(it->second));

			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendGroup( const char* path, const list<vals_string>& p, bool newGroup /* = true */ )
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto lit = p.begin(); lit != p.end(); lit++)
		{
			auto& t = tTree.add_child(lit->name, boost::property_tree::ptree());
			for (auto mit = lit->vals.begin(); mit != lit->vals.end(); mit++)
			{
				putValue(t, NULL, mit->first, mit->second);
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendGroup(const char* path, const list<vals_string::ptr>& p, bool newGroup /* = true */)
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto lit = p.begin(); lit != p.end(); lit++)
		{
			auto& t = tTree.add_child((*lit)->name, boost::property_tree::ptree());
			for (auto mit = (*lit)->vals.begin(); mit != (*lit)->vals.end(); mit++)
			{
				putValue(t, NULL, mit->first, mit->second);
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendGroup( const char* path, const list<vals_int>& p, bool newGroup /* = true */ )
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto lit = p.begin(); lit != p.end(); lit++)
		{
			auto& t = tTree.add_child(lit->name, boost::property_tree::ptree());
			for (auto mit = lit->vals.begin(); mit != lit->vals.end(); mit++)
			{
				putValue(t, NULL, mit->first, boost::lexical_cast<string>(mit->second));
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendGroup( const char* path, const list<vals_int::ptr>& p, bool newGroup /* = true */ )
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto lit = p.begin(); lit != p.end(); lit++)
		{
			auto& t = tTree.add_child((*lit)->name, boost::property_tree::ptree());
			for (auto mit = (*lit)->vals.begin(); mit != (*lit)->vals.end(); mit++)
			{
				putValue(t, NULL, mit->first, boost::lexical_cast<string>(mit->second));
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendGroup( const char* path, const list<vals_double>& p, bool newGroup /* = true */, int fmt /*= 3*/ )
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto lit = p.begin(); lit != p.end(); lit++)
		{
			auto& t = tTree.add_child(lit->name, boost::property_tree::ptree());
			for (auto mit = lit->vals.begin(); mit != lit->vals.end(); mit++)
			{
				if (fmt > 0)
				{
					putValue(t, NULL, mit->first, floatFormat(mit->second, fmt));
				} 
				else
				{
					putValue(t, NULL, mit->first, boost::lexical_cast<string>(mit->second));
				}
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool xml_config::appendGroup( const char* path, const list<vals_double::ptr>& p, bool newGroup /* = true */, int fmt /*= 3*/ )
{
	try
	{
		boost::property_tree::ptree& tTree = getChild(path, newGroup);
		for (auto lit = p.begin(); lit != p.end(); lit++)
		{
			auto& t = tTree.add_child((*lit)->name, boost::property_tree::ptree());
			for (auto mit = (*lit)->vals.begin(); mit != (*lit)->vals.end(); mit++)
			{
				if (fmt > 0)
				{
					putValue(t, NULL, mit->first, floatFormat(mit->second, fmt));
				} 
				else
				{
					putValue(t, NULL, mit->first, boost::lexical_cast<string>(mit->second));
				}
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}
