#ifdef ENABLE_DIAG
class HST_AmbientVehicleSaveValidationReport
{
	bool m_bUnclaimedRowsExcludedExact;
	bool m_bUnclaimedCargoExcludedExact;
	bool m_bClaimedFieldRoundTripExact;
	bool m_bLegacyClaimMigrationExact;
	bool m_bDurableControlRoundTripExact;
	string m_sUnclaimedEvidence;
	string m_sClaimedEvidence;
	string m_sDurableControlEvidence;

	bool AllExact()
	{
		return m_bUnclaimedRowsExcludedExact
			&& m_bUnclaimedCargoExcludedExact
			&& m_bClaimedFieldRoundTripExact
			&& m_bLegacyClaimMigrationExact
			&& m_bDurableControlRoundTripExact;
	}

	string BuildReport()
	{
		return string.Format(
			"ambient vehicle save proof | all exact %1 | unclaimed rows %2 | unclaimed cargo %3 | claimed field %4 | legacy claim %5 | durable control %6",
			AllExact(),
			m_bUnclaimedRowsExcludedExact,
			m_bUnclaimedCargoExcludedExact,
			m_bClaimedFieldRoundTripExact,
			m_bLegacyClaimMigrationExact,
			m_bDurableControlRoundTripExact);
	}
}

// Save-copy proof for the boundary between disposable physical ambience and
// deliberately claimed persistent field vehicles. No world entities are used.
class HST_AmbientVehicleSaveValidationService
{
	static const string TRAFFIC_ID = "proof_ambient_traffic";
	static const string PARKED_ID = "proof_ambient_parked";
	static const string MILITARY_ID = "proof_ambient_military";
	static const string CLAIMED_ID = "proof_claimed_field";
	static const string LEGACY_CLAIMED_ID = "proof_legacy_claimed_ambient";
	static const string CONTROL_ID = "proof_durable_control";

	HST_AmbientVehicleSaveValidationReport Run()
	{
		HST_CampaignState source = new HST_CampaignState();
		InsertVehicleWithCargo(source, TRAFFIC_ID, "CIV_TRAFFIC_VEHICLE", false, false);
		InsertVehicleWithCargo(source, PARKED_ID, "CIV_VEHICLE", false, true);
		InsertVehicleWithCargo(source, MILITARY_ID, "MILITARY_VEHICLE", false, false);
		InsertVehicleWithCargo(source, CLAIMED_ID, "field_vehicle", false, false);
		InsertVehicleWithCargo(source, LEGACY_CLAIMED_ID, "CIV_VEHICLE", true, false);
		InsertVehicleWithCargo(source, CONTROL_ID, "garage_redeploy", false, false);

		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(source);
		HST_CampaignState restored = new HST_CampaignState();
		save.ApplyTo(restored);

		HST_AmbientVehicleSaveValidationReport report
			= new HST_AmbientVehicleSaveValidationReport();
		report.m_bUnclaimedRowsExcludedExact
			= !FindVehicle(save.m_aRuntimeVehicles, TRAFFIC_ID)
			&& !FindVehicle(save.m_aRuntimeVehicles, PARKED_ID)
			&& !FindVehicle(save.m_aRuntimeVehicles, MILITARY_ID)
			&& !restored.FindRuntimeVehicle(TRAFFIC_ID)
			&& !restored.FindRuntimeVehicle(PARKED_ID)
			&& !restored.FindRuntimeVehicle(MILITARY_ID);
		report.m_bUnclaimedCargoExcludedExact
			= !FindCargo(save.m_aVehicleCargoItems, TRAFFIC_ID)
			&& !FindCargo(save.m_aVehicleCargoItems, PARKED_ID)
			&& !FindCargo(save.m_aVehicleCargoItems, MILITARY_ID)
			&& !restored.FindVehicleCargoItem(TRAFFIC_ID, BuildCargoPrefab(TRAFFIC_ID))
			&& !restored.FindVehicleCargoItem(PARKED_ID, BuildCargoPrefab(PARKED_ID))
			&& !restored.FindVehicleCargoItem(MILITARY_ID, BuildCargoPrefab(MILITARY_ID));

		HST_RuntimeVehicleState claimed = restored.FindRuntimeVehicle(CLAIMED_ID);
		report.m_bClaimedFieldRoundTripExact = claimed
			&& claimed.m_sRuntimeKind == "field_vehicle"
			&& !claimed.m_bDetached
			&& !claimed.m_bDeleted
			&& restored.FindVehicleCargoItem(
				CLAIMED_ID,
				BuildCargoPrefab(CLAIMED_ID));
		HST_RuntimeVehicleState legacyClaimed
			= restored.FindRuntimeVehicle(LEGACY_CLAIMED_ID);
		report.m_bLegacyClaimMigrationExact = legacyClaimed
			&& legacyClaimed.m_sRuntimeKind == "field_vehicle"
			&& !legacyClaimed.m_bDetached
			&& !legacyClaimed.m_bDeleted
			&& restored.FindVehicleCargoItem(
				LEGACY_CLAIMED_ID,
				BuildCargoPrefab(LEGACY_CLAIMED_ID));
		HST_RuntimeVehicleState control = restored.FindRuntimeVehicle(CONTROL_ID);
		report.m_bDurableControlRoundTripExact = control
			&& control.m_sRuntimeKind == "garage_redeploy"
			&& restored.FindVehicleCargoItem(
				CONTROL_ID,
				BuildCargoPrefab(CONTROL_ID));

		report.m_sUnclaimedEvidence = string.Format(
			"save/restored vehicles %1/%2 | cargo %3/%4 | excluded rows/cargo %5/%6",
			save.m_aRuntimeVehicles.Count(),
			restored.m_aRuntimeVehicles.Count(),
			save.m_aVehicleCargoItems.Count(),
			restored.m_aVehicleCargoItems.Count(),
			report.m_bUnclaimedRowsExcludedExact,
			report.m_bUnclaimedCargoExcludedExact);
		report.m_sClaimedEvidence = string.Format(
			"claimed present/kind/detached/deleted/cargo %1/%2/%3/%4/%5",
			claimed != null,
			claimed && claimed.m_sRuntimeKind,
			claimed && claimed.m_bDetached,
			claimed && claimed.m_bDeleted,
			restored.FindVehicleCargoItem(CLAIMED_ID, BuildCargoPrefab(CLAIMED_ID)) != null);
		report.m_sDurableControlEvidence = string.Format(
			"control present/kind/cargo %1/%2/%3",
			control != null,
			control && control.m_sRuntimeKind,
			restored.FindVehicleCargoItem(CONTROL_ID, BuildCargoPrefab(CONTROL_ID)) != null);
		return report;
	}

