#pragma once
#include "Piece.hpp"
#include <random>
#include <unordered_map>
#include <iostream>

class Echiquier
{
public:
    Echiquier();
    ~Echiquier();

    void PositionInitiale();
    void VidePlateau();
    bool ChargePositionFEN(std::string);

    const bool bEstEchec();

    void JoueCoup(uint_fast16_t, uint_fast8_t = 0);
    void AnnuleCoup();
    bool CoupsDisponibles();
	int nCoupsDisponibles;

    std::vector<uint_fast16_t> vectCoupsDisponibles;

    void initHash(unsigned long ulSeed = std::mt19937_64::default_seed);
	//std::array<std::vector<Piece>, 2> arrPieces;
	std::vector<Piece> arrPieces[2];

    bool bNoirsAuTrait;
    int nRegle50Coups; // Demi-coups sans prise ni mouvement de pion.
    int nCoupsJoues; // Incrémenté après le coup des noirs.
    bool bRoques[4];
    uint_fast64_t EnPassant;
    int nEnPassant;
    std::vector<uint_fast64_t> vectHashPositionCourant;
    //std::map<uint_fast64_t, int> mapHashPositionCompteur;

protected:
    uint_fast64_t BaseHashTable[12][64];
    uint_fast64_t HashTraitAuxNoirs;
    uint_fast64_t HashRoques[4];
    uint_fast64_t HashColonnesPriseEnPassant[8];
	inline std::vector<uint_fast16_t> & GenereBitboardsListeCoups();
	inline std::vector<uint_fast16_t> & GenereBitboardsListeCoups(uint_fast16_t);

	uint_fast64_t occupation;
	uint_fast64_t side[2];
	uint_fast64_t uint64MasqueAttaques;
	uint_fast64_t uint64MasqueAttaquesAdv;
	//Piece * objVectPieceRoi[2];

	class Historique
	{
	public:
		bool bRoques[4];
		uint_fast64_t EnPassant;
		int nEnPassant;
		//std::vector<uint_fast16_t> vectCoupsDisponibles;
		int nRegle50Coups;
		int nCoupsJoues;
		//std::array<std::vector<Piece>, 2> arrPieces;
		std::vector<Piece> arrPieces[2];
		uint_fast64_t occupation;
		uint_fast64_t side[2];
		uint_fast64_t uint64MasqueAttaques;
		//Piece * objVectPieceRoi[2];
	};

	std::vector<Historique> Partie;

	inline std::vector <Piece>::iterator GetPiece(uint_fast8_t, bool);
    inline void DeplacePiece(std::vector<Piece>::iterator &, uint_fast8_t, uint_fast8_t);
    inline void SupprimePiece(std::vector<Piece>::iterator &);
    inline void AjoutePiece(uint_fast8_t, uint_fast8_t);
    inline void PromeutPiece(std::vector<Piece>::iterator &, uint_fast8_t);
	inline void ValideListeCoups();
	inline void ValideCoup(Piece&);
	inline void ValideCoupAdv(Piece&);
};

int TraduitCoup(const std::string &, uint_fast16_t &);
std::string TraduitCoup(const uint_fast16_t coup);
static const std::string strStartPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
// "fen 5r1k/2q3pp/p2p1p2/3Bb2R/4P3/7P/Pr2Q1P1/5R1K w - - 2 29"
static const std::map<int, int> mapCodeIndexHashPiece = { { 1, 0 }, { 2, 1 }, { 3, 2 }, { 4, 3 }, { 5, 4 }, { 6, 5 }, { 9, 6 }, { 10, 7 }, { 11, 8 }, { 12, 9 }, { 13, 10 }, { 14, 11 } };
