class HST_VehicleCapabilityPolicy
{
	static string ResolveSourceVehicleKind(string prefab)
	{
		if (IsAmmoSourcePrefab(prefab))
			return "ammo";

		if (IsRepairSourcePrefab(prefab))
			return "repair";

		if (IsFuelSourcePrefab(prefab))
			return "fuel";

		if (IsArmedVehiclePrefab(prefab))
			return "armed";

		return "transport";
	}

	static string ResolveSourceVehicleKindFromState(string prefab, bool ammoSource, bool repairSource, bool fuelSource)
	{
		if (ammoSource)
			return "ammo";

		if (repairSource)
			return "repair";

		if (fuelSource)
			return "fuel";

		return ResolveSourceVehicleKind(prefab);
	}

	static bool IsAmmoSourcePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		string basename = HST_VehicleRootPolicy.GetResourceBasename(prefab);
		return basename.Contains("Ammo")
			|| basename.Contains("ammo")
			|| basename.Contains("Ammunition")
			|| basename.Contains("ammunition");
	}

	static bool IsRepairSourcePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		string basename = HST_VehicleRootPolicy.GetResourceBasename(prefab);
		return basename.Contains("Repair")
			|| basename.Contains("repair")
			|| basename.Contains("Service")
			|| basename.Contains("service")
			|| basename.Contains("Maintenance")
			|| basename.Contains("maintenance")
			|| basename.Contains("Workshop")
			|| basename.Contains("workshop");
	}

	static bool IsFuelSourcePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		string basename = HST_VehicleRootPolicy.GetResourceBasename(prefab);
		return basename.Contains("Fuel")
			|| basename.Contains("fuel")
			|| basename.Contains("Refuel")
			|| basename.Contains("refuel")
			|| basename.Contains("Tanker")
			|| basename.Contains("tanker");
	}

	static bool IsArmedVehiclePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		string basename = HST_VehicleRootPolicy.GetResourceBasename(prefab);
		return basename.Contains("APC")
			|| basename.Contains("BTR")
			|| basename.Contains("BMP")
			|| basename.Contains("BRDM")
			|| basename.Contains("DShK")
			|| basename.Contains("PKM")
			|| basename.Contains("M2")
			|| basename.Contains("Armed")
			|| basename.Contains("armed");
	}

	static void ApplyToGarageVehicle(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return;

		vehicle.m_bAmmoSource = IsAmmoSourcePrefab(vehicle.m_sPrefab);
		vehicle.m_bRepairSource = IsRepairSourcePrefab(vehicle.m_sPrefab);
		vehicle.m_bFuelSource = IsFuelSourcePrefab(vehicle.m_sPrefab);
		vehicle.m_sSourceVehicleKind = ResolveSourceVehicleKind(vehicle.m_sPrefab);
	}

	static void ApplyToRuntimeVehicle(HST_RuntimeVehicleState vehicle)
	{
		if (!vehicle)
			return;

		vehicle.m_bAmmoSource = IsAmmoSourcePrefab(vehicle.m_sPrefab);
		vehicle.m_bRepairSource = IsRepairSourcePrefab(vehicle.m_sPrefab);
		vehicle.m_bFuelSource = IsFuelSourcePrefab(vehicle.m_sPrefab);
		vehicle.m_sSourceVehicleKind = ResolveSourceVehicleKind(vehicle.m_sPrefab);
	}
}
