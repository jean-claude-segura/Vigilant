#include "common.h"
#include "Echiquier.hpp"

// ********************************************************************************************************************************************************
Echiquier::Echiquier()
{
	initHash();
	vectHashPositionCourant.push_back(0);
	occupation = static_cast<uint_fast64_t>(0);
	side[false] = static_cast<uint_fast64_t>(0);
	side[true] = static_cast<uint_fast64_t>(0);
	uint64MasqueAttaques = static_cast<uint_fast64_t>(0);
	arrPieces[false].resize(16);
	arrPieces[true].resize(16);
	nCoupsDisponibles = 0;
}

Echiquier::~Echiquier()
{
	VidePlateau();
}

void Echiquier::initHash(unsigned long ulSeed)
{
	std::mt19937_64 Generateur;
	std::vector< uint_fast64_t> vAlreadyUsedNumbers;

	Generateur.seed(ulSeed);

	std::mt19937_64::default_seed;
	std::uniform_int_distribution<uint_fast64_t> NombreAleatoire;
	for (uint_fast8_t uint8Case = 0; uint8Case <= uint_fast8_t(63); ++uint8Case)
	{
		for (int nPiece = 0; nPiece < 12; ++nPiece)
		{
			uint_fast64_t n;
			do {
				n = NombreAleatoire(Generateur);

			} while (vAlreadyUsedNumbers.end() != std::find(vAlreadyUsedNumbers.begin(), vAlreadyUsedNumbers.end(), n));
			vAlreadyUsedNumbers.push_back(n);
			BaseHashTable[nPiece][uint8Case] = n;
		}
	}

	uint_fast64_t n;
	do {
		n = NombreAleatoire(Generateur);

	} while (vAlreadyUsedNumbers.end() != std::find(vAlreadyUsedNumbers.begin(), vAlreadyUsedNumbers.end(), n));
	vAlreadyUsedNumbers.push_back(n);
	HashTraitAuxNoirs = n;
	for (int i = 0; i < 4; ++i) {
		do {
			n = NombreAleatoire(Generateur);

		} while (vAlreadyUsedNumbers.end() != std::find(vAlreadyUsedNumbers.begin(), vAlreadyUsedNumbers.end(), n));
		vAlreadyUsedNumbers.push_back(n);
		HashRoques[i] = NombreAleatoire(Generateur);
	}
	for (int i = 0; i < 8; ++i) {
		do {
			n = NombreAleatoire(Generateur);

		} while (vAlreadyUsedNumbers.end() != std::find(vAlreadyUsedNumbers.begin(), vAlreadyUsedNumbers.end(), n));
		vAlreadyUsedNumbers.push_back(n);
		HashColonnesPriseEnPassant[i] = n;
	}
}

void Echiquier::PositionInitiale()
{
	VidePlateau();
	ChargePositionFEN(strStartPos);
}

void Echiquier::VidePlateau()
{
	arrPieces[false].clear();
	arrPieces[true].clear();
	vectHashPositionCourant.clear();
	vectHashPositionCourant.push_back(0);
	occupation = static_cast<uint_fast64_t>(0);
	side[true] = 0;
	side[false] = 0;
}

