#pragma once

#include "common.h"

enum class PatchType
{
	MOV_REG_MEM,
	MOV_MEM_REG,
	MOV_MEM_IMM,

	ADD_REG_MEM,
	ADD_MEM_REG,
	ADD_MEM_IMM,

	AND_REG_MEM,
	AND_MEM_REG,
	AND_MEM_IMM,

	CMP_REG_MEM,
	CMP_MEM_REG,
	CMP_MEM_IMM,

	MOVSXD_REG_MEM,
	MOVZX_REG_MEM,
	LEA_REG_MEM,
	OR_MEM_REG,
	OR_MEM_IMM,
	MOVSS_REG_MEM,
	MOVSS_MEM_REG,
	INC_MEM,
	DEC_MEM,
	SUBSS_REG_MEM,
	ADDSS_REG_MEM,
	MOVUPS_REG_MEM,
	MOVUPS_MEM_REG,
	MOVAPS_REG_MEM,
	MOVAPS_MEM_REG,
	SHUFPS_REG_MEM,
	MOVSD_REG_MEM,
	MOVSD_MEM_REG,
	XADD_MEM_REG,
};

struct PatchEntry
{
	PatchType Type;
	uintptr_t ExeOffset;
	union
	{
		uint64_t Immediate;
		ZydisRegister Register;
	};
	uintptr_t Offset;
	int MemSize;

	ZydisRegister Base;
	ZydisRegister Index;
	uint32_t Scale;
};

struct OpTableEntry
{
	ZydisMnemonic Mnemonic;
	ZydisOperandType Operand1;
	ZydisOperandType Operand2;
	const char *OutputType;
};

struct PatchCodeGen : public Xbyak::CodeGenerator
{
public:
	PatchCodeGen(const PatchEntry *Patch, uintptr_t Memory, size_t MemorySize);

private:
	void SetTlsBase(const Xbyak::Reg64& Register);

	const Xbyak::Reg64& GetFreeScratch(ZydisRegister Operand, ZydisRegister Base, ZydisRegister Index);
	const Xbyak::AddressFrame& MemOpSize(int BitSize);
	const Xbyak::Reg& ZydisToXbyak(ZydisRegister Register);
	const Xbyak::Reg64& ZydisToXbyak64(ZydisRegister Register);
	const Xbyak::Xmm& ZydisToXbyakXmm(ZydisRegister Register);
};

void CreateXbyakPatches();
void CreateXbyakCodeBlock();
void GenerateInstruction(uintptr_t Address);
void GenerateCommonInstruction(ZydisDecodedInstruction *Instruction, ZydisDecodedOperand *Operands, const char *Type);
void WriteCodeHook(uintptr_t TargetAddress, void *Code);
uint32_t crc32c(unsigned char *Data, size_t Len);

const uintptr_t XrefList[] =
{
#include "d3d11_tls_xreflist.inl"
};

const uintptr_t XrefList2[] =
{
	0x14131f1d0,
	0x14131f1f0,
	0x14131ff73,
	0x1413200b5,
	0x1413200d0,
	0x1413200f2,
	0x1413200fc,
	0x1413201a6,
	0x141320221,
	0x141320230,
	0x141320249,
	0x14132025d,
	0x141320274,
	0x14132027b,
	0x14131e919,
	0x14131e92b,
	0x14131ea33,
	0x14131ea45,
	0x14131efbd,
	0x14131efdb,
	0x14131f0e3,
	0x14131f1b9,
	0x14131f373,
	0x14131f38a,
	0x14131f3a8,
	0x14131f3ca,
	0x14131f403,
	0x14131f415,
	0x14131f439,
	0x14131f539,
	0x14131fd84,
	0x14131fd9d,
	0x14131fe90,
	0x14131fea0,
	0x14131e90a,
	0x14131e924,
	0x14131ea24,
	0x14131ea3e,
	0x14131efae,
	0x14131efd4,
	0x14131f105,
	0x14131f1b2,
	0x14131f364,
	0x14131f380,
	0x14131f3a1,
	0x14131f3c3,
	0x14131f3f4,
	0x14131f40e,
	0x14131f432,
	0x14131f532,
	0x14131fd66,
	0x14131fd96,
	0x14131fe81,
	0x14131fe99,

	0x14131e931,
	0x14131ea4b,
	0x14131efe1,
	0x14131f134,
	0x14131f14e,
	0x14131f1bf,
	0x14131f390,
	0x14131f41b,
	0x14131f43f,
	0x14131f53f,
	0x14131fda4,
	0x14131fea7,

	// qword_1431F5490
	0x1412AABFD,
	0x1412AD330,
	0x1412AD340,
};

