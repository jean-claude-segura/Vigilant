#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <mutex>
#include <array>
#include <intrin.h>

class Piece
{
public:
    uint_fast64_t uint64MasquePosition;
    uint_fast8_t uint8Case;
    uint_fast64_t uint64MasqueAttaques;
    uint_fast64_t uint64MasqueMouvements;
	uint_fast16_t nValeur;
    uint_fast64_t * BaseHashTable;
	std::array<uint_fast64_t, 64>::const_iterator BaseAttaques;
    std::array<uint_fast64_t, 64>::const_iterator BaseMouvements; //hg, bg, bd, hd.
	uint_fast8_t nPiece;
    bool bNoir;

    Piece(uint_fast8_t, bool, uint_fast8_t, uint_fast64_t *);
	Piece(const Piece &);
	Piece();
    ~Piece();
	bool operator == (const Piece&);
};

constexpr uint_fast64_t gauche(uint_fast64_t*fou, int i, uint_fast64_t g) {
	if (i + 9 < 64) {
		if ((i + 9) >> 3 == (i >> 3) + 1) {
			return fou[i + 9] = gauche(fou, i + 9, g | (g << 9));
		}
	}
	return g;
};

constexpr uint_fast64_t droite(uint_fast64_t*fou, int i, uint_fast64_t d) {
	if (i + 7 < 64) {
		if ((i + 7) >> 3 == (i >> 3) + 1) {
			return fou[i + 7] = droite(fou, i + 7, d | (d << 7));
		}
	}
	return d;
};

constexpr uint_fast64_t zz(uint_fast64_t i) {
	uint_fast64_t p = static_cast<uint_fast64_t>(1) << i;
	while ((p + 7) >> 3 == (p >> 3) + 1) {
		p |= p >> 7;
	}
	return p;
}

constexpr std::array<std::array<uint_fast64_t, 64>, 4> GenereMouvementsTourCpx()
{
	std::array<std::array<uint_fast64_t, 64>, 4> uint64MouvementsCpx = {};
	// Mouvements tour :
	for (uint_fast64_t i = 0; i < 64; ++i) {
		uint_fast64_t uint64pos = static_cast<uint_fast64_t>(i);
		// Haut :
		uint64MouvementsCpx[0][i] = static_cast<uint_fast64_t>(0x0101010101010101) << uint64pos;
		uint64MouvementsCpx[0][i] &= ~(static_cast<uint_fast64_t>(1) << uint64pos);
		// Droite:
		uint64MouvementsCpx[1][i] = static_cast<uint_fast64_t>(0xFF00000000000000) >> (static_cast<uint_fast64_t>(63) - uint64pos);
		auto decal = (uint64pos >> 3) << 3;
		uint64MouvementsCpx[1][i] = uint64MouvementsCpx[1][i] >> decal;
		uint64MouvementsCpx[1][i] &= static_cast<uint_fast64_t>(0x00FFFFFFFFFFFFFF);
		uint64MouvementsCpx[1][i] = uint64MouvementsCpx[1][i] << decal;
		uint64MouvementsCpx[1][i] &= ~(static_cast<uint_fast64_t>(1) << uint64pos);
		// Bas :
		uint64MouvementsCpx[2][i] = static_cast<uint_fast64_t>(0x8080808080808080) >> (static_cast<uint_fast64_t>(63) - uint64pos);
		uint64MouvementsCpx[2][i] &= ~(static_cast<uint_fast64_t>(1) << uint64pos);
		// Gauche :
		uint64MouvementsCpx[3][i] = static_cast<uint_fast64_t>(0xFF) << uint64pos;
		decal = 56 - ((uint64pos >> 3) << 3);
		uint64MouvementsCpx[3][i] = uint64MouvementsCpx[3][i] << decal;
		uint64MouvementsCpx[3][i] = uint64MouvementsCpx[3][i] >> decal;
		uint64MouvementsCpx[3][i] &= ~(static_cast<uint_fast64_t>(1) << uint64pos);
	}
	return uint64MouvementsCpx;
}