bool Echiquier::ChargePositionFEN(std::string strFEN)
{
	bool bRetour = false;

	VidePlateau();

	std::map<char, uint_fast8_t>::const_iterator itMapCodeNumeroPiece;

	uint_fast8_t nIndex = 64;
	std::string::iterator itFEN = strFEN.begin();
	// Placement des pièces :
	for (; strFEN.end() != itFEN && ' ' != (*itFEN) && nIndex>0; ++itFEN)
	{
		itMapCodeNumeroPiece = mapCodeNumeroPiece.find(*itFEN);
		if (mapCodeNumeroPiece.end() != itMapCodeNumeroPiece)
		{
			AjoutePiece(itMapCodeNumeroPiece->second, --nIndex);
		}
		else if ('/' == (*itFEN))
		{
			// Passage à la ligne suivante :
		}
		else
		{
			// Saut de n case :
			nIndex -= (*itFEN) - '1' + 1;
		}
	}
	// Espace :
	if (strFEN.end() != itFEN)
		++itFEN;
	// Camp au trait :
	if (strFEN.end() != itFEN)
	{
		bNoirsAuTrait = (*itFEN == 'b');
		++itFEN;
	}
	// Espace :
	if (strFEN.end() != itFEN)
		++itFEN;
	// Roques :
	std::string strCodesRoques = "KQkq";
	for (int i = 0; i < 4; ++i)
	{
		if (strFEN.end() != itFEN)
		{
			bRoques[i] = (*itFEN == strCodesRoques[i]);
			if (bRoques[i])
				++itFEN;
		}
	}
	// Traitement de l'absence de roques :
	if (strFEN.end() != itFEN && (*itFEN == '-'))
	{
		++itFEN;
	}
	// Espace :
	if (strFEN.end() != itFEN)
		++itFEN;
	// Prise en passant :
	// Traitement de l'absence de prise en passant :
	if (strFEN.end() != itFEN)
	{
		if (*itFEN == '-')
		{
			++itFEN;
			nEnPassant = 0;
			EnPassant = 0;
		}
		else
		{
			char cX = *itFEN;
			++itFEN;
			if (strFEN.end() != itFEN)
			{
				char cY = *itFEN;
				nEnPassant = ('h' - cX) + ((cY - '1') << 3);
				EnPassant = static_cast<uint_fast64_t>(1) << nEnPassant;
			}
		}
	}
	// Espace :
	if (strFEN.end() != itFEN)
		++itFEN;

	nRegle50Coups = 0;
	while (strFEN.end() != itFEN && ' ' != *itFEN)
	{
		nRegle50Coups = nRegle50Coups * 10 + *itFEN - '1' + 1;
		++itFEN;
	}

	// Espace :
	if (strFEN.end() != itFEN)
		++itFEN;

	nCoupsJoues = 0;
	while (strFEN.end() != itFEN && ' ' != *itFEN)
	{
		nCoupsJoues = nCoupsJoues * 10 + *itFEN - '1' + 1;
		++itFEN;
		bRetour = strFEN.end() == itFEN;
	}

	// Finalisation du hash :
	uint_fast64_t uint64Hash;
	// 0 : Trait aux noirs;
	uint64Hash = bNoirsAuTrait ? HashTraitAuxNoirs : 0;
	for (int i = 0; i < 4; ++i)
	{
		if (bRoques[i])
			uint64Hash ^= HashRoques[i];
	}
	vectHashPositionCourant.back() ^= uint64Hash;

	uint_fast64_t bside[] = { static_cast<uint_fast64_t>(0), static_cast<uint_fast64_t>(0) };
	uint_fast64_t boccupation = static_cast<uint_fast64_t>(0);
	for (int nside = 0; nside < 2; ++nside) {
		for (Piece objVectPiece : arrPieces[nside]) {
			bside[objVectPiece.bNoir] |= objVectPiece.uint64MasquePosition;
			boccupation |= objVectPiece.uint64MasquePosition;
		}
	}
	if (boccupation != occupation)
		throw(1);
	if (bside[true] != side[true])
		throw(1);
	if (bside[false] != side[false])
		throw(1);

	GenereBitboardsListeCoups();

	if (boccupation != occupation)
		throw(1);
	if (bside[true] != side[true])
		throw(1);
	if (bside[false] != side[false])
		throw(1);

	return bRetour;
}

/*
	Indique si le roi du camp au trait est en échec.
*/
const bool Echiquier::bEstEchec()
{
	// On vérifie si le roi est sur l'une des cases attaquées.
	return 0 != (uint64MasqueAttaques & arrPieces[bNoirsAuTrait][0].uint64MasquePosition);
}

