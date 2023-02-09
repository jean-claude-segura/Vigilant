#pragma once
#include <string>
#include "Utils.hpp"
#include "Echiquier.hpp"
#include <algorithm>
#include <thread>
#include <atomic>
#include <iostream>
#include "Moteur.hpp"

class UCI
{
public:
	UCI();
	int nThreads;
	Moteur _moteur;
	void CommandeUCI_Stop();
	void CommandeUCI_Position(const std::string&);
	void CommandeUCI_Go(const std::string&);
	void CommandeUCI_UCINewGame();
	void CommandeUCI_IsReady();
	void CommandeUCI_SetOption(const std::string&);

	void Analyse(int);

	std::mutex mtxDemarrer;

	std::unique_ptr<std::thread> lpThreadAnalyse;
	std::mutex mtxIsReady;
};

void threadAnalyseProcedure(UCI &, int);