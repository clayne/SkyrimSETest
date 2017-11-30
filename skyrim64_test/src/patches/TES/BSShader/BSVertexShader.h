#pragma once

#include "../../../common.h"
#include "BSShaderManager.h"

struct BSVertexShader
{
	uint32_t m_TechniqueID;				// Bit flags
	ID3D11VertexShader *m_Shader;
	uint32_t m_ShaderLength;			// Raw bytecode length
	BSConstantGroup m_PerGeometry;
	BSConstantGroup m_PerMaterial;
	BSConstantGroup m_PerTechnique;
	uint64_t m_InputLayoutFlags;		// ID3D11Device::CreateInputLayout (for VSMain())
	uint8_t m_ConstantOffsets[20];		// Actual offset is multiplied by 4
										// Raw bytecode appended after this
};
static_assert(offsetof(BSVertexShader, m_TechniqueID) == 0x0, "");
static_assert(offsetof(BSVertexShader, m_Shader) == 0x8, "");
static_assert(offsetof(BSVertexShader, m_ShaderLength) == 0x10, "");
static_assert(offsetof(BSVertexShader, m_PerGeometry) == 0x18, "");
static_assert(offsetof(BSVertexShader, m_PerMaterial) == 0x28, "");
static_assert(offsetof(BSVertexShader, m_PerTechnique) == 0x38, "");
static_assert(offsetof(BSVertexShader, m_InputLayoutFlags) == 0x48, "");
static_assert(offsetof(BSVertexShader, m_ConstantOffsets) == 0x50, "");
static_assert(sizeof(BSVertexShader) == 0x68, "");

class VertexShaderDecoder : public ShaderDecoder
{
private:
	BSVertexShader *m_Shader;

public:
	VertexShaderDecoder(const char *Type, BSVertexShader *Shader);

private:
	virtual uint32_t GetTechnique() override;
	virtual const uint8_t *GetConstantArray() override;
	virtual size_t GetConstantArraySize() override;
	virtual void DumpShaderSpecific(const char *TechName, std::vector<ParamIndexPair>& PerGeo, std::vector<ParamIndexPair>& PerMat, std::vector<ParamIndexPair>& PerTec, std::vector<ParamIndexPair>& Undefined) override;

	void GetInputLayoutString(char *Buffer, size_t BufferSize);
};