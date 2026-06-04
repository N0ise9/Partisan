$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Assert-EqualSet {
	param(
		[string] $Label,
		[string[]] $Expected,
		[string[]] $Actual
	)

	$diff = @(Compare-Object @($Expected | Sort-Object) @($Actual | Sort-Object))
	if ($diff.Count -gt 0) {
		throw "$Label mismatch:`n$($diff | Out-String)"
	}

	Write-Host "$Label match: $($Expected.Count)"
}

function Get-ConfigBlocks {
	param(
		[string] $Text,
		[string] $ClassName
	)

	$blocks = @()
	$pattern = [regex]::Escape($ClassName) + "\s*\{"
	$matches = [regex]::Matches($Text, $pattern)
	foreach ($match in $matches) {
		$start = $match.Index
		$depth = 0
		for ($i = $start; $i -lt $Text.Length; $i++) {
			if ($Text[$i] -eq "{") {
				$depth++
			} elseif ($Text[$i] -eq "}") {
				$depth--
				if ($depth -eq 0) {
					$blocks += $Text.Substring($start, $i - $start + 1)
					break
				}
			}
		}
	}

	return $blocks
}

function Get-VectorXZKey {
	param([string] $Block)

	if ($Block -notmatch "m_vPosition\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)") {
		return ""
	}

	$x = [double] $Matches[1]
	$z = [double] $Matches[3]
	return "$([math]::Round($x, 3)),$([math]::Round($z, 3))"
}

function Get-VectorXZObject {
	param([string] $Block)

	if ($Block -notmatch "m_vPosition\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)") {
		return $null
	}

	return [pscustomobject]@{
		X = [double] $Matches[1]
		Z = [double] $Matches[3]
	}
}

$project = Get-Content -Raw "addon.gproj"
if ($project -notmatch '"58D0FB3206B6F859"' -or $project -notmatch '"595F2BF2F44836FB"') {
	throw "addon.gproj must depend on base Reforger and RHS Status Quo"
}
Write-Host "Project dependencies OK"

$files = Get-ChildItem -Recurse -File -Include *.c,*.conf,*.ent,*.et,*.meta,*.layer,*.layout,*.gproj,*.md,.gitignore |
	Where-Object { $_.FullName -notlike "*\.git\*" }

foreach ($file in $files) {
	$text = Get-Content -Raw $file.FullName
	if (([regex]::Matches($text, "\{")).Count -ne ([regex]::Matches($text, "\}")).Count) {
		throw "Brace imbalance: $($file.FullName)"
	}

	$lineNumber = 0
	foreach ($line in Get-Content $file.FullName) {
		$lineNumber++
		if ($line -match "[ `t]+$") {
			throw "Trailing whitespace: $($file.FullName):$lineNumber"
		}
	}
}
Write-Host "Brace and whitespace checks OK"

$missionHeaders = Get-ChildItem -File "Missions" -Filter *.conf
foreach ($missionHeader in $missionHeaders) {
	$text = Get-Content -Raw $missionHeader.FullName
	if ($text -match "\bm_bIsSavingEnabled\b") {
		throw "Unsupported Reforger 1.7 mission-header field in $($missionHeader.FullName): m_bIsSavingEnabled"
	}
}
Write-Host "Mission header fields OK"

