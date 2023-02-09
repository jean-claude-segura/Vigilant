// Vigilant.cpp : Defines the entry point for the console application.
//

#include "common.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <map>
#include "UCI.hpp"

static const std::map<std::string, int> mapCommandeIndex =
{
	{ "quit", 0 },
	{ "uci", 1 },
	{ "isready", 2 },
	{ "ucinewgame", 3 },
	{ "position", 4 },
	{ "go", 5 },
	{ "stop", 6 },
	{ "setoption", 7 },
	{ "test", 9 },
	{ "test2", 10 }
};

void lisConsole()
{
	bool bContinuer = true;
	std::map<std::string, int>::const_iterator itMapCommandeIndex;

	std::string strConsole;
	UCI mtrPosition;
	//std::cout << "uciok" << std::endl;
	do
	{
		std::cin >> strConsole;

		itMapCommandeIndex = mapCommandeIndex.find(strConsole);
		if (itMapCommandeIndex != mapCommandeIndex.end())
		{
			switch (itMapCommandeIndex->second)
			{
			case 0: // quit:
				mtrPosition.CommandeUCI_Stop();
				std::cout << "info";
				std::cout << " string ";
				std::cout << "Fermeture de l'application..." << std::endl;
				bContinuer = false;
				break;
			case 1: // uci :
				std::cout << "id name Vigilant" << std::endl;
				std::cout << "id author Jean-Claude Segura" << std::endl;
				std::cout <<"option name Clear Hash type button" << std::endl;
				std::cout << "option name Hash type spin default 0 min 0 max 4" << std::endl;
				std::cout << "option name Thread type spin default 1 min 1 max 8" << std::endl;
				std::cout << "uciok" << std::endl;
				break;
			case 2: // isready : attente de la complète finalisation des demandes d'initialisation :
				mtrPosition.CommandeUCI_IsReady();
				std::cout << "readyok" << std::endl;
				break;
			case 3: // ucinewgame : indique une nouvelle partie : nettoyage des informations de la partie précédente :
				mtrPosition.CommandeUCI_UCINewGame();
				break;
			case 4: // position : position et déplacements ( Exemple : position startpos moves e2e4 e7e5 )
				std::getline(std::cin, strConsole);
				mtrPosition.CommandeUCI_Position(strConsole);
				break;
			case 5: // go : demande explicite de recherche ; go infini : jusqu'à une solution définitive (Mat, pat, nulle...) :
				std::getline(std::cin, strConsole);
				mtrPosition.CommandeUCI_Go(strConsole);
				break;
			case 6: // stop : meilleure réponse trouvée à l'instant de la demande et arrêt de l'analyse :
				mtrPosition.CommandeUCI_Stop();
				break;
			case 7: // setoption name Clear Hash
				std::getline(std::cin, strConsole);
				mtrPosition.CommandeUCI_SetOption(strConsole);
				break;
			case 8:
				break;
			case 9: // position : position et déplacements ( Exemple : position startpos moves e2e4 e7e5 )
				//mtrPosition.CommandeUCI_Position("startpos moves f2f4 e7e6 g2g4 d8h4"); // Mat.
				//mtrPosition.CommandeUCI_Position("startpos moves f2f4 e7e6 g2g4"); // Mat en 1.
				//mtrPosition.CommandeUCI_Position("startpos moves f2f4 e7e6");
				//mtrPosition.CommandeUCI_Position("startpos moves e2e4 e7e5 f1c4 b8c6 d1h5"); // Mat du berger (2 demi-coups).
				//mtrPosition.CommandeUCI_Position("startpos moves e2e4 e7e5 f1c4 b8c6 d1h5 f8b4"); // Mat du berger (1 demi-coup) : réponse incorrecte des noirs.
				//mtrPosition.CommandeUCI_Position("startpos");

				// Mat : depth 3
				//mtrPosition.CommandeUCI_Position("fen 5r1k/2q3pp/p2p1p2/3Bb2R/4P3/7P/Pr2Q1P1/5R1K w - - 2 29");
				//mtrPosition.CommandeUCI_Position("fen 5r1k/2q3pp/p2p1p2/1r1Bb2R/4P3/7P/P3Q1P1/5R1K b - - 1 28 moves b5b2");

				// Mat : depth 2 :
				//mtrPosition.CommandeUCI_Position("fen 5r1k/2q3pR/p2p1p2/3Bb3/4P3/7P/Pr2Q1P1/5R1K b - - 0 29");
				//mtrPosition.CommandeUCI_Position("fen 5r1k/2q3pp/p2p1p2/3Bb2R/4P3/7P/Pr2Q1P1/5R1K w - - 2 29 moves h5h7");
				
				
				// Prévention du mat : depth 4.
				mtrPosition.CommandeUCI_Position("fen 5r1k/2q3pp/p2p1p2/1r1Bb2R/4P3/7P/P3Q1P1/5R1K b - - 1 28"); // Pour test alpha au noeu d'origine.
				//mtrPosition.CommandeUCI_Position("fen 5r1k/2q3pp/p2p1p2/1r1Bb2R/4P3/7P/P3Q1P1/5R1K b - - 1 28 moves f8f7 h5h7"); // Pour test alpha au noeu d'origine.

				// Mat imaginaire : depth 3.
				//mtrPosition.CommandeUCI_Position("fen 5r1k/2q3pp/p2p1p2/1r1Bb2R/4P3/7P/P3Q1P1/5R1K b - - 1 28 moves c7c1 h5h7"); // Pour test alpha au noeu d'origine.

																												 // Après g6 :
				//mtrPosition.CommandeUCI_Position("fen 5r1k/2q4p/p2p1pp1/1r1Bb2R/4P3/7P/P3Q1P1/5R1K w - - 0 29"); // Sauvegarde de la tour.

				//mtrPosition.CommandeUCI_Position("fen 5r2/2q3pk/p2p1p2/3Bb3/4P3/7P/Pr2Q1P1/5R1K w - - 0 30"); // Mat en 1 dc.
				//mtrPosition.CommandeUCI_Position("fen 5r2/2q3pk/p2p1p2/3Bb3/4P3/7P/Pr2Q1P1/5R1K w - - 0 30 moves e2e3 b2a2"); // Mat raté : position problématique.
				//mtrPosition.CommandeUCI_Position("fen 5r2/2q3pk/p2p1p2/3Bb2Q/4P3/7P/Pr4P1/5R1K b - - 1 30"); // Mat.
				// https://lichess.org/QlzFK5ME/white#59
				// https://lichess.org/training/18398
				
				// Mat : depth 5: https://lichess.org/6hjaeyoa/black#63
				//mtrPosition.CommandeUCI_Position("fen 8/2RQ1p1p/1p2pk2/b2p2r1/P7/3P1K2/5P1q/2R5 b - - 3 32");
				
				//mtrPosition.CommandeUCI_Position("startpos moves e2e4 e7e5 g1f3 d7d6 d2d4 e5d4 f1c4 f8e7 c2c3 g8f6 c4f7 e8f7 d1b3 f7g6 f3h4 g6h5 c3d4"); //h5h4 perdant.
				//mtrPosition.CommandeUCI_Position("startpos moves e2e4 e7e5 g1f3 d7d6 d2d4 e5d4 f1c4 f8e7 c2c3 g8f6 c4f7 e8f7 d1b3 f7g6 f3h4 g6h5 c3d4 h5h4");

				
				//mtrPosition.CommandeUCI_Position("startpos moves e2e4 e7e5 g1f3 d7d6 d2d4 e5d4 f1c4 f8e7 c2c3 g8f6 c4f7 e8f7 d1b3 f7g6 f3h4 g6h5 c3d4 h5h4 b3h3");
				// h5h7 h8h7 e2h5
				//mtrPosition.CommandeUCI_Position("startpos moves e2e4 e7e5 d2d4");

				/*
				1. Nf3 e5 2. g3 Ke7 3. Bg2 Kd6 4. e4 Kc5
				5. Qe2 Kb4 6. c4 Bc5 7. a4 Bd4 8. b3 Bxa1
				9. d4 Bxd4 10. Nbd2 Kc3 11. Bb2+ Kxb2 12. Nxe5 Bxe5
				13. Nf3+ Ka1
				rnbq2nr/pppp1ppp/8/4b3/P1P1P3/1P3NP1/4QPBP/k3K2R w K - 2 14
				*/
#ifdef _DEBUG
				mtrPosition.CommandeUCI_Go("depth 4");
#else
				mtrPosition.CommandeUCI_Go("depth 4");
#endif
				//mtrPosition.CommandeUCI_Stop();
				break;
			case 10: // position : position et déplacements ( Exemple : position startpos moves e2e4 e7e5 )
				mtrPosition.CommandeUCI_SetOption("name Hash value 4");
				mtrPosition.CommandeUCI_SetOption("name Thread value 4");
				break;
			}
		}
		else {
			std::cout << "info";
			std::cout << " string erreur de traitement...";
			std::cout << std::endl;
		}

		std::cin.clear();

	} while (bContinuer);
}

int main(/*int argc, char* argv[]*/)
{
	//uint_fast64_t baseAttaques = objVectPiece.BaseAttaques[uint8Case];
	//uint_fast64_t index = _pext_u64(occupation, baseAttaques);
	//uint_fast64_t bloqueurs = _pdep_u64(index, baseAttaques);
	//if (bloqueurs != (occupation & objVectPiece.BaseAttaques[uint8Case]))
	//	throw(1);
	//GenereMouvementsDame();
	// https://stackoverflow.com/questions/31162367/significance-of-ios-basesync-with-stdiofalse-cin-tienull
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(NULL);
	//std::string strConsole;
	//std::cin >> strConsole;
	//if ("uci" == strConsole)
	//{
		//std::cout << "id name Vigilant" << std::endl;
		//std::cout << "id author Jean-Claude Segura" << std::endl;
		std::thread threadConsole(lisConsole);
		threadConsole.join();
	//}

	return 0;
}