constexpr std::array<std::array<uint_fast64_t, 64>, 4> GenereMouvementsFouCpx()
{
	std::array<std::array<uint_fast64_t, 64>, 4> uint64MouvementsCpx = {};
	// Mouvements fou :
	// HD :
	for (unsigned int i = 0; i < 64; ++i) {
		auto p = i;
		uint64MouvementsCpx[0][i] = static_cast<uint_fast64_t>(0);
		while (p <= 55 && (p + 7) >> 3 == (p >> 3) + 1) {
			p += 7;
			uint64MouvementsCpx[0][i] |= static_cast<uint_fast64_t>(1) << p;
		}
	}

	// BD
	for (unsigned int i = 0; i < 64; ++i) {
		auto p = i;
		uint64MouvementsCpx[1][i] = static_cast<uint_fast64_t>(0);
		while (p > 8 && (p - 9) >> 3 == (p >> 3) - 1) {
			p -= 9;
			uint64MouvementsCpx[1][i] |= static_cast<uint_fast64_t>(1) << p;
		}
	}

	// BG
	for (unsigned int i = 0; i < 64; ++i) {
		auto p = i;
		uint64MouvementsCpx[2][i] = static_cast<uint_fast64_t>(0);
		while (p >= 8 && (p - 7) >> 3 == (p >> 3) - 1) {
			p -= 7;
			uint64MouvementsCpx[2][i] |= static_cast<uint_fast64_t>(1) << p;
		}
	}

	// HG
	for (unsigned int i = 0; i < 64; ++i) {
		auto p = i;
		uint64MouvementsCpx[3][i] = static_cast<uint_fast64_t>(0);
		while (p < 55 && (p + 9) >> 3 == (p >> 3) + 1) {
			p += 9;
			uint64MouvementsCpx[3][i] |= static_cast<uint_fast64_t>(1) << p;
		}
	}

	return uint64MouvementsCpx;
}

constexpr std::array<std::array<uint_fast64_t, 64>, 10> GenereMouvements()
{
	std::array<std::array<uint_fast64_t, 64>, 10> uint64Mouvements = {};

	// Mouvements tour :
	for (uint_fast64_t i = 0; i < 64; ++i) {
		uint_fast64_t uint64pos = static_cast<uint_fast64_t>(i);
		uint64Mouvements[4][i] = static_cast<uint_fast64_t>(0xFF) << ((uint64pos >> static_cast<uint_fast64_t>(3)) << static_cast<uint_fast64_t>(3));
		//uint64Mouvements[4][i] |= static_cast<uint_fast64_t>(0x0101010101010101) << (i % 8);
		uint64Mouvements[4][i] |= static_cast<uint_fast64_t>(0x0101010101010101) << (uint64pos - ((uint64pos >> static_cast<uint_fast64_t>(3)) << static_cast<uint_fast64_t>(3)));
		uint64Mouvements[4][i] &= ~(static_cast<uint_fast64_t>(1) << uint64pos);
	}

	// Mouvements fou :
	uint_fast64_t fou_gauche[64] = {};
	uint_fast64_t fou_droite[64] = {};
	for (int i = 0; i < 64; ++i) {
		if (fou_gauche[i] == 0) fou_gauche[i] = gauche(fou_gauche, i, static_cast<uint_fast64_t>(1) << i);
		if (fou_droite[i] == 0) fou_droite[i] = droite(fou_droite, i, static_cast<uint_fast64_t>(1) << i);
		uint_fast64_t p = static_cast<uint_fast64_t>(0x1) << i;
		uint64Mouvements[3][i] = (fou_gauche[i] | fou_droite[i]) & (~p);
	}

	// Mouvements dame : tour + fou :
	for (uint_fast64_t i = 0; i < 64; ++i) {
		uint64Mouvements[5][i] = uint64Mouvements[4][i] | uint64Mouvements[3][i];
	}

	// Mouvements roi :
	for (uint_fast64_t i = 0; i < 64; ++i) {
		if (i <= 56 && (i + 7) >> 3 == (i >> 3) + 1)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i + 7);
		if (i <= 54 && (i + 9) >> 3 == (i >> 3) + 1)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i + 9);
		if (i >= 7 && (i - 7) >> 3 == (i >> 3) - 1)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i - 7);
		if (i >= 9 && (i - 9) >> 3 == (i >> 3) - 1)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i - 9);
		if (i >= 8)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i - 8);
		if (i <= 55)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i + 8);
		if (i % 8)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i - 1);
		if (i % 8 != 7)
			uint64Mouvements[6][i] |= static_cast<uint_fast64_t>(1) << (i + 1);
	}

	// Mouvements cavalier :
	for (uint_fast64_t i = 0; i < 64; ++i) {
		if (i <= 47 && (i + 15) >> 3 == (i >> 3) + 2)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i + 15);
		if (i <= 46 && (i + 17) >> 3 == (i >> 3) + 2)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i + 17);
		if (i <= 53 && (i + 10) >> 3 == (i >> 3) + 1)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i + 10);
		if (i <= 55 && (i + 6) >> 3 == (i >> 3) + 1)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i + 6);
		if (i >= 17 && (i - 17) >> 3 == (i >> 3) - 2)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i - 17);
		if (i >= 16 && (i - 15) >> 3 == (i >> 3) - 2)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i - 15);
		if (i >= 8 && (i - 6) >> 3 == (i >> 3) - 1)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i - 6);
		if (i >= 10 && (i - 10) >> 3 == (i >> 3) - 1)
			uint64Mouvements[2][i] |= static_cast<uint_fast64_t>(1) << (i - 10);
	}

	// Mouvements pions :
	for (uint_fast64_t i = 8; i < 56; ++i)
		uint64Mouvements[9][i] = static_cast<uint_fast64_t>(1) << (i - 8);
	for (uint_fast64_t i = 48; i < 56; ++i)
		uint64Mouvements[9][i] |= static_cast<uint_fast64_t>(1) << (i - 16);

	for (uint_fast64_t i = 8; i < 56; ++i)
		uint64Mouvements[1][i] = static_cast<uint_fast64_t>(1) << (i + 8);
	for (uint_fast64_t i = 8; i < 16; ++i)
		uint64Mouvements[1][i] |= static_cast<uint_fast64_t>(1) << (i + 16);

	return uint64Mouvements;
}

