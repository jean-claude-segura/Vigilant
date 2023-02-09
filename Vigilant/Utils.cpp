#include "common.h"
#include "Utils.hpp"

void trim_all(std::string &strInOut)
{
	// Tassement des espaces :
	size_t nPosition = strInOut.find("  ");
	while (nPosition != std::string::npos)
	{
		strInOut.replace(nPosition, 2, " ");
		nPosition = strInOut.find("  ", nPosition);
	}

	trim_right(strInOut);
	trim_left(strInOut);
}

void trim_right(std::string &strInOut)
{
	if (0 != strInOut.size())
	{
		std::string::iterator itStrInOut = --strInOut.end();
		while (strInOut.begin() != itStrInOut && ' ' == *itStrInOut)
		{
			itStrInOut = --strInOut.erase(itStrInOut);
		}
	}
}

void trim_left(std::string &strInOut)
{
	if (0 != strInOut.size())
	{
		std::string::iterator itStrInOut = strInOut.begin();
		while (strInOut.end() != itStrInOut && ' ' == *itStrInOut)
		{
			itStrInOut = strInOut.erase(itStrInOut);
		}
	}
}

void split(const std::string &strIn, std::vector<std::string> &vectMots)
{
	size_t nDebut;
	size_t nFin;

	if (0 < strIn.length())
	{
		nDebut = 0;
		nFin = strIn.find(" ");
		while (nFin != std::string::npos)
		{
			vectMots.push_back(strIn.substr(nDebut, nFin - nDebut));
			nDebut = ++nFin;
			nFin = strIn.find(" ", nDebut);
		}
		vectMots.push_back(strIn.substr(nDebut, strIn.length() - nDebut));
	}
}
