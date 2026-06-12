class HST_GeneratedContentService
{
	static const int ROADBLOCK_OFFSET_METERS = 180;
	static const int SUPPORT_OFFSET_METERS = 120;
	static const int CRASHSITE_OFFSET_METERS = 260;

	bool EnsureGeneratedContent(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || state.m_aZones.Count() == 0)
			return false;

		if (state.m_aGeneratedSites.Count() > 0 && state.m_aGeneratedRoutes.Count() > 0)
			return false;

		state.m_aGeneratedSites.Clear();
		state.m_aGeneratedRoutes.Clear();
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			HST_EGeneratedSiteType primaryType = SiteTypeForZone(zone);
			HST_GeneratedSiteState primary = CreateSite(state, zone, "primary", primaryType, zone.m_vPosition, 160, 4);
			ApplySourceMetadata(primary, zone, "primary");
			state.m_aGeneratedSites.Insert(primary);
			state.m_aGeneratedSites.Insert(CreateSourceSite(state, zone, "roadblock", "BarricadesAndSandbags.layer", "roadblock", HST_EGeneratedSiteType.HST_SITE_ROADBLOCK, OffsetPosition(zone.m_vPosition, ROADBLOCK_OFFSET_METERS, 0), 90, 2));
			state.m_aGeneratedSites.Insert(CreateSourceSite(state, zone, "support", "SupplyCaches.layer", "supply_cache", HST_EGeneratedSiteType.HST_SITE_SUPPORT, OffsetPosition(zone.m_vPosition, 0, SUPPORT_OFFSET_METERS), 80, 1));

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
				state.m_aGeneratedSites.Insert(CreateSourceSite(state, zone, "stash", "SupplyCaches.layer", "civilian_stash", HST_EGeneratedSiteType.HST_SITE_STASH, OffsetPosition(zone.m_vPosition, -SUPPORT_OFFSET_METERS, -SUPPORT_OFFSET_METERS), 60, 1));
			else
				state.m_aGeneratedSites.Insert(CreateSourceSite(state, zone, "crashsite", "SupplyCaches.layer", "salvage_anchor", HST_EGeneratedSiteType.HST_SITE_CRASHSITE, OffsetPosition(zone.m_vPosition, -CRASHSITE_OFFSET_METERS, CRASHSITE_OFFSET_METERS), 110, 1));

			if (zone.m_sSourceLayerName == "SupplyDepots.layer")
				state.m_aGeneratedSites.Insert(CreateSourceSite(state, zone, "depot_loot", "SupplyDepots.layer", "supply_depot_loot", HST_EGeneratedSiteType.HST_SITE_STASH, zone.m_vPosition, 120, 3));

			state.m_aGeneratedRoutes.Insert(CreateRoute(state, zone, primary));
		}

		Print(string.Format("h-istasi | generated %1 alpha site(s) and %2 route(s)", state.m_aGeneratedSites.Count(), state.m_aGeneratedRoutes.Count()));
		return true;
	}

	HST_GeneratedSiteState FindPrimarySiteForZone(HST_CampaignState state, string zoneId)
	{
		return FindSiteForZone(state, zoneId, "primary");
	}

	HST_GeneratedSiteState FindMissionSiteForZone(HST_CampaignState state, string zoneId, HST_EMissionCategory category, string missionId = "")
	{
		if (!state)
			return null;

		if (category == HST_EMissionCategory.HST_MISSION_CONVOY)
			return FindSiteForZone(state, zoneId, "roadblock");

		if (missionId == "destroy_downed_helicopter" || missionId == "logistics_salvage_supplies")
			return FindSiteForZone(state, zoneId, "crashsite");

		if (category == HST_EMissionCategory.HST_MISSION_LOGISTICS || category == HST_EMissionCategory.HST_MISSION_SUPPORT || missionId == "dynamic_gun_shop")
			return FindSiteForZone(state, zoneId, "support");

		if (missionId == "logistics_resource_cache")
			return FindSiteForZone(state, zoneId, "stash");

		return FindPrimarySiteForZone(state, zoneId);
	}

	HST_GeneratedSiteState FindSiteForZone(HST_CampaignState state, string zoneId, string suffix)
	{
		if (!state || zoneId.IsEmpty() || suffix.IsEmpty())
			return null;

		string siteId = string.Format("site_%1_%2", zoneId, suffix);
		return state.FindGeneratedSite(siteId);
	}

	bool IsMissionPositionValid(HST_CampaignState state, vector position, int minDistanceMeters = 180)
	{
		if (!state)
			return false;

		if (!HST_WorldPositionService.IsDryGroundPosition(position))
			return false;

		float minDistanceSq = minDistanceMeters * minDistanceMeters;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
			if (site && DistanceSq2D(site.m_vPosition, position) < minDistanceSq)
				return false;
		}

		return true;
	}

	string BuildContentReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi content | state not ready";

		int validSites;
		int roadblocks;
		int crashsites;
		int vehicleSafeRoutes;
		int invalidRoutes;
		int minWaypointCount = 999999;
		string routeDetails = "";
		foreach (HST_GeneratedSiteState site : state.m_aGeneratedSites)
		{
			if (!site)
				continue;

			if (site.m_bValid)
				validSites++;

			if (site.m_eType == HST_EGeneratedSiteType.HST_SITE_ROADBLOCK)
				roadblocks++;

			if (site.m_eType == HST_EGeneratedSiteType.HST_SITE_CRASHSITE)
				crashsites++;
		}

		foreach (HST_GeneratedRouteState route : state.m_aGeneratedRoutes)
		{
			if (!route)
				continue;

			EnsureRouteWaypointMetadata(route);
			ValidateRouteForVehicles(route);
			if (route.m_bValidatedForVehicles)
				vehicleSafeRoutes++;
			else
				invalidRoutes++;
			if (route.m_iWaypointCount < minWaypointCount)
				minWaypointCount = route.m_iWaypointCount;
			routeDetails = routeDetails + BuildGeneratedRouteReport(route);
		}

		if (minWaypointCount == 999999)
			minWaypointCount = 0;

		string summary = string.Format("h-istasi generated content | sites %1/%2 valid | routes %3", validSites, state.m_aGeneratedSites.Count(), state.m_aGeneratedRoutes.Count());
		summary = summary + string.Format(" | roadblocks %1 | crashsites %2", roadblocks, crashsites);
		summary = summary + string.Format(" | vehicle-safe routes %1 | invalid routes %2 | min route waypoints %3", vehicleSafeRoutes, invalidRoutes, minWaypointCount);
		return summary + routeDetails;
	}

	protected HST_GeneratedSiteState CreateSite(HST_CampaignState state, HST_ZoneState zone, string suffix, HST_EGeneratedSiteType siteType, vector position, int radiusMeters, int weight)
	{
		HST_GeneratedSiteState site = new HST_GeneratedSiteState();
		site.m_sSiteId = string.Format("site_%1_%2", zone.m_sZoneId, suffix);
		site.m_sZoneId = zone.m_sZoneId;
		site.m_sRouteId = string.Format("route_%1_alpha", zone.m_sZoneId);
		site.m_eType = siteType;
		site.m_vPosition = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		site.m_vSecondaryPosition = HST_WorldPositionService.ResolveGroundPosition(OffsetPosition(position, 45, -45), HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		site.m_iRadiusMeters = radiusMeters;
		site.m_iWeight = Math.Max(1, weight);
		site.m_bValid = IsMissionPositionValidIgnoringActive(state, site.m_vPosition);
		site.m_sOwnerFactionKey = zone.m_sOwnerFactionKey;
		return site;
	}

	protected HST_GeneratedSiteState CreateSourceSite(HST_CampaignState state, HST_ZoneState zone, string suffix, string sourceLayerName, string sourceCategory, HST_EGeneratedSiteType siteType, vector position, int radiusMeters, int weight)
	{
		HST_GeneratedSiteState site = CreateSite(state, zone, suffix, siteType, position, radiusMeters, weight);
		site.m_sSourceLayerName = sourceLayerName;
		site.m_sSourceCategory = sourceCategory;
		site.m_sSourceLayoutId = zone.m_sSourceLayoutId;
		return site;
	}

	protected void ApplySourceMetadata(HST_GeneratedSiteState site, HST_ZoneState zone, string sourceCategory)
	{
		if (!site || !zone)
			return;

		site.m_sSourceLayerName = zone.m_sSourceLayerName;
		site.m_sSourceCategory = sourceCategory;
		site.m_sSourceLayoutId = zone.m_sSourceLayoutId;
	}

	protected HST_GeneratedRouteState CreateRoute(HST_CampaignState state, HST_ZoneState zone, HST_GeneratedSiteState site)
	{
		HST_GeneratedRouteState route = new HST_GeneratedRouteState();
		route.m_sRouteId = string.Format("route_%1_alpha", zone.m_sZoneId);
		route.m_sSourceZoneId = zone.m_sZoneId;
		route.m_sTargetZoneId = zone.m_sZoneId;
		route.m_sSourceLayerName = "VehiclePatrols.layer";
		route.m_sSourceCategory = "patrol_route";
		route.m_sSourceLayoutId = zone.m_sSourceLayoutId;
		route.m_vStartPosition = ResolveRouteWaypointPosition(OffsetPosition(zone.m_vPosition, -360, 0), "start");
		route.m_vMidPosition = ResolveRouteWaypointPosition(site.m_vSecondaryPosition, "midpoint");
		route.m_vEndPosition = ResolveRouteWaypointPosition(site.m_vPosition, "destination");
		route.m_bRoadRoute = true;
		route.m_aWaypoints.Insert(CreateRouteWaypoint(route, 0, route.m_vStartPosition, "start"));
		route.m_aWaypoints.Insert(CreateRouteWaypoint(route, 1, route.m_vMidPosition, "midpoint"));
		route.m_aWaypoints.Insert(CreateRouteWaypoint(route, 2, route.m_vEndPosition, "destination"));
		EnsureRouteWaypointMetadata(route);
		ValidateRouteForVehicles(route);
		return route;
	}

	protected HST_RouteWaypointState CreateRouteWaypoint(HST_GeneratedRouteState route, int index, vector position, string hint)
	{
		HST_RouteWaypointState waypoint = new HST_RouteWaypointState();
		if (route)
			waypoint.m_sRouteId = route.m_sRouteId;
		waypoint.m_iIndex = index;
		waypoint.m_vPosition = position;
		waypoint.m_iRadiusMeters = 35;
		waypoint.m_sHint = hint;
		return waypoint;
	}

	protected vector ResolveRouteWaypointPosition(vector preferred, string hint)
	{
		vector resolved;
		if (HST_WorldPositionService.TryResolveLargeVehicleSpawnPosition(preferred, resolved, true) && !HST_WorldPositionService.IsLikelyOpenWater(resolved))
			return resolved;

		return HST_WorldPositionService.ResolveSafeGroundPosition(preferred, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 8.0);
	}

	protected void EnsureRouteWaypointMetadata(HST_GeneratedRouteState route)
	{
		if (!route)
			return;

		if (route.m_aWaypoints.Count() == 0)
		{
			route.m_aWaypoints.Insert(CreateRouteWaypoint(route, 0, route.m_vStartPosition, "start"));
			route.m_aWaypoints.Insert(CreateRouteWaypoint(route, 1, route.m_vMidPosition, "midpoint"));
			route.m_aWaypoints.Insert(CreateRouteWaypoint(route, 2, route.m_vEndPosition, "destination"));
		}

		for (int i = 0; i < route.m_aWaypoints.Count(); i++)
		{
			HST_RouteWaypointState waypoint = route.m_aWaypoints[i];
			if (!waypoint)
				continue;

			waypoint.m_sRouteId = route.m_sRouteId;
			waypoint.m_iIndex = i;
			if (waypoint.m_iRadiusMeters <= 0)
				waypoint.m_iRadiusMeters = 35;
			if (waypoint.m_sHint.IsEmpty())
				waypoint.m_sHint = "route";
		}

		route.m_iWaypointCount = route.m_aWaypoints.Count();
		route.m_iDistanceMeters = CalculateRouteDistanceMeters(route);
		if (route.m_aWaypoints.Count() >= 3)
		{
			route.m_vStartPosition = route.m_aWaypoints[0].m_vPosition;
			route.m_vMidPosition = route.m_aWaypoints[1].m_vPosition;
			route.m_vEndPosition = route.m_aWaypoints[route.m_aWaypoints.Count() - 1].m_vPosition;
		}
	}

	protected void ValidateRouteForVehicles(HST_GeneratedRouteState route)
	{
		if (!route)
			return;

		route.m_bValidatedForVehicles = false;
		route.m_sValidationFailureReason = "";
		EnsureRouteWaypointMetadata(route);
		if (route.m_iWaypointCount < 3)
		{
			route.m_sValidationFailureReason = "route has fewer than three waypoints";
			return;
		}

		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint)
				continue;

			vector resolved;
			if (!HST_WorldPositionService.TryResolveLargeVehicleSpawnPosition(waypoint.m_vPosition, resolved, true) || HST_WorldPositionService.IsLikelyOpenWater(resolved))
			{
				route.m_sValidationFailureReason = "waypoint not dry vehicle-safe: " + waypoint.m_sHint;
				return;
			}
		}

		route.m_bValidatedForVehicles = true;
	}

	protected int CalculateRouteDistanceMeters(HST_GeneratedRouteState route)
	{
		if (!route || route.m_aWaypoints.Count() < 2)
			return 0;

		float distance;
		for (int i = 1; i < route.m_aWaypoints.Count(); i++)
		{
			HST_RouteWaypointState previous = route.m_aWaypoints[i - 1];
			HST_RouteWaypointState current = route.m_aWaypoints[i];
			if (!previous || !current)
				continue;

			distance += Math.Sqrt(DistanceSq2D(previous.m_vPosition, current.m_vPosition));
		}

		return Math.Round(distance);
	}

	protected string BuildGeneratedRouteReport(HST_GeneratedRouteState route)
	{
		if (!route)
			return "\n  generated route | missing";

		string failure = route.m_sValidationFailureReason;
		if (failure.IsEmpty())
			failure = "none";
		string report = string.Format("\n  generated route | route %1 | source zone %2 | target zone %3", route.m_sRouteId, route.m_sSourceZoneId, route.m_sTargetZoneId);
		report = report + string.Format(" | road %1 | vehicle-safe %2 | waypoint count %3 | distance %4m", route.m_bRoadRoute, route.m_bValidatedForVehicles, route.m_iWaypointCount, route.m_iDistanceMeters);
		report = report + " | validation " + failure;
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint)
				continue;

			report = report + string.Format("\n    waypoint %1 | hint %2 | radius %3m | position %4", waypoint.m_iIndex, waypoint.m_sHint, waypoint.m_iRadiusMeters, waypoint.m_vPosition);
		}

		return report;
	}

	protected HST_EGeneratedSiteType SiteTypeForZone(HST_ZoneState zone)
	{
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return HST_EGeneratedSiteType.HST_SITE_TOWN_CENTER;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return HST_EGeneratedSiteType.HST_SITE_RESOURCE;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return HST_EGeneratedSiteType.HST_SITE_FACTORY;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return HST_EGeneratedSiteType.HST_SITE_RADIO;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return HST_EGeneratedSiteType.HST_SITE_AIRFIELD;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return HST_EGeneratedSiteType.HST_SITE_SEAPORT;

		return HST_EGeneratedSiteType.HST_SITE_OUTPOST;
	}

	protected vector OffsetPosition(vector source, float x, float z)
	{
		vector result = source;
		result[0] = result[0] + x;
		result[2] = result[2] + z;
		return result;
	}

	protected bool IsMissionPositionValidIgnoringActive(HST_CampaignState state, vector position)
	{
		if (!state)
			return false;

		return HST_WorldPositionService.IsDryGroundPosition(position);
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
