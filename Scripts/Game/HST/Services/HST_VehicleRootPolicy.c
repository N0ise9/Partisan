class HST_VehicleRootPolicy
{
	static bool IsEligibleVehicleRootPrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		if (IsRejectedVehicleRootPrefab(prefab))
			return false;

		if (IsVehiclePartPrefab(prefab))
			return false;

		if (prefab.Contains("Prefabs/Vehicles/") || prefab.Contains("/Vehicles/"))
			return true;

		return IsKnownVehicleRootName(prefab);
	}

	static bool IsRejectedVehicleRootPrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return true;

		if (IsKnownBrokenVehiclePrefab(prefab))
			return true;

		if (IsRejectedWorldPrefab(prefab))
			return true;

		bool isVehiclePath = prefab.Contains("Prefabs/Vehicles/") || prefab.Contains("/Vehicles/");
		if (!isVehiclePath && IsKnownVehicleRootName(prefab))
			return false;

		if (!isVehiclePath)
		{
			if (prefab.Contains("Supply") || prefab.Contains("supply"))
				return true;

			if (prefab.Contains("Crate") || prefab.Contains("crate") || prefab.Contains("Cache") || prefab.Contains("cache"))
				return true;

			if (prefab.Contains("Box") || prefab.Contains("box") || prefab.Contains("Cargo") || prefab.Contains("cargo") || prefab.Contains("Container") || prefab.Contains("container"))
				return true;
		}

		if (prefab.Contains("Arsenal") || prefab.Contains("arsenal") || prefab.Contains("Composition") || prefab.Contains("composition"))
			return true;

		return false;
	}

	static bool IsKnownBrokenVehiclePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("Prefabs/Vehicles/Wheeled/UAZ469/UAZ469.et"))
			return true;

		if (prefab.Contains("Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_FIA_transport.et"))
			return true;

		if (prefab.Contains("Car_M1151.et"))
			return true;

		return false;
	}

	static bool IsKnownVehicleRootName(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		string basename = GetResourceBasename(prefab);
		if (basename.IsEmpty())
			return false;

		if (BasenameStartsWith(basename, "M998") || BasenameStartsWith(basename, "M1025") || BasenameStartsWith(basename, "M151") || BasenameStartsWith(basename, "M923") || BasenameStartsWith(basename, "M939"))
			return true;

		if (BasenameStartsWith(basename, "M11") || BasenameStartsWith(basename, "M1151") || basename.Contains("Humvee") || basename.Contains("HMMWV"))
			return true;

		if (basename.Contains("M998") || basename.Contains("M1025") || basename.Contains("M151") || basename.Contains("M923") || basename.Contains("M939"))
			return true;

		if (basename.Contains("M11") || basename.Contains("M1151") || basename.Contains("S105") || basename.Contains("S1203") || basename.Contains("UAZ") || basename.Contains("Ural"))
			return true;

		if (BasenameStartsWith(basename, "S105") || BasenameStartsWith(basename, "S1203") || BasenameStartsWith(basename, "UAZ") || BasenameStartsWith(basename, "Ural") || BasenameStartsWith(basename, "GAZ") || BasenameStartsWith(basename, "Kamaz"))
			return true;

		if (basename.Contains("GAZ") || basename.Contains("Kamaz") || basename.Contains("BTR") || basename.Contains("BMP") || basename.Contains("BRDM"))
			return true;

		if (BasenameStartsWith(basename, "BTR") || BasenameStartsWith(basename, "BMP") || BasenameStartsWith(basename, "BRDM") || BasenameStartsWith(basename, "APC"))
			return true;

		if (BasenameStartsWith(basename, "Truck") || BasenameStartsWith(basename, "Car_") || BasenameStartsWith(basename, "Jeep") || BasenameStartsWith(basename, "Offroad"))
			return true;

		if (BasenameStartsWith(basename, "Pickup") || BasenameStartsWith(basename, "Van") || BasenameStartsWith(basename, "Bus_") || basename == "Bus.et")
			return true;

		return false;
	}

	static bool IsVehiclePartPrefab(string prefab)
	{
		return IsVehiclePartName(prefab);
	}

	static bool IsVehiclePartName(string name)
	{
		if (name.IsEmpty())
			return false;

		if (name.Contains("/VehParts/") || name.Contains("VehParts/") || name.Contains("VehicleParts"))
			return true;

		if (name.Contains("Prefabs/Vehicles/") || name.Contains("/Vehicles/"))
			return IsVehiclePartBasename(GetResourceBasename(name));

		return IsVehiclePartBasename(name);
	}

	static bool IsVehiclePartBasename(string name)
	{
		if (name.IsEmpty())
			return false;

		if (name.Contains("LicensePlate") || name.Contains("licenseplate") || name.Contains("License_Plate"))
			return true;

		if (name.Contains("Wheel") || name.Contains("wheel") || name.Contains("Tire") || name.Contains("tire") || name.Contains("Tyre") || name.Contains("tyre"))
			return true;

		if (name.Contains("Suspension") || name.Contains("suspension") || name.Contains("Axle") || name.Contains("axle"))
			return true;

		if (name.Contains("Turret") || name.Contains("turret") || name.Contains("Rotor") || name.Contains("rotor") || name.Contains("Door") || name.Contains("door") || name.Contains("Seat") || name.Contains("seat"))
			return true;

		if (name.Contains("Mirror") || name.Contains("mirror") || name.Contains("Window") || name.Contains("window") || name.Contains("Headlight") || name.Contains("headlight"))
			return true;

		if (name.Contains("Light") || name.Contains("light") || name.Contains("Attachment") || name.Contains("attachment") || name.Contains("Proxy") || name.Contains("proxy") || name.Contains("Damage") || name.Contains("damage"))
			return true;

		return false;
	}

	static string BuildRejectReason(IEntity entity, string prefab)
	{
		if (!entity)
			return "candidate missing";

		if (prefab.IsEmpty())
			return "candidate had no prefab";

		if (IsRejectedWorldPrefab(prefab))
			return "prefab is scenery, prop, character, item, weapon, or vegetation";

		if (IsRejectedVehicleRootPrefab(prefab))
			return "prefab is cargo/supply/arsenal/prop, not vehicle root";

		if (IsVehiclePartPrefab(prefab) || IsVehiclePartName(entity.GetName()))
			return "candidate is a vehicle part, license plate, seat, turret, wheel, or proxy";

		if (!IsEligibleVehicleRootPrefab(prefab))
			return "prefab is not an eligible vehicle root resource";

		return "candidate passed prefab checks";
	}

	static bool IsRejectedWorldPrefab(string prefab)
	{
		if (prefab.Contains("Prefabs/Vegetation/") || prefab.Contains("/Vegetation/"))
			return true;

		if (prefab.Contains("Prefabs/Props/") || prefab.Contains("/Props/"))
			return true;

		if (prefab.Contains("Prefabs/Characters/") || prefab.Contains("/Characters/"))
			return true;

		if (prefab.Contains("Prefabs/Items/") || prefab.Contains("/Items/"))
			return true;

		if (prefab.Contains("Prefabs/Weapons/") || prefab.Contains("/Weapons/"))
			return true;

		if (prefab.Contains("Prefabs/Structures/") || prefab.Contains("/Structures/"))
			return true;

		if (prefab.Contains("Prefabs/Compositions/") || prefab.Contains("/Compositions/"))
			return true;

		string basename = GetResourceBasename(prefab);
		if (BasenameStartsWith(basename, "b_") || BasenameStartsWith(basename, "Bush") || BasenameStartsWith(basename, "Tree"))
			return true;

		if (basename.Contains("Bush") || basename.Contains("Tree") || basename.Contains("Grass"))
			return true;

		return false;
	}

	static string GetResourceBasename(string prefab)
	{
		string basename = prefab;
		int lastSlash = -1;
		for (int index = 0; index < prefab.Length(); index++)
		{
			string current = prefab.Substring(index, 1);
			if (current == "/")
				lastSlash = index;
		}

		if (lastSlash >= 0 && lastSlash + 1 < prefab.Length())
			basename = prefab.Substring(lastSlash + 1, prefab.Length() - lastSlash - 1);

		return basename;
	}

	static bool BasenameStartsWith(string basename, string prefix)
	{
		if (basename.IsEmpty() || prefix.IsEmpty())
			return false;

		if (basename.Length() < prefix.Length())
			return false;

		return basename.Substring(0, prefix.Length()) == prefix;
	}
}
