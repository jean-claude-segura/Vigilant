#include "common.h"
#include "Moteur.hpp"

std::unordered_map<uint_fast64_t, int> Moteur::hashTable;
std::unique_ptr<std::atomic<transTableItem>[]> Moteur::transpositionTableAtomic;
std::unique_ptr<transTableItem[]> Moteur::transpositionTable;
uint_fast64_t Moteur::tableRoot = tableRoots[0];
std::mutex Moteur::mtxHashtablePrises;
std::unordered_map<uint_fast64_t, int> Moteur::hashTablePrises;

#ifdef _FAST
inline void Moteur::_ValideCoup(Piece & objPiece)
{
	uint_fast64_t arrivees = objPiece.uint64MasqueMouvements;
	uint_fast16_t coup = objPiece.uint8Case << 8;
	unsigned long index = static_cast<unsigned long>(0);
	while (_BitScanForward64(&index, arrivees)) {
		// Joue le coup :
		auto arriveecur = static_cast<uint_fast64_t>(1) << index;
		auto occupationcur = occupation;
		occupationcur ^= objPiece.uint64MasquePosition;
		occupationcur |= arriveecur;
		auto prevPos = objPiece.uint64MasquePosition;
		objPiece.uint64MasquePosition = arriveecur; // objVectPieceRoi[this->bNoirsAuTrait]->uint64MasquePosition

		uint_fast64_t attaques = static_cast<uint_fast64_t>(0);
		// On calcule les attaques du camp adverse pour tester si un échec persiste après le mouvement:
		uint_fast64_t attaquants = side[!bNoirsAuTrait];
		unsigned long index_attaquants;
		while (_BitScanForward64(&index_attaquants, attaquants)) {
			Piece & objVectPiece = *_board[index_attaquants];
			if (arriveecur != objVectPiece.uint64MasquePosition /* Une pièce prise n'attaque pas... */ &&
				/* Il n'y a pas d'échec possible si on n'attaque pas le roi... */
				static_cast<uint_fast64_t>(0) != (objVectPiece.BaseAttaques[objVectPiece.uint8Case] & objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition)) {
				attaques |= GenereAttaques(objVectPiece, occupationcur);
			}
			attaquants ^= static_cast<uint_fast64_t>(1) << index_attaquants;
		}

		bool bEchec = false;
		bEchec = static_cast<uint_fast64_t>(0) != (objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition & attaques);

		arrivees ^= _rotl64(static_cast<uint_fast64_t>(1), index);
		if (!bEchec) {
			coup |= index;
			vectCoupsDisponibles.push_back(coup);
			coup &= 0xFF00;
			++nCoupsDisponibles;
		}
		else {
			// Mouvement invalide :
			objPiece.uint64MasqueMouvements ^= arriveecur;
		}

		// Annule le coup :
		objPiece.uint64MasquePosition = prevPos;
	}
}

inline bool Moteur::_CoupValide(Piece & objPiece, unsigned long index)
{
	// Joue le coup :
	auto arriveecur = static_cast<uint_fast64_t>(1) << index;
	auto occupationcur = occupation;
	occupationcur |= arriveecur;
	occupationcur ^= objPiece.uint64MasquePosition;
	auto prevPos = objPiece.uint64MasquePosition;
	objPiece.uint64MasquePosition = arriveecur; // objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition

	uint_fast64_t attaques = static_cast<uint_fast64_t>(0);
	// On calcule les attaques du camp adverse pour tester si un échec persiste après le mouvement:
	uint_fast64_t attaquants = side[!bNoirsAuTrait];
	unsigned long index_attaquants;
	while (_BitScanForward64(&index_attaquants, attaquants)) {
		Piece & objVectPiece = *_board[index_attaquants];
		if (arriveecur != objVectPiece.uint64MasquePosition /* Une pièce prise n'attaque pas... */ &&
			/* Il n'y a pas d'échec possible si on n'attaque pas le roi... */
			static_cast<uint_fast64_t>(0) != (objVectPiece.BaseAttaques[objVectPiece.uint8Case] & objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition)) {
			attaques |= GenereAttaques(objVectPiece, occupationcur);
		}
		attaquants ^= static_cast<uint_fast64_t>(1) << index_attaquants;
	}
	bool bValide;
#ifdef _DEBUG
	if (objVectPieceRoi[bNoirsAuTrait])
#endif
	bValide = static_cast<uint_fast64_t>(0) == (objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition & attaques);
#ifndef _FAST
	if (!bValide) {
		// Mouvement invalide :
		objPiece.uint64MasqueMouvements ^= arriveecur;
	}
#endif

	// Annule le coup :
	objPiece.uint64MasquePosition = prevPos;
	return bValide;
}

inline void Moteur::_GenereBitboardsListePrises(uint_fast16_t uint16Coup)
{
	uint_fast64_t uint64MasqueDepart = static_cast<uint_fast64_t>(1) << (uint16Coup >> 8);
	uint_fast64_t uint64MasqueArrivee = static_cast<uint_fast64_t>(1) << (uint16Coup & 0x00FF);
	uint_fast64_t uint64MasqueCoup = (uint64MasqueDepart | uint64MasqueArrivee);

	unsigned long index = static_cast<unsigned long>(0);
	uint_fast64_t arrivees = side[bNoirsAuTrait];
	uint_fast64_t attaques[] = { static_cast<uint_fast64_t>(0), static_cast<uint_fast64_t>(0) };
	for (int nside = 0; nside < 2; ++nside) {
		arrivees = side[nside];
		while (_BitScanForward64(&index, arrivees)) {
			Piece & objVectPiece = *_board[index];
			/*
			objVectPiece.uint64MasqueAttaques = objVectPiece.uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
			attaques[nside] |= _GenerePrises(objVectPiece);
			/**/
			/**/if (
				objVectPiece.nPiece == 14 || objVectPiece.nPiece == 6 ||
				// Si la pièce déplacée est dans le rayon d'action de la pièce courante :
				static_cast<uint_fast64_t>(0) != (objVectPiece.uint64MasqueAttaques & uint64MasqueCoup) ||
				static_cast<uint_fast64_t>(0) != (objVectPiece.BaseMouvements[objVectPiece.uint8Case] & uint64MasqueCoup) ||
				// Ou s'il s'agit de la pièce déplacée :
				objVectPiece.uint64MasquePosition == uint64MasqueArrivee) {
				objVectPiece.uint64MasqueAttaques = objVectPiece.uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
				attaques[nside] |= GenereAttaques(objVectPiece, occupation);
			}
			else {
				attaques[nside] |= objVectPiece.uint64MasqueAttaques;
			}/**/
			arrivees ^= static_cast<uint_fast64_t>(1) << index;
		}
	}
	uint64MasqueAttaques = attaques[!bNoirsAuTrait];
	objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements &= ~attaques[!bNoirsAuTrait];
	objVectPieceRoi[!bNoirsAuTrait]->uint64MasqueMouvements &= ~attaques[bNoirsAuTrait];
}

inline void Moteur::_GenereBitboardsListeMouvements()
{
	unsigned long index_piece = static_cast<unsigned long>(0);
	uint_fast64_t pos_pieces;
	objVectPieceRoi[bNoirsAuTrait]->uint64MasqueAttaques = objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
	GenereMouvements(*objVectPieceRoi[bNoirsAuTrait], occupation, side);
	//objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements &= ~uint64MasqueAttaques;
	objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements = _andn_u64(uint64MasqueAttaques, objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements);
	pos_pieces = side[bNoirsAuTrait];
	while (_BitScanForward64(&index_piece, pos_pieces)) {
		if (index_piece != objVectPieceRoi[bNoirsAuTrait]->uint8Case) {
			Piece & objVectPiece = *_board[index_piece];
			objVectPiece.uint64MasqueAttaques = objVectPiece.uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
			// Aucune position légale n'autorise le cas où une pièce n'a aucun mouvement de base...
			/*if (objVectPiece.BaseMouvements[objVectPiece.uint8Case] != 0)*/ {
				GenereMouvements(objVectPiece, occupation, side);
			}
		}
		pos_pieces ^= static_cast<uint_fast64_t>(1) << index_piece;
	}
}

inline void Moteur::_GenereBitboardListeAttaques(uint_fast16_t uint16Coup)
{
	uint_fast64_t uint64MasqueDepart = static_cast<uint_fast64_t>(1) << (uint16Coup >> 8);
	uint_fast64_t uint64MasqueArrivee = static_cast<uint_fast64_t>(1) << (uint16Coup & 0x00FF);
	uint_fast64_t uint64MasqueCoup = (uint64MasqueDepart | uint64MasqueArrivee);

	uint64MasqueAttaques = static_cast<uint_fast64_t>(0);

	unsigned long index_piece = static_cast<unsigned long>(0);
	uint_fast64_t pos_pieces;
	pos_pieces = side[!bNoirsAuTrait];
	while (_BitScanForward64(&index_piece, pos_pieces)) {
		Piece & objVectPiece = *_board[index_piece];
		if (
			// Si la pièce déplacée est dans le rayon d'action de la pièce courante :
			static_cast<uint_fast64_t>(0) != (objVectPiece.uint64MasqueAttaques & uint64MasqueCoup) ||
			// Ou s'il s'agit de la pièce déplacée :
			objVectPiece.uint64MasquePosition == uint64MasqueArrivee)
		{
			objVectPiece.uint64MasqueAttaques = GenereAttaques(objVectPiece, occupation);
		}
		uint64MasqueAttaques |= objVectPiece.uint64MasqueAttaques;
		pos_pieces ^= static_cast<uint_fast64_t>(1) << index_piece;
	}
	uint64MasqueAttaquesAdv = static_cast<uint_fast64_t>(0);
	// Nécessaire : l'analyse ne génère pas tous les mouvements et donc pas toutes les attaques...
	pos_pieces = side[bNoirsAuTrait];
	while (_BitScanForward64(&index_piece, pos_pieces)) {
		Piece & objVectPiece = *_board[index_piece];
		if (
			// Si la pièce déplacée est dans le rayon d'action de la pièce courante :
			static_cast<uint_fast64_t>(0) != (objVectPiece.uint64MasqueAttaques & uint64MasqueCoup) ||
			// Ou s'il s'agit de la pièce déplacée :
			objVectPiece.uint64MasquePosition == uint64MasqueArrivee)
		{
			objVectPiece.uint64MasqueAttaques = GenereAttaques(objVectPiece, occupation);
		}
		uint64MasqueAttaquesAdv |= objVectPiece.uint64MasqueAttaques;
		pos_pieces ^= static_cast<uint_fast64_t>(1) << index_piece;
	}
	return;
}

inline void Moteur::_JoueCoup(uint_fast16_t uint16Coup, uint_fast8_t nPiecePromue)
{
	Historique temp;
	_JoueCoup(temp, uint16Coup, nPiecePromue);
	_GenereBitboardsListeMouvements();
}

inline std::array<Piece *, 2> Moteur::_JoueCoup(Historique & histCourant, uint_fast16_t uint16Coup, uint_fast8_t nPiecePromue)
{
	uint_fast8_t nCaseDepart;
	uint_fast8_t nCaseArrivee;

	histCourant.nRegle50Coups = nRegle50Coups;
	histCourant.EnPassant = EnPassant;
	histCourant.nEnPassant = nEnPassant;
	for (int i = 0; i < 4; ++i)
		histCourant.bRoques[i] = bRoques[i];
	histCourant.uint64MasqueAttaques = uint64MasqueAttaques;
	histCourant.nCoupsDisponibles = nCoupsDisponibles;

	unsigned long index = static_cast<unsigned long>(0);
	uint_fast64_t arrivees = occupation;
	while (_BitScanForward64(&index, arrivees)) {
		histCourant.attaques[index] = _board[index]->uint64MasqueAttaques;
		histCourant.mouvements[index] = _board[index]->uint64MasqueMouvements;
		arrivees ^= static_cast<uint_fast64_t>(1) << index;
	}

	vectHashPositionCourant.push_back(vectHashPositionCourant.back());
	++nRegle50Coups;
	if (bNoirsAuTrait)
		++nCoupsJoues;

	nCaseDepart = uint16Coup >> 8;
	nCaseArrivee = uint16Coup & 0x00FF;

	Piece * lpPiecePrise = nullptr;
	if (side[!bNoirsAuTrait] & static_cast<uint_fast64_t>(static_cast<uint_fast64_t>(1) << nCaseArrivee)) {
		lpPiecePrise = _board[nCaseArrivee];

		occupation ^= lpPiecePrise->uint64MasquePosition;
		side[!bNoirsAuTrait] ^= lpPiecePrise->uint64MasquePosition;
		vectHashPositionCourant.back() ^= lpPiecePrise->BaseHashTable[nCaseArrivee];
		_board[nCaseArrivee] = nullptr;

		nRegle50Coups = 0;
	}
	Piece * lpPiecePromue = nullptr;

	if (uint_fast8_t(1) == (_board[nCaseDepart]->nPiece & uint_fast8_t(7)))
		nRegle50Coups = 0;
	if (0 != nPiecePromue) {
		lpPiecePromue = _board[nCaseDepart];
		/**/
		occupation ^= lpPiecePromue->uint64MasquePosition;
		side[bNoirsAuTrait] ^= lpPiecePromue->uint64MasquePosition;
		vectHashPositionCourant.back() ^= lpPiecePromue->BaseHashTable[nCaseDepart];
		_board[nCaseDepart] = nullptr;

		uint_fast64_t * BaseHashTableTemp = BaseHashTable[mapCodeIndexHashPiece.find(nPiecePromue)->second];
		_board[nCaseDepart] = new Piece(nPiecePromue, bNoirsAuTrait, nCaseDepart, BaseHashTableTemp);
		vectHashPositionCourant.back() ^= BaseHashTableTemp[nCaseDepart];
		/**/
	}
	// Mouvements :
	Piece * lpPieceDeplacee = _board[nCaseDepart];
	occupation ^= lpPieceDeplacee->uint64MasquePosition;
	side[bNoirsAuTrait] ^= lpPieceDeplacee->uint64MasquePosition;

	_board[nCaseArrivee] = lpPieceDeplacee;
	_board[nCaseDepart] = nullptr;
	lpPieceDeplacee->uint8Case = nCaseArrivee;
	lpPieceDeplacee->uint64MasquePosition = static_cast<uint_fast64_t>(1) << nCaseArrivee;
	vectHashPositionCourant.back() ^= lpPieceDeplacee->BaseHashTable[nCaseDepart];
	vectHashPositionCourant.back() ^= lpPieceDeplacee->BaseHashTable[nCaseArrivee];

	occupation ^= lpPieceDeplacee->uint64MasquePosition;
	side[bNoirsAuTrait] ^= lpPieceDeplacee->uint64MasquePosition;

	/*if (1 == (itVectPieceDeplacee->nPiece & 0x7)) {
		if(abs(nCaseArrivee - nCaseDepart) == 16)
			;
	}*/
	// A mettre à jour :
	/*bRoques[4];
	uint_fast64_t EnPassant;
	int nEnPassant;*/

	bNoirsAuTrait = !bNoirsAuTrait;

	// Finalisation du hash :
	/*uint_fast64_t uint64Hash;
	// 0 : Trait aux noirs;
	uint64Hash = 0;
	for (int i = 0; i < 4; ++i)
	{
		if (bRoques[i])
			uint64Hash ^= HashRoques[i];
	}*/
	vectHashPositionCourant.back() ^= HashTraitAuxNoirs;
	_GenereBitboardListeAttaques(uint16Coup);
	//_GenereBitboardsListeCoups(uint16Coup);
	return { lpPiecePromue, lpPiecePrise };
}

inline std::array<Piece *, 2> Moteur::_JouePrise(Historique & histCourant, uint_fast16_t uint16Coup, uint_fast8_t nPiecePromue)
{
	uint_fast8_t nCaseDepart;
	uint_fast8_t nCaseArrivee;

	histCourant.nRegle50Coups = nRegle50Coups;
	histCourant.EnPassant = EnPassant;
	histCourant.nEnPassant = nEnPassant;
	for (int i = 0; i < 4; ++i)
		histCourant.bRoques[i] = bRoques[i];
	histCourant.uint64MasqueAttaques = uint64MasqueAttaques;
	histCourant.nCoupsDisponibles = nCoupsDisponibles;

	unsigned long index = static_cast<unsigned long>(0);
	uint_fast64_t arrivees = occupation;
	while (_BitScanForward64(&index, arrivees)) {
		histCourant.attaques[index] = _board[index]->uint64MasqueAttaques;
		histCourant.mouvements[index] = _board[index]->uint64MasqueMouvements;
		arrivees ^= static_cast<uint_fast64_t>(1) << index;
	}

	vectHashPositionCourant.push_back(vectHashPositionCourant.back());
	++nRegle50Coups;
	if (bNoirsAuTrait)
		++nCoupsJoues;

	nCaseDepart = uint16Coup >> 8;
	nCaseArrivee = uint16Coup & 0x00FF;

	Piece * lpPiecePrise = nullptr;
	lpPiecePrise = _board[nCaseArrivee];

	occupation ^= lpPiecePrise->uint64MasquePosition;
	side[!bNoirsAuTrait] ^= lpPiecePrise->uint64MasquePosition;
	vectHashPositionCourant.back() ^= lpPiecePrise->BaseHashTable[nCaseArrivee];
	_board[nCaseArrivee] = nullptr;

	nRegle50Coups = 0;

	Piece * lpPiecePromue = nullptr;

	if (uint_fast8_t(1) == (_board[nCaseDepart]->nPiece & uint_fast8_t(7)))
		nRegle50Coups = 0;
	if (0 != nPiecePromue) {
		lpPiecePromue = _board[nCaseDepart];
		/**/
		occupation ^= lpPiecePromue->uint64MasquePosition;
		side[bNoirsAuTrait] ^= lpPiecePromue->uint64MasquePosition;
		vectHashPositionCourant.back() ^= lpPiecePromue->BaseHashTable[nCaseDepart];
		_board[nCaseDepart] = nullptr;

		uint_fast64_t * BaseHashTableTemp = BaseHashTable[mapCodeIndexHashPiece.find(nPiecePromue)->second];
		_board[nCaseDepart] = new Piece(nPiecePromue, bNoirsAuTrait, nCaseDepart, BaseHashTableTemp);
		vectHashPositionCourant.back() ^= BaseHashTableTemp[nCaseDepart];
		/**/
	}
	// Mouvements :
	Piece * lpPieceDeplacee = _board[nCaseDepart];
	occupation ^= lpPieceDeplacee->uint64MasquePosition;
	side[bNoirsAuTrait] ^= lpPieceDeplacee->uint64MasquePosition;

	_board[nCaseArrivee] = lpPieceDeplacee;
	_board[nCaseDepart] = nullptr;
	lpPieceDeplacee->uint8Case = nCaseArrivee;
	lpPieceDeplacee->uint64MasquePosition = static_cast<uint_fast64_t>(1) << nCaseArrivee;
	vectHashPositionCourant.back() ^= lpPieceDeplacee->BaseHashTable[nCaseDepart];
	vectHashPositionCourant.back() ^= lpPieceDeplacee->BaseHashTable[nCaseArrivee];

	occupation ^= lpPieceDeplacee->uint64MasquePosition;
	side[bNoirsAuTrait] ^= lpPieceDeplacee->uint64MasquePosition;

	/*if (1 == (itVectPieceDeplacee->nPiece & 0x7)) {
		if(abs(nCaseArrivee - nCaseDepart) == 16)
			;
	}*/
	// A mettre à jour :
	/*bRoques[4];
	uint_fast64_t EnPassant;
	int nEnPassant;*/

	bNoirsAuTrait = !bNoirsAuTrait;

	// Finalisation du hash :
	/*uint_fast64_t uint64Hash;
	// 0 : Trait aux noirs;
	uint64Hash = 0;
	for (int i = 0; i < 4; ++i)
	{
		if (bRoques[i])
			uint64Hash ^= HashRoques[i];
	}*/
	vectHashPositionCourant.back() ^= HashTraitAuxNoirs;
	//uint64MasqueAttaques = _GenereBitboardListeAttaques(uint16Coup);
	_GenereBitboardsListePrises(uint16Coup);
	return { lpPiecePromue, lpPiecePrise };
}

inline void Moteur::_AnnuleCoup(Historique & histCourant, uint_fast16_t uint16Coup, std::array<Piece *, 2> ProRem)
{
	vectHashPositionCourant.pop_back();

	uint_fast8_t nCaseDepart = uint16Coup >> 8;
	uint_fast8_t nCaseArrivee = uint16Coup & 0x00FF;
	// On remet la pièce sur sa case de départ :
	Piece * lpPieceDeplacee = _board[nCaseArrivee];
	{
		occupation ^= lpPieceDeplacee->uint64MasquePosition;
		side[!bNoirsAuTrait] ^= lpPieceDeplacee->uint64MasquePosition;

		_board[nCaseDepart] = lpPieceDeplacee;
		_board[nCaseArrivee] = nullptr;
		lpPieceDeplacee->uint8Case = nCaseDepart;
		lpPieceDeplacee->uint64MasquePosition = static_cast<uint_fast64_t>(1) << nCaseDepart;
		occupation ^= lpPieceDeplacee->uint64MasquePosition;
		side[!bNoirsAuTrait] ^= lpPieceDeplacee->uint64MasquePosition;
	}

	if (nullptr != ProRem[0]) {
		// Promotion : restoration de la pièce de départ :
		delete _board[nCaseDepart];
		_board[nCaseDepart] = ProRem[0];
	}
	if (nullptr != ProRem[1]) {
		// Prise : restoration de la pièce prise :
		_board[nCaseArrivee] = ProRem[1];
		occupation ^= ProRem[1]->uint64MasquePosition;
		side[bNoirsAuTrait] ^= ProRem[1]->uint64MasquePosition;
	}

	if (bNoirsAuTrait)
		--nCoupsJoues;
	nRegle50Coups = histCourant.nRegle50Coups;
	EnPassant = histCourant.EnPassant;
	nEnPassant = histCourant.nEnPassant;
	for (int i = 0; i < 4; ++i)
		bRoques[i] = histCourant.bRoques[i];
	uint64MasqueAttaques = histCourant.uint64MasqueAttaques;
	nCoupsDisponibles = histCourant.nCoupsDisponibles;
	bNoirsAuTrait = !bNoirsAuTrait;

	unsigned long index = static_cast<unsigned long>(0);
	uint_fast64_t arrivees = occupation;
	while (_BitScanForward64(&index, arrivees)) {
		_board[index]->uint64MasqueAttaques = histCourant.attaques[index];
		_board[index]->uint64MasqueMouvements = histCourant.mouvements[index];
		arrivees ^= static_cast<uint_fast64_t>(1) << index;
	}
}
#endif

