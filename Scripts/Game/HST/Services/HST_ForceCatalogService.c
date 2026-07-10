class HST_ForceCatalogService
{
	static const string CATALOG_VERSION = "force_catalog_1";

	array<ref HST_ForceMemberCatalogEntry> BuildMemberCatalog(string factionKey)
	{
		array<ref HST_ForceMemberCatalogEntry> entries = {};
		if (factionKey == "FIA")
		{
			AddMember(entries, "fia_rifleman", factionKey, "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et", "rifleman");
			AddMember(entries, "fia_medic", factionKey, "{45A02CA25CBA9443}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Medic.et", "medic");
			AddMember(entries, "fia_machine_gunner", factionKey, "{58E47E5A4D599432}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_MG.et", "machine_gunner");
			AddMember(entries, "fia_light_anti_tank", factionKey, "{C77DFB8546B3F2A2}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_LAT.et", "light_anti_tank");
		}
		else if (factionKey == "US")
		{
			AddMember(entries, "us_rifleman", factionKey, "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et", "rifleman");
			AddMember(entries, "us_grenadier", factionKey, "{84029128FA6F6BB9}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_GL.et", "grenadier");
			AddMember(entries, "us_machine_gunner", factionKey, "{1623EA3AEFACA0E4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_MG.et", "machine_gunner");
			AddMember(entries, "us_light_anti_tank", factionKey, "{27BF1FF235DD6036}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_LAT.et", "light_anti_tank");
		}
		else if (factionKey == "USSR")
		{
			AddMember(entries, "ussr_rifleman", factionKey, "{DCB41B3746FDD1BE}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_Rifleman.et", "rifleman");
			AddMember(entries, "ussr_grenadier", factionKey, "{8E0FE664CE7D1CA9}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_GL.et", "grenadier");
			AddMember(entries, "ussr_machine_gunner", factionKey, "{96C784C502AC37DA}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_MG.et", "machine_gunner");
			AddMember(entries, "ussr_light_anti_tank", factionKey, "{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et", "light_anti_tank");
		}

		return entries;
	}

	array<ref HST_ForceGroupCatalogEntry> BuildGroupCatalog(string factionKey)
	{
		array<ref HST_ForceGroupCatalogEntry> entries = {};
		if (factionKey == "FIA")
			BuildFiaGroups(entries);
		else if (factionKey == "US")
			BuildUsGroups(entries);
		else if (factionKey == "USSR")
			BuildUssrGroups(entries);
		return entries;
	}

	HST_ForceCatalogValidationResult ValidateFactionCatalog(string factionKey, bool validateResources = true)
	{
		HST_ForceCatalogValidationResult result = ValidateMemberCatalog(factionKey, validateResources);
		array<ref HST_ForceGroupCatalogEntry> groups = BuildGroupCatalog(factionKey);
		result.m_iGroupEntryCount = groups.Count();
		if (groups.Count() == 0)
			AddFailure(result, "no group entries for faction " + factionKey);

		array<string> entryIds = {};
		foreach (HST_ForceGroupCatalogEntry group : groups)
		{
			if (!group)
			{
				AddFailure(result, "null group entry");
				continue;
			}
			if (group.m_sEntryId.IsEmpty() || group.m_sFactionKey != factionKey || group.m_sRole.IsEmpty() || group.m_sAuthoredPrefab.IsEmpty() || group.m_sExecutionPrefab.IsEmpty())
				AddFailure(result, "incomplete group entry " + group.m_sEntryId);
			if (entryIds.Contains(group.m_sEntryId))
				AddFailure(result, "duplicate group entry " + group.m_sEntryId);
			else
				entryIds.Insert(group.m_sEntryId);
			if (group.m_iWeight <= 0 || group.m_aMemberSlots.Count() == 0)
				AddFailure(result, "invalid group policy " + group.m_sEntryId);

			array<string> slotIds = {};
			foreach (HST_ForceGroupCatalogSlot slot : group.m_aMemberSlots)
			{
				result.m_iGroupSlotCount++;
				if (!slot || slot.m_sSlotId.IsEmpty() || slot.m_sPrefab.IsEmpty() || slot.m_sRole.IsEmpty())
				{
					AddFailure(result, "incomplete group slot " + group.m_sEntryId);
					continue;
				}
				if (slotIds.Contains(slot.m_sSlotId))
					AddFailure(result, "duplicate group slot " + group.m_sEntryId + "/" + slot.m_sSlotId);
				else
					slotIds.Insert(slot.m_sSlotId);
			}

			if (!validateResources)
				continue;
			Resource authoredResource = Resource.Load(group.m_sAuthoredPrefab);
			if (!authoredResource || !authoredResource.IsValid())
				AddFailure(result, "authored group prefab unavailable " + group.m_sEntryId);
			array<ResourceName> authoredSlots = {};
			string slotFailure;
			if (!TryReadGroupSlots(group.m_sExecutionPrefab, authoredSlots, slotFailure))
			{
				AddFailure(result, group.m_sEntryId + " execution prefab: " + slotFailure);
				continue;
			}
			if (authoredSlots.Count() != group.m_aMemberSlots.Count())
			{
				AddFailure(result, string.Format("%1 roster count catalog %2 authored %3", group.m_sEntryId, group.m_aMemberSlots.Count(), authoredSlots.Count()));
				continue;
			}
			for (int slotIndex = 0; slotIndex < authoredSlots.Count(); slotIndex++)
			{
				if (group.m_aMemberSlots[slotIndex].m_sPrefab != authoredSlots[slotIndex])
					AddFailure(result, string.Format("%1 slot %2 prefab conflict", group.m_sEntryId, slotIndex));
			}
		}

		result.m_bValid = result.m_aFailures.Count() == 0;
		result.m_sFailureReason = "";
		if (!result.m_bValid)
			result.m_sFailureReason = JoinFailures(result.m_aFailures);
		return result;
	}

	HST_ForceCatalogValidationResult ValidateMemberCatalog(string factionKey, bool validateResources = true)
	{
		HST_ForceCatalogValidationResult result = new HST_ForceCatalogValidationResult();
		array<ref HST_ForceMemberCatalogEntry> entries = BuildMemberCatalog(factionKey);
		result.m_iMemberEntryCount = entries.Count();
		if (entries.Count() == 0)
			AddFailure(result, "no member entries for faction " + factionKey);

		array<string> entryIds = {};
		foreach (HST_ForceMemberCatalogEntry entry : entries)
		{
			if (!entry)
			{
				AddFailure(result, "null member entry");
				continue;
			}
			if (entry.m_sEntryId.IsEmpty() || entry.m_sFactionKey != factionKey || entry.m_sPrefab.IsEmpty() || entry.m_sRole.IsEmpty())
				AddFailure(result, "incomplete member entry " + entry.m_sEntryId);
			if (entryIds.Contains(entry.m_sEntryId))
				AddFailure(result, "duplicate member entry " + entry.m_sEntryId);
			else
				entryIds.Insert(entry.m_sEntryId);
			if (entry.m_iHRCost < 0 || entry.m_iMoneyCost < 0 || entry.m_iEquipmentCost < 0 || entry.m_iWeight <= 0)
				AddFailure(result, "invalid member policy " + entry.m_sEntryId);
			if (validateResources && !entry.m_sPrefab.IsEmpty())
			{
				Resource memberResource = Resource.Load(entry.m_sPrefab);
				if (!memberResource || !memberResource.IsValid())
					AddFailure(result, "member prefab unavailable " + entry.m_sEntryId);
			}
		}

		result.m_bValid = result.m_aFailures.Count() == 0;
		if (!result.m_bValid)
			result.m_sFailureReason = JoinFailures(result.m_aFailures);
		return result;
	}

	protected void AddMember(notnull array<ref HST_ForceMemberCatalogEntry> entries, string entryId, string factionKey, string prefab, string role)
	{
		HST_ForceMemberCatalogEntry entry = new HST_ForceMemberCatalogEntry();
		entry.m_sEntryId = entryId;
		entry.m_sFactionKey = factionKey;
		entry.m_sPrefab = prefab;
		entry.m_sRole = role;
		entries.Insert(entry);
	}

	protected void BuildFiaGroups(notnull array<ref HST_ForceGroupCatalogEntry> entries)
	{
		HST_ForceGroupCatalogEntry rifle = AddGroup(entries, "fia_rifle_squad", "FIA", "rifle_squad", "{CE41AF625D05D0F0}Prefabs/Groups/INDFOR/Group_FIA_RifleSquad.et", "{265B11E8E53F9FBA}Prefabs/Groups/INDFOR/AmbientPatrols/Group_FIA_RifleSquad_NotSpawned.et");
		AddGroupSlot(rifle, "sl", "{677B515F119222C2}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_SL.et", "squad_leader");
		AddGroupSlot(rifle, "mg", "{58E47E5A4D599432}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_MG.et", "machine_gunner");
		AddGroupSlot(rifle, "amg", "{75FC25863194612A}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_AMG.et", "assistant_machine_gunner");
		AddGroupSlot(rifle, "lat_1", "{C77DFB8546B3F2A2}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_LAT.et", "light_anti_tank");
		AddGroupSlot(rifle, "rto", "{23D81C023DBF85AC}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_RTO.et", "radio_operator");
		AddGroupSlot(rifle, "rifleman", "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et", "rifleman");
		AddGroupSlot(rifle, "lat_2", "{C77DFB8546B3F2A2}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_LAT.et", "light_anti_tank");

		HST_ForceGroupCatalogEntry fire = AddGroup(entries, "fia_fire_team", "FIA", "fire_team", "{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et", "{8D5E0E187752F5FB}Prefabs/Groups/INDFOR/AmbientPatrols/Group_FIA_FireTeam_NotSpawned.et");
		AddGroupSlot(fire, "sl", "{677B515F119222C2}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_SL.et", "squad_leader");
		AddGroupSlot(fire, "medic", "{45A02CA25CBA9443}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Medic.et", "medic");
		AddGroupSlot(fire, "rifleman_1", "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et", "rifleman");
		AddGroupSlot(fire, "rifleman_2", "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et", "rifleman");
		AddGroupSlot(fire, "lat", "{C77DFB8546B3F2A2}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_LAT.et", "light_anti_tank");

		HST_ForceGroupCatalogEntry sentry = AddGroup(entries, "fia_sentry_team", "FIA", "sentry_team", "{6E725D44CA973C24}Prefabs/Groups/INDFOR/Group_FIA_SentryTeam.et", "{6E725D44CA973C24}Prefabs/Groups/INDFOR/Group_FIA_SentryTeam.et");
		AddGroupSlot(sentry, "rifleman_1", "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et", "rifleman");
		AddGroupSlot(sentry, "rifleman_2", "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et", "rifleman");

		HST_ForceGroupCatalogEntry machineGun = AddGroup(entries, "fia_machine_gun_team", "FIA", "machine_gun_team", "{22F33D3EC8F281AB}Prefabs/Groups/INDFOR/Group_FIA_MachineGunTeam.et", "{421CA62CC2E5F16A}Prefabs/Groups/INDFOR/AmbientPatrols/Group_FIA_MachineGunTeam_NotSpawned.et");
		AddGroupSlot(machineGun, "mg", "{58E47E5A4D599432}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_MG.et", "machine_gunner");
		AddGroupSlot(machineGun, "amg", "{75FC25863194612A}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_AMG.et", "assistant_machine_gunner");
	}

	protected void BuildUsGroups(notnull array<ref HST_ForceGroupCatalogEntry> entries)
	{
		HST_ForceGroupCatalogEntry rifle = AddGroup(entries, "us_rifle_squad", "US", "rifle_squad", "{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et", "{4F6811B5E789FA88}Prefabs/Groups/BLUFOR/AmbientPatrols/Group_US_RifleSquad_NotSpawned.et");
		AddGroupSlot(rifle, "sl", "{E45F1E163F5CA080}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_SL.et", "squad_leader");
		AddGroupSlot(rifle, "tl_1", "{E398E44759DA1A43}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_TL.et", "team_leader");
		AddGroupSlot(rifle, "tl_2", "{E398E44759DA1A43}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_TL.et", "team_leader");
		AddGroupSlot(rifle, "ar_1", "{5B1996C05B1E51A4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_AR.et", "automatic_rifleman");
		AddGroupSlot(rifle, "ar_2", "{5B1996C05B1E51A4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_AR.et", "automatic_rifleman");
		AddGroupSlot(rifle, "gl_1", "{84029128FA6F6BB9}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_GL.et", "grenadier");
		AddGroupSlot(rifle, "gl_2", "{84029128FA6F6BB9}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_GL.et", "grenadier");
		AddGroupSlot(rifle, "lat_1", "{27BF1FF235DD6036}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_LAT.et", "light_anti_tank");
		AddGroupSlot(rifle, "lat_2", "{27BF1FF235DD6036}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_LAT.et", "light_anti_tank");

		HST_ForceGroupCatalogEntry fire = AddGroup(entries, "us_fire_team", "US", "fire_team", "{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et", "{94B5E9E3D8E4B887}Prefabs/Groups/BLUFOR/AmbientPatrols/Group_US_FireTeam_NotSpawned.et");
		AddGroupSlot(fire, "tl", "{E398E44759DA1A43}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_TL.et", "team_leader");
		AddGroupSlot(fire, "ar", "{5B1996C05B1E51A4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_AR.et", "automatic_rifleman");
		AddGroupSlot(fire, "gl", "{84029128FA6F6BB9}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_GL.et", "grenadier");
		AddGroupSlot(fire, "lat", "{27BF1FF235DD6036}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_LAT.et", "light_anti_tank");

		HST_ForceGroupCatalogEntry sentry = AddGroup(entries, "us_sentry_team", "US", "sentry_team", "{3BF36BDEEB33AEC9}Prefabs/Groups/BLUFOR/Group_US_SentryTeam.et", "{FB7A6A8BF391D93E}Prefabs/Groups/BLUFOR/AmbientPatrols/Group_US_SentryTeam_NotSpawned.et");
		AddGroupSlot(sentry, "rifleman_1", "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et", "rifleman");
		AddGroupSlot(sentry, "rifleman_2", "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et", "rifleman");

		HST_ForceGroupCatalogEntry machineGun = AddGroup(entries, "us_machine_gun_team", "US", "machine_gun_team", "{958039B857396B7B}Prefabs/Groups/BLUFOR/Group_US_MachineGunTeam.et", "{DA987D8C5A311713}Prefabs/Groups/BLUFOR/AmbientPatrols/Group_US_MachineGunTeam_NotSpawned.et");
		AddGroupSlot(machineGun, "mg", "{1623EA3AEFACA0E4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_MG.et", "machine_gunner");
		AddGroupSlot(machineGun, "amg", "{6058AB54781A0C52}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_AMG.et", "assistant_machine_gunner");
	}

	protected void BuildUssrGroups(notnull array<ref HST_ForceGroupCatalogEntry> entries)
	{
		HST_ForceGroupCatalogEntry rifle = AddGroup(entries, "ussr_rifle_squad", "USSR", "rifle_squad", "{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et", "{7159E20D1B547A4E}Prefabs/Groups/OPFOR/AmbientPatrols/Group_USSR_RifleSquad_NotSpawned.et");
		AddGroupSlot(rifle, "sl", "{5436629450D8387A}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_SL.et", "squad_leader");
		AddGroupSlot(rifle, "ar", "{23ADBBC31B6A3DC6}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AR.et", "automatic_rifleman");
		AddGroupSlot(rifle, "at", "{1C78331E156A3D65}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AT.et", "anti_tank");
		AddGroupSlot(rifle, "aat", "{631158F6898738A4}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AAT.et", "assistant_anti_tank");
		AddGroupSlot(rifle, "sr", "{333DA6244C7DA34C}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_SR.et", "senior_rifleman");
		AddGroupSlot(rifle, "lat", "{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et", "light_anti_tank");

		HST_ForceGroupCatalogEntry fire = AddGroup(entries, "ussr_fire_group", "USSR", "fire_team", "{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et", "{EC8BAC199BAF3740}Prefabs/Groups/OPFOR/AmbientPatrols/Group_USSR_FireGroup_NotSpawned.et");
		AddGroupSlot(fire, "sl", "{5436629450D8387A}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_SL.et", "squad_leader");
		AddGroupSlot(fire, "ar", "{23ADBBC31B6A3DC6}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AR.et", "automatic_rifleman");
		AddGroupSlot(fire, "at", "{1C78331E156A3D65}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AT.et", "anti_tank");
		AddGroupSlot(fire, "aat", "{631158F6898738A4}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AAT.et", "assistant_anti_tank");

		HST_ForceGroupCatalogEntry sentry = AddGroup(entries, "ussr_sentry_team", "USSR", "sentry_team", "{CB58D90EA14430AD}Prefabs/Groups/OPFOR/Group_USSR_SentryTeam.et", "{C54B99330F4C59F8}Prefabs/Groups/OPFOR/AmbientPatrols/Group_USSR_SentryTeam_NotSpawned.et");
		AddGroupSlot(sentry, "rifleman_1", "{DCB41B3746FDD1BE}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_Rifleman.et", "rifleman");
		AddGroupSlot(sentry, "rifleman_2", "{DCB41B3746FDD1BE}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_Rifleman.et", "rifleman");

		HST_ForceGroupCatalogEntry machineGun = AddGroup(entries, "ussr_machine_gun_team", "USSR", "machine_gun_team", "{A2F75E45C66B1C0A}Prefabs/Groups/OPFOR/Group_USSR_MachineGunTeam.et", "{06D32ED260762934}Prefabs/Groups/OPFOR/AmbientPatrols/Group_USSR_MachineGunTeam_NotSpawned.et");
		AddGroupSlot(machineGun, "mg", "{96C784C502AC37DA}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_MG.et", "machine_gunner");
		AddGroupSlot(machineGun, "amg", "{E9AEEF2D9E41321B}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_AMG.et", "assistant_machine_gunner");
	}

	protected HST_ForceGroupCatalogEntry AddGroup(notnull array<ref HST_ForceGroupCatalogEntry> entries, string entryId, string factionKey, string role, string authoredPrefab, string executionPrefab)
	{
		HST_ForceGroupCatalogEntry entry = new HST_ForceGroupCatalogEntry();
		entry.m_sEntryId = entryId;
		entry.m_sFactionKey = factionKey;
		entry.m_sRole = role;
		entry.m_sAuthoredPrefab = authoredPrefab;
		entry.m_sExecutionPrefab = executionPrefab;
		entries.Insert(entry);
		return entry;
	}

	protected void AddGroupSlot(HST_ForceGroupCatalogEntry group, string slotId, string prefab, string role)
	{
		if (!group)
			return;
		HST_ForceGroupCatalogSlot slot = new HST_ForceGroupCatalogSlot();
		slot.m_sSlotId = slotId;
		slot.m_sPrefab = prefab;
		slot.m_sRole = role;
		slot.m_iOrdinal = group.m_aMemberSlots.Count();
		group.m_aMemberSlots.Insert(slot);
	}

	protected bool TryReadGroupSlots(string groupPrefab, out array<ResourceName> slots, out string failure)
	{
		slots = {};
		failure = "";
		if (groupPrefab.IsEmpty())
		{
			failure = "empty group prefab";
			return false;
		}
		ResourceName groupName = groupPrefab;
		Resource groupResource = Resource.Load(groupName);
		if (!groupResource || !groupResource.IsValid())
		{
			failure = "group resource is missing or invalid";
			return false;
		}
		IEntitySource groupSource = SCR_BaseContainerTools.FindEntitySource(groupResource);
		if (!groupSource)
		{
			failure = "resource is not an entity prefab";
			return false;
		}
		typename groupType = groupSource.GetClassName().ToType();
		if (!groupType || !groupType.IsInherited(SCR_AIGroup))
		{
			failure = "entity prefab is not an SCR_AIGroup";
			return false;
		}
		if (!groupSource.Get("m_aUnitPrefabSlots", slots) || !slots || slots.IsEmpty())
		{
			failure = "group has no readable authored member slots";
			return false;
		}
		for (int slotIndex = 0; slotIndex < slots.Count(); slotIndex++)
		{
			ResourceName memberName = slots[slotIndex];
			Resource memberResource = Resource.Load(memberName);
			if (memberName.IsEmpty() || !memberResource || !memberResource.IsValid())
			{
				failure = string.Format("member slot %1 resource is invalid", slotIndex);
				return false;
			}
			IEntitySource memberSource = SCR_BaseContainerTools.FindEntitySource(memberResource);
			typename memberType;
			if (memberSource)
				memberType = memberSource.GetClassName().ToType();
			if (!memberSource || !memberType || !memberType.IsInherited(SCR_ChimeraCharacter))
			{
				failure = string.Format("member slot %1 is not a character", slotIndex);
				return false;
			}
		}
		return true;
	}

	protected void AddFailure(HST_ForceCatalogValidationResult result, string failure)
	{
		if (!result || failure.IsEmpty())
			return;
		result.m_aFailures.Insert(failure);
	}

	protected string JoinFailures(array<string> failures)
	{
		string joined;
		foreach (string failure : failures)
		{
			if (!joined.IsEmpty())
				joined = joined + "; ";
			joined = joined + failure;
		}
		return joined;
	}
}