/*
	uint16Coup : [0xAABB] :
		AA : case de départ;
		BB : case d'arrivée.
	nPiecePromue : code de pièce après promotion.
*/
void Echiquier::JoueCoup(uint_fast16_t uint16Coup, uint_fast8_t nPiecePromue)
{
	std::vector<Piece>::iterator itVectPieceDeplacee;
	uint_fast8_t nCaseDepart;
	uint_fast8_t nCaseArrivee;

	Historique histCourant;
	histCourant.nRegle50Coups = nRegle50Coups;
	histCourant.nCoupsJoues = nCoupsJoues;
	//histCourant.vectCoupsDisponibles = vectCoupsDisponibles;
	histCourant.arrPieces[bNoirsAuTrait] = arrPieces[bNoirsAuTrait]; //vectPieces;
	histCourant.arrPieces[!bNoirsAuTrait] = arrPieces[!bNoirsAuTrait]; //vectPieces;
	histCourant.EnPassant = EnPassant;
	histCourant.nEnPassant = nEnPassant;
	for (int i = 0; i < 4; ++i)
		histCourant.bRoques[i] = bRoques[i];
	histCourant.occupation = occupation;
	histCourant.side[false] = side[false];
	histCourant.side[true] = side[true];
	histCourant.uint64MasqueAttaques = uint64MasqueAttaques;

	Partie.push_back(histCourant);

	vectHashPositionCourant.push_back(vectHashPositionCourant.back());
	++nRegle50Coups;
	if (bNoirsAuTrait)
		++nCoupsJoues;

	nCaseDepart = uint16Coup >> 8;
	nCaseArrivee = uint16Coup & uint_fast16_t(0x00FF);

	if (side[!bNoirsAuTrait] & static_cast<uint_fast64_t>(static_cast<uint_fast64_t>(1) << nCaseArrivee)) {
		auto itVectPiecePrise = GetPiece(nCaseArrivee, !bNoirsAuTrait);
		if (arrPieces[!bNoirsAuTrait].end() != itVectPiecePrise)
		{
			SupprimePiece(itVectPiecePrise);
			nRegle50Coups = 0;
		}
	}
	itVectPieceDeplacee = GetPiece(nCaseDepart, bNoirsAuTrait);

	if (uint_fast8_t(1) == (itVectPieceDeplacee->nPiece & uint_fast8_t(7)))
		nRegle50Coups = 0;
	if (uint_fast8_t(0) != nPiecePromue) {
		PromeutPiece(itVectPieceDeplacee, nPiecePromue);
		itVectPieceDeplacee = --(arrPieces[bNoirsAuTrait].end());
	}
	DeplacePiece(itVectPieceDeplacee, nCaseDepart, nCaseArrivee);

	/*if (1 == (itVectPieceDeplacee->nPiece & 0x7)) {
		if(abs(nCaseArrivee - nCaseDepart) == 16)
			;
	}*/
	// A mettre à jour :
	/*bRoques[4];
	uint_fast64_t EnPassant;
	int nEnPassant;*/

	/*std::map<uint_fast64_t, int>::iterator itMapHashPositionCompteur = mapHashPositionCompteur.find(vectHashPositionCourant.back());
	if (mapHashPositionCompteur.end() == itMapHashPositionCompteur)
		mapHashPositionCompteur[1] = 1;
	else
		itMapHashPositionCompteur->second++;/**/

	bNoirsAuTrait = !bNoirsAuTrait;

	// Finalisation du hash :
	/*uint_fast64_t uint64Hash;
	// 0 : Trait aux noirs;
	uint64Hash = bNoirsAuTrait ? HashTraitAuxNoirs : 0;
	for (int i = 0; i < 4; ++i)
	{
		if (bRoques[i])
			uint64Hash ^= HashRoques[i];
	}
	vectHashPositionCourant.back() ^= uint64Hash;*/
	vectHashPositionCourant.back() ^= HashTraitAuxNoirs;

	GenereBitboardsListeCoups(uint16Coup);
	//GenereBitboardsListeCoups();
}

