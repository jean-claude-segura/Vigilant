#include "common.h"
#include "UCI.hpp"

void threadAnalyseProcedure(UCI &_uci, int nProfondeurMaximale)
{
	_uci._moteur.bSetAnalyseEnCours();
	_uci.mtxDemarrer.lock();

	_uci._moteur.AnalyseInit(nProfondeurMaximale, _uci.nThreads);

	_uci.mtxDemarrer.unlock();
	_uci._moteur.bUnsetAnalyseEnCours();
}

UCI::UCI()
{
	lpThreadAnalyse = nullptr;
	nThreads = 1;
	Moteur::SetHash(0);
}

/* position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
set up the position described in fenstring on the internal board and play the moves on the internal chess board.
if the game was played  from the start position the string "startpos" will be sent.
Note: no "new" command is needed. However, if this position is from a different game than the last position sent to the engine, the GUI should have sent a "ucinewgame" inbetween.
*/
void UCI::CommandeUCI_Position(const std::string & strPositionUCI)
{
	mtxIsReady.lock();
	_moteur.ChargePosition(strPositionUCI);
	mtxIsReady.unlock();
}

void UCI::Analyse(int nProfondeur)
{
	mtxDemarrer.lock();
	if (lpThreadAnalyse != nullptr && lpThreadAnalyse.get()->joinable()) {
		lpThreadAnalyse.get()->join();
		lpThreadAnalyse.reset();
	}
	lpThreadAnalyse = std::make_unique<std::thread>(threadAnalyseProcedure, std::ref(*this), nProfondeur);
	mtxDemarrer.unlock();
}

/* go
start calculating on the current position set up with the "position" command.
There are a number of commands that can follow this command, all will be sent in the same string.
If one command is not sent its value should be interpreted as it would not influence the search.
* searchmoves <move1> .... <movei>
restrict search to this moves only
Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
the engine should only search the two moves e2e4 and d2d4 in the initial position.
* ponder
start searching in pondering mode.
Do not exit the search in ponder mode, even if it's mate!
This means that the last move sent in in the position string is the ponder move.
The engine can do what it wants to do, but after a "ponderhit" command
it should execute the suggested move to ponder on. This means that the ponder move sent by
the GUI can be interpreted as a recommendation about which move to ponder. However, if the
engine decides to ponder on a different move, it should not display any mainlines as they are
likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
on the suggested move.
* wtime <x>
white has x msec left on the clock
* btime <x>
black has x msec left on the clock
* winc <x>
white increment per move in mseconds if x > 0
* binc <x>
black increment per move in mseconds if x > 0
* movestogo <x>
there are x moves to the next time control,
this will only be sent if x > 0,
if you don't get this and get the wtime and btime it's sudden death
* depth <x>
search x plies only.
* nodes <x>
search x nodes only,
* mate <x>
search for a mate in x moves
* movetime <x>
search exactly x mseconds
* infinite
search until the "stop" command. Do not exit the search without being told so in this mode!
*/
void UCI::CommandeUCI_Go(const std::string & strParametres)
{
	if (!_moteur.bGetAnalyseEnCours()) {
		auto _strParametres = strParametres;
		trim_all(_strParametres);
		std::vector<std::string> vectParametresCommande;
		split(_strParametres, vectParametresCommande);
		if (std::string::npos != _strParametres.find("infinite")) {
			Analyse(-1);
		}
		else if (std::string::npos != _strParametres.find("ponder")) {
		}
		else if (std::string::npos != _strParametres.find("movetime")) {
			Analyse(5);
		}
		else {
			auto nPos = _strParametres.find("depth");
			if (std::string::npos != nPos) {
				nPos += std::string("depth ").length();
				auto nPosEnd = _strParametres.find(" ", nPos);
				if (std::string::npos == nPosEnd)
					nPosEnd = _strParametres.length();
				std::string strDepth = _strParametres.substr(nPos, nPosEnd - nPos);
				Analyse(std::stoi(strDepth));
			}
			else
				Analyse(5);
		}
	}
}

void UCI::CommandeUCI_UCINewGame()
{
	mtxIsReady.lock();
	_moteur.VidePlateau();
	mtxIsReady.unlock();
}

/* stop
stop calculating as soon as possible,
don't forget the "bestmove" and possibly the "ponder" token when finishing the search*/
void UCI::CommandeUCI_Stop()
{
	if (_moteur.bGetAnalyseEnCours()) {
		_moteur.bContinuerAnalyse = false;
		lpThreadAnalyse.get()->join();
		lpThreadAnalyse.reset();
		_moteur.bContinuerAnalyse = true;
	}
	else if (lpThreadAnalyse != nullptr && lpThreadAnalyse.get()->joinable()) {
		lpThreadAnalyse.get()->join();
		lpThreadAnalyse.reset();
	}
}

/*
* setoption name <id> [value <x>]
	this is sent to the engine when the user wants to change the internal parameters
	of the engine. For the "button" type no value is needed.
	One string will be sent for each parameter and this will only be sent when the engine is waiting.
	The name and value of the option in <id> should not be case sensitive and can inlude spaces.
	The substrings "value" and "name" should be avoided in <id> and <x> to allow unambiguous parsing,
	for example do not use <name> = "draw value".
	Here are some strings for the example below:
	   "setoption name Nullmove value true\n"
	  "setoption name Selectivity value 3\n"
	   "setoption name Style value Risky\n"
	   "setoption name Clear Hash\n"
	   "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
*/
void UCI::CommandeUCI_SetOption(const std::string & strParametres)
{
	if (!_moteur.bGetAnalyseEnCours()) {
		auto _strParametres = strParametres;
		trim_all(_strParametres);
		std::vector<std::string> vectParametresCommande;
		split(_strParametres, vectParametresCommande);
		if (vectParametresCommande.size() >= 2 && 0 == vectParametresCommande[0].compare("name")) {
			if (vectParametresCommande.size() >= 3) {
				if (vectParametresCommande.size() >= 4) {
					if (0 == vectParametresCommande[2].compare("value")) {
						if (0 == vectParametresCommande[1].compare("Hash")) {
							std::cout << "info";
							std::cout << " string Index de la taille de la table de transposition : " << vectParametresCommande[3] << ".";
							std::cout << std::endl;
							Moteur::SetHash(std::stoi(vectParametresCommande[3]));
						}
						else if (0 == vectParametresCommande[1].compare("Thread")) {
							std::cout << "info";
							std::cout << " string Nombre de threads : " << vectParametresCommande[3] << ".";
							std::cout << std::endl;
							nThreads = std::stoi(vectParametresCommande[3]);
						}
						else {
							std::cout << "info string Erreur!";
						}
					}
				}
				else {
					if (0 == vectParametresCommande[1].compare("Clear")) {
						if (0 == vectParametresCommande[2].compare("Hash")) {
							std::cout << "info";
							std::cout << " string Nettoyage du hash...";
							std::cout << std::endl;
							Moteur::ClearHash();
						}
					}
				}
			}
		}
	}
}

void UCI::CommandeUCI_IsReady()
{
	mtxIsReady.lock();
	mtxIsReady.unlock();
}
