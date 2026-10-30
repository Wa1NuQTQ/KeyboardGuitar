#pragma once

#include "CoreMinimal.h"
#include "GuitarFrettingPose.h"
#include "Character/Animation/Shared/GuitarFretboard.h"

struct FGuitarChord;

namespace GuitarFretting
{
	// 同指按弦点汇总 → 选择基础握姿 → 搜索手腕并拟合五指；仅在和弦切换时调用。
	// 输入/输出均为吉他局部空间；Geometry 顺序为食指、中指、无名指、小指、拇指。
	bool SolveChord(const FGuitarChord& Chord, const FGuitarFretboard& Board,
		const TArray<FGuitarFingerGeometry>& Geometry, const FGuitarFrettingPose& OpenGrip,
		const FGuitarFrettingPose& BarreGrip, FGuitarFrettingPose& OutPose);

	// 平滑后调用，修正过渡帧中不可用的末节朝向与 Pole，保持指尖目标位置。
	// 指弹额外保持共同弯曲平面，避免关节角正常但手指向侧面拧弯。
	void ConstrainFingerBends(FGuitarFrettingPose& Pose, const TArray<FGuitarFingerGeometry>& Geometry,
		bool bKeepBendsPlanar = false);
}
