#include "common.h"
#include "Piece.hpp"

Piece::Piece(uint_fast8_t nPiece, bool bNoir, uint_fast8_t nPosition, uint_fast64_t * BaseHashTable)
{
	this->nPiece = nPiece;
	this->bNoir = bNoir;
	this->uint8Case = nPosition;
	this->uint64MasquePosition = static_cast<uint_fast64_t>(1) << nPosition;
	this->BaseHashTable = BaseHashTable;

	uint_fast8_t nIndexPiece = ((uint_fast8_t(9) == nPiece) ? nPiece : nPiece & uint_fast8_t(0x7));
	this->nValeur = mapCodeValeurPiece.find(nIndexPiece)->second;
	// Prises :
	this->BaseAttaques = uint64Attaques[nIndexPiece].begin();
	// Mouvements :
	this->BaseMouvements = uint64Mouvements[nIndexPiece].begin();
	/*this->uint64MasqueAttaques = uint64Attaques[nIndexPiece][nPosition];
	this->uint64MasqueMouvements = uint64Mouvements[nIndexPiece][nPosition];*/
	this->uint64MasqueAttaques = static_cast<uint_fast64_t>(0);
	this->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
}

Piece::Piece(const Piece& _pieceSource)
{
	this->nPiece = _pieceSource.nPiece;
	this->bNoir = _pieceSource.bNoir;
	this->uint8Case = _pieceSource.uint8Case;
	this->uint64MasquePosition = _pieceSource.uint64MasquePosition;
	this->BaseHashTable = _pieceSource.BaseHashTable;
	this->nValeur = _pieceSource.nValeur;
	this->BaseAttaques = _pieceSource.BaseAttaques;
	this->BaseMouvements = _pieceSource.BaseMouvements;
	this->uint64MasqueAttaques = _pieceSource.uint64MasqueAttaques;
	this->uint64MasqueMouvements = _pieceSource.uint64MasqueMouvements;
}

Piece::Piece()
{
	this->nPiece = 0;
	this->bNoir = false;
	this->uint8Case = 0;
	this->uint64MasquePosition = static_cast<uint_fast64_t>(0);
	this->BaseHashTable = nullptr;
	this->nValeur = 0;
	this->BaseAttaques = uint64Attaques[0].end();
	this->BaseMouvements = uint64Attaques[0].end();
	this->uint64MasqueAttaques = static_cast<uint_fast64_t>(0);
	this->uint64MasqueMouvements = static_cast<uint_fast64_t>(0);
}

Piece::~Piece()
{
}

bool Piece::operator==(const Piece & _pieceSource)
{
	bool egal = true;
	egal &= this->nPiece == _pieceSource.nPiece;
	egal &= this->bNoir == _pieceSource.bNoir;
	egal &= this->uint8Case == _pieceSource.uint8Case;
	egal &= this->uint64MasquePosition == _pieceSource.uint64MasquePosition;
	egal &= this->BaseHashTable == _pieceSource.BaseHashTable;
	egal &= this->nValeur == _pieceSource.nValeur;
	egal &= *this->BaseAttaques == *_pieceSource.BaseAttaques;
	egal &= *this->BaseMouvements == *_pieceSource.BaseMouvements;
	egal &= this->uint64MasqueAttaques == _pieceSource.uint64MasqueAttaques;
	egal &= this->uint64MasqueMouvements == _pieceSource.uint64MasqueMouvements;
	return egal;
}

std::array<std::unique_ptr<uint_fast64_t[]>, 64> moves::arrUint64MouvementsDame;
std::array<std::unique_ptr<uint_fast64_t[]>, 64> moves::arrUint64MouvementsTour;
std::array<std::unique_ptr<uint_fast64_t[]>, 64> moves::arrUint64MouvementsFou;

