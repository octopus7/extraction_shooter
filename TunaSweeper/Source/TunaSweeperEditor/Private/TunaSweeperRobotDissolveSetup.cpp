#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{
namespace
{
	const TCHAR* RobotMaterialObjectPath =
		TEXT("/Game/Characters/Robot/Materials/M_Robot_Customizable.M_Robot_Customizable");
	const FName DeathDissolveParameterName(TEXT("DeathDissolve"));

	bool HasDeathDissolveParameter(const UMaterial* Material)
	{
		if (!Material)
		{
			return false;
		}

		for (UMaterialExpression* Expression : Material->GetExpressions())
		{
			if (const UMaterialExpressionScalarParameter* ScalarParameter =
				Cast<UMaterialExpressionScalarParameter>(Expression))
			{
				if (ScalarParameter->ParameterName == DeathDissolveParameterName)
				{
					return true;
				}
			}
		}
		return false;
	}
}

bool EnsureRobotDeathDissolveMaterial()
{
	UMaterial* Material = LoadObject<UMaterial>(nullptr, RobotMaterialObjectPath);
	if (!Material)
	{
		UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing robot material: %s"), RobotMaterialObjectPath);
		return false;
	}
	if (HasDeathDissolveParameter(Material))
	{
		UE_LOG(LogTunaSweeperEditor, Display, TEXT("Robot death dissolve is already configured."));
		return true;
	}

	UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
	if (!EditorOnlyData)
	{
		return false;
	}

	Material->Modify();
	Material->BlendMode = BLEND_Masked;
	Material->OpacityMaskClipValue = 0.5f;

	UMaterialExpressionScalarParameter* DissolveParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
	DissolveParameter->Material = Material;
	DissolveParameter->ParameterName = DeathDissolveParameterName;
	DissolveParameter->DefaultValue = 0.0f;
	DissolveParameter->MaterialExpressionEditorX = 320;
	DissolveParameter->MaterialExpressionEditorY = 920;
	Material->GetExpressionCollection().AddExpression(DissolveParameter);

	UMaterialExpressionTextureCoordinate* TextureCoordinate = NewObject<UMaterialExpressionTextureCoordinate>(Material);
	TextureCoordinate->Material = Material;
	TextureCoordinate->CoordinateIndex = 0;
	TextureCoordinate->MaterialExpressionEditorX = 320;
	TextureCoordinate->MaterialExpressionEditorY = 800;
	Material->GetExpressionCollection().AddExpression(TextureCoordinate);

	UMaterialExpressionCustom* DissolveMask = NewObject<UMaterialExpressionCustom>(Material);
	DissolveMask->Material = Material;
	DissolveMask->Description = TEXT("Robot death dissolve noise");
	DissolveMask->OutputType = CMOT_Float1;
	DissolveMask->Code =
		TEXT("float2 cell = floor(UV * 96.0f);\n")
		TEXT("float noise = frac(sin(dot(cell, float2(12.9898f, 78.233f))) * 43758.5453f);\n")
		TEXT("return noise >= saturate(Amount) ? 1.0f : 0.0f;");
	DissolveMask->MaterialExpressionEditorX = 560;
	DissolveMask->MaterialExpressionEditorY = 820;
	FCustomInput UvInput;
	UvInput.InputName = TEXT("UV");
	UvInput.Input.Connect(0, TextureCoordinate);
	DissolveMask->Inputs.Add(UvInput);
	FCustomInput AmountInput;
	AmountInput.InputName = TEXT("Amount");
	AmountInput.Input.Connect(0, DissolveParameter);
	DissolveMask->Inputs.Add(AmountInput);
	Material->GetExpressionCollection().AddExpression(DissolveMask);

	UMaterialExpressionMultiply* CombinedOpacityMask = NewObject<UMaterialExpressionMultiply>(Material);
	CombinedOpacityMask->Material = Material;
	if (EditorOnlyData->OpacityMask.Expression)
	{
		CombinedOpacityMask->A = EditorOnlyData->OpacityMask;
	}
	else
	{
		CombinedOpacityMask->ConstA = 1.0f;
	}
	CombinedOpacityMask->B.Connect(0, DissolveMask);
	CombinedOpacityMask->MaterialExpressionEditorX = 820;
	CombinedOpacityMask->MaterialExpressionEditorY = 820;
	Material->GetExpressionCollection().AddExpression(CombinedOpacityMask);
	EditorOnlyData->OpacityMask.Connect(0, CombinedOpacityMask);

	Material->PostEditChange();
	Material->MarkPackageDirty();
	const bool bSaved = SaveAsset(Material);
	UE_LOG(LogTunaSweeperEditor, Display, TEXT("Robot death dissolve setup saved=%s"), bSaved ? TEXT("true") : TEXT("false"));
	return bSaved;
}
}