Moteur::Moteur(Moteur& mtrSrc)
{
	EnPassant = mtrSrc.EnPassant;
	for(int j = 0; j <8; ++j)
		HashColonnesPriseEnPassant[j] = mtrSrc.HashColonnesPriseEnPassant[j];
	for (int k = 0; k < 4; ++k)
		bRoques[k] = mtrSrc.bRoques[k];
	for (int k = 0; k < 4; ++k)
		HashRoques[k] = mtrSrc.HashRoques[k];
	vectCoupsDisponibles = mtrSrc.vectCoupsDisponibles;
	arrPieces[0] = mtrSrc.arrPieces[0];
	arrPieces[1] = mtrSrc.arrPieces[1];
	nEnPassant = mtrSrc.nEnPassant;
	Partie = mtrSrc.Partie;
	bNoirsAuTrait = mtrSrc.bNoirsAuTrait;
	HashTraitAuxNoirs = mtrSrc.HashTraitAuxNoirs;
	for (int i = 0; i < 64; ++i) {
		if (mtrSrc._board[i] != nullptr) {
			_board[i] = new Piece(*mtrSrc._board[i]);
			if(mtrSrc.objVectPieceRoi[0] == mtrSrc._board[i])
				objVectPieceRoi[0] = _board[i];
			if (mtrSrc.objVectPieceRoi[1] == mtrSrc._board[i])
				objVectPieceRoi[1] = _board[i];
		}
		else {
			_board[i] = nullptr;
		}
	}

	vectHashPositionCourant = mtrSrc.vectHashPositionCourant;
	nRegle50Coups = mtrSrc.nRegle50Coups;
	nCoupsJoues = mtrSrc.nCoupsJoues;
	nCoupsDisponibles = mtrSrc.nCoupsDisponibles;
	HashTraitAuxNoirs = mtrSrc.HashTraitAuxNoirs;
	for (int x = 0; x < 12; ++x) {
		for (int y = 0; y < 64; ++y) {
			BaseHashTable[x][y] = mtrSrc.BaseHashTable[x][y];
		}
	}
	uint64MasqueAttaquesAdv = mtrSrc.uint64MasqueAttaquesAdv;
	uint64MasqueAttaques = mtrSrc.uint64MasqueAttaques;
	occupation = mtrSrc.occupation;
	side[0] = mtrSrc.side[0];
	side[1] = mtrSrc.side[1];
	if (occupation != (side[0] | side[1]))
		throw(1);
}

void thrSubAnalyseInit(Moteur & engine, std::vector<CoupEvaluation>::iterator it, int nProfondeurCourante, uint_fast16_t uint16Coup, int & cur, int total, std::atomic<uint_fast64_t> & nodes)
{
	/*if (!Moteur::GetEvaluationFromHash(engine.vectHashPositionCourant.back(), it->Evaluation, nProfondeurCourante))*/ {
#ifdef _FAST
		Moteur::Historique histCourant;
		auto temp = engine._JoueCoup(histCourant, uint16Coup, 0);
#else
		engine.JoueCoup(uint16Coup, 0);
#endif
		++nodes;
#ifdef _FAST
		//i.Evaluation = -AnalysePVS(nProfondeurCourante - 1, -beta, -alpha, ++nodes, i.uint16Coup);
		it->Evaluation = -engine.AnalysePVS(nProfondeurCourante - 1, -32767, 32767, nodes, uint16Coup);
		//it->Evaluation = -engine.AnalysePVSRec(nProfondeurCourante - 1, -32767, 32767, nodes, uint16Coup, temp[1] == nullptr);
		//i.Evaluation = -Analyse(nProfondeurCourante - 1, -32767, 32767, ++nodes);
#else
		//i.Evaluation = -AnalysePVS(nProfondeurCourante - 1, -32767, -alpha, ++nodes);
		it->Evaluation = -engine.AnalysePVS(nProfondeurCourante - 1, -32767, 32767, nodes);
		//i.Evaluation = -Analyse(nProfondeurCourante - 1, -32767, 32767, ++nodes);
#endif
#ifdef _FAST
		engine._AnnuleCoup(histCourant, uint16Coup, temp);
#else
		engine.AnnuleCoup();
#endif
	}
}

inline void Moteur::subAnalyseInit(int nThread, std::vector<CoupEvaluation>::iterator & _it, int nProfondeurCourante, int & cur, int total, std::atomic<uint_fast64_t> & nodes, std::chrono::steady_clock::time_point & _begin)
{
	auto it = _it;
	std::cout << "info";
	std::cout << " currmove " << TraduitCoup(it->uint16Coup);
	std::cout << " currmovenumber " << ++cur << "/" << total;
	std::cout << " depth " << nProfondeurCourante;
	std::cout << std::endl;
	if (nThread > 1) {
		Moteur mtrTemp = *this;
		std::thread analyse(thrSubAnalyseInit, std::ref(mtrTemp), it, nProfondeurCourante, it->uint16Coup, std::ref(cur), total, std::ref(nodes));
		if (++_it != vectListeCoupsEvaluations.end() && nThread)// > 1)
			subAnalyseInit(--nThread, _it, nProfondeurCourante, cur, total, nodes, _begin);
		analyse.join();
	}
	else {
		++_it;
		thrSubAnalyseInit(*this, it, nProfondeurCourante, it->uint16Coup, cur, total, nodes);
	}
	std::cout << "info";
	std::cout << " currmove " << TraduitCoup(it->uint16Coup);
	std::cout << " score ";
	if (it->Evaluation == 32767)
		std::cout << "mate " << (nProfondeurCourante >> 1);
	else if (it->Evaluation == -32767)
		std::cout << "mate -" << (nProfondeurCourante >> 1);
	else
		std::cout << "cp " << it->Evaluation;
	std::cout << " nodes " << nodes;
	std::cout << " depth " << nProfondeurCourante;
	auto _endtemp = std::chrono::high_resolution_clock::now();
	auto elapsedtime = std::chrono::duration_cast<std::chrono::milliseconds>(_endtemp - _begin).count();
	if (elapsedtime != 0) {
		std::cout << " time " << elapsedtime;
		std::cout << " nps " << nodes / elapsedtime * 1000;
	}
	std::cout << std::endl;
}

