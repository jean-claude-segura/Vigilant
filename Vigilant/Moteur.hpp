#pragma once
#include "Echiquier.hpp"
#include <algorithm>
#include <thread>
#include <atomic>
#include <iostream>

constexpr std::array< uint_fast64_t, 8> tableRoots = {
	(static_cast<uint_fast64_t>(1) << 22) - static_cast<uint_fast64_t>(1), //83 - 12 = 71
	(static_cast<uint_fast64_t>(1) << 23) - static_cast<uint_fast64_t>(1), //153 - 12 = 141
	(static_cast<uint_fast64_t>(1) << 24) - static_cast<uint_fast64_t>(1), //295 - 12 = 283
	(static_cast<uint_fast64_t>(1) << 25) - static_cast<uint_fast64_t>(1), //577 - 12 = 565
	(static_cast<uint_fast64_t>(1) << 26) - static_cast<uint_fast64_t>(1), // <1130
	(static_cast<uint_fast64_t>(1) << 27) - static_cast<uint_fast64_t>(1), // <2260
	(static_cast<uint_fast64_t>(1) << 28) - static_cast<uint_fast64_t>(1), // <4520
	(static_cast<uint_fast64_t>(1) << 29) - static_cast<uint_fast64_t>(1) // <9040
};

struct transTableItem {
	uint_fast64_t hash;
	int nEvaluation;
	int nDepth;
	uint_fast16_t uint16Coup;
	//transTableItem() { hash = static_cast<uint_fast64_t>(0);  nEvaluation = 0; nDepth = 0; }
	//transTableItem(uint_fast64_t _hash, int _nEvaluation, int _nDepth) { hash = _hash;  nEvaluation = _nEvaluation; nDepth = _nDepth; }
	//void operator =(const transTableItem & _in) { hash = _in.hash;  nEvaluation = _in.nEvaluation; nDepth = _in.nDepth; }
};

struct CoupEvaluation
{
	uint_fast16_t uint16Coup;
	int Evaluation;
	bool operator < (const CoupEvaluation & b)
	{
		return Evaluation < b.Evaluation;
	}
	bool operator > (const CoupEvaluation & b)
	{
		return Evaluation > b.Evaluation;
	}
	CoupEvaluation(const CoupEvaluation & ceSourse) {
		uint16Coup = ceSourse.uint16Coup;
		Evaluation = ceSourse.Evaluation;
	}
	CoupEvaluation(const uint_fast16_t _uint16Coup, const int _Evaluation) {
		uint16Coup = _uint16Coup;
		Evaluation = _Evaluation;
	}
};

class Moteur : public Echiquier
{
public:
	Moteur();
	Moteur(Moteur&);
	~Moteur();

	bool bGetAnalyseEnCours();
	void bSetAnalyseEnCours();
	void bUnsetAnalyseEnCours();
	inline void subAnalyseInit(int, std::vector<CoupEvaluation>::iterator&, int, int &, int, std::atomic<uint_fast64_t> &, std::chrono::steady_clock::time_point &);
	void AnalyseInit(int, int);
	inline void getCurLine(uint_fast16_t);
	std::atomic<bool> bContinuerAnalyse;

public:
#ifdef _FAST
	int AnalysePVS(int, int, int, std::atomic<uint_fast64_t> &, uint_fast16_t);
	int AnalysePrises(int, int, int, std::atomic<uint_fast64_t> &, uint_fast16_t);
#else
	int AnalysePVS(int, int, int, std::atomic<uint_fast64_t> &);
#endif
	int Analyse(int, int, int, uint_fast64_t &);
private:
#ifdef _FAST
	inline int Evalue(uint_fast16_t);
	inline int _Evalue();
#endif
	inline int Evalue();

	std::vector<CoupEvaluation> vectListeCoupsEvaluations;

	Piece * _board[64];
	Piece * objVectPieceRoi[2];

public:
	class Historique
	{
	public:
		bool bRoques[4];
		uint_fast64_t EnPassant;
		uint_fast8_t nEnPassant;
		uint_fast8_t nRegle50Coups;
		uint_fast64_t attaques[64];
		uint_fast64_t mouvements[64];
		uint_fast64_t uint64MasqueAttaques;
		int nCoupsDisponibles;
	};

#ifdef _FAST
	inline void _JoueCoup(uint_fast16_t, uint_fast8_t = 0);
	inline void _AnnuleCoup(Historique &, uint_fast16_t, std::array<Piece *, 2>);
	inline std::array<Piece *, 2> _JoueCoup(Historique &, uint_fast16_t, uint_fast8_t = 0);
	inline std::array<Piece *, 2> _JouePrise(Historique &, uint_fast16_t, uint_fast8_t = 0);
private:
	inline bool _CoupValide(Piece&, unsigned long);
	inline void _ValideCoup(Piece&);
	inline void _GenereBitboardsListePrises(uint_fast16_t);
	inline void _GenereBitboardListeAttaques(uint_fast16_t);
	inline void _GenereBitboardsListeMouvements();
	inline bool MouvementExiste();
#endif
	struct MeilleureLigne {
		std::vector<uint_fast16_t> vectCoups = {};
		int Evaluation = -32767;
	};

	static std::mutex mtxHashtablePrises;
	static std::unordered_map<uint_fast64_t, int> hashTable;
	static std::unordered_map<uint_fast64_t, int> hashTablePrises;
	static inline bool GetEvaluationPrisesFromHash(const uint_fast64_t &, int&);
	static inline void SetEvaluationPrisesToHash(const uint_fast64_t &, int);

	std::atomic<bool> bAnalyseEncours;
	static std::unique_ptr<std::atomic<transTableItem>[]>transpositionTableAtomic;
	static std::unique_ptr<transTableItem[]>transpositionTable;
	static uint_fast64_t tableRoot;

public:
	static inline bool GetEvaluationFromHash(const uint_fast64_t &, int&, int);
	static inline void SetEvaluationToHash(const uint_fast64_t &, int, int, uint_fast16_t);
	void ChargePosition(const std::string & strPositionUCI);
	static void ClearHash();
	static void SetHash(unsigned long long);
};