constexpr std::array<std::array<uint_fast64_t, 64>, 10>	GenereAttaques()
{
	std::array<std::array<uint_fast64_t, 64>, 10> uint64Attaques = GenereMouvements();
	// Attaques pions noirs :
	uint64Attaques[9] = {};
	for (uint_fast64_t i = 8; i < 56; ++i) {
		if ((i - 7) >> 3 == (i >> 3) - 1)
			uint64Attaques[9][i] |= static_cast<uint_fast64_t>(1) << (i - 7);
		if (i >= 9 && (i - 9) >> 3 == (i >> 3) - 1)
			uint64Attaques[9][i] |= static_cast<uint_fast64_t>(1) << (i - 9);
	}
	// Attaques pions blancs :
	uint64Attaques[1] = {};
	for (uint_fast64_t i = 8; i < 56; ++i) {
		// Attaques :
		if ((i + 7) >> 3 == (i >> 3) + 1)
			uint64Attaques[1][i] |= static_cast<uint_fast64_t>(1) << (i + 7);
		if (i <= 54 && (i + 9) >> 3 == (i >> 3) + 1)
			uint64Attaques[1][i] |= static_cast<uint_fast64_t>(1) << (i + 9);
	}
	return uint64Attaques;
}

constexpr std::array<std::array<uint_fast64_t, 64>, 4> uint64MouvementsTourCpx = GenereMouvementsTourCpx();
constexpr std::array<std::array<uint_fast64_t, 64>, 4> uint64MouvementsFouCpx = GenereMouvementsFouCpx();
constexpr std::array<std::array<uint_fast64_t, 64>, 10> uint64Mouvements = GenereMouvements();
constexpr std::array<std::array<uint_fast64_t, 64>, 10> uint64Attaques = GenereAttaques();

static const std::map<uint_fast8_t, uint_fast16_t> mapCodeValeurPiece = { { 1, 100 }, { 2, 300 }, { 3, 300 }, { 4, 500 }, { 5, 950 }, { 6, 2850 }, { 9, 100 } };
static const std::map<char, uint_fast8_t> mapCodeNumeroPiece =
{
    { 'p', uint_fast8_t(9) },
    { 'n', uint_fast8_t(10) },
    { 'b', uint_fast8_t(11) },
    { 'r', uint_fast8_t(12) },
    { 'q', uint_fast8_t(13) },
    { 'k', uint_fast8_t(14) },
    { 'P', uint_fast8_t(1) },
    { 'N', uint_fast8_t(2) },
    { 'B', uint_fast8_t(3) },
    { 'R', uint_fast8_t(4) },
    { 'Q', uint_fast8_t(5) },
    { 'K', uint_fast8_t(6) }
};

