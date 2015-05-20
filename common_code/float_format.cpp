#include "float_format.h"
#include <boost/lexical_cast.hpp>

string floatFormat(double fnum, int acc)
{
	double fn = (fnum);
	if (std::abs(fn) >= 1000000)
	{
		try
		{
			return boost::lexical_cast<string>(fn);
		}
		catch(...) {}
		return "NaN";
	}
	else
	{
		int nac = 1;
		for (int i = 0; i <= acc; i++)
		{
			nac *= 10;
		}

		int zs = (int)std::abs(fn);
		int xs = int((std::abs(fn)-zs)*nac);
		xs += 5;
		zs += xs/nac;
		xs = xs%nac/10;

		char buff[32];
		char fmt[32];
		if (zs == 0 && xs == 0)
		{
			return "0";
		}
		else
		{
			if (fn > 0)
			{
				sprintf_s(fmt, "%%d.%%0%dd", acc);
			} 
			else
			{
				sprintf_s(fmt, "-%%d.%%0%dd", acc);
			}
			sprintf_s(buff, fmt, zs, xs);
		}
		return buff;
	}
}

extern int hexToInt( const char* ptr, size_t size )
{
	assert(size && size <= 8);
	int res = 0;
	for (size_t i = 0; i < size; i++)
	{
		char tc = ptr[i];
		if (tc >= '0' && tc <= '9')
		{
			res = res*16 + tc - '0';
		} 
		else if (tc >= 'A' && tc <= 'F')
		{
			res = res * 16 + tc-'A'+10;
		}
		else if (tc >= 'a' && tc <= 'f')
		{
			res = res * 16 + tc-'a'+10;
		}
		else
		{
			assert(false);
		}
	}
	return res;
}
