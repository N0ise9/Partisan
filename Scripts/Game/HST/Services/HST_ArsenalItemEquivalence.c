class HST_ArsenalItemEquivalence
{
	static bool AreEquivalentPrefabs(string lhs, string rhs)
	{
		if (lhs == rhs)
			return true;

		return IsPaperMapPrefab(lhs) && IsPaperMapPrefab(rhs);
	}

	static bool IsPaperMapPrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		prefab.ToLower();
		if (prefab.Contains("prefabs/items/equipment/maps/"))
			return true;
		if (prefab.Contains("papermap_") || prefab.Contains("paper_map_"))
			return true;
		if (prefab.Contains("map_paper_"))
			return true;

		return false;
	}
}
