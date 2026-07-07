class HST_FactionRelationService
{
	static const string RELATION_SAME = "same";
	static const string RELATION_RESISTANCE_ENEMY = "resistance_enemy";
	static const string RELATION_RIVAL = "enemy_rival";
	static const string RELATION_NEUTRAL = "neutral";
	static const string RELATION_UNKNOWN = "unknown";

	static string ResolveRelation(HST_CampaignPreset preset, string subjectFactionKey, string otherFactionKey)
	{
		if (subjectFactionKey.IsEmpty() || otherFactionKey.IsEmpty())
			return RELATION_UNKNOWN;
		if (subjectFactionKey == otherFactionKey)
			return RELATION_SAME;
		if (!preset)
			return RELATION_UNKNOWN;

		bool subjectResistance = IsResistanceFaction(preset, subjectFactionKey);
		bool otherResistance = IsResistanceFaction(preset, otherFactionKey);
		bool subjectEnemy = IsEnemyFaction(preset, subjectFactionKey);
		bool otherEnemy = IsEnemyFaction(preset, otherFactionKey);

		if ((subjectResistance && otherEnemy) || (subjectEnemy && otherResistance))
			return RELATION_RESISTANCE_ENEMY;
		if (subjectEnemy && otherEnemy)
			return RELATION_RIVAL;
		if (subjectResistance || subjectEnemy || otherResistance || otherEnemy)
			return RELATION_NEUTRAL;

		return RELATION_UNKNOWN;
	}

	static bool IsResistanceFaction(HST_CampaignPreset preset, string factionKey)
	{
		if (!preset || factionKey.IsEmpty())
			return false;

		return factionKey == preset.m_sResistanceFactionKey;
	}

	static bool IsEnemyFaction(HST_CampaignPreset preset, string factionKey)
	{
		if (!preset || factionKey.IsEmpty())
			return false;
		if (factionKey == preset.m_sResistanceFactionKey)
			return false;

		return factionKey == preset.m_sOccupierFactionKey || factionKey == preset.m_sInvaderFactionKey;
	}

	static bool IsSameFaction(string relation)
	{
		return relation == RELATION_SAME;
	}

	static bool IsResistanceEnemy(string relation)
	{
		return relation == RELATION_RESISTANCE_ENEMY;
	}

	static bool IsRivalEnemy(string relation)
	{
		return relation == RELATION_RIVAL;
	}

	static bool IsHostileRelation(string relation)
	{
		return relation == RELATION_RESISTANCE_ENEMY || relation == RELATION_RIVAL;
	}

	static string ResolveRivalEnemyFactionKey(HST_CampaignPreset preset, string factionKey)
	{
		if (!preset || factionKey.IsEmpty())
			return "";
		if (factionKey == preset.m_sOccupierFactionKey && !preset.m_sInvaderFactionKey.IsEmpty() && preset.m_sInvaderFactionKey != factionKey)
			return preset.m_sInvaderFactionKey;
		if (factionKey == preset.m_sInvaderFactionKey && !preset.m_sOccupierFactionKey.IsEmpty() && preset.m_sOccupierFactionKey != factionKey)
			return preset.m_sOccupierFactionKey;

		return "";
	}
}