$runtimeLayers = @(
	"Worlds/HST_Dev/HST_Dev_Layers/default.layer",
	"Worlds/HST_Everon/HST_Everon_Layers/default.layer"
)
$requiredRuntimeScaffold = @(
	'm_bAutoPlayerRespawn 1',
	'Prefabs/AI/SCR_AIWorld_Eden.et',
	'Configs/Navmesh/Navmesh_CTI_Campaign_Eden_Soldier.conf',
	'Configs/Navmesh/Navmesh_CTI_Campaign_Eden_Vehicle.conf',
	'Prefabs/World/Game/PerceptionManager.et',
	'SCR_RespawnSystemComponent',
	'HST_PlayerSpawnLogic',
	'm_bEnableRespawn 1',
	'm_bEnablePauseMenuRespawn 0',
	'{6985327711303200}Prefabs/Characters/HST/HST_PlayerController.et',
	'm_bUseSpawnPreload 0',
	'SCR_MapConfigComponent',
	'Configs/Map/MapSpawnConflict.conf',
	'SCR_MapMarkerManagerComponent',
	'Configs/Map/CampaignMapMarkerConfig.conf',
	'SCR_MapMarkerDotCircle',
	'HST_NativeMapMarker_hq',
	'HST_CommandMenuComponent',
	'SCR_PlayerSpawnPointManagerComponent',
	'SCR_SpawnProtectionComponent',
	'SCR_TimedSpawnPointComponent',
	'Prefabs/MP/Managers/Factions/FactionManager_USxUSSR.et',
	'SCR_FactionAliasComponent',
	'm_sAlias "PLAYERS"',
	'm_sFactionKey "FIA"',
	'm_sAlias "OPFOR"',
	'm_sFactionKey "RHS_AFRF"',
	'FactionKey "RHS_ION"',
	'Configs/Factions/FIA_Campaign.conf',
	'Configs/Factions/CIV.conf',
	'Prefabs/MP/Managers/Loadouts/LoadoutManager_Base.et',
	'm_aPlayerLoadouts',
	'SCR_FactionPlayerLoadout',
	'Configs/Loadouts/FIA/Loadout_FIA_Base.conf',
	'm_sAffiliatedFaction "FIA"',
	'Prefabs/Systems/Radio/RadioManager.et',
	'Prefabs/MP/ScriptedChatEntity.et',
	'SCR_MapLocator',
	'Configs/Map/MapLocatorHints/LocationHintsEveron.conf'
)
$requiredFiaLoadouts = @(
	'Character_FIA_Rifleman.et',
	'Character_FIA_Medic.et',
	'Character_FIA_MG.et',
	'Character_FIA_LAT.et'
)
foreach ($runtimeLayer in $runtimeLayers) {
	$text = Get-Content -Raw $runtimeLayer
	foreach ($requiredEntry in $requiredRuntimeScaffold) {
		if ($text -notmatch [regex]::Escape($requiredEntry)) {
			throw "Missing runtime scaffold entry in ${runtimeLayer}: $requiredEntry"
		}
	}

	if ($text -match "\bNavmeshFileOveride\b") {
		throw "Runtime layer must not depend on authored reference-world navmesh overrides: $runtimeLayer"
	}

	if ($text -match "\bSCR_PlayerArsenalLoadout\b") {
		throw "Runtime layer must use stock role-selection player loadouts, not arsenal loadouts: $runtimeLayer"
	}

	if ($text -match "\bSCR_MenuSpawnLogic\b") {
		throw "Runtime layer must not use stock role-selection spawn logic: $runtimeLayer"
	}

	if ($text -match "Loadout_USAF_") {
		throw "Runtime layer must not expose RHS_USAF role-selection loadouts in the primary FIA deploy path: $runtimeLayer"
	}

	if ($text -notmatch 'PlayerControllerPrefab "\{6985327711303200\}Prefabs/Characters/HST/HST_PlayerController\.et"') {
		throw "Runtime layer must use the HST player-controller request bridge: $runtimeLayer"
	}

	if ($text -match 'PlayerControllerPrefab "Prefabs/Characters/HST/HST_PlayerController\.et"') {
		throw "Runtime layer must not use path-only HST player-controller resources: $runtimeLayer"
	}

	foreach ($requiredLoadout in $requiredFiaLoadouts) {
		if ($text -notmatch [regex]::Escape($requiredLoadout)) {
			throw "Missing primary FIA player loadout in ${runtimeLayer}: $requiredLoadout"
		}
	}

	$fiaLoadoutCount = ([regex]::Matches($text, "Character_FIA_[A-Za-z0-9_]+\.et")).Count
	if ($fiaLoadoutCount -lt $requiredFiaLoadouts.Count) {
		throw "Primary FIA loadout list is empty or incomplete in ${runtimeLayer}"
	}

	$fiaAffiliationCount = ([regex]::Matches($text, 'm_sAffiliatedFaction "FIA"')).Count
	if ($fiaAffiliationCount -lt $requiredFiaLoadouts.Count) {
		throw "Primary FIA player loadouts must be explicitly affiliated with FIA in ${runtimeLayer}"
	}

	if ($text -notmatch "SCR_MapMarkerManagerComponent" -or $text -notmatch "SCR_MapMarkerDotCircle" -or $text -notmatch "HST_NativeMapMarker_hq") {
		throw "Runtime layer must expose native map marker manager plus FIA HQ native marker: $runtimeLayer"
	}

	$runtimeNativeMarkerCount = ([regex]::Matches($text, "SCR_MapMarkerDotCircle\s+HST_NativeMapMarker_")).Count
	$runtimeNativeMarkerRplCount = ([regex]::Matches($text, "SCR_MapMarkerDotCircle\s+HST_NativeMapMarker_[A-Za-z0-9_]+\s*\{\s*components\s*\{\s*RplComponent")).Count
	if ($runtimeNativeMarkerRplCount -lt $runtimeNativeMarkerCount) {
		throw "Runtime native map marker lacks RplComponent in ${runtimeLayer}"
	}
}
Write-Host "World runtime scaffold OK"

$startingPointLayers = @(
	"Worlds/HST_Dev/HST_Dev_Layers/StartingPoints.layer",
	"Worlds/HST_Everon/HST_Everon_Layers/StartingPoints.layer"
)
foreach ($startingPointLayer in $startingPointLayers) {
	if (!(Test-Path $startingPointLayer)) {
		throw "Missing playable HQ starting point layer: $startingPointLayer"
	}

	$text = Get-Content -Raw $startingPointLayer
	if ($text -match '"faction affiliation" "RHS_USAF"') {
		throw "Starting point layer must not keep RHS_USAF as the primary playable deploy affiliation: $startingPointLayer"
	}

	foreach ($requiredEntry in @(
		"Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et",
		"SCR_CampaignMilitaryBaseComponent",
		"SCR_CampaignSpawnPointGroup",
		'"faction affiliation" "FIA"',
		"Prefabs/Systems/ScenarioFramework/Components/Area.et",
		"Prefabs/Systems/ScenarioFramework/Components/Layer.et",
		"Prefabs/Systems/ScenarioFramework/Components/Slot.et",
		"SCR_ScenarioFrameworkPluginSpawnPoint",
		'PrefabsEditable/SpawnPoints/E_SpawnPoint_FIA.et',
		'm_sFactionKey "PLAYERS"',
		'm_sFactionKey "FIA"',
		'm_sFaction "FIA"',
		"m_bExcludeFromDynamicDespawn 1"
	)) {
		if ($text -notmatch [regex]::Escape($requiredEntry)) {
			throw "Missing HQ spawn entry in ${startingPointLayer}: $requiredEntry"
		}
	}

	$expectedSpawnpoints = 1
	if ($startingPointLayer -match "HST_Everon") {
		$expectedSpawnpoints = 3
	}

	$spawnpointCount = ([regex]::Matches($text, "SCR_ScenarioFrameworkPluginSpawnPoint")).Count
	if ($spawnpointCount -lt $expectedSpawnpoints) {
		throw "Expected at least $expectedSpawnpoints Scenario Framework deploy spawnpoints in ${startingPointLayer}, found $spawnpointCount"
	}
}
Write-Host "Playable HQ starting points OK"