class moves {
	inline unsigned long long subGenereMouvement1(uint_fast64_t uint64MouvementsPiece, uint_fast64_t uint64MasquePosition, uint_fast64_t occupation)
	{
		return ((_blsi_u64(uint64MouvementsPiece & occupation) << 1) - uint64MasquePosition) & uint64MouvementsPiece;
	}

	inline unsigned long long subGenereMouvement2(uint_fast64_t uint64MouvementsPiece, uint_fast64_t uint64MasquePosition, uint_fast64_t occupation)
	{
		uint_fast64_t contact = uint64MouvementsPiece & occupation;
		if (contact) {
			unsigned long index;
			_BitScanReverse64(&index, contact);
			//return (uint64MasquePosition - _rotl64(static_cast<uint_fast64_t>(1), index)) & uint64MouvementsPiece;
			return (uint64MasquePosition - (static_cast<uint_fast64_t>(1) << index)) & uint64MouvementsPiece;
		}
		else {
			return uint64MouvementsPiece;
		}
	}

	void GenereMouvementsDame();
	void GenereMouvementsTour();
	void GenereMouvementsFou();
	void GenereMouvementsPionNoir();
	void GenereMouvementsPionBlanc();
public:
	static std::array<std::unique_ptr<uint_fast64_t[]>, 64> arrUint64MouvementsDame;
	static std::array<std::unique_ptr<uint_fast64_t[]>, 64> arrUint64MouvementsTour;
	static std::array<std::unique_ptr<uint_fast64_t[]>, 64> arrUint64MouvementsFou;
	static std::array<std::unique_ptr<uint_fast64_t[]>, 64> arrUint64MouvementsPionNoir;
	static std::array<std::unique_ptr<uint_fast64_t[]>, 64> arrUint64MouvementsPionBlanc;
	moves() {
		//GenereMouvementsDame();
		GenereMouvementsTour();
		GenereMouvementsFou();
		GenereMouvementsPionNoir();
		//GenereMouvementsPionBlanc();
	}
};

static const moves _moves;