inline void Moteur::getCurLine(uint_fast16_t bestMove)
{
	bool continuer = false;
	std::cout << "info currline";
	std::vector<Historique> vHistoriqueCourant;
	std::vector<std::array<Piece *, 2>> vRetours;
	std::vector<uint_fast16_t> vCoups;
	do {
		Historique histCourant;
		vRetours.push_back(std::move(_JoueCoup(histCourant, bestMove, 0)));
		vCoups.push_back(bestMove);
		std::cout << " " << TraduitCoup(bestMove);
		vHistoriqueCourant.push_back(std::move(histCourant));
		auto HashPositionCourant = vectHashPositionCourant.back();
		const auto & eval = transTableItem(transpositionTableAtomic[static_cast<uint_fast32_t>(HashPositionCourant & tableRoot)]);
		continuer = eval.hash == HashPositionCourant && eval.uint16Coup != static_cast<uint_fast16_t>(0);
		if (continuer) {
			bestMove = eval.uint16Coup;
		}
	} while (continuer);
	while (!vCoups.empty()) {
		_AnnuleCoup(vHistoriqueCourant.back(), vCoups.back(), vRetours.back());
		vRetours.pop_back();
		vCoups.pop_back();
		vHistoriqueCourant.pop_back();
	}
	std::cout << std::endl;
}

void Moteur::AnalyseInit(int nProfondeurMaximale, int nThread)
{
	int nProfondeurCourante = 1;

	vectListeCoupsEvaluations.clear();
#ifdef _FAST
	unsigned long index_depart = static_cast<unsigned long>(0);
	nCoupsDisponibles = 0;
	for (uint_fast64_t depart = side[bNoirsAuTrait]; _BitScanForward64(&index_depart, depart); depart ^= static_cast<uint_fast64_t>(1) << index_depart) {
		unsigned long index = static_cast<unsigned long>(0);
		for (uint_fast64_t arrivees = _board[index_depart]->uint64MasqueMouvements; _BitScanForward64(&index, arrivees); arrivees ^= static_cast<uint_fast64_t>(1) << index) {
			if (_CoupValide(*_board[index_depart], index)) {
				++nCoupsDisponibles;
				uint_fast16_t coup = (_board[index_depart]->uint8Case << 8) | index;
				vectListeCoupsEvaluations.push_back({ coup, 0 });
			}
		}
	}
#else
	if (!vectCoupsDisponibles.empty()) {
		std::sort(vectCoupsDisponibles.begin(), vectCoupsDisponibles.end());
		for (const uint_fast16_t & coup : vectCoupsDisponibles)
			vectListeCoupsEvaluations.push_back({ coup, 0 });
	}
#endif
	if (!vectListeCoupsEvaluations.empty()) {
#ifndef _FAST
		auto vectCoupsDisponiblesTemp = vectCoupsDisponibles;
#endif
		bool mat = false;
		auto total = vectListeCoupsEvaluations.size();
		std::atomic<uint_fast64_t> nodes = 0;

		auto _begin = std::chrono::high_resolution_clock::now();
		auto bestMove = vectListeCoupsEvaluations.front().uint16Coup;
		for (; (nProfondeurMaximale < 0 || nProfondeurCourante <= nProfondeurMaximale) && bContinuerAnalyse /*&& !mat*/; ++nProfondeurCourante) {
			int cur = 0;
			int alpha = -32767;
			int beta = -alpha;
			std::vector<CoupEvaluation>::iterator it = vectListeCoupsEvaluations.begin();
			while (it != vectListeCoupsEvaluations.end()) {
				subAnalyseInit(nThread, it, nProfondeurCourante, cur, total, nodes, _begin);
			}
			if (!vectListeCoupsEvaluations.empty()) {
				std::sort(vectListeCoupsEvaluations.begin(), vectListeCoupsEvaluations.end(), std::greater<>());
				auto bestEval = vectListeCoupsEvaluations.front().Evaluation;
				bestMove = vectListeCoupsEvaluations.front().uint16Coup;
				SetEvaluationToHash(vectHashPositionCourant.back(), alpha, nProfondeurCourante, bestMove);
				auto it = std::find_if(vectListeCoupsEvaluations.begin(), vectListeCoupsEvaluations.end(),
					[](CoupEvaluation & ceCur) { return -32767 == ceCur.Evaluation; });
				if (vectListeCoupsEvaluations.end() != it) {
					if (vectListeCoupsEvaluations.begin() == it)
						vectListeCoupsEvaluations.clear();
					else
						vectListeCoupsEvaluations.erase(it, vectListeCoupsEvaluations.end());
				}
				std::cout << "info";
				std::cout << " pv " << TraduitCoup(bestMove);
				std::cout << " score ";
				if (bestEval == 32767)
					std::cout << "mate " << (nProfondeurCourante >> 1);
				else if (bestEval == -32767)
					std::cout << "mate -" << (nProfondeurCourante >> 1);
				else
					std::cout << "cp " << bestEval;
				std::cout << " nodes " << nodes;
				std::cout << std::endl;
				getCurLine(bestMove);
			}
		}
		auto _end = std::chrono::high_resolution_clock::now();
		std::cout << "info";
		auto elapsedtime = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _begin).count();
		if (elapsedtime != 0) {
			std::cout << " time " << elapsedtime;
			std::cout << " nps " << nodes / elapsedtime * 1000;
		}
		std::cout << std::endl;
		std::cout << "bestmove " << TraduitCoup(bestMove) << std::endl;
		//getCurLine(bestMove);
#ifndef _FAST
		vectCoupsDisponibles.clear();
		vectCoupsDisponibles = vectCoupsDisponiblesTemp;
#endif
	}
	else {
		std::cout << "info";
		int bestEval = Evalue();
		std::cout << " score ";
		if (bestEval == 32767)
			std::cout << "mate " << 0;
		else if (bestEval == -32767)
			std::cout << "mate -" << 0;
		else
			std::cout << "cp " << bestEval;
		std::cout << std::endl;
	}
}