$petrosPrefabPath = "Prefabs/Characters/HST/Character_HST_Petros.et"
if (!(Test-Path $petrosPrefabPath)) {
	throw "Missing dedicated Petros prefab: $petrosPrefabPath"
}

$petrosPrefabMetaPath = "$petrosPrefabPath.meta"
if (!(Test-Path $petrosPrefabMetaPath)) {
	throw "Missing dedicated Petros prefab metadata: $petrosPrefabMetaPath"
}

if ((Get-Content -Raw $petrosPrefabMetaPath) -notmatch '\{6985327711303300\}Prefabs/Characters/HST/Character_HST_Petros\.et') {
	throw "Dedicated Petros prefab metadata must expose the GUID-qualified resource name"
}

$petrosPrefabText = Get-Content -Raw $petrosPrefabPath
$hqServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_HQService.c"
foreach ($requiredPetrosPrefabEntry in @(
	"SCR_ChimeraCharacter Character_HST_Petros",
	"Character_FIA_Rifleman.et",
	"ActionsManagerComponent",
	"HST_PetrosCommandMenuAction",
	"HST_PetrosMoveBaseHereAction",
	"HST_PetrosArsenalMenuAction",
	"ParentContextList",
	"UIInfo"
)) {
	if ($petrosPrefabText -notmatch [regex]::Escape($requiredPetrosPrefabEntry)) {
		throw "Dedicated Petros prefab is missing editable inheritance entry: $requiredPetrosPrefabEntry"
	}
}

foreach ($requiredPetrosServiceEntry in @(
	"PETROS_BASE_PREFAB",
	'PETROS_PREFAB = "{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et"',
	'ARSENAL_PREFAB = "{6985327711303400}Prefabs/Objects/HST/HST_HQArsenal.et"',
	"ARSENAL_FALLBACK_PREFAB",
	"ResolvePetrosPrefab",
	"SpawnPetros",
	"ResolveArsenalPrefab",
	"SpawnArsenal",
	"state.m_sPetrosPrefab = PETROS_PREFAB",
	"state.m_sArsenalPrefab = ARSENAL_PREFAB",
	"MoveHQToPosition",
	"ClearRuntimeObjects",
	"SCR_EntityHelper.DeleteEntityAndChildren",
	"failed to spawn; using base FIA fallback",
	"failed to spawn; using FIA supply-cache fallback",
	"GetArsenalPrefab"
)) {
	if ($hqServiceText -notmatch [regex]::Escape($requiredPetrosServiceEntry)) {
		throw "HQ service is missing dedicated Petros prefab entry: $requiredPetrosServiceEntry"
	}
}

if ($hqServiceText -notmatch "SpawnPetros\(respawnSystem, state\)") {
	throw "HQ runtime spawn must route Petros through the custom-prefab fallback helper"
}
if ($hqServiceText -notmatch "SpawnArsenal\(respawnSystem, state\)") {
	throw "HQ runtime spawn must route the HQ arsenal through the custom-prefab fallback helper"
}
if ($hqServiceText -match 'PETROS_PREFAB = "Prefabs/Characters/HST/Character_HST_Petros\.et"' -or $hqServiceText -match 'ARSENAL_PREFAB = "Prefabs/Objects/HST/HST_HQArsenal\.et"') {
	throw "HQ service must not keep path-only HST prefab constants"
}
Write-Host "Dedicated Petros prefab OK"

$hqArsenalPrefabPath = "Prefabs/Objects/HST/HST_HQArsenal.et"
if (!(Test-Path $hqArsenalPrefabPath)) {
	throw "Missing HST HQ arsenal prefab: $hqArsenalPrefabPath"
}

$hqArsenalPrefabMetaPath = "$hqArsenalPrefabPath.meta"
if (!(Test-Path $hqArsenalPrefabMetaPath)) {
	throw "Missing HST HQ arsenal prefab metadata: $hqArsenalPrefabMetaPath"
}

if ((Get-Content -Raw $hqArsenalPrefabMetaPath) -notmatch '\{6985327711303400\}Prefabs/Objects/HST/HST_HQArsenal\.et') {
	throw "HST HQ arsenal prefab metadata must expose the GUID-qualified resource name"
}