void moves::GenereMouvementsDame()
{
	arrUint64MouvementsDame = {};
	for (int uint8Case = 0; uint8Case < 64; ++uint8Case) {
		uint_fast64_t baseAttaques = uint64Mouvements[5][uint8Case];
		uint_fast64_t uint64MasquePosition = static_cast<uint_fast64_t>(1) << uint8Case;
		uint_fast64_t taille = static_cast<uint_fast64_t>(1) << _mm_popcnt_u64(baseAttaques);
		arrUint64MouvementsDame[uint8Case] = std::make_unique<uint_fast64_t[]>(taille);
		for (uint_fast64_t index = 0; index < taille; ++index) {
			uint_fast64_t occupation = _pdep_u64(index, baseAttaques);
			uint_fast64_t arrivee = subGenereMouvement1(uint64MouvementsTourCpx[0][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsTourCpx[1][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsTourCpx[2][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement1(uint64MouvementsTourCpx[3][uint8Case], uint64MasquePosition, occupation);

			arrivee |= subGenereMouvement1(uint64MouvementsFouCpx[0][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsFouCpx[1][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsFouCpx[2][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement1(uint64MouvementsFouCpx[3][uint8Case], uint64MasquePosition, occupation);

			uint_fast64_t uint64MasqueAttaques = arrivee;
			uint_fast64_t uint64MasqueMouvements = uint64MasqueAttaques;
			arrUint64MouvementsDame[uint8Case][index] = (uint64MasqueMouvements);
		}
	}
}

void moves::GenereMouvementsTour()
{
	for (int uint8Case = 0; uint8Case < 64; ++uint8Case) {
		uint_fast64_t baseAttaques = uint64Mouvements[4][uint8Case]; // 4 & 12
		uint_fast64_t uint64MasquePosition = static_cast<uint_fast64_t>(1) << uint8Case;
		uint_fast64_t taille = static_cast<uint_fast64_t>(1) << _mm_popcnt_u64(baseAttaques);
		arrUint64MouvementsTour[uint8Case] = std::make_unique<uint_fast64_t[]>(taille);
		for (uint_fast64_t index = 0; index < taille; ++index) {
			uint_fast64_t occupation = _pdep_u64(index, baseAttaques);
			uint_fast64_t arrivee = subGenereMouvement1(uint64MouvementsTourCpx[0][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsTourCpx[1][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsTourCpx[2][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement1(uint64MouvementsTourCpx[3][uint8Case], uint64MasquePosition, occupation);

			uint_fast64_t uint64MasqueMouvements = arrivee;
			arrUint64MouvementsTour[uint8Case][index] = (uint64MasqueMouvements);
		}
	}
}

void moves::GenereMouvementsFou()
{
	for (int uint8Case = 0; uint8Case < 64; ++uint8Case) {
		uint_fast64_t baseAttaques = uint64Mouvements[3][uint8Case]; // 3 & 11
		uint_fast64_t uint64MasquePosition = static_cast<uint_fast64_t>(1) << uint8Case;
		uint_fast64_t taille = static_cast<uint_fast64_t>(1) << _mm_popcnt_u64(baseAttaques);
		arrUint64MouvementsFou[uint8Case] = std::make_unique<uint_fast64_t[]>(taille);
		for (uint_fast64_t index = 0; index < taille; ++index) {
			uint_fast64_t occupation = _pdep_u64(index, baseAttaques);
			uint_fast64_t arrivee = subGenereMouvement1(uint64MouvementsFouCpx[0][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsFouCpx[1][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement2(uint64MouvementsFouCpx[2][uint8Case], uint64MasquePosition, occupation);
			arrivee |= subGenereMouvement1(uint64MouvementsFouCpx[3][uint8Case], uint64MasquePosition, occupation);
			uint_fast64_t uint64MasqueMouvements = arrivee;
			arrUint64MouvementsFou[uint8Case][index] = (uint64MasqueMouvements);
		}
	}
}

std::array<std::unique_ptr<uint_fast64_t[]>, 64> moves::arrUint64MouvementsPionNoir;

void moves::GenereMouvementsPionNoir()
{
	for (int uint8Case = 0; uint8Case < 64; ++uint8Case) {
		uint_fast64_t baseMouvements = uint64Mouvements[9][uint8Case]; // 3 & 11
		uint_fast64_t uint64MasquePosition = static_cast<uint_fast64_t>(1) << uint8Case;
		uint_fast64_t taille = static_cast<uint_fast64_t>(1) << _mm_popcnt_u64(baseMouvements);
		arrUint64MouvementsPionNoir[uint8Case] = std::make_unique<uint_fast64_t[]>(taille);
		for (uint_fast64_t index = 0; index < taille; ++index) {
			uint_fast64_t occupation = _pdep_u64(index, baseMouvements);
			uint_fast64_t arrivee;
			unsigned long index_pos = static_cast<unsigned long>(0);
			uint_fast64_t contact = baseMouvements & occupation;
			if (contact) {
				_BitScanReverse64(&index_pos, contact);
				arrivee = (uint64MasquePosition - _rotl64(static_cast<uint_fast64_t>(1), index_pos + 1));
				arrivee &= baseMouvements;
			}
			else {
				arrivee = baseMouvements;
			}
			uint_fast64_t uint64MasqueMouvements = arrivee;
			arrUint64MouvementsPionNoir[uint8Case][index] = uint64MasqueMouvements;
		}
	}
}

void moves::GenereMouvementsPionBlanc()
{
	for (int uint8Case = 0; uint8Case < 64; ++uint8Case) {
		uint_fast64_t baseMouvements = uint64Mouvements[1][uint8Case]; // 3 & 11
		uint_fast64_t uint64MasquePosition = static_cast<uint_fast64_t>(1) << uint8Case;
		uint_fast64_t taille = static_cast<uint_fast64_t>(1) << _mm_popcnt_u64(baseMouvements);
		arrUint64MouvementsPionNoir[uint8Case] = std::make_unique<uint_fast64_t[]>(taille);
		for (uint_fast64_t index = 0; index < taille; ++index) {
			uint_fast64_t occupation = _pdep_u64(index, baseMouvements);
			uint_fast64_t arrivee;
			unsigned long index_pos = static_cast<unsigned long>(0);
			uint_fast64_t contact = baseMouvements & occupation;
			if (contact) {
				_BitScanReverse64(&index_pos, contact);
				arrivee = (uint64MasquePosition - _rotl64(static_cast<uint_fast64_t>(1), index_pos + 1));
				arrivee &= baseMouvements;
			}
			else {
				arrivee = baseMouvements;
			}
			uint_fast64_t uint64MasqueMouvements = arrivee;
			arrUint64MouvementsPionNoir[uint8Case][index] = uint64MasqueMouvements;
		}
	}
}