void Echiquier::AnnuleCoup()
{
	/*std::map<uint_fast64_t, int>::iterator itMapHashPositionCompteur = mapHashPositionCompteur.find(vectHashPositionCourant.back());
	if (mapHashPositionCompteur.end() != itMapHashPositionCompteur)
	{
		if (1 == itMapHashPositionCompteur->second)
			mapHashPositionCompteur.erase(itMapHashPositionCompteur);
		else
			itMapHashPositionCompteur->second--;
	}/**/
	vectHashPositionCourant.pop_back();

	nCoupsJoues = Partie.back().nCoupsJoues;
	nRegle50Coups = Partie.back().nRegle50Coups;
	//vectCoupsDisponibles = Partie.back().vectCoupsDisponibles;
	arrPieces[bNoirsAuTrait] = Partie.back().arrPieces[bNoirsAuTrait];
	arrPieces[!bNoirsAuTrait] = Partie.back().arrPieces[!bNoirsAuTrait];
	EnPassant = Partie.back().EnPassant;
	nEnPassant = Partie.back().nEnPassant;
	for (int i = 0; i < 4; ++i)
		bRoques[i] = Partie.back().bRoques[i];

	side[false] = Partie.back().side[false];
	side[true] = Partie.back().side[true];
	uint64MasqueAttaques = Partie.back().uint64MasqueAttaques;
	occupation = Partie.back().occupation;
	bNoirsAuTrait = !bNoirsAuTrait;

	Partie.pop_back();
}

inline void Echiquier::PromeutPiece(std::vector<Piece>::iterator &itVectPiecePromue, uint_fast8_t nPiecePromue)
{
	uint_fast8_t uint8Case = itVectPiecePromue->uint8Case;
	SupprimePiece(itVectPiecePromue);
	AjoutePiece(nPiecePromue, uint8Case);
}

inline void Echiquier::DeplacePiece(std::vector<Piece>::iterator &itVectPieceDeplacee, uint_fast8_t nCaseDepart, uint_fast8_t nCaseArrivee)
{
	// Mouvements :
	occupation ^= itVectPieceDeplacee->uint64MasquePosition;
	side[itVectPieceDeplacee->bNoir] ^= itVectPieceDeplacee->uint64MasquePosition;

	itVectPieceDeplacee->uint8Case = nCaseArrivee;
	itVectPieceDeplacee->uint64MasquePosition = static_cast<uint_fast64_t>(1) << nCaseArrivee;
	vectHashPositionCourant.back() ^= itVectPieceDeplacee->BaseHashTable[nCaseDepart];
	vectHashPositionCourant.back() ^= itVectPieceDeplacee->BaseHashTable[nCaseArrivee];

	occupation ^= itVectPieceDeplacee->uint64MasquePosition;
	side[itVectPieceDeplacee->bNoir] ^= itVectPieceDeplacee->uint64MasquePosition;
}

inline void Echiquier::SupprimePiece(std::vector<Piece>::iterator &itVectPieceSupprimee)
{
	occupation ^= itVectPieceSupprimee->uint64MasquePosition;
	side[itVectPieceSupprimee->bNoir] ^= itVectPieceSupprimee->uint64MasquePosition;
	vectHashPositionCourant.back() ^= itVectPieceSupprimee->BaseHashTable[itVectPieceSupprimee->uint8Case];
	arrPieces[itVectPieceSupprimee->bNoir].erase(itVectPieceSupprimee);
}

inline void Echiquier::AjoutePiece(uint_fast8_t nPiece, uint_fast8_t uint8Pos)
{
	uint_fast64_t * BaseHashTableTemp = BaseHashTable[mapCodeIndexHashPiece.find(nPiece)->second];

	if (nPiece == uint_fast8_t(6))
		arrPieces[false].insert(arrPieces[false].begin(), std::move(Piece(nPiece, false, uint8Pos, BaseHashTableTemp)));
	else if (nPiece == uint_fast8_t(14))
		arrPieces[true].insert(arrPieces[true].begin(), std::move(Piece(nPiece, true, uint8Pos, BaseHashTableTemp)));
	else
		arrPieces[(nPiece >> 3) == 1].push_back(std::move(Piece(nPiece, (nPiece >> 3) == uint_fast8_t(1), uint8Pos, BaseHashTableTemp)));
	occupation |= static_cast<uint_fast64_t>(1) << uint8Pos;
	side[(nPiece >> 3) == uint_fast8_t(1)] |= static_cast<uint_fast64_t>(1) << uint8Pos;
	vectHashPositionCourant.back() ^= BaseHashTableTemp[uint8Pos];
}

bool Echiquier::CoupsDisponibles()
{
	return !vectCoupsDisponibles.empty();
}