inline uint_fast64_t GenereMouvements(Piece & objVectPiece, uint_fast64_t occupation, uint_fast64_t * side)
{
	uint_fast64_t attaques = static_cast<uint_fast64_t>(0);
	uint_fast8_t uint8Case = objVectPiece.uint8Case;
	uint_fast64_t arrivee = static_cast<uint_fast64_t>(0);

	bool bNoirsAuTrait = objVectPiece.bNoir;

	switch (objVectPiece.nPiece) {
	case 1: // Pion.
		objVectPiece.uint64MasqueAttaques = objVectPiece.BaseAttaques[uint8Case];
		attaques = objVectPiece.BaseAttaques[uint8Case];

		arrivee = ((_blsi_u64(objVectPiece.BaseMouvements[uint8Case] & occupation)) - objVectPiece.uint64MasquePosition) & objVectPiece.BaseMouvements[uint8Case];
		objVectPiece.uint64MasqueMouvements = arrivee | (objVectPiece.uint64MasqueAttaques & side[!bNoirsAuTrait]);
		break;
	case 9: // Pion.
	{
		objVectPiece.uint64MasqueAttaques = objVectPiece.BaseAttaques[uint8Case];
		attaques = objVectPiece.BaseAttaques[uint8Case];

		uint_fast64_t baseAttaques = objVectPiece.BaseMouvements[uint8Case];
		uint_fast64_t index = _pext_u64(occupation, baseAttaques);
		arrivee = moves::arrUint64MouvementsPionNoir[uint8Case][index];
		objVectPiece.uint64MasqueMouvements = arrivee | (objVectPiece.uint64MasqueAttaques & side[!bNoirsAuTrait]);
	}
	break;
	case 4: // Tour.
	case 12:
	{
		uint_fast64_t baseAttaques = objVectPiece.BaseAttaques[uint8Case];
		uint_fast64_t index = _pext_u64(occupation, baseAttaques);
		arrivee = moves::arrUint64MouvementsTour[uint8Case][index];

		objVectPiece.uint64MasqueAttaques = arrivee;
		attaques = objVectPiece.uint64MasqueAttaques;
		//objVectPiece.uint64MasqueMouvements = objVectPiece.uint64MasqueAttaques & (~side[bNoirsAuTrait]);
		objVectPiece.uint64MasqueMouvements = _andn_u64(side[bNoirsAuTrait], objVectPiece.uint64MasqueAttaques);
	}
		break;
	case 3: // Fou.
	case 11:
	{
		uint_fast64_t baseAttaques = objVectPiece.BaseAttaques[uint8Case];
		uint_fast64_t index = _pext_u64(occupation, baseAttaques);
		arrivee = moves::arrUint64MouvementsFou[uint8Case][index];

		objVectPiece.uint64MasqueAttaques = arrivee;
		attaques = objVectPiece.uint64MasqueAttaques;
		//objVectPiece.uint64MasqueMouvements |= objVectPiece.uint64MasqueAttaques & (~side[bNoirsAuTrait]);
		objVectPiece.uint64MasqueMouvements = _andn_u64(side[bNoirsAuTrait], objVectPiece.uint64MasqueAttaques);
	}
		break;
	case 5: // Dame.
	case 13:
	{
		uint_fast64_t baseAttaques = uint64Attaques[4][uint8Case]; // Tour.
		uint_fast64_t index = _pext_u64(occupation, baseAttaques);
		arrivee = moves::arrUint64MouvementsTour[uint8Case][index];

		baseAttaques = uint64Attaques[3][uint8Case]; // Fou.
		index = _pext_u64(occupation, baseAttaques);
		arrivee |= moves::arrUint64MouvementsFou[uint8Case][index];

		objVectPiece.uint64MasqueAttaques = arrivee;
		attaques = objVectPiece.uint64MasqueAttaques;
		//objVectPiece.uint64MasqueMouvements |= objVectPiece.uint64MasqueAttaques & (~side[bNoirsAuTrait]);
		objVectPiece.uint64MasqueMouvements = _andn_u64(side[bNoirsAuTrait], objVectPiece.uint64MasqueAttaques);
	}
		break;
	case 6: // Roi.
	case 14:
	case 2: // Cavalier.
	case 10:
		objVectPiece.uint64MasqueAttaques = objVectPiece.BaseMouvements[uint8Case];
		attaques = objVectPiece.uint64MasqueAttaques;
		//objVectPiece.uint64MasqueMouvements = objVectPiece.uint64MasqueAttaques & (~side[bNoirsAuTrait]);
		objVectPiece.uint64MasqueMouvements = _andn_u64(side[bNoirsAuTrait], objVectPiece.uint64MasqueAttaques);
		break;
	}
	return attaques;
}

inline uint_fast64_t GenereAttaques(Piece & objVectPiece, uint_fast64_t occupation)
{
	uint_fast64_t attaques = static_cast<uint_fast64_t>(0);
	uint_fast8_t uint8Case = objVectPiece.uint8Case;
	switch (objVectPiece.nPiece) {
	case 1: // Pion.
	case 9: // Pion.
		attaques = objVectPiece.BaseAttaques[uint8Case];
		break;
	case 4: // Tour.
	case 12:
	{
		uint_fast64_t baseAttaques = objVectPiece.BaseAttaques[uint8Case];
		uint_fast64_t index = _pext_u64(occupation, baseAttaques);
		attaques = moves::arrUint64MouvementsTour[uint8Case][index];
	}
		break;
	case 3: // Fou.
	case 11:
	{
		uint_fast64_t baseAttaques = objVectPiece.BaseAttaques[uint8Case];
		uint_fast64_t index = _pext_u64(occupation, baseAttaques);
		attaques = moves::arrUint64MouvementsFou[uint8Case][index];
	}
		break;
	case 5: // Dame.
	case 13:
	{
		uint_fast64_t baseAttaques = uint64Attaques[4][uint8Case]; // Tour.
		uint_fast64_t index = _pext_u64(occupation, baseAttaques);
		attaques = moves::arrUint64MouvementsTour[uint8Case][index];

		baseAttaques = uint64Attaques[3][uint8Case]; // Fou.
		index = _pext_u64(occupation, baseAttaques);
		attaques |= moves::arrUint64MouvementsFou[uint8Case][index];
	}
		break;
	case 6: // Roi.
	case 14:
	case 2: // Cavalier.
	case 10:
		attaques = objVectPiece.BaseAttaques[uint8Case];
		break;
	}
	return attaques;
}
