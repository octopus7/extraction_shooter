#include "TunaWarpTransitionProfile.h"

#include "Materials/MaterialInterface.h"

UTunaWarpTransitionProfile::UTunaWarpTransitionProfile()
{
	WarpMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/TunaWarpTransition/Materials/MI_PP_TunaWarpRadial_Default.MI_PP_TunaWarpRadial_Default")));
	ArrivalRimMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/TunaWarpTransition/Materials/MI_PP_TunaWarpArrivalRim_Default.MI_PP_TunaWarpArrivalRim_Default")));
}