inline uint_fast64_t FlipAndMirrorHorizontal(uint_fast64_t _x) {
	const uint_fast64_t k1 = static_cast<uint_fast64_t>(0x5555555555555555);
	const uint_fast64_t k2 = static_cast<uint_fast64_t>(0x3333333333333333);
	const uint_fast64_t k4 = static_cast<uint_fast64_t>(0x0f0f0f0f0f0f0f0f);
	uint_fast64_t x = _byteswap_uint64(_x);
	x = ((x >> 1) & k1) | ((x & k1) << 1);
	x = ((x >> 2) & k2) | ((x & k2) << 2);
	x = ((x >> 4) & k4) | ((x & k4) << 4);
	return x;
}

inline void Echiquier::ValideCoupAdv(Piece & objPiece)
{
	uint_fast64_t arrivees = objPiece.uint64MasqueMouvements;
	uint_fast16_t coup = objPiece.uint8Case << 8;
	unsigned long index = static_cast<unsigned long>(0);
	while (static_cast<uint_fast64_t>(0) != arrivees) {
		_BitScanForward64(&index, arrivees);

		// Joue le coup :
		auto arriveecur = static_cast<uint_fast64_t>(1) << index;
		auto occupationcur = occupation;
		occupationcur ^= objPiece.uint64MasquePosition;
		occupationcur |= arriveecur;
		auto prevPos = objPiece.uint64MasquePosition;
		objPiece.uint64MasquePosition = arriveecur; // objVectPieceRoi[this->bNoirsAuTrait]->uint64MasquePosition

		uint_fast64_t attaques = static_cast<uint_fast64_t>(0);
		// On calcule les attaques du camp adverse pour tester si un échec persiste après le mouvement:
		for (Piece & objVectPiece : arrPieces[bNoirsAuTrait]) {
			if (arriveecur != objVectPiece.uint64MasquePosition /* Une pièce prise n'attaque pas... */ &&
				/* Il n'y a pas d'échec possible si on n'attaque pas le roi... */
				static_cast<uint_fast64_t>(0) != (objVectPiece.BaseAttaques[objVectPiece.uint8Case] & arrPieces[!bNoirsAuTrait][0].uint64MasquePosition)) {
				attaques |= GenereAttaques(objVectPiece, occupationcur);
			}
		}

		bool bEchec = false;
		bEchec = static_cast<uint_fast64_t>(0) != (arrPieces[!bNoirsAuTrait][0].uint64MasquePosition & attaques);

		arrivees ^= _rotl64(static_cast<uint_fast64_t>(1), index);
		if (!bEchec) {
			coup |= index;
			coup &= 0xFF00;
		}
		else {
			// Mouvement invalide :
			objPiece.uint64MasqueMouvements ^= arriveecur;
		}

		// Annule le coup :
		objPiece.uint64MasquePosition = prevPos;
	}
}

