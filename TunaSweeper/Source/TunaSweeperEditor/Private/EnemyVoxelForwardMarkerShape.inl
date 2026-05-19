static void AppendEnemyForwardMarkerVoxelBoxes(TArray<FEnemyVoxelBox>& OutBoxes)
{
	const FLinearColor OutlineColor(0.20f, 0.09f, 0.01f, 1.00f);
	const FLinearColor EdgeOrange(0.95f, 0.32f, 0.02f, 1.00f);
	const FLinearColor CoreAmber(1.00f, 0.58f, 0.04f, 1.00f);
	const FLinearColor HotYellow(1.00f, 0.86f, 0.18f, 1.00f);

	OutBoxes.Add({ 3, 13, 13, 19, 19, 19, OutlineColor });
	OutBoxes.Add({ 18, 9, 12, 23, 23, 20, OutlineColor });
	OutBoxes.Add({ 23, 11, 12, 27, 21, 20, OutlineColor });
	OutBoxes.Add({ 27, 14, 12, 31, 18, 20, OutlineColor });

	OutBoxes.Add({ 5, 14, 14, 19, 18, 18, CoreAmber });
	OutBoxes.Add({ 19, 11, 13, 23, 21, 19, EdgeOrange });
	OutBoxes.Add({ 23, 13, 13, 27, 19, 19, EdgeOrange });
	OutBoxes.Add({ 27, 15, 13, 30, 17, 19, HotYellow });

	OutBoxes.Add({ 8, 15, 18, 20, 17, 21, HotYellow });
	OutBoxes.Add({ 20, 13, 19, 24, 19, 22, HotYellow });
	OutBoxes.Add({ 24, 14, 19, 28, 18, 22, CoreAmber });
	OutBoxes.Add({ 28, 15, 19, 30, 17, 22, HotYellow });
}