#ifdef _FAST
int Moteur::Analyse(int nProfondeur, int alpha, int beta, uint_fast64_t & nodes)
{
	int nEvaluation = -32767;
	if (nProfondeur != 0) {
		Historique histCourant;
		unsigned long index_depart = static_cast<unsigned long>(0);
		nCoupsDisponibles = 0;
		for (uint_fast64_t depart = side[bNoirsAuTrait]; _BitScanForward64(&index_depart, depart); depart ^= static_cast<uint_fast64_t>(1) << index_depart) {
			_board[index_depart]->uint64MasqueAttaques = _board[index_depart]->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
			GenereMouvements(*_board[index_depart], occupation, side);
			if (_board[index_depart] == objVectPieceRoi[bNoirsAuTrait])
				objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements = _andn_u64(uint64MasqueAttaques, objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements);
			unsigned long index = static_cast<unsigned long>(0);
			for (uint_fast64_t arrivees = _board[index_depart]->uint64MasqueMouvements; _BitScanForward64(&index, arrivees); arrivees ^= static_cast<uint_fast64_t>(1) << index) {
				if (_CoupValide(*_board[index_depart], index)) {
					++nCoupsDisponibles;
					uint_fast16_t coup = (_board[index_depart]->uint8Case << 8) | index;
					std::array<Piece *, 2> temp = _JoueCoup(histCourant, coup, 0);
					nEvaluation = std::max(nEvaluation, -Analyse(nProfondeur - 1, -beta, -alpha, ++nodes));
					_AnnuleCoup(histCourant, coup, temp);
					alpha = std::max(alpha, nEvaluation);
					/*if (alpha > beta)
						return alpha;/**/
				}
			}
		}
		if (nCoupsDisponibles == 0) {
			nEvaluation = Evalue();
		}
	}
	else {
		nEvaluation = Evalue();
	}

	return nEvaluation;
}
#else
int Moteur::Analyse(int nProfondeur, int alpha, int beta, uint_fast64_t & nodes)
{
	int nEvaluation = -32767;
	if (GetEvaluationFromHash(vectHashPositionCourant.back(), nEvaluation, nProfondeur))
		return nEvaluation;
	if (nProfondeur != 0 && !vectCoupsDisponibles.empty()) {
		std::sort(vectCoupsDisponibles.begin(), vectCoupsDisponibles.end());
		auto _vectCoupsDisponibles = vectCoupsDisponibles;
		for (const uint_fast16_t & coup : _vectCoupsDisponibles) {
			JoueCoup(coup, 0);
			vCurLigne.push_back(coup);
			nEvaluation = std::max(nEvaluation, -Analyse(nProfondeur - 1, -beta, -alpha, ++nodes));
			vCurLigne.pop_back();
			AnnuleCoup();
			alpha = std::max(alpha, nEvaluation);
			/*if (alpha > beta)
				break;/**/
		}
	}
	else {
		nEvaluation = Evalue();
	}

	SetEvaluationToHash(vectHashPositionCourant.back(), nEvaluation, nProfondeur);
	return nEvaluation;
}
#endif
#ifdef _FAST
int Moteur::AnalysePVS(int nProfondeur, int alpha, int beta, std::atomic<uint_fast64_t> & nodes, uint_fast16_t _coup)
{
	int nEvaluation = -32767;
	if (!GetEvaluationFromHash(vectHashPositionCourant.back(), nEvaluation, nProfondeur)) {
		if (nProfondeur != 0) {
			Historique histCourant;
			bool bFirst = true;
			uint_fast16_t meilleur_coup;
			unsigned long index_depart = static_cast<unsigned long>(0);
			nCoupsDisponibles = 0;
			for (uint_fast64_t depart = side[bNoirsAuTrait]; _BitScanForward64(&index_depart, depart); depart ^= static_cast<uint_fast64_t>(1) << index_depart) {
				_board[index_depart]->uint64MasqueAttaques = _board[index_depart]->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
				GenereMouvements(*_board[index_depart], occupation, side);
				if (_board[index_depart] == objVectPieceRoi[bNoirsAuTrait])
					objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements = _andn_u64(uint64MasqueAttaques, objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements);
				unsigned long index = static_cast<unsigned long>(0);
				for (uint_fast64_t arrivees = _board[index_depart]->uint64MasqueMouvements; _BitScanForward64(&index, arrivees); arrivees ^= static_cast<uint_fast64_t>(1) << index) {
					/*if (_CoupValide(*_board[index_depart], index)) */ {
						uint_fast16_t coup = (_board[index_depart]->uint8Case << 8) | index;
						std::array<Piece *, 2> temp = _JoueCoup(histCourant, coup, 0);
						if (static_cast<uint_fast64_t>(0) == (objVectPieceRoi[!bNoirsAuTrait]->uint64MasquePosition & uint64MasqueAttaquesAdv)) {
							++nCoupsDisponibles;
							if (bFirst) {
								meilleur_coup = coup;
								++nodes;
								nEvaluation = std::max(nEvaluation, -AnalysePVS(nProfondeur - 1, -beta, -alpha, nodes, coup));
								bFirst = false;
							}
							else {
								// Exploration :
								++nodes;
								nEvaluation = std::max(nEvaluation, -AnalysePVS(nProfondeur - 1, -alpha - 1, -alpha, nodes, coup));
								if (alpha < nEvaluation && nEvaluation < beta) {
									++nodes;
									nEvaluation = std::max(nEvaluation, -AnalysePVS(nProfondeur - 1, -beta, -alpha, nodes, coup));
								}
							}
						}
						_AnnuleCoup(histCourant, coup, temp);
						if (alpha < nEvaluation) {
							meilleur_coup = coup;
							alpha = nEvaluation;
						}
						//alpha = std::max(alpha, nEvaluation);
						/**/if (alpha > beta) {
							SetEvaluationToHash(vectHashPositionCourant.back(), alpha, nProfondeur, meilleur_coup);
							return alpha;
						}/**/
					}
				}
			}
			if (bFirst) {
				// Pas de coups disponibles.
				if (static_cast<uint_fast64_t>(0) != (uint64MasqueAttaques & objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition))
					nEvaluation = -32767;
				else
					nEvaluation = 0;
				//SetEvaluationToHash(vectHashPositionCourant.back(), nEvaluation, nProfondeur, 0);
			}
			else {
				SetEvaluationToHash(vectHashPositionCourant.back(), alpha, nProfondeur, meilleur_coup);
			}
		}
		else {
			nEvaluation = Evalue(_coup);
			//SetEvaluationToHash(vectHashPositionCourant.back(), nEvaluation, nProfondeur, _coup);
		}
	}
	return nEvaluation;// > 0 ? nEvaluation - nProfondeur : nEvaluation + nProfondeur;
}

#else
int Moteur::AnalysePVS(int nProfondeur, int alpha, int beta, std::atomic<uint_fast64_t> & nodes)
{
	++nodes;
	int nEvaluation = -32767;
	if (GetEvaluationFromHash(vectHashPositionCourant.back(), nEvaluation, nProfondeur))
		return nEvaluation;
	if (nProfondeur != 0 && !vectCoupsDisponibles.empty()) {
		std::sort(vectCoupsDisponibles.begin(), vectCoupsDisponibles.end());
		auto _vectCoupsDisponibles = vectCoupsDisponibles;
		auto it = _vectCoupsDisponibles.begin();
		if(it != _vectCoupsDisponibles.end()) {
			JoueCoup(*it, 0);
			vCurLigne.push_back(*it);
			nEvaluation = std::max(nEvaluation, -AnalysePVS(nProfondeur - 1, -beta, -alpha, nodes));
			vCurLigne.pop_back();
			AnnuleCoup();
			alpha = std::max(alpha, nEvaluation);
			/**/if (alpha <= beta) {
				for (++it; it != _vectCoupsDisponibles.end(); ++it) {
					JoueCoup(*it, 0);
					vCurLigne.push_back(*it);
					// Exploration :
					auto _vectCoupsDisponiblesPVS = vectCoupsDisponibles;
					nEvaluation = std::max(nEvaluation, -AnalysePVS(nProfondeur - 1, -alpha - 1, -alpha, nodes));
					if (alpha < nEvaluation && nEvaluation < beta) {
						vectCoupsDisponibles = _vectCoupsDisponiblesPVS;
						nEvaluation = std::max(nEvaluation, -AnalysePVS(nProfondeur - 1, -beta, -alpha, nodes));
					}
					vCurLigne.pop_back();
					AnnuleCoup();
					alpha = std::max(alpha, nEvaluation);
					/**/if (alpha > beta) {
						break;/**/
					}
				}
			}
		}
	}
	else {
		nEvaluation = Evalue();
	}
	
	SetEvaluationToHash(vectHashPositionCourant.back(), nEvaluation, nProfondeur);
	return nEvaluation;
}
#endif

#ifdef _FAST
inline bool Moteur::MouvementExiste()
{
	unsigned long index_depart = static_cast<unsigned long>(0);
	uint_fast64_t departs = side[bNoirsAuTrait];
	while (_BitScanForward64(&index_depart, departs)) {
		auto uint64MasqueMouvements = _board[index_depart]->uint64MasqueMouvements;
		GenereMouvements(*_board[index_depart], occupation, side);
		if (_board[index_depart] == objVectPieceRoi[bNoirsAuTrait])
			objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements = _andn_u64(uint64MasqueAttaques, objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements);
		unsigned long index = static_cast<unsigned long>(0);
		for (uint_fast64_t arrivees = _board[index_depart]->uint64MasqueMouvements; _BitScanForward64(&index, arrivees); arrivees ^= static_cast<uint_fast64_t>(1) << index) {
			if (_CoupValide(*_board[index_depart], index)) {
				_board[index_depart]->uint64MasqueMouvements = uint64MasqueMouvements;
				return true;
			}
		}
		departs ^= static_cast<uint_fast64_t>(1) << index_depart;
		_board[index_depart]->uint64MasqueMouvements = uint64MasqueMouvements;
	}
	return false;
}