$hqArsenalPrefabText = Get-Content -Raw $hqArsenalPrefabPath
foreach ($requiredArsenalPrefabEntry in @(
	"GenericEntity HST_HQArsenal",
	"SupplyCache_S_FIA_01.et",
	"ActionsManagerComponent",
	"HST_HQArsenalOpenAction",
	"HST_HQArsenalLootNearbyAction",
	"Open h-istasi Arsenal",
	"Loot nearby to h-istasi Arsenal"
)) {
	if ($hqArsenalPrefabText -notmatch [regex]::Escape($requiredArsenalPrefabEntry)) {
		throw "HST HQ arsenal prefab is missing contextual action entry: $requiredArsenalPrefabEntry"
	}
}
Write-Host "HST HQ arsenal prefab OK"

$playerControllerPrefabPath = "Prefabs/Characters/HST/HST_PlayerController.et"
if (!(Test-Path $playerControllerPrefabPath)) {
	throw "Missing HST player controller prefab: $playerControllerPrefabPath"
}

$playerControllerPrefabMetaPath = "$playerControllerPrefabPath.meta"
if (!(Test-Path $playerControllerPrefabMetaPath)) {
	throw "Missing HST player controller prefab metadata: $playerControllerPrefabMetaPath"
}

if ((Get-Content -Raw $playerControllerPrefabMetaPath) -notmatch '\{6985327711303200\}Prefabs/Characters/HST/HST_PlayerController\.et') {
	throw "HST player controller prefab metadata must expose the GUID-qualified resource name"
}

$playerControllerPrefabText = Get-Content -Raw $playerControllerPrefabPath
foreach ($requiredPlayerControllerEntry in @(
	"SCR_PlayerController HST_PlayerController",
	"DefaultPlayerControllerMP_ScenarioFramework.et",
	"HST_CommandMenuRequestComponent"
)) {
	if ($playerControllerPrefabText -notmatch [regex]::Escape($requiredPlayerControllerEntry)) {
		throw "HST player controller prefab is missing request/RPC bridge entry: $requiredPlayerControllerEntry"
	}
}
Write-Host "HST player controller request bridge OK"

$defaultCatalog = Get-Content -Raw "Scripts/Game/HST/Config/HST_DefaultCatalog.c"
$missionConfig = Get-Content -Raw "Configs/HST/Missions/HST_CE311_Missions.conf"
$mapConfig = Get-Content -Raw "Configs/HST/Maps/HST_Everon.conf"

if ($defaultCatalog -notmatch 'preset\.m_sResistanceFactionKey = "FIA"' -or $defaultCatalog -notmatch 'preset\.m_sOccupierFactionKey = "RHS_USAF"') {
	throw "Campaign catalog must keep FIA as the resistance while RHS_USAF remains the occupier"
}
Write-Host "Campaign faction identity OK"