inline void Echiquier::ValideCoup(Piece & objPiece)
{
	uint_fast64_t arrivees = objPiece.uint64MasqueMouvements;
	uint_fast16_t coup = objPiece.uint8Case << 8;
	unsigned long index = static_cast<unsigned long>(0);
	while (static_cast<uint_fast64_t>(0) != arrivees) {
		_BitScanForward64(&index, arrivees);

		// Joue le coup :
		auto arriveecur = static_cast<uint_fast64_t>(1) << index;
		auto occupationcur = occupation;
		occupationcur ^= objPiece.uint64MasquePosition;
		occupationcur |= arriveecur;
		auto prevPos = objPiece.uint64MasquePosition;
		objPiece.uint64MasquePosition = arriveecur; // objVectPieceRoi[this->bNoirsAuTrait]->uint64MasquePosition

		uint_fast64_t attaques = static_cast<uint_fast64_t>(0);
		// On calcule les attaques du camp adverse pour tester si un échec persiste après le mouvement:
		for (Piece & objVectPiece : arrPieces[!bNoirsAuTrait]) {
			if (arriveecur != objVectPiece.uint64MasquePosition /* Une pièce prise n'attaque pas... */ &&
				/* Il n'y a pas d'échec possible si on n'attaque pas le roi... */
				static_cast<uint_fast64_t>(0) != (objVectPiece.BaseAttaques[objVectPiece.uint8Case] & arrPieces[bNoirsAuTrait][0].uint64MasquePosition)) {
				attaques |= GenereAttaques(objVectPiece, occupationcur);
			}
		}

		bool bEchec = false;
		bEchec = static_cast<uint_fast64_t>(0) != (arrPieces[bNoirsAuTrait][0].uint64MasquePosition & attaques);

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

inline void Echiquier::ValideListeCoups()
{
	nCoupsDisponibles = 0;
	vectCoupsDisponibles.clear();
	for (Piece & objPiece : arrPieces[bNoirsAuTrait]) {
		ValideCoup(objPiece);
	}
}

std::vector<uint_fast16_t> & Echiquier::GenereBitboardsListeCoups()
{
	uint64MasqueAttaques = static_cast<uint_fast64_t>(0);
	for (Piece & objVectPiece : arrPieces[!bNoirsAuTrait]) {
		/**/objVectPiece.uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
		//uint64MasqueAttaques |= objVectPiece.uint64MasqueAttaques = GenereAttaques(objVectPiece, occupation);
		uint64MasqueAttaques |= GenereMouvements(objVectPiece, occupation, side);
		/*
		objVectPiece.uint64MasqueAttaques = objVectPiece.uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
		uint64MasqueAttaques |= objVectPiece.uint64MasqueAttaques = GenereMouvements(objVectPiece, occupation, bNoirsAuTrait, side);/**/
	}
	vectCoupsDisponibles.clear();
	nCoupsDisponibles = 0;
	auto itPiece = arrPieces[bNoirsAuTrait].begin();
	auto uint64MasqueAttaquesAuTrait = GenereMouvements(*itPiece, occupation, side);
	uint64MasqueAttaquesAdv = uint64MasqueAttaquesAuTrait;
	//arrPieces[bNoirsAuTrait][0].uint64MasqueMouvements &= ~uint64MasqueAttaques;
	arrPieces[bNoirsAuTrait][0].uint64MasqueMouvements = _andn_u64(uint64MasqueAttaques, arrPieces[bNoirsAuTrait][0].uint64MasqueMouvements);
	ValideCoup(*itPiece);
	for (++itPiece; itPiece != arrPieces[bNoirsAuTrait].end(); ++itPiece) {
		itPiece->uint64MasqueAttaques = itPiece->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
		// Aucune position légale n'autorise le cas où une pièce n'a aucun mouvement de base...
		/*if (objVectPiece.BaseMouvements[objVectPiece.uint8Case] != 0)*/ {
			uint64MasqueAttaquesAuTrait |= GenereMouvements(*itPiece, occupation, side);
			ValideCoup(*itPiece);
		}
		uint64MasqueAttaquesAdv |= uint64MasqueAttaquesAuTrait;
	}
	// Validation des coups du camp en attente :
	itPiece = arrPieces[!bNoirsAuTrait].begin();
	//GenereMouvements(*itPiece, occupation, side);
	//arrPieces[!bNoirsAuTrait][0].uint64MasqueMouvements &= ~uint64MasqueAttaquesAuTrait;
	arrPieces[!bNoirsAuTrait][0].uint64MasqueMouvements = _andn_u64(uint64MasqueAttaquesAuTrait, arrPieces[!bNoirsAuTrait][0].uint64MasqueMouvements);
	ValideCoupAdv(*itPiece);
	for (++itPiece; itPiece != arrPieces[!bNoirsAuTrait].end(); ++itPiece) {
		//itPiece->uint64MasqueAttaques = itPiece->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
		// Aucune position légale n'autorise le cas où une pièce n'a aucun mouvement de base...
		/*if (objVectPiece.BaseMouvements[objVectPiece.uint8Case] != 0)*/ {
			ValideCoupAdv(*itPiece);
		}
	}
	return vectCoupsDisponibles;
}

inline std::vector<uint_fast16_t> & Echiquier::GenereBitboardsListeCoups(uint_fast16_t uint16Coup)
{
	uint_fast64_t uint64MasqueDepart = static_cast<uint_fast64_t>(1) << (uint16Coup >> 8);
	uint_fast64_t uint64MasqueArrivee = static_cast<uint_fast64_t>(1) << (uint16Coup & 0x00FF);
	uint_fast64_t uint64MasqueCoup = (uint64MasqueDepart | uint64MasqueArrivee);

	uint64MasqueAttaques = static_cast<uint_fast64_t>(0);
	for (Piece & objVectPiece : arrPieces[!bNoirsAuTrait]) {
		if (
			// Si la pièce déplacée est dans le rayon d'action de la pièce courante :
			static_cast<uint_fast64_t>(0) != (objVectPiece.uint64MasqueAttaques & uint64MasqueCoup) ||
			// Ou s'il s'agit de la pièce déplacée :
			objVectPiece.uint64MasquePosition == uint64MasqueArrivee)
		{
			uint64MasqueAttaques |= objVectPiece.uint64MasqueAttaques = GenereAttaques(objVectPiece, occupation);
		}
		else {
			uint64MasqueAttaques |= objVectPiece.uint64MasqueAttaques;
		}
	}
	vectCoupsDisponibles.clear();
	nCoupsDisponibles = 0;
	auto itPiece = arrPieces[bNoirsAuTrait].begin();
	GenereMouvements(*itPiece, occupation, side);
	//arrPieces[bNoirsAuTrait][0].uint64MasqueMouvements &= ~uint64MasqueAttaques;
	arrPieces[bNoirsAuTrait][0].uint64MasqueMouvements = _andn_u64(uint64MasqueAttaques, arrPieces[bNoirsAuTrait][0].uint64MasqueMouvements);
	ValideCoup(*itPiece);
	for (++itPiece; itPiece != arrPieces[bNoirsAuTrait].end();++itPiece){
		itPiece->uint64MasqueAttaques = itPiece->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
		// Aucune position légale n'autorise le cas où une pièce n'a aucun mouvement de base...
		/*if (objVectPiece.BaseMouvements[objVectPiece.uint8Case] != 0)*/ {
			GenereMouvements(*itPiece, occupation, side);
			ValideCoup(*itPiece);
		}
	}
	//ValideListeCoups();
	return vectCoupsDisponibles;
}

/*
	Retrouve une pièce dans le vector des pièces à partir de la position et retourne un iterator vers cette dernière.
*/
inline std::vector<Piece>::iterator Echiquier::GetPiece(uint_fast8_t nPosition, bool bSide)
{
	return std::find_if(arrPieces[bSide].begin(), arrPieces[bSide].end(),
		[&nPosition](Piece & curPiece) { return nPosition == curPiece.uint8Case; });
}

/*
	Traduit un coup en notation algébrique.
	Promotion :
	-f7f8Q;
	-g2g1q.
*/
int TraduitCoup(const std::string &strMove, uint_fast16_t &uint16Coup)
{
	std::string::const_iterator itFEN = strMove.begin();
	int nPiecePromue = 0;
	char cX = *itFEN;
	++itFEN;
	if (strMove.end() != itFEN)
	{
		char cY = *itFEN;
		uint16Coup = ('h' - cX) + ((cY - '1') << 3);
		uint16Coup = uint16Coup << 8;

		++itFEN;
		if (strMove.end() != itFEN)
		{
			cX = *itFEN;
			++itFEN;
			if (strMove.end() != itFEN)
			{
				cY = *itFEN;
				uint16Coup = uint16Coup | ('h' - cX) + ((cY - '1') << 3);
				++itFEN;
				if (strMove.end() != itFEN)
				{
					nPiecePromue = mapCodeNumeroPiece.find(*itFEN)->second;
				}
			}
		}
	}
	return nPiecePromue;
}

std::string TraduitCoup(const uint_fast16_t coup)
{
	std::string strCoup;
	strCoup += char((7 - (coup >> 8) % 8) + 'a');
	strCoup += char(((coup >> 8) >> 3) + '1');
	strCoup += char((7 - (coup & 0x00FF) % 8) + 'a');
	strCoup += char(((coup & 0x00FF) >> 3) + '1');
	return strCoup;
}
