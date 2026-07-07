class HST_CampaignMapMarkerDirector
{
	static const int MAX_NATIVE_MARKERS = 192;
	static const int MAX_NATIVE_TACTICAL_MARKERS = 48;

	protected int m_iLastEligibleCount;
	protected int m_iLastSkippedCount;

	void BuildDesiredMarkers(HST_CampaignState state, HST_CampaignPreset preset, notnull map<string, ref HST_MapMarkerRecord> desired)
	{
		desired.Clear();
		m_iLastEligibleCount = 0;
		m_iLastSkippedCount = 0;

		if (!state)
			return;

		int tacticalCount;
		int publishedCount;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!IsNativeMarkerCandidate(marker))
				continue;

			m_iLastEligibleCount++;
			if (!CanPublishNativeMarker(marker, tacticalCount, publishedCount))
			{
				m_iLastSkippedCount++;
				continue;
			}

			HST_MapMarkerRecord record = BuildRecord(marker, preset, state.m_iElapsedSeconds);
			if (!record || record.m_sId.IsEmpty())
			{
				m_iLastSkippedCount++;
				continue;
			}

			desired.Set(record.m_sId, record);
			publishedCount++;
			if (IsTacticalNativeMarker(marker))
				tacticalCount++;
		}
	}

	int GetLastEligibleCount()
	{
		return m_iLastEligibleCount;
	}

	int GetLastSkippedCount()
	{
		return m_iLastSkippedCount;
	}

	protected HST_MapMarkerRecord BuildRecord(HST_MapMarkerState marker, HST_CampaignPreset preset, int elapsedSeconds)
	{
		if (!marker)
			return null;

		HST_MapMarkerRecord record = new HST_MapMarkerRecord();
		record.m_sId = marker.m_sMarkerId;
		record.m_eRenderMode = ResolveRenderMode(marker);
		record.m_vWorldPosition = marker.m_vPosition;
		record.m_sTargetRuntimeId = marker.m_sLinkedId;
		record.m_sLabel = marker.m_sLabel;
		record.m_sShortLabel = ResolveNativeMarkerText(marker);
		record.m_sCategory = marker.m_sCategory;
		record.m_sFactionKey = marker.m_sOwnerFactionKey;
		record.m_sTone = marker.m_sTextColorHint;
		record.m_iPriority = ResolveMarkerPriority(marker);
		record.m_eMarkerType = SCR_EMapMarkerType.PLACED_CUSTOM;
		record.m_iConfigId = -1;
		record.m_iIconEntry = ResolveNativeIcon(marker.m_sIconHint, marker.m_sCategory, marker.m_sStyleHint);
		record.m_iColorEntry = ResolveNativeColor(marker, preset);
		record.m_iFactionFlags = 0;
		record.m_bVisible = marker.m_bVisible;
		record.m_bCanPlayerRemove = false;
		record.m_bLocalOnly = false;
		record.m_bServerMarker = true;
		record.m_iLastChangedSecond = elapsedSeconds;
		return record;
	}

	protected HST_EMapMarkerRenderMode ResolveRenderMode(HST_MapMarkerState marker)
	{
		if (!marker)
			return HST_EMapMarkerRenderMode.STATIC_WORLD;

		if (marker.m_sCategory == "qrf" || marker.m_sCategory == "support")
			return HST_EMapMarkerRenderMode.SIMULATED_TRACKED;

		if (marker.m_sStyleHint.Contains("convoy") || marker.m_sStyleHint.Contains("incoming"))
			return HST_EMapMarkerRenderMode.SIMULATED_TRACKED;

		return HST_EMapMarkerRenderMode.STATIC_WORLD;
	}

	protected int ResolveMarkerPriority(HST_MapMarkerState marker)
	{
		if (!marker)
			return 0;

		if (marker.m_sCategory == "hq")
			return 100;
		if (marker.m_sCategory == "mission" || marker.m_sCategory == "mission_objective" || marker.m_sCategory == "mission_asset")
			return 80;
		if (marker.m_sCategory == "qrf" || marker.m_sCategory == "support")
			return 70;
		if (marker.m_sCategory == "town")
			return 40;

		return 50;
	}

	protected string ResolveNativeMarkerText(HST_MapMarkerState marker)
	{
		if (!marker)
			return "";

		if (marker.m_sCategory != "mission")
			return ShortMarkerText(marker.m_sLabel, 42);

		int separator = marker.m_sLabel.IndexOf(" |");
		if (separator <= 0)
			return ShortMarkerText(marker.m_sLabel, 42);

		return ShortMarkerText(marker.m_sLabel.Substring(0, separator), 42);
	}

	protected bool IsNativeMarkerCandidate(HST_MapMarkerState marker)
	{
		return marker && marker.m_bVisible && marker.m_bRuntimeNative;
	}

	protected bool CanPublishNativeMarker(HST_MapMarkerState marker, int tacticalCount, int publishedCount)
	{
		if (!IsNativeMarkerCandidate(marker))
			return false;

		if (marker.m_sCategory == "mission_route")
			return false;

		if (publishedCount >= MAX_NATIVE_MARKERS)
			return false;

		if (IsTacticalNativeMarker(marker) && tacticalCount >= MAX_NATIVE_TACTICAL_MARKERS)
			return false;

		return true;
	}

	protected bool IsTacticalNativeMarker(HST_MapMarkerState marker)
	{
		if (!marker)
			return false;

		return marker.m_sCategory == "mission" || marker.m_sCategory == "mission_objective" || marker.m_sCategory == "mission_asset" || marker.m_sCategory == "qrf" || marker.m_sCategory == "support";
	}

	protected SCR_EScenarioFrameworkMarkerCustom ResolveNativeIcon(string iconHint, string category, string styleHint)
	{
		if (iconHint == "PICK_UP2")
			return SCR_EScenarioFrameworkMarkerCustom.PICK_UP2;

		if (iconHint == "MINE_SINGLE")
			return SCR_EScenarioFrameworkMarkerCustom.MINE_SINGLE;

		if (iconHint == "POINT_SPECIAL")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (iconHint == "POINT_OF_INTEREST")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST;

		if (iconHint == "OBSERVATION_POST")
			return SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST;

		if (iconHint == "OBJECTIVE_MARKER" && (styleHint == "town" || category == "town"))
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST;

		if (iconHint == "OBJECTIVE_MARKER" && (styleHint == "enemy_base" || styleHint == "stronghold" || category == "enemy_base"))
			return SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST;

		if (iconHint == "OBJECTIVE_MARKER" && (styleHint == "mission_site" || category == "mission_site"))
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (iconHint == "OBJECTIVE_MARKER")
			return SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER;

		if (styleHint == "resource" || styleHint == "depot")
			return SCR_EScenarioFrameworkMarkerCustom.MINE_SINGLE;

		if (styleHint == "town" || category == "town")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST;

		if (styleHint == "enemy_base" || styleHint == "stronghold" || category == "enemy_base")
			return SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST;

		if (styleHint == "mission_site" || category == "mission_site")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (styleHint == "support" || category == "hq" || category == "hideout")
			return SCR_EScenarioFrameworkMarkerCustom.PICK_UP2;

		return SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER;
	}

	protected SCR_EScenarioFrameworkMarkerCustomColor ResolveNativeColor(HST_MapMarkerState marker, HST_CampaignPreset preset)
	{
		if (!marker)
			return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;

		string colorHint = marker.m_sColorHint;
		if (colorHint == "GREEN")
			return SCR_EScenarioFrameworkMarkerCustomColor.GREEN;

		if (colorHint == "BLUFOR")
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR;

		if (colorHint == "BLUE")
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUE;

		if (colorHint == "RED")
			return SCR_EScenarioFrameworkMarkerCustomColor.RED;

		if (colorHint == "OPFOR")
			return SCR_EScenarioFrameworkMarkerCustomColor.OPFOR;

		if (colorHint == "MAGENTA" || marker.m_sTextColorHint == "magenta")
			return SCR_EScenarioFrameworkMarkerCustomColor.MAGENTA;

		if (colorHint == "CIVILIAN")
			return SCR_EScenarioFrameworkMarkerCustomColor.CIVILIAN;

		if (colorHint == "REFORGER_ORANGE" || marker.m_sTextColorHint == "gold")
			return SCR_EScenarioFrameworkMarkerCustomColor.REFORGER_ORANGE;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.GREEN;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sOccupierFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sInvaderFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.RED;

		return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;
	}

	protected string ShortMarkerText(string text, int maxCharacters)
	{
		if (text.IsEmpty() || maxCharacters <= 0)
			return "";
		if (text.Length() <= maxCharacters)
			return text;
		if (maxCharacters <= 3)
			return text.Substring(0, maxCharacters);

		return text.Substring(0, maxCharacters - 3) + "...";
	}
}