$runtimeMissions = @([regex]::Matches($defaultCatalog, 'NewMission\("([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
$configMissions = @([regex]::Matches($missionConfig, 'm_sMissionId "([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
Assert-EqualSet "Mission registries" $runtimeMissions $configMissions

$runtimeZones = @([regex]::Matches($defaultCatalog, 'NewZoneState\("([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
$configZones = @([regex]::Matches($mapConfig, 'm_sZoneId "([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
Assert-EqualSet "Zone registries" $runtimeZones $configZones

foreach ($group in @($configMissions | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate mission ID: $($group.Name)"
}

foreach ($group in @($configZones | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate zone ID: $($group.Name)"
}
Write-Host "Unique IDs OK"

$expectedEveronTownIds = @(
	"town_saint_pierre",
	"town_provins",
	"town_entre_deux",
	"town_chotain",
	"town_montignac",
	"town_laruns",
	"town_levie",
	"town_morton",
	"town_meaux",
	"town_tyrone",
	"town_gravette",
	"town_villeneuve",
	"town_le_moule",
	"town_lamentin",
	"town_regina",
	"town_figari",
	"town_durras",
	"town_saint_philippe"
)
$townDisplayNames = @{
	"town_saint_pierre" = "Saint-Pierre"
	"town_provins" = "Provins"
	"town_entre_deux" = "Entre-Deux"
	"town_chotain" = "Chotain"
	"town_montignac" = "Montignac"
	"town_laruns" = "Laruns"
	"town_levie" = "Levie"
	"town_morton" = "Morton"
	"town_meaux" = "Meaux"
	"town_tyrone" = "Tyrone"
	"town_gravette" = "Gravette"
	"town_villeneuve" = "Villeneuve"
	"town_le_moule" = "Le Moule"
	"town_lamentin" = "Lamentin"
	"town_regina" = "Regina"
	"town_figari" = "Figari"
	"town_durras" = "Durras"
	"town_saint_philippe" = "Saint-Philippe"
}
foreach ($townId in $expectedEveronTownIds) {
	if ($townId -notin $runtimeZones) {
		throw "Missing Everon town in runtime catalog: $townId"
	}

	if ($townId -notin $configZones) {
		throw "Missing Everon town in map config: $townId"
	}
}

$worldResourceText = (Get-ChildItem -Recurse -File "Worlds" -Include *.layer |
	ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
$runtimeMarkerLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/default.layer"
$townLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/Towns.layer"
$strategicZonesLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/StrategicZones.layer"
$markerServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
$coordinatorMarkerText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
foreach ($requiredNativeMarkerEntry in @(
	"SCR_MapMarkerManagerComponent",
	"Configs/Map/CampaignMapMarkerConfig.conf",
	"SCR_MapMarkerDotCircle",
	"HST_NativeMapMarker_hq"
)) {
	if ($runtimeMarkerLayer -notmatch [regex]::Escape($requiredNativeMarkerEntry)) {
		throw "Missing native map marker scaffold entry: $requiredNativeMarkerEntry"
	}
}

foreach ($requiredRuntimeMarkerEntry in @(
	"HST_MapMarkerService",
	"RebuildAllMarkers",
	"RefreshHQMarker",
	"RefreshZoneMarker",
	"RefreshMissionMarkers",
	"CleanupMarkers",
	"m_bRuntimeNative",
	"NATIVE_MARKER_MANAGER_COMPONENT",
	"SCR_MapMarkerManagerComponent"
)) {
	if ($markerServiceText -notmatch [regex]::Escape($requiredRuntimeMarkerEntry) -and $coordinatorMarkerText -notmatch [regex]::Escape($requiredRuntimeMarkerEntry)) {
		throw "Missing runtime native marker service entry: $requiredRuntimeMarkerEntry"
	}
}

if ($worldResourceText -match "SCR_ScenarioFrameworkSlotMarker" -and $worldResourceText -match "m_eActivationType ON_TRIGGER_ACTIVATION") {
	throw "Map markers must not use trigger activation without an active Scenario Framework parent"
}

if ($worldResourceText -match "SCR_ScenarioFrameworkSlotMarker" -and ($worldResourceText -notmatch "SCR_MapMarkerManagerComponent" -or $markerServiceText -notmatch "HST_MapMarkerService")) {
	throw "Scenario Framework markers may only remain as fallback when native marker manager and runtime service are present"
}

foreach ($townId in $expectedEveronTownIds) {
	$suffix = $townId -replace "^town_", ""
	if ($townLayer -notmatch [regex]::Escape("HST_TownAnchor_$suffix")) {
		throw "Missing town anchor for $townId"
	}

	if ($runtimeMarkerLayer -notmatch [regex]::Escape("HST_NativeMapMarker_$townId")) {
		throw "Missing native town map marker for $townId"
	}

	if ($strategicZonesLayer -notmatch [regex]::Escape("HST_ZoneAnchor_$townId")) {
		throw "Missing strategic zone anchor for $townId"
	}
}

$townAnchorIds = @([regex]::Matches($townLayer, "HST_TownAnchor_([A-Za-z0-9_]+)") | ForEach-Object { $_.Groups[1].Value })
foreach ($group in @($townAnchorIds | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate town layer anchor ID: $($group.Name)"
}

$nativeMarkerIds = @([regex]::Matches($runtimeMarkerLayer, "HST_NativeMapMarker_([A-Za-z0-9_]+)") | ForEach-Object { $_.Groups[1].Value })
foreach ($group in @($nativeMarkerIds | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate native map marker ID: $($group.Name)"
}

foreach ($zoneId in $configZones) {
	if ($runtimeMarkerLayer -notmatch [regex]::Escape("HST_NativeMapMarker_$zoneId")) {
		throw "Configured zone lacks native runtime marker: $zoneId"
	}
}

$nativeMarkerCount = ([regex]::Matches($runtimeMarkerLayer, "SCR_MapMarkerDotCircle\s+HST_NativeMapMarker_")).Count
if ($nativeMarkerCount -lt $configZones.Count) {
	throw "Native runtime marker block count is unexpectedly low: $nativeMarkerCount"
}

$nativeMarkerRplCount = ([regex]::Matches($runtimeMarkerLayer, "SCR_MapMarkerDotCircle\s+HST_NativeMapMarker_[A-Za-z0-9_]+\s*\{\s*components\s*\{\s*RplComponent")).Count
if ($nativeMarkerRplCount -lt $nativeMarkerCount) {
	throw "Native map marker is missing RplComponent for SCR_MapMarkerEntity init"
}
Write-Host "Everon town coverage OK: $($expectedEveronTownIds.Count)"

$hideoutBlocks = @(Get-ConfigBlocks $mapConfig "HST_HideoutDefinition")
$zoneBlocks = @(Get-ConfigBlocks $mapConfig "HST_ZoneDefinition")
$hideoutPositions = @{}
$hideoutVectors = @{}
foreach ($block in $hideoutBlocks) {
	if ($block -notmatch 'm_sHideoutId "([^"]+)"') {
		continue
	}

	$hideoutId = $Matches[1]
	$hideoutPositions[$hideoutId] = Get-VectorXZKey $block
	$hideoutVectors[$hideoutId] = Get-VectorXZObject $block
}

$zonePositions = @{}
$zoneVectors = @{}
foreach ($block in $zoneBlocks) {
	if ($block -notmatch 'm_sZoneId "([^"]+)"') {
		continue
	}

	$zoneId = $Matches[1]
	$zonePositions[$zoneId] = Get-VectorXZKey $block
	$zoneVectors[$zoneId] = Get-VectorXZObject $block
	foreach ($requiredZoneField in @("m_iIncomeValue", "m_iSupport", "m_iGarrisonSlots", "m_sPatrolRouteId", "m_sQRFRouteId", "m_sMissionSiteId")) {
		if ($block -notmatch [regex]::Escape($requiredZoneField)) {
			throw "Configured zone $zoneId is missing $requiredZoneField"
		}
	}
}

foreach ($hideoutId in $hideoutPositions.Keys) {
	foreach ($zoneId in $zonePositions.Keys) {
		if ($hideoutPositions[$hideoutId] -eq $zonePositions[$zoneId]) {
			throw "Hideout $hideoutId shares x/z with strategic zone $zoneId"
		}
	}
}
Write-Host "Hideout placement separation OK"

$defaultHideout = $hideoutVectors["hideout_central_hills"]
if (!$defaultHideout) {
	throw "Default central hideout is missing from map config"
}

foreach ($zoneId in $zoneVectors.Keys) {
	$zoneVector = $zoneVectors[$zoneId]
	if (!$zoneVector) {
		continue
	}

	$distance = [math]::Sqrt([math]::Pow($defaultHideout.X - $zoneVector.X, 2) + [math]::Pow($defaultHideout.Z - $zoneVector.Z, 2))
	if ($distance -lt 900) {
		throw "Default HQ hideout is inside HQ safe radius of zone ${zoneId}: $([math]::Round($distance, 1))m"
	}
}
Write-Host "Default HQ safe radius separation OK"

$scriptFiles = Get-ChildItem -Recurse -File "Scripts" -Filter *.c
$scriptText = ($scriptFiles | ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
$definedSymbols = @([regex]::Matches($scriptText, "(?:class|enum)\s+(HST_[A-Za-z0-9_]+)") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
$codeOnly = [regex]::Replace($scriptText, '"(?:\\.|[^"\\])*"', "")
$referencedSymbols = @([regex]::Matches($codeOnly, "\b(HST_[A-Za-z0-9_]+)\b") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
$missingSymbols = @($referencedSymbols |
	Where-Object { $_ -notin $definedSymbols -and $_ -notmatch "^HST_(CAMPAIGN|ZONE|MISSION)_" })
if ($missingSymbols.Count -gt 0) {
	throw "Potential undefined script symbols:`n$($missingSymbols -join "`n")"
}
Write-Host "Script symbol references OK: $($definedSymbols.Count)"

foreach ($requiredService in @(
	"HST_PlayerSpawnLogic",
	"HST_PlayerLifecycleService",
	"HST_PlayerSpawnService",
	"HST_TownService",
	"HST_GarrisonService",
	"HST_RecruitmentService",
	"HST_ZoneCaptureService",
	"HST_PhysicalWarService",
	"HST_MapMarkerService",
	"HST_CommandUIService",
	"HST_RuntimeSettings",
	"HST_RuntimeSettingsService",
	"HST_LootService",
	"HST_CommandMenuComponent",
	"HST_CommandMenuRequestComponent",
	"HST_ContextualUserActionBase",
	"HST_PetrosCommandMenuAction",
	"HST_PetrosMoveBaseHereAction",
	"HST_PetrosArsenalMenuAction",
	"HST_HQArsenalOpenAction",
	"HST_HQArsenalLootNearbyAction"
)) {
	if ($requiredService -notin $definedSymbols) {
		throw "Missing Antistasi framework service: $requiredService"
	}
}

foreach ($requiredSaveEntry in @(
	"HST_CampaignSaveData",
	"CaptureState",
	"GetLastCapturedSave",
	"m_iSchemaVersion",
	"m_aActiveMissions",
	"m_aActiveGroups",
	"m_aQRFs",
	"HST_MapMarkerState",
	"m_aMapMarkers",
	"m_bRuntimeNative",
	"m_iQrfCooldownUntilSecond",
	"m_iEnemyResourceAccumulatorSeconds",
	"m_iResistanceCaptureProgress",
	"m_sDisplayName",
	"m_sCategory",
	"m_vArsenalPosition",
	"m_sArsenalPrefab"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSaveEntry)) {
		throw "Missing campaign save scaffold entry: $requiredSaveEntry"
	}
}
Write-Host "Campaign save scaffold OK"

foreach ($requiredCoordinatorEntry in @(
	"RegisterConnectedPlayer",
	"SpawnOrRespawnPlayer",
	"GetPlayerSpawnFactionKey",
	"GetPlayerHQSpawnPosition",
	"GetDefaultPlayerPrefab",
	"GetDefaultSpawnPointPrefab",
	"CaptureZoneForResistance",
	"CompleteMission",
	"FailMission",
	"ApplyIncomeNow",
	"AddAbstractGarrison",
	"FoldGarrisonSurvivors",
	"TrainTroops",
	"RecruitResistanceGarrison",
	"AwardFactionResources",
	"AwardPlayerResources",
	"RequestCommanderMoveHQ",
	"MoveHQToPlayer",
	"RequestCommanderMoveHQToPlayer",
	"RequestCommanderStartMission",
	"RequestCommanderStartZoneMission",
	"RequestCommanderRecruitGarrison",
	"RequestCommanderTrainTroops",
	"RequestCommanderApplyIncomeNow",
	"RequestCommanderCompleteMission",
	"RequestCommanderDepositArsenalItem",
	"RequestCommanderStoreGarageVehicle",
	"RequestMemberManualCheckpoint",
	"RequestMemberInspectCampaign",
	"RequestMemberInspectMarkers",
	"RequestMemberInspectEconomy",
	"RequestMemberInspectZones",
	"RequestMemberInspectMissions",
	"GetAlphaMemberMenu",
	"GetAlphaCommanderMenu",
	"GetAlphaAdminMenu",
	"RequestAlphaUICommand",
	"BuildVisibleMenuPayload",
	"RequestVisibleMenuCommand",
	"ResolveAuthoritativePlayerId",
	"RequestMemberInspectArsenal",
	"RequestMemberLootNearby",
	"RequestAdminSetZoneActive",
	"RequestAdminCaptureZone",
	"RequestAdminCaptureZoneForResistance",
	"RequestAdminAddCaptureProgress",
	"RequestAdminStartDebugMission",
	"RequestAdminCompleteMission",
	"RequestAdminFailMission",
	"RequestAdminAwardResources",
	"RequestAdminAddEnemyResources",
	"RequestAdminInspectZone"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCoordinatorEntry)) {
		throw "Missing coordinator dev/framework entry point: $requiredCoordinatorEntry"
	}
}
Write-Host "Antistasi framework service spine OK"

foreach ($requiredCommandMenuEntry in @(
	'COMMAND_MENU_ACTION = "Inventory"',
	'COMMAND_MENU_CUSTOM_ACTION = "HST_CommandMenu"',
	'MENU_INPUT_CONTEXT = "InGameMenuContext"',
	'MENU_LAYOUT = "UI/layouts/HST_CommandMenu.layout"',
	'AddActionListener',
	'RemoveActionListener',
	'CreateWidgetInWorkspace',
	'FrameSlot.SetPos',
	'WidgetFlags.VISIBLE',
	'SCR_HintManagerComponent',
	'STAT|',
	'SECTION|',
	'ROW|',
	'FEED|',
	'RenderStats',
	'RenderMainSections',
	'RenderActivityPanel',
	'OpenPetrosMenu',
	'OpenMenuToTab',
	'RunCommandFromContext',
	'ParseActionsFromPayload',
	'OnServerSnapshot',
	'HST_CommandMenuRequestComponent.GetLocalOwner',
	'RequestSnapshot',
	'RequestAction',
	'ExecuteSelectedAction',
	'BuildVisibleMenuPayload',
	'RequestVisibleMenuCommand',
	'inspect_campaign',
	'inspect_markers',
	'inspect_economy',
	'inspect_zones',
	'inspect_missions',
	'inspect_arsenal',
	'loot_nearby',
	'move_hq',
	'move_hq_here',
	'recruit_zone',
	'mission_zone',
	'capture_zone',
	'award_small'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCommandMenuEntry)) {
		throw "Missing I-key alpha command menu contract entry: $requiredCommandMenuEntry"
	}
}
if ($scriptText -match "\bCreateWidgets\b") {
	throw "Command menu must remain procedural until HST_CommandMenu.layout is indexed with verified resource metadata"
}
Write-Host "I-key alpha command menu OK"

if (!(Test-Path "UI/layouts/HST_CommandMenu.layout")) {
	throw "Missing command menu layout"
}

$commandMenuLayoutText = Get-Content -Raw "UI/layouts/HST_CommandMenu.layout"
foreach ($requiredCommandMenuLayoutEntry in @(
	"HST_CommandMenuRoot",
	"HST_CommandMenuDynamicCanvas"
)) {
	if ($commandMenuLayoutText -notmatch [regex]::Escape($requiredCommandMenuLayoutEntry)) {
		throw "Command menu layout is missing widget entry: $requiredCommandMenuLayoutEntry"
	}
}

foreach ($requiredSettingsEntry in @(
	"HST_RuntimeSettings",
	"HST_RuntimeSettingsService",
	"HST_Settings.json",
	"LoadOrCreate",
	"WriteDefault",
	"schemaVersion",
	"startingFactionMoney",
	"startingHR",
	"autosaveIntervalSeconds",
	"activationRadiusMeters",
	"deactivationRadiusMeters",
	"arsenalUnlockThreshold",
	"magazineUnlockMultiplier",
	"lootRadiusMeters",
	"lootOnlyLockedItems",
	"removeLootedItems",
	"ApplyTo"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSettingsEntry)) {
		throw "Missing runtime settings generated-config contract entry: $requiredSettingsEntry"
	}
}
Write-Host "Runtime settings generated-config contract OK"

foreach ($requiredLootEntry in @(
	"HST_LootService",
	"HST_LootResult",
	"LootNearbyToArsenal",
	"SCR_InventoryStorageManagerComponent",
	"QueryEntitiesBySphere",
	"BaseMagazineComponent",
	"BaseWeaponComponent",
	"GrenadeMoveComponent",
	"TryDeleteItem",
	"TryRemoveItemFromStorage",
	"IsGuidedLauncher",
	"m_bLootOnlyLockedItems",
	"m_bRemoveLootedItems",
	"m_bAllowExplosiveUnlocks",
	"m_bAllowGuidedLauncherUnlocks",
	"m_iMagazineUnlockMultiplier",
	"HST_HQArsenalOpenAction",
	"HST_HQArsenalLootNearbyAction"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredLootEntry)) {
		throw "Missing loot-to-arsenal contract entry: $requiredLootEntry"
	}
}
Write-Host "Loot-to-arsenal contract OK"

foreach ($requiredFiaSpawnContract in @(
	'string m_sFactionKey = "FIA"',
	'PRIMARY_PLAYER_FACTION = "FIA"',
	'Character_FIA_Rifleman.et',
	'E_SpawnPoint_FIA.et',
	'SCR_FreeSpawnData',
	'RequestSpawn',
	'RequestPlayerSpawn',
	'OnPlayerSpawned',
	'OnPlayerSpawnFailed',
	'HasPendingSpawn',
	'HasLivingPlayerEntity',
	'SCR_DamageManagerComponent',
	'FindComponent(SCR_DamageManagerComponent)',
	'EDamageState.DESTROYED',
	'DEAD_RESPAWN_DELAY_SECONDS',
	'SCR_PossessingManagerComponent',
	'GetPlayerRespawnComponent',
	'CloseRespawnMenu',
	'SpawnMissingConnectedPlayers',
	'AreConnectedPlayersSpawnStable',
	'NeedsSpawnSweep',
	'ArmPlayerSpawnSweep',
	'ShouldProcessFrameSpawnSweep',
	'SCR_BaseGameModeComponent',
	'OnPlayerConnected',
	'OnGameStateChanged',
	'ProcessPlayerSpawnSweep',
	'HST_WorldPositionService',
	'GetSurfaceY',
	'EnsureRuntimeObjects',
	'SupplyCache_S_FIA_01.et',
	'ARSENAL_PREFAB',
	'm_vArsenalPosition',
	'HST_PetrosUserActionBase',
	'ScriptedUserAction',
	'PerformAction',
	'HasLocalEffectOnlyScript',
	'TentSmallUS_01.et'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredFiaSpawnContract)) {
		throw "Missing FIA player spawn contract entry: $requiredFiaSpawnContract"
	}
}
Write-Host "FIA player spawn contract OK"

if ($scriptText -match "\bNotifySpawn\b") {
	throw "FIA player spawn must use native spawn requests instead of manual NotifySpawn possession handoff"
}
if ($scriptText -match "already has a controlled entity") {
	throw "FIA player spawn sweeps must not spam controlled-entity diagnostics after successful spawn"
}
if ($scriptText -match "\bHasPlayerEntity\b") {
	throw "FIA player spawn stability must be based on living entities, not merely any controlled entity"
}
if ($scriptText -match "GetDamageManagerComponent") {
	throw "FIA death checks must use component lookup; SCR_ChimeraCharacter.GetDamageManagerComponent is not available in Reforger 1.7"
}
if ($scriptText -match 'string\.Format\([^\r\n;]*%1[0-9]') {
	throw "Enforce string.Format calls must not use %10 or higher placeholders; split long reports into smaller format calls"
}
Write-Host "Native spawn request contract OK"

foreach ($requiredPhysicalWarEntry in @(
	"UpdateZoneActivation",
	"HST_ActiveGroupState",
	"HST_QRFState",
	"CreateFactionTemplate",
	"TrySpawnActiveGroup",
	"FoldActiveGroup",
	"UpdateQRF",
	"TickResources",
	"AddResistanceCaptureProgress",
	"CAPTURE_PROGRESS_REQUIRED",
	"HQ_SAFE_RADIUS_METERS",
	"IsZoneInsideHQSafeArea",
	"IsAnyLivingPlayerNearZone",
	"DeleteEntityAndChildren",
	"m_vPosition",
	"m_iActivationRadiusMeters",
	"m_sPatrolRouteId",
	"m_sQRFRouteId",
	"m_sMissionSiteId",
	"m_aGroupPrefabs",
	"m_aPatrolGroupPrefabs",
	"m_aQRFGroupPrefabs"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPhysicalWarEntry) -and $configResourceText -notmatch [regex]::Escape($requiredPhysicalWarEntry)) {
		throw "Missing physical AI war scaffold entry: $requiredPhysicalWarEntry"
	}
}
Write-Host "Physical AI war scaffold OK"

$codeWithoutEnumDeclarations = [regex]::Replace($codeOnly, "(?s)enum\s+HST_[A-Za-z0-9_]+\s*\{.*?\}", "")
$unscopedEnumReferences = @([regex]::Matches($codeWithoutEnumDeclarations, "(?<!\.)\bHST_(?:CAMPAIGN|ZONE|MISSION)_[A-Z_]+\b") |
	ForEach-Object { $_.Value } |
	Sort-Object -Unique)
if ($unscopedEnumReferences.Count -gt 0) {
	throw "Unscoped HST enum references:`n$($unscopedEnumReferences -join "`n")"
}
Write-Host "Enum references scoped OK"

$configResourceText = (Get-ChildItem -Recurse -File "Configs" -Include *.conf |
	ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
$worldResourceText = (Get-ChildItem -Recurse -File "Worlds" -Include *.layer |
	ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
$configInstantiatedClasses = @([regex]::Matches($configResourceText, "(?m)^\s*(HST_[A-Za-z0-9_]+)\s*(?:\{|""\{)") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
$worldInstantiatedClasses = @([regex]::Matches($worldResourceText, "(?m)^\s*(HST_[A-Za-z0-9_]+)\s*(?:""\{|:)") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
$instantiatedClasses = @($configInstantiatedClasses + $worldInstantiatedClasses | Sort-Object -Unique)
$missingClasses = @($instantiatedClasses | Where-Object { $_ -notin $definedSymbols })
if ($missingClasses.Count -gt 0) {
	throw "Missing resource class definitions:`n$($missingClasses -join "`n")"
}
Write-Host "Resource class references OK: $($instantiatedClasses.Count)"

Write-Host "h-istasi foundation validation passed"