int Moteur::AnalysePrises(int nProfondeur, int alpha, int beta, std::atomic<uint_fast64_t> & nodes, uint_fast16_t _coup)
{
	int nEvaluation = -32767;
	if (nProfondeur != 0) {
		Historique histCourant;
		bool bFirst = true;
		unsigned long index_depart = static_cast<unsigned long>(0);
		nCoupsDisponibles = 0;
		for (uint_fast64_t depart = side[bNoirsAuTrait]; _BitScanForward64(&index_depart, depart); depart ^= static_cast<uint_fast64_t>(1) << index_depart) {
			//_board[index_depart]->uint64MasqueAttaques = _board[index_depart]->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
			/*GenereMouvements(*_board[index_depart], occupation, side);
			if (_board[index_depart] == objVectPieceRoi[bNoirsAuTrait])
				objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements = _andn_u64(uint64MasqueAttaques, objVectPieceRoi[bNoirsAuTrait]->uint64MasqueMouvements);*/
			unsigned long index = static_cast<unsigned long>(0);
			for (uint_fast64_t arrivees = _board[index_depart]->uint64MasqueAttaques; _BitScanForward64(&index, arrivees); arrivees ^= static_cast<uint_fast64_t>(1) << index) {
				if (static_cast<uint_fast64_t>(0) != (side[!bNoirsAuTrait] & (static_cast<uint_fast64_t>(1) << index))) {
					++nCoupsDisponibles;
					uint_fast16_t coup = (_board[index_depart]->uint8Case << 8) | index;
					std::array<Piece *, 2> temp = _JouePrise(histCourant, coup, 0);
					if (static_cast<uint_fast64_t>(0) == (objVectPieceRoi[!bNoirsAuTrait]->uint64MasquePosition & uint64MasqueAttaquesAdv)) {
						if (bFirst) {
							++nodes;
							nEvaluation = std::max(nEvaluation, -AnalysePrises(nProfondeur - 1, -beta, -alpha, nodes, coup));
							bFirst = false;
						}
						else {
							// Exploration :
							++nodes;
							nEvaluation = std::max(nEvaluation, -AnalysePrises(nProfondeur - 1, -alpha - 1, -alpha, nodes, coup));
							if (alpha < nEvaluation && nEvaluation < beta) {
								++nodes;
								nEvaluation = std::max(nEvaluation, -AnalysePrises(nProfondeur - 1, -beta, -alpha, nodes, coup));
							}
						}
					}
					_AnnuleCoup(histCourant, coup, temp);
					alpha = std::max(alpha, nEvaluation);
					/**/if (alpha > beta)
						return alpha;/**/
				}
			}
		}
		if (bFirst) {
			nEvaluation = _Evalue();
		}
	}
	else {
		nEvaluation = _Evalue();
	}

	return nEvaluation;
}

inline int Moteur::Evalue(uint_fast16_t uint16Coup)
{
	int nEvaluation = 0;
	if (100 > nRegle50Coups) {
		if (!MouvementExiste()) {
			if (static_cast<uint_fast64_t>(0) != (uint64MasqueAttaques & objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition))
				nEvaluation = -32767;
			else
				nEvaluation = 0;
		}
		else {
			/*
			unsigned long index = static_cast<unsigned long>(0);
			uint_fast64_t arrivees = side[bNoirsAuTrait];
			while (_BitScanForward64(&index, arrivees)) {
				nEvaluation += _board[index]->nValeur;
				//nEvaluation += _mm_popcnt_u64(_board[index]->uint64MasqueAttaques) * 50;
				arrivees ^= static_cast<uint_fast64_t>(1) << index;
			}
			arrivees = side[!bNoirsAuTrait];
			while (_BitScanForward64(&index, arrivees)) {
				nEvaluation -= _board[index]->nValeur;
				//nEvaluation -= _mm_popcnt_u64(_board[index]->uint64MasqueAttaques) * 50;
				arrivees ^= static_cast<uint_fast64_t>(1) << index;
			}
			/**/
			/**/
			std::atomic<uint_fast64_t> nodes = 0;
			if(static_cast<uint_fast64_t>(0) != (uint64MasqueAttaques & side[bNoirsAuTrait]))
				nEvaluation = AnalysePrises( -1, -32767, 32767, nodes, uint16Coup);
			/**/
			//nEvaluation = ((_mm_popcnt_u64(attaques[bNoirsAuTrait]) - _mm_popcnt_u64(attaques[!bNoirsAuTrait])) & 0xFF) * 50;
			/*
			_mm_popcnt_u64(1); // Nombre de uns.
			_lzcnt_u64(0); // Zéros du début.
			_tzcnt_u64(0); // Zéros de la fin.
			*/
		}
	}
	return nEvaluation;
}

inline int Moteur::_Evalue()
{
	int nEvaluation = 0;
	unsigned long index = static_cast<unsigned long>(0);
	uint_fast64_t arrivees = side[bNoirsAuTrait];
	while (_BitScanForward64(&index, arrivees)) {
		//nEvaluation += _board[index]->nValeur;
		nEvaluation += _mm_popcnt_u64(_board[index]->uint64MasqueAttaques) * 50;
		arrivees ^= static_cast<uint_fast64_t>(1) << index;
	}
	arrivees = side[!bNoirsAuTrait];
	while (_BitScanForward64(&index, arrivees)) {
		//nEvaluation -= _board[index]->nValeur;
		nEvaluation -= _mm_popcnt_u64(_board[index]->uint64MasqueAttaques) * 50;
		arrivees ^= static_cast<uint_fast64_t>(1) << index;
	}
	return nEvaluation;
}
#endif

inline int Moteur::Evalue()
{
	int nEvaluation = 0;
	if (100 > nRegle50Coups /*&& !GetEvaluationFromHash(vectHashPositionCourant.back(), nEvaluation)*/) {
		if (nCoupsDisponibles == 0 /*!CoupsDisponibles()*/) {
#ifdef _FAST
			if (0 != (uint64MasqueAttaques & objVectPieceRoi[bNoirsAuTrait]->uint64MasquePosition))
#else		
			if (bEstEchec())
#endif
				nEvaluation = -32767;
			else
				nEvaluation = 0;
		}
		else {
#ifdef _FAST
			unsigned long index = static_cast<unsigned long>(0);
			uint_fast64_t arrivees = side[bNoirsAuTrait];
			while (_BitScanForward64(&index, arrivees)) {
				nEvaluation += _board[index]->nValeur;
				arrivees ^= static_cast<uint_fast64_t>(1) << index;
			}
			arrivees = side[!bNoirsAuTrait];
			while (_BitScanForward64(&index, arrivees)) {
				nEvaluation -= _board[index]->nValeur;
				arrivees ^= static_cast<uint_fast64_t>(1) << index;
			}
#else
			for (const Piece & i : arrPieces[bNoirsAuTrait])
				nEvaluation += i.nValeur;
			for (const Piece & i : arrPieces[!bNoirsAuTrait])
				nEvaluation -= i.nValeur;
#endif
			//nEvaluation = ((_mm_popcnt_u64(attaques[bNoirsAuTrait]) - _mm_popcnt_u64(attaques[!bNoirsAuTrait])) & 0xFF) * 50;
			/*
			_mm_popcnt_u64(1); // Nombre de uns.
			_lzcnt_u64(0); // Zéros du début.
			_tzcnt_u64(0); // Zéros de la fin.
			*/
		}
		//SetEvaluationToHash(vectHashPositionCourant.back(), nEvaluation);
	}
	return nEvaluation; // _bNoirsAuTrait == bNoirsAuTrait ? nEvaluation : -nEvaluation;
}

