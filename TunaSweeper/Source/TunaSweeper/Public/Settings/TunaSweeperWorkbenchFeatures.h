#pragma once

#include "Subsystem/TunaSweeperItemDataSubsystem.h"

namespace TunaSweeperWorkbenchFeatures
{
	// 분해와 설계도 등록은 잠시 비활성화한다. 기존 위젯/컴포넌트/데이터는 보존하며,
	// 이 값 하나를 true로 바꾸고 재빌드하면 두 기능의 표시, 진입, 실행이 함께 복구된다.
	inline constexpr bool bEnableWorkbenchExtraModes = false;

	inline bool IsModeEnabled(ETunaSweeperWorkbenchMode Mode)
	{
		return Mode == ETunaSweeperWorkbenchMode::Craft || bEnableWorkbenchExtraModes;
	}
}