	protected void InsertVehicleWithCargo(
		HST_CampaignState state,
		string runtimeId,
		string runtimeKind,
		bool detached,
		bool deleted)
	{
		HST_RuntimeVehicleState vehicle = new HST_RuntimeVehicleState();
		vehicle.m_sVehicleRuntimeId = runtimeId;
		vehicle.m_sPrefab = "proof_vehicle_prefab";
		vehicle.m_sRuntimeKind = runtimeKind;
		vehicle.m_vPosition = "100 0 100";
		vehicle.m_bDetached = detached;
		vehicle.m_bDeleted = deleted;
		state.m_aRuntimeVehicles.Insert(vehicle);

		HST_VehicleCargoItemState cargo = new HST_VehicleCargoItemState();
		cargo.m_sVehicleRuntimeId = runtimeId;
		cargo.m_sItemPrefab = BuildCargoPrefab(runtimeId);
		cargo.m_iCount = 1;
		state.m_aVehicleCargoItems.Insert(cargo);
	}

	protected HST_VehicleCargoItemState FindCargo(
		array<ref HST_VehicleCargoItemState> cargoItems,
		string runtimeId)
	{
		if (!cargoItems)
			return null;
		foreach (HST_VehicleCargoItemState cargo : cargoItems)
		{
			if (cargo && cargo.m_sVehicleRuntimeId == runtimeId)
				return cargo;
		}
		return null;
	}

	protected HST_RuntimeVehicleState FindVehicle(
		array<ref HST_RuntimeVehicleState> vehicles,
		string runtimeId)
	{
		if (!vehicles)
			return null;
		foreach (HST_RuntimeVehicleState vehicle : vehicles)
		{
			if (vehicle && vehicle.m_sVehicleRuntimeId == runtimeId)
				return vehicle;
		}
		return null;
	}

	protected string BuildCargoPrefab(string runtimeId)
	{
		return "proof_cargo_" + runtimeId;
	}
}
#endif