Moteur::Moteur()
{
	for (int i = 0; i < 64; ++i) {
		_board[i] = nullptr;
	}
	bContinuerAnalyse = true;
	bAnalyseEncours = false;
}

Moteur::~Moteur()
{
	for (int i = 0; i < 64; ++i) {
		if (_board[i] != nullptr)
			delete _board[i];
		_board[i] = nullptr;
	}
}

void Moteur::bSetAnalyseEnCours()
{
	bAnalyseEncours = true;
}

void Moteur::bUnsetAnalyseEnCours()
{
	bAnalyseEncours = false;
}

bool Moteur::bGetAnalyseEnCours()
{
	return bAnalyseEncours;
}

void Moteur::ClearHash()
{
	unsigned long long _level = 0;
	for (unsigned long long index = 0; index < tableRoots.size(); ++index) {
		if (tableRoots[index] == tableRoot) {
			_level = index;
			break;
		}
	}
	SetHash(_level);
}

void Moteur::SetHash(unsigned long long _level)
{
	transpositionTableAtomic.reset();
	if (0 < _level) {
		unsigned int level = std::min(_level, tableRoots.size() - 1);
		tableRoot = tableRoots[level];
		try {
			transpositionTableAtomic = std::make_unique<std::atomic<transTableItem>[]>(tableRoot);
		}
		catch (std::exception) {
			--level;
			SetHash(level);
		}
	}
	else {
		//transpositionTableAtomic.reset();
		transpositionTableAtomic = std::make_unique<std::atomic<transTableItem>[]>(0);
	}
}

inline bool Moteur::GetEvaluationFromHash(const uint_fast64_t & HashPositionCourant, int & nEvaluation, int nDepth)
{
	bool bRetour = false;
	//const auto & eval = transpositionTable[static_cast<uint_fast32_t>(HashPositionCourant & tableRoot)];
	const auto & eval = transTableItem(transpositionTableAtomic[static_cast<uint_fast32_t>(HashPositionCourant & tableRoot)]);
	if (eval.hash == HashPositionCourant && eval.nDepth > nDepth) {
		nEvaluation = eval.nEvaluation;
		bRetour = true;
	}
	return bRetour;
}

inline void Moteur::SetEvaluationToHash(const uint_fast64_t & HashPositionCourant, int nEvaluation, int nDepth, uint_fast16_t uint16Coup)
{
	//const auto & eval = transpositionTable[static_cast<uint_fast32_t>(HashPositionCourant & tableRoot)];
	//const auto & eval = transTableItem(transpositionTableAtomic[static_cast<uint_fast32_t>(HashPositionCourant & tableRoot)]);
	const transTableItem & eval = transpositionTableAtomic[static_cast<uint_fast32_t>(HashPositionCourant & tableRoot)];
	if (eval.hash != HashPositionCourant || (eval.hash == HashPositionCourant && eval.nDepth <= nDepth)) {
		transpositionTableAtomic[static_cast<uint_fast32_t>(HashPositionCourant & tableRoot)] = { HashPositionCourant, nEvaluation, nDepth, uint16Coup };
	}
}

inline bool Moteur::GetEvaluationPrisesFromHash(const uint_fast64_t & HashPositionCourant, int & nEvaluation)
{
	bool bRetour = false;
	/**/mtxHashtablePrises.lock();
	if (!hashTablePrises.empty()) {
		std::unordered_map<uint_fast64_t, int>::iterator itHashTable = hashTablePrises.find(HashPositionCourant);
		if (hashTablePrises.end() != itHashTable) {
			nEvaluation = itHashTable->second;
			bRetour = true;
		}
	}
	mtxHashtablePrises.unlock();
	/**/
	return bRetour;
}

inline void Moteur::SetEvaluationPrisesToHash(const uint_fast64_t & HashPositionCourant, int nEvaluation)
{
	mtxHashtablePrises.lock();
	if (hashTablePrises.size() == hashTablePrises.max_size())
		hashTablePrises.erase(hashTablePrises.begin());
	hashTablePrises.emplace(HashPositionCourant, nEvaluation);
	mtxHashtablePrises.unlock();
}

void Moteur::ChargePosition(const std::string & strPositionUCI)
{
	std::vector<std::string> vectMots;
	auto _strPositionUCI = strPositionUCI;
	trim_all(_strPositionUCI);
	split(_strPositionUCI, vectMots);
	std::vector<std::string>::iterator itVectMots;
	itVectMots = vectMots.begin();
	if (vectMots.end() != itVectMots && "startpos" == *itVectMots)
	{
		ChargePositionFEN(strStartPos);
		++itVectMots;
	}
	else if (vectMots.end() != itVectMots && "fen" == *itVectMots)
	{
		// "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
		std::string strFenString;
		++itVectMots;
		if (vectMots.end() != itVectMots)
		{
			strFenString.append(*itVectMots);// "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"
			++itVectMots;
			if (vectMots.end() != itVectMots)
			{
				strFenString.append(" ");
				strFenString.append(*itVectMots);// "w"
				++itVectMots;
				if (vectMots.end() != itVectMots)
				{
					strFenString.append(" ");
					strFenString.append(*itVectMots);// "KQkq"
					++itVectMots;
					if (vectMots.end() != itVectMots)
					{
						strFenString.append(" ");
						strFenString.append(*itVectMots);// "-"
						++itVectMots;
						if (vectMots.end() != itVectMots)
						{
							strFenString.append(" ");
							strFenString.append(*itVectMots);// "0"
							++itVectMots;
							if (vectMots.end() != itVectMots)
							{
								strFenString.append(" ");
								strFenString.append(*itVectMots);// "1"
#ifdef _FAST
								for (int i = 0; i < 64; ++i) {
									if (_board[i] != nullptr)
										delete _board[i];
									_board[i] = nullptr;
								}
#endif
								ChargePositionFEN(strFenString);
								++itVectMots;
							}
						}
					}
				}
			}
		}
	}
	// Moves :
#ifdef _FAST
	for (int nside = 0; nside < 2; ++nside) {
		for (const auto & refPiece : arrPieces[nside]) {
			_board[refPiece.uint8Case] = new Piece(refPiece);
			if (refPiece.nPiece == (nside ? 14 : 6))
				objVectPieceRoi[nside] = _board[refPiece.uint8Case];
		}
	}
#endif
	if (vectMots.end() != itVectMots && "moves" == *itVectMots) {
		for (++itVectMots; vectMots.end() != itVectMots; ++itVectMots) {
			uint_fast16_t uint16Coup;
			int nPiece = TraduitCoup(*itVectMots, uint16Coup);
#ifdef _FAST
			_JoueCoup(uint16Coup, nPiece);
#else
			JoueCoup(uint16Coup, nPiece);
#endif
		}
	}
}