const PatchEntry XrefGeneratedPatches[] =
{
#define DO_THREADING_PATCH_REG(TYPE, EXE_OFFSET, REG, OFFSET, MEM_SIZE, B, I, S) { PatchType::##TYPE, EXE_OFFSET, ZYDIS_REGISTER_##REG, OFFSET, MEM_SIZE, ZYDIS_REGISTER_##B, ZYDIS_REGISTER_##I, S },
#define DO_THREADING_PATCH_IMM(TYPE, EXE_OFFSET, IMM, OFFSET, MEM_SIZE, B, I, S) { PatchType::##TYPE, EXE_OFFSET, (uint64_t)IMM, OFFSET, MEM_SIZE, ZYDIS_REGISTER_##B, ZYDIS_REGISTER_##I, S },
#define DO_SHUFPS_FIXUP(EXE_OFFSET, IMM)

#include "d3d11_tls_patchlist.inl"

#undef DO_SHUFPS_FIXUP
#undef DO_THREADING_PATCH_REG
#undef DO_THREADING_PATCH_IMM
};

//static_assert(arraysize(XrefGeneratedPatches) == arraysize(XrefList), "WARNING: Array sizes differ");

const std::unordered_map<uintptr_t, int> XrefGeneratedShufps
({
#define DO_THREADING_PATCH_REG(TYPE, EXE_OFFSET, REG, OFFSET, MEM_SIZE, B, I, S)
#define DO_THREADING_PATCH_IMM(TYPE, EXE_OFFSET, IMM, OFFSET, MEM_SIZE, B, I, S)
#define DO_SHUFPS_FIXUP(EXE_OFFSET, IMM) { EXE_OFFSET, IMM },

#include "d3d11_tls_patchlist.inl"

#undef DO_SHUFPS_FIXUP
#undef DO_THREADING_PATCH_REG
#undef DO_THREADING_PATCH_IMM
});

const std::vector<OpTableEntry> OpTable =
{
	{ ZYDIS_MNEMONIC_MOV, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOV_REG_MEM" },		// mov reg, [memory]
	{ ZYDIS_MNEMONIC_MOV, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOV_MEM_REG" },		// mov [memory], reg
	{ ZYDIS_MNEMONIC_MOV, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "MOV_MEM_IMM" },		// mov [memory], imm

	{ ZYDIS_MNEMONIC_ADD, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "ADD_REG_MEM" },		// add reg, [memory]
	{ ZYDIS_MNEMONIC_ADD, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "ADD_MEM_REG" },		// add [memory], reg
	{ ZYDIS_MNEMONIC_ADD, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "ADD_MEM_IMM" },		// add [memory], imm

	{ ZYDIS_MNEMONIC_AND, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "AND_REG_MEM" },		// and reg, [memory]
	{ ZYDIS_MNEMONIC_AND, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "AND_MEM_REG" },		// and [memory], reg
	{ ZYDIS_MNEMONIC_AND, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "AND_MEM_IMM" },		// and [memory], imm

	{ ZYDIS_MNEMONIC_CMP, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "CMP_REG_MEM" },		// cmp reg, [memory]
	{ ZYDIS_MNEMONIC_CMP, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "CMP_MEM_REG" },		// cmp [memory], reg
	{ ZYDIS_MNEMONIC_CMP, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "CMP_MEM_IMM" },		// cmp [memory], imm

	{ ZYDIS_MNEMONIC_MOVSXD, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVSXD_REG_MEM" },// movsxd reg, [memory]

	{ ZYDIS_MNEMONIC_MOVZX, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVZX_REG_MEM" },	// movzx reg, [memory]

	{ ZYDIS_MNEMONIC_LEA, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "LEA_REG_MEM" },		// lea reg, [memory]

	{ ZYDIS_MNEMONIC_OR, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "OR_MEM_REG" },		// or [memory], reg
	{ ZYDIS_MNEMONIC_OR, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "OR_MEM_IMM" },		// or [memory], imm

	{ ZYDIS_MNEMONIC_MOVSS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVSS_REG_MEM" },	// movss reg, [memory]
	{ ZYDIS_MNEMONIC_MOVSS, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVSS_MEM_REG" },	// movss [memory], reg

	{ ZYDIS_MNEMONIC_INC, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_UNUSED, "INC_MEM" },			// inc [memory]

	{ ZYDIS_MNEMONIC_DEC, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_UNUSED, "DEC_MEM" },			// dec [memory]

	{ ZYDIS_MNEMONIC_SUBSS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "SUBSS_REG_MEM" },	// subss reg, [memory]

	{ ZYDIS_MNEMONIC_ADDSS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "ADDSS_REG_MEM" },	// addss reg, [memory]

	{ ZYDIS_MNEMONIC_MOVUPS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVUPS_REG_MEM" },// movups reg, [memory]
	{ ZYDIS_MNEMONIC_MOVUPS, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVUPS_MEM_REG" },// movups [memory], reg

	{ ZYDIS_MNEMONIC_MOVAPS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVAPS_REG_MEM" },// movaps reg, [memory]
	{ ZYDIS_MNEMONIC_MOVAPS, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVAPS_MEM_REG" },// movaps [memory], reg

	{ ZYDIS_MNEMONIC_SHUFPS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "SHUFPS_REG_MEM" },// shufps reg, [memory], imm8

	{ ZYDIS_MNEMONIC_MOVSD, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVSD_REG_MEM" },  // movsd xmm, [memory]
	{ ZYDIS_MNEMONIC_MOVSD, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVSD_MEM_REG" },  // movsd [memory], xmm

	{ ZYDIS_MNEMONIC_XADD, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "XADD_MEM_REG" },	// lock xadd [memory], reg
};