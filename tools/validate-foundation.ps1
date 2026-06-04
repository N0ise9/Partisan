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

function Get-CoordinateXZKey {
	param([string] $Coordinate)

	if ($Coordinate -notmatch "(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)") {
		return ""
	}

	$x = [double] $Matches[1]
	$z = [double] $Matches[3]
	return "$([math]::Round($x, 3)),$([math]::Round($z, 3))"
}

$project = Get-Content -Raw "addon.gproj"
foreach ($requiredProjectDependency in @(
	'"58D0FB3206B6F859"',
	'"595F2BF2F44836FB"',
	'"1337C0DE5DABBEEF"',
	'"BADC0DEDABBEDA5E"'
)) {
	if ($project -notmatch [regex]::Escape($requiredProjectDependency)) {
		throw "addon.gproj is missing required dependency: $requiredProjectDependency"
	}
}

$allowedProjectDependencies = @(
	"58D0FB3206B6F859",
	"595F2BF2F44836FB",
	"1337C0DE5DABBEEF",
	"BADC0DEDABBEDA5E"
)
if ($project -notmatch 'Dependencies\s*\{([\s\S]*?)\}') {
	throw "addon.gproj is missing a Dependencies block"
}
$projectDependenciesBlock = $Matches[1]
foreach ($projectDependencyMatch in [regex]::Matches($projectDependenciesBlock, '"([0-9A-F]{16})"')) {
	$dependencyGuid = $projectDependencyMatch.Groups[1].Value
	if ($dependencyGuid -notin $allowedProjectDependencies) {
		throw "addon.gproj has non-RHS dependency GUID: $dependencyGuid"
	}
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

		if ($file.Extension -eq ".c") {
			$unescapedQuoteCount = ([regex]::Matches($line, '(?<!\\)"')).Count
			if (($unescapedQuoteCount % 2) -ne 0) {
				throw "Unbalanced script string quote: $($file.FullName):$lineNumber"
			}
		}
	}
}
Write-Host "Brace, string, and whitespace checks OK"

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

	if ($text -match "\bHST_CommandMenuComponent\b") {
		throw "Runtime layer must mount HST_CommandMenuComponent on the HST player controller, not the game mode: $runtimeLayer"
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

	if ($text -notmatch "SCR_MapMarkerManagerComponent") {
		throw "Runtime layer must expose the native map marker manager: $runtimeLayer"
	}

	if ($text -match "HST_TonkaMapMarkerArea" -or $text -match "HST_DevMapMarkerArea" -or $text -match "HST_CallsignMarker_" -or $text -match "GenericEntity\s+HST_MapMarker_") {
		throw "Runtime layer must not keep stale Scenario Framework fallback map markers: $runtimeLayer"
	}

	if ($text -match "SCR_MapMarkerDotCircle\s+HST_NativeMapMarker_") {
		throw "Runtime layer must not use red dot-circle native markers for h-istasi map locations: $runtimeLayer"
	}

	foreach ($requiredRhsIonEntry in @(
		'FactionKey "RHS_ION"',
		'm_bIsPlayable 0',
		'm_bShowInWelcomeScreenIfNonPlayable 0',
		'm_aEntityCatalogs',
		'Configs/EntityCatalog/USMC/USMC_Characters.conf',
		'Configs/EntityCatalog/USMC/USMC_Groups.conf',
		'Configs/EntityCatalog/USMC/USMC_Vehicles.conf',
		'Configs/EntityCatalog/USMC/USMC_WeaponTripod.conf',
		'Configs/EntityCatalog/USMC/USMC_InventoryItems.conf'
	)) {
		if ($text -notmatch [regex]::Escape($requiredRhsIonEntry)) {
			throw "Runtime layer has catalog-less RHS_ION faction stub entry in ${runtimeLayer}: $requiredRhsIonEntry"
		}
	}

	if ($text -match "HST_NativeMapMarker_") {
		throw "Runtime layer still contains stale native-dot marker IDs: $runtimeLayer"
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
$coordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
foreach ($requiredPetrosPrefabEntry in @(
	"SCR_ChimeraCharacter Character_HST_Petros",
	"Character_FIA_Rifleman.et",
	"ActionsManagerComponent",
	'ActionsManagerComponent "{520EA1D2F659CE02}"',
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

if ($petrosPrefabText -match 'ActionsManagerComponent "\{6985327711303301\}"') {
	throw "Dedicated Petros prefab must override the inherited FIA action manager, not add a duplicate HST-owned action manager"
}

foreach ($requiredPetrosServiceEntry in @(
	"PETROS_BASE_PREFAB",
	'PETROS_PREFAB = "{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et"',
	'ARSENAL_PREFAB = "{6985327711303400}Prefabs/Objects/HST/HST_HQArsenal.et"',
	"BootstrapInitialHideout",
	"ResolvePetrosPrefab",
	"SpawnPetros",
	"ResolveArsenalPrefab",
	"SpawnArsenal",
	"IsUsableArsenalEntity",
	"ARSENAL_VISIBLE_LIFT_METERS",
	"state.m_sPetrosPrefab = PETROS_PREFAB",
	"state.m_sArsenalPrefab = ARSENAL_PREFAB",
	"MoveHQToPosition",
	"ClearRuntimeObjects",
	"ResolveRuntimeObjectGroundPosition",
	"TryResolveGroundPosition",
	"SCR_EntityHelper.DeleteEntityAndChildren",
	"failed to spawn; using base FIA fallback",
	"no supply-cache fallback will be used",
	"successful pieces were preserved for retry",
	"LogRuntimeObjectSpawnSuccess",
	"LogRuntimeObjectSpawnFailure",
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
if ($hqServiceText -match "\bARSENAL_FALLBACK_PREFAB\b" -or $hqServiceText -match "using FIA supply-cache fallback") {
	throw "HQ service must not use a supply-cache fallback for the HQ arsenal"
}
if ($coordinatorText -notmatch "BootstrapInitialHideout\(m_State, HST_DefaultCatalog\.GetDefaultHideoutId\(\)\)") {
	throw "Coordinator must bootstrap a visible starter HQ for fresh setup campaigns"
}
if ($coordinatorText -match "m_State\.m_bHQDeployed = false") {
	throw "Setup foundation must not clear an already bootstrapped HQ"
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
	"EquipmentBox_US.et",
	"RplComponent",
	"ActionsManagerComponent",
	"HST_HQArsenalOpenAction",
	"HST_HQArsenalLootNearbyAction",
	"Open h-istasi Arsenal",
	"Loot nearby to h-istasi Arsenal"
)) {
	if ($hqArsenalPrefabText -notmatch [regex]::Escape($requiredArsenalPrefabEntry)) {
		throw "HST HQ arsenal prefab is missing h-istasi-only arsenal entry: $requiredArsenalPrefabEntry"
	}
}
foreach ($forbiddenArsenalPrefabEntry in @(
	"ArsenalBox_FIA",
	"SCR_Arsenal",
	"MSAR",
	"SupplyCache_S_FIA_01.et",
	"Ural4320_arsenal_box_tan.et"
)) {
	if ($hqArsenalPrefabText -match [regex]::Escape($forbiddenArsenalPrefabEntry)) {
		throw "HST HQ arsenal prefab must not use stock arsenal/MSAR/fake supply-cache entry: $forbiddenArsenalPrefabEntry"
	}
}
Write-Host "HST HQ arsenal prefab h-istasi-only contract OK"

$civilianGroupPrefabPath = "Prefabs/Groups/CIV/HST_CivilianTownGroup.et"
if (!(Test-Path $civilianGroupPrefabPath)) {
	throw "Missing HST civilian town group prefab: $civilianGroupPrefabPath"
}
$civilianGroupPrefabMetaPath = "$civilianGroupPrefabPath.meta"
if (!(Test-Path $civilianGroupPrefabMetaPath)) {
	throw "Missing HST civilian town group prefab metadata: $civilianGroupPrefabMetaPath"
}
if ((Get-Content -Raw $civilianGroupPrefabMetaPath) -notmatch '\{6985327711303600\}Prefabs/Groups/CIV/HST_CivilianTownGroup\.et') {
	throw "HST civilian town group prefab metadata must expose the GUID-qualified resource name"
}
$civilianGroupPrefabText = Get-Content -Raw $civilianGroupPrefabPath
foreach ($requiredCivilianGroupEntry in @(
	"SCR_AIGroup HST_CivilianTownGroup",
	'm_faction "CIV"',
	"m_aUnitPrefabSlots",
	"Character_CIV_baseLoadout.et",
	"Character_CIV.et"
)) {
	if ($civilianGroupPrefabText -notmatch [regex]::Escape($requiredCivilianGroupEntry)) {
		throw "HST civilian town group prefab is missing entry: $requiredCivilianGroupEntry"
	}
}
Write-Host "HST civilian town group prefab OK"

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
	"HST_CommandMenuRequestComponent",
	"HST_CommandMenuComponent"
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

$missionRuntimeServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionRuntimeService.c"
foreach ($missionId in $configMissions) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape("`"$missionId`"")) {
		throw "Mission runtime primitive mapper does not cover mission ID: $missionId"
	}
}
foreach ($requiredMissionPrimitive in @(
	"kill_hvt",
	"hold_area",
	"clear_area",
	"destroy_target",
	"recover_cargo",
	"rescue_extract",
	"deliver_supplies",
	"convoy_intercept",
	"abstract_fallback",
	"InitializeMissionRuntime",
	"PrimitiveForMission",
	"PrimitiveForMissionId",
	"FindCompletedActiveMissionId",
	"BuildRuntimeReport"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredMissionPrimitive)) {
		throw "Mission runtime service is missing physical primitive contract: $requiredMissionPrimitive"
	}
}
Write-Host "Mission runtime primitive coverage OK"

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

$worldResourceText = (Get-ChildItem -Recurse -File "Worlds" -Include *.layer |
	ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
$runtimeMarkerLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/default.layer"
$townLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/Towns.layer"
$strategicZonesLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/StrategicZones.layer"
$markerServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
$coordinatorMarkerText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$zoneBlocks = @(Get-ConfigBlocks $mapConfig "HST_ZoneDefinition")

if ($configZones.Count -ne 79 -or $runtimeZones.Count -ne 79) {
	throw "Tonka Everon catalog must contain 79 zones in config/runtime, found config=$($configZones.Count) runtime=$($runtimeZones.Count)"
}

$tonkaBaseBlocks = @($zoneBlocks | Where-Object { $_ -match 'm_sSourceLayerName "Bases\.layer"' })
$tonkaDepotBlocks = @($zoneBlocks | Where-Object { $_ -match 'm_sSourceLayerName "SupplyDepots\.layer"' })
$tonkaCallsigns = @($zoneBlocks | ForEach-Object {
	if ($_ -match 'm_sMarkerCallsign "([^"]+)"') {
		$Matches[1]
	}
} | Where-Object { ![string]::IsNullOrWhiteSpace($_) })
if ($tonkaBaseBlocks.Count -ne 71) {
	throw "Expected 71 Tonka Bases.layer nodes, found $($tonkaBaseBlocks.Count)"
}
if ($tonkaDepotBlocks.Count -ne 8) {
	throw "Expected 8 Tonka SupplyDepots.layer nodes, found $($tonkaDepotBlocks.Count)"
}
if ($tonkaCallsigns.Count -ne 56) {
	throw "Expected 56 paired Tonka callsigns, found $($tonkaCallsigns.Count)"
}

foreach ($group in @($tonkaCallsigns | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate Tonka callsign: $($group.Name)"
}

$zoneDisplayNames = @($zoneBlocks | ForEach-Object {
	if ($_ -match 'm_sDisplayName "([^"]+)"') {
		$Matches[1]
	}
})
foreach ($group in @($zoneDisplayNames | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate Tonka zone display label: $($group.Name)"
}

foreach ($block in $zoneBlocks) {
	if ($block -notmatch 'm_sZoneId "([^"]+)"') {
		continue
	}

	$zoneId = $Matches[1]
	foreach ($requiredTonkaField in @("m_sSourceLayoutId", "m_sSourceLayerName", "m_sMarkerLabel", "m_sMarkerTextColor", "m_sMarkerStyle", "m_aLinkedZoneIds")) {
		if ($block -notmatch [regex]::Escape($requiredTonkaField)) {
			throw "Tonka zone $zoneId is missing $requiredTonkaField"
		}
	}
}

foreach ($oldPlaceholderId in @(
	"outpost_north",
	"outpost_south",
	"airfield_main",
	"seaport_main",
	"factory_central",
	"resource_north",
	"resource_south",
	"radio_north",
	"radio_south",
	"resource_depot_central",
	"resource_depot_south",
	"resource_depot_east",
	"bank_saint_pierre",
	"bank_montignac",
	"police_saint_pierre",
	"police_morton",
	"police_lamentin"
)) {
	if ($oldPlaceholderId -in $configZones -or $oldPlaceholderId -in $runtimeZones) {
		throw "Old rough alpha placeholder zone remains after Tonka import: $oldPlaceholderId"
	}
}

foreach ($requiredNativeMarkerEntry in @(
	"SCR_MapMarkerManagerComponent",
	"Configs/Map/CampaignMapMarkerConfig.conf"
)) {
	if ($runtimeMarkerLayer -notmatch [regex]::Escape($requiredNativeMarkerEntry)) {
		throw "Missing Tonka-style map marker scaffold entry: $requiredNativeMarkerEntry"
	}
}

if ($runtimeMarkerLayer -match "HST_TonkaMapMarkerArea" -or $runtimeMarkerLayer -match "HST_CallsignMarker_" -or $runtimeMarkerLayer -match "GenericEntity\s+HST_MapMarker_") {
	throw "Everon runtime layer must not contain stale Scenario Framework marker entities"
}

foreach ($requiredRuntimeMarkerEntry in @(
	"HST_MapMarkerService",
	"RebuildAllMarkers",
	"RefreshHQMarker",
	"RefreshZoneMarker",
	"RefreshMissionMarkers",
	"CleanupMarkers",
	"TickNativePublish",
	"PublishRuntimeNativeMarkers",
	"CreateRuntimeNativeMarker",
	"ClearRuntimeNativeMarkers",
	"m_bRuntimeNative",
	"m_aRuntimeNativeMarkers",
	"m_bNativePublishPending",
	"hst_zone_callsign_",
	"BuildCallsignMarkerPosition",
	"magenta",
	"NATIVE_MARKER_MANAGER_COMPONENT",
	"SCR_MapMarkerManagerComponent",
	"SCR_MapMarkerBase",
	"SCR_EMapMarkerType.PLACED_CUSTOM",
	"SetIconEntry",
	"SetColorEntry",
	"SetCustomText",
	"SetWorldPos",
	"InsertStaticMarker",
	"RemoveStaticMarker",
	"GetStaticMarkerByID",
	"POINT_SPECIAL",
	"OBSERVATION_POST"
)) {
	if ($markerServiceText -notmatch [regex]::Escape($requiredRuntimeMarkerEntry) -and $coordinatorMarkerText -notmatch [regex]::Escape($requiredRuntimeMarkerEntry)) {
		throw "Missing runtime Tonka-style marker service entry: $requiredRuntimeMarkerEntry"
	}
}

foreach ($zoneId in $configZones) {
	if ($strategicZonesLayer -notmatch [regex]::Escape("HST_ZoneAnchor_$zoneId")) {
		throw "Missing strategic zone anchor for configured Tonka node: $zoneId"
	}
}

$townZoneIds = @($zoneBlocks | Where-Object { $_ -match "\bm_eType HST_ZONE_TOWN\b" } | ForEach-Object {
	if ($_ -match 'm_sZoneId "([^"]+)"') {
		$Matches[1]
	}
})
foreach ($townId in $townZoneIds) {
	$suffix = $townId -replace "^town_", ""
	if ($townLayer -notmatch [regex]::Escape("HST_TownAnchor_$suffix")) {
		throw "Missing town anchor for $townId"
	}

	if ($strategicZonesLayer -notmatch [regex]::Escape("HST_ZoneAnchor_$townId")) {
		throw "Missing strategic zone anchor for $townId"
	}
}

$townAnchorIds = @([regex]::Matches($townLayer, "HST_TownAnchor_([A-Za-z0-9_]+)") | ForEach-Object { $_.Groups[1].Value })
foreach ($group in @($townAnchorIds | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate town layer anchor ID: $($group.Name)"
}

$conflictMarkerIds = @([regex]::Matches($runtimeMarkerLayer, "HST_ConflictMapMarker_([A-Za-z0-9_]+)") | ForEach-Object { $_.Groups[1].Value })
foreach ($group in @($conflictMarkerIds | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate native Conflict campaign marker ID: $($group.Name)"
}

foreach ($zoneId in $configZones) {
	if ($runtimeMarkerLayer -notmatch [regex]::Escape("HST_ConflictMapMarker_$zoneId")) {
		throw "Configured zone lacks visible native Conflict campaign marker: $zoneId"
	}

	if ($strategicZonesLayer -notmatch [regex]::Escape("HST_ZoneAnchor_$zoneId")) {
		throw "Configured zone lacks strategic anchor: $zoneId"
	}
}

$conflictMarkerCount = ([regex]::Matches($runtimeMarkerLayer, "HST_ConflictMapMarker_")).Count
if ($conflictMarkerCount -ne $configZones.Count) {
	throw "Visible native Conflict campaign marker count must equal configured zone count: markers=$conflictMarkerCount zones=$($configZones.Count)"
}

if ($runtimeMarkerLayer -notmatch "HST_ConflictMapMarker_[\s\S]*?SCR_FactionAffiliationComponent" -or $runtimeMarkerLayer -notmatch "HST_ConflictMapMarker_[\s\S]*?SCR_CampaignMilitaryBaseComponent") {
	throw "Visible native Conflict campaign markers must carry faction affiliation and military base components"
}

if ($runtimeMarkerLayer -match "SCR_MapMarkerDotCircle\s+HST_NativeMapMarker_") {
	throw "Tonka-style marker layer must not include red dot-circle native marker overlays"
}

Write-Host "Tonka Everon coverage OK: zones=$($configZones.Count) bases=$($tonkaBaseBlocks.Count) depots=$($tonkaDepotBlocks.Count) callsigns=$($tonkaCallsigns.Count) towns=$($townZoneIds.Count)"

$hideoutBlocks = @(Get-ConfigBlocks $mapConfig "HST_HideoutDefinition")
$zoneBlocks = @(Get-ConfigBlocks $mapConfig "HST_ZoneDefinition")
$hideoutPositions = @{}
$hideoutVectors = @{}
$expectedHideoutCoordinates = @{
	"hideout_north_forest" = "6332.167 75.926 8446.294"
	"hideout_central_hills" = "4280.766 14.317 3468.06"
	"hideout_south_woods" = "8355.991 237.817 4765.673"
}
foreach ($block in $hideoutBlocks) {
	if ($block -notmatch 'm_sHideoutId "([^"]+)"') {
		continue
	}

	$hideoutId = $Matches[1]
	$hideoutPositions[$hideoutId] = Get-VectorXZKey $block
	$hideoutVectors[$hideoutId] = Get-VectorXZObject $block
}

foreach ($expectedHideoutId in $expectedHideoutCoordinates.Keys) {
	if (!$hideoutPositions.ContainsKey($expectedHideoutId)) {
		throw "Expected hideout is missing from map config: $expectedHideoutId"
	}

	$expectedCoord = $expectedHideoutCoordinates[$expectedHideoutId]
	$expectedKey = Get-CoordinateXZKey $expectedCoord
	if ($hideoutPositions[$expectedHideoutId] -ne $expectedKey) {
		throw "Hideout $expectedHideoutId must use inland coordinate $expectedCoord, found $($hideoutPositions[$expectedHideoutId])"
	}
}

$everonStartingPointsLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/StartingPoints.layer"
$everonHideoutsLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/Hideouts.layer"
foreach ($expectedHideoutId in $expectedHideoutCoordinates.Keys) {
	$expectedCoord = $expectedHideoutCoordinates[$expectedHideoutId]
	if ($defaultCatalog -notmatch [regex]::Escape($expectedCoord)) {
		throw "Default catalog is missing inland hideout coordinate ${expectedHideoutId}: $expectedCoord"
	}

	if ($everonStartingPointsLayer -notmatch [regex]::Escape("coords $expectedCoord")) {
		throw "Everon starting points layer is missing inland hideout coordinate ${expectedHideoutId}: $expectedCoord"
	}

	if ($everonHideoutsLayer -notmatch [regex]::Escape("coords $expectedCoord")) {
		throw "Everon hideouts layer is missing inland hideout anchor ${expectedHideoutId}: $expectedCoord"
	}
}

foreach ($oldHideoutCoordinate in @("2500 0 9700", "4000 0 3000", "5400 0 1600", "2300 0 10300", "1800 0 3500", "900 0 7200", "3200 0 4100", "3400 0 4500", "2300 0 8500")) {
	foreach ($hideoutSource in @(
		@{ Label = "Default catalog"; Text = $defaultCatalog },
		@{ Label = "Everon map config"; Text = $mapConfig },
		@{ Label = "Everon starting points"; Text = $everonStartingPointsLayer },
		@{ Label = "Everon hideouts"; Text = $everonHideoutsLayer },
		@{ Label = "Everon marker layer"; Text = $runtimeMarkerLayer }
	)) {
		if ($hideoutSource.Text -match [regex]::Escape($oldHideoutCoordinate)) {
			throw "$($hideoutSource.Label) still contains ocean-risk hideout coordinate: $oldHideoutCoordinate"
		}
	}
}
Write-Host "Inland hideout coordinates OK"

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

	foreach ($requiredTonkaRuntimeField in @("m_sDisplayName", "m_sResourceKind", "m_iCaptureRadiusMeters", "m_iPriority", "m_sCompositionId", "m_sSpawnProfileId")) {
		if ($block -notmatch [regex]::Escape($requiredTonkaRuntimeField)) {
			throw "Tonka zone $zoneId is missing $requiredTonkaRuntimeField"
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
$configResourceText = (Get-ChildItem -Recurse -File "Configs" -Include *.conf |
	ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
$worldPositionServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_WorldPositionService.c"
if ($worldPositionServiceText -match "source\[1\]\s*<=\s*MIN_DRY_SURFACE_Y") {
	throw "World position dry-ground rejection must not accept water-level catalog coordinates"
}
if ($worldPositionServiceText -notmatch "if \(rejectWater && surfaceY < MIN_DRY_SURFACE_Y\)\s*\r?\n\s*return false;") {
	throw "World position service must reject water surfaces when dry ground is requested"
}
Write-Host "Dry-ground rejection contract OK"

$playerSpawnServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PlayerSpawnService.c"
$hqServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_HQService.c"
if ($defaultCatalog -notmatch "GetEmergencySpawnPosition") {
	throw "Default catalog must expose a positive emergency spawn position"
}
if ($playerSpawnServiceText -match "ResolveGroundPosition\(fallbackPosition,\s*HST_WorldPositionService\.CHARACTER_GROUND_OFFSET,\s*false\)") {
	throw "FIA player spawn must not fall back to water-permissive ground snapping"
}
if ($hqServiceText -match "ResolveGroundPosition\(state\.m_vHQPosition,\s*HST_WorldPositionService\.HQ_GROUND_OFFSET,\s*false\)") {
	throw "HQ runtime placement must not fall back to water-permissive ground snapping"
}
Write-Host "Dry HQ/player emergency fallback contract OK"

$mapMarkerServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
if ($mapMarkerServiceText -match "SCR_BaseGameMode\s+gameMode\s*=\s*GetGame\(\)\.GetGameMode\(\)") {
	throw "Map marker manager resolver must not unsafe-assign BaseGameMode to SCR_BaseGameMode"
}
if ($mapMarkerServiceText -match "MARK_QUESTION") {
	throw "Map marker service must not publish question-mark icons for Tonka-style campaign markers"
}
foreach ($requiredMarkerColorContract in @(
	'return "GREEN";',
	'return "BLUFOR";',
	'return "RED";',
	'return "green";',
	'return "blue";',
	'return "red";'
)) {
	if ($mapMarkerServiceText -notmatch [regex]::Escape($requiredMarkerColorContract)) {
		throw "Map marker service is missing faction color contract: $requiredMarkerColorContract"
	}
}
foreach ($requiredNativeMarkerSyncContract in @(
	"SyncVisibleNativeMarkerOwnership",
	"HST_ConflictMapMarker_",
	"SetAffiliatedFactionByKey(zone.m_sOwnerFactionKey)",
	"AddNativeMarkerCandidate"
)) {
	if ($mapMarkerServiceText -notmatch [regex]::Escape($requiredNativeMarkerSyncContract)) {
		throw "Map marker service is missing native marker ownership sync contract: $requiredNativeMarkerSyncContract"
	}
}
foreach ($requiredNativeMarkerPublishContract in @(
	"SCR_EScenarioFrameworkMarkerCustomColor.GREEN",
	"SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR",
	"SCR_EScenarioFrameworkMarkerCustomColor.RED",
	"SCR_EScenarioFrameworkMarkerCustomColor.MAGENTA",
	"SCR_EScenarioFrameworkMarkerCustom.MINE_SINGLE",
	"SCR_EScenarioFrameworkMarkerCustom.PICK_UP2",
	"SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL",
	"SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST",
	"SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER",
	"markerManager.InsertStaticMarker(nativeMarker, false, true)",
	"markerManager.RemoveStaticMarker(activeMarker)"
)) {
	if ($mapMarkerServiceText -notmatch [regex]::Escape($requiredNativeMarkerPublishContract)) {
		throw "Map marker service is missing native marker publish contract: $requiredNativeMarkerPublishContract"
	}
}
Write-Host "Faction marker color contract OK"

$definedSymbols = @([regex]::Matches($scriptText, "(?:class|enum)\s+(HST_[A-Za-z0-9_]+)") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
$codeOnly = [regex]::Replace($scriptText, '"(?:\\.|[^"\\])*"', "")
$referencedSymbols = @([regex]::Matches($codeOnly, "\b(HST_[A-Za-z0-9_]+)\b") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
$missingSymbols = @($referencedSymbols |
	Where-Object {
		$_ -notin $definedSymbols `
			-and $_ -notin @("HST_HQArsenal", "HST_Settings") `
			-and $_ -notmatch "^HST_(CAMPAIGN|ZONE|MISSION|SITE|OBJECTIVE|SUPPORT|ENEMY_ORDER|UNDERCOVER)_"
	})
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
	"HST_GeneratedContentService",
	"HST_MissionObjectiveService",
	"HST_MissionRuntimeService",
	"HST_SupportRequestService",
	"HST_CivilianService",
	"HST_EnemyCommanderService",
	"HST_CommandMenuComponent",
	"HST_CommandMenuRequestComponent",
	"HST_ContextualUserActionBase",
	"HST_PetrosCommandMenuAction",
	"HST_PetrosMoveBaseHereAction",
	"HST_PetrosArsenalMenuAction",
	"HST_HQArsenalOpenAction",
	"HST_HQArsenalLootNearbyAction",
	"HST_VehicleCollectLootAction",
	"HST_VehicleUnloadLootAction"
)) {
	if ($requiredService -notin $definedSymbols) {
		throw "Missing Antistasi framework service: $requiredService"
	}
}

foreach ($requiredSaveEntry in @(
	"HST_CampaignSaveData",
	"CaptureState",
	"GetLastCapturedSave",
	"RestoreOrCreateCampaignState",
	"CaptureAndTrackState",
	"ApplyRestoredCampaignState",
	"MigrateToCurrentSchema",
	"PersistenceSystem.GetInstance",
	"GetPersistentState",
	"StartTracking",
	"IsSavingEnabled",
	"RequestSavePoint",
	"m_iSchemaVersion",
	"m_iLastLoadedSchemaVersion",
	"m_iLastSaveSecond",
	"m_iLastRestoreSecond",
	"m_bRestoredFromPersistence",
	"m_sLastPersistenceStatus",
	"m_aActiveMissions",
	"m_aActiveGroups",
	"m_sRuntimePrimitive",
	"m_sRuntimeEntityId",
	"m_iRuntimeStartedAtSecond",
	"m_iRuntimeHoldSeconds",
	"m_bRuntimeSpawned",
	"m_bRuntimeFallback",
	"m_bRuntimeCleanupComplete",
	"m_iHoldSeconds",
	"m_iRequiredHoldSeconds",
	"m_bWorldDetected",
	"m_bAbstractFallback",
	"m_sRouteId",
	"m_vSourcePosition",
	"m_vTargetPosition",
	"m_sRuntimeStatus",
	"m_iLastSeenAliveCount",
	"m_iSurvivorInfantryCount",
	"m_iSurvivorVehicleCount",
	"m_aQRFs",
	"HST_MapMarkerState",
	"m_aMapMarkers",
	"m_bRuntimeNative",
	"m_iQrfCooldownUntilSecond",
	"m_iEnemyResourceAccumulatorSeconds",
	"m_iResistanceCaptureProgress",
	"m_sDisplayName",
	"m_sSourceLayoutId",
	"m_sSourceLayerName",
	"m_sMarkerCallsign",
	"m_sMarkerTextColor",
	"m_sMarkerStyle",
	"m_sResourceKind",
	"m_iCaptureRadiusMeters",
	"m_iPriority",
	"m_sCompositionId",
	"m_sSpawnProfileId",
	"m_aLinkedZoneIds",
	"m_sCallsign",
	"m_sTextColorHint",
	"m_sStyleHint",
	"m_sCategory",
	"m_vArsenalPosition",
	"m_sArsenalPrefab",
	"HST_VehicleCargoItemState",
	"m_aVehicleCargoItems",
	"m_sVehicleRuntimeId",
	"m_sVehiclePrefab",
	"m_sVehicleDisplayName",
	"m_iRedeployCost",
	"m_sSourceZoneId",
	"m_sCapabilityId",
	"m_sAssetProfileId",
	"m_sStrikeKind",
	"m_sStrikeConfigResource",
	"m_bPhysicalStrikeSpawned",
	"m_bAbstractResolved",
	"m_iMoneyCost",
	"m_iCooldownUntilSecond",
	"m_sTargetZoneId",
	"m_sPhysicalEntityId",
	"m_bCleanupComplete",
	"m_aGeneratedSites",
	"m_aGeneratedRoutes",
	"m_sSourceCategory",
	"m_aMissionObjectives",
	"m_aSupportRequests",
	"m_aEnemyOrders",
	"m_aCivilianZones",
	"m_aUndercoverPlayers",
	"m_aCampaignTasks",
	"HST_SupportRequestState",
	"HST_EnemyOrderState",
	"HST_CivilianZoneState"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSaveEntry)) {
		throw "Missing campaign save scaffold entry: $requiredSaveEntry"
	}
}
Write-Host "Campaign save scaffold OK"

foreach ($requiredSupportStrikeEntry in @(
	"HST_SUPPORT_AIRSTRIKE_GBU",
	"HST_SUPPORT_AIRSTRIKE_UMPK",
	"HST_SUPPORT_CRUISE_MISSILE_KH55",
	"RHS_CallIn_Blue_CAS_GBU.conf",
	"RHS_CallIn_Opfor_CAS_FAB500UMPK.conf",
	"RHS_CallIn_Opfor_Missle_KH55.conf",
	"StrikeKindForSupport",
	"StrikeConfigForSupport",
	"ResolveStrikeSupport",
	"HasResistanceAirSupportCapability",
	"air_support_unlockable",
	"support_umpk",
	"support_kh55"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSupportStrikeEntry)) {
		throw "Missing RHS support strike contract entry: $requiredSupportStrikeEntry"
	}
}
Write-Host "RHS support strike contract OK"

$requiredLocalAfrfGroupResources = @(
	"{A6A3EDA237E3D336}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_FireGroup_NotSpawned.et",
	"{20882FD7E695BA4F}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_SentryTeam_NotSpawned.et",
	"{949A54E9F28D99F9}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_RifleSquad_NotSpawned.et",
	"{FBB6A99E9B1C7EC6}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_MachineGunTeam_NotSpawned.et",
	"{84856BD6BFABA047}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_Team_GL_NotSpawned.et",
	"{CBE67BBD93170CF8}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_Team_AT_NotSpawned.et",
	"{7D9243C3C0CCED4B}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/SSO/Group_RHS_hSPEC_SSO_TEAM.et",
	"{622DDA327B669B67}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/SSO/Group_RHS_hSPEC_SSO_ATTeam.et",
	"{DEEA32376CC7BA04}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/SSO/Group_RHS_hSPEC_SSO_BoFTeam.et",
	"{E368C2565A31C25F}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/SSO/Group_RHS_hSPEC_SSO_GLTeam.et",
	"{78A5969A0A1F45A8}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/SSO/Group_RHS_hSPEC_SSO_SniperTeam.et",
	"{39DB4846F673AF9F}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/VVRG/Group_RHS_hSPEC_VVRG_TEAM.et"
)
foreach ($requiredAfrfResource in $requiredLocalAfrfGroupResources) {
	if ($requiredAfrfResource -notmatch '^\{([0-9A-F]{16})\}(.+)$') {
		throw "Malformed copied AFRF group resource entry: $requiredAfrfResource"
	}

	$resourceGuid = $Matches[1]
	$resourcePath = $Matches[2]
	$resourceMetaPath = "$resourcePath.meta"
	if (!(Test-Path $resourcePath)) {
		throw "Copied AFRF group prefab is missing locally: $resourcePath"
	}
	if (!(Test-Path $resourceMetaPath)) {
		throw "Copied AFRF group prefab metadata is missing locally: $resourceMetaPath"
	}
	if ((Get-Content -Raw $resourceMetaPath) -notmatch [regex]::Escape($requiredAfrfResource)) {
		throw "Copied AFRF group metadata GUID/path mismatch: $requiredAfrfResource"
	}
	if ($scriptText -notmatch [regex]::Escape($requiredAfrfResource) -and $configResourceText -notmatch [regex]::Escape($requiredAfrfResource)) {
		throw "Copied AFRF group resource is not used by faction pools: $requiredAfrfResource"
	}
}
if ($scriptText -match "D9523F26504A8D59" -or $configResourceText -match "D9523F26504A8D59") {
	throw "AFRF faction pools still reference the stale VVRG GUID instead of the copied local prefab GUID"
}
Write-Host "Local copied AFRF group resources OK: $($requiredLocalAfrfGroupResources.Count)"

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
	"RequestCommanderSelectInitialHideout",
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
	"RequestMemberInspectGarage",
	"RequestMemberInspectSupport",
	"RequestMemberInspectCivilians",
	"RequestMemberInspectUndercover",
	"RequestMemberInspectGeneratedContent",
	"RequestMemberInspectPersistence",
	"RequestMemberInspectMissionRuntime",
	"RequestMemberLootNearby",
	"RequestMemberWithdrawBestArsenalItem",
	"RequestMemberCaptureNearbyVehicle",
	"RequestMemberRedeployGarageVehicle",
	"RequestCommanderStartRandomMission",
	"RequestCommanderProgressMission",
	"RequestCommanderCallSupplyDrop",
	"RequestCommanderCallPlayerSupport",
	"RequestCommanderCancelSupport",
	"RequestCommanderAidNearestTown",
	"RequestAdminSetZoneActive",
	"RequestAdminCaptureZone",
	"RequestAdminCaptureZoneForResistance",
	"RequestAdminAddCaptureProgress",
	"RequestAdminStartDebugMission",
	"RequestAdminCompleteMission",
	"RequestAdminFailMission",
	"RequestAdminAwardResources",
	"RequestAdminAddEnemyResources",
	"RequestAdminNewCampaignReset",
	"RequestAdminSetMembership",
	"RequestAdminSetAdminRole",
	"RequestAdminInspectZone"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCoordinatorEntry)) {
		throw "Missing coordinator dev/framework entry point: $requiredCoordinatorEntry"
	}
}
Write-Host "Antistasi framework service spine OK"

foreach ($requiredCommandMenuEntry in @(
	'ScriptComponentClass',
	'COMMAND_MENU_CUSTOM_ACTION = "HST_CommandMenu"',
	'MENU_INPUT_CONTEXT = "InGameMenuContext"',
	'INPUT_CONFIG = "Configs/HST/Input/HST_Input.conf"',
	'MENU_LAYOUT = "UI/layouts/HST_CommandMenu.layout"',
	'AddActionListener',
	'RemoveActionListener',
	'SetCustomConfigs',
	'EnsureInputConfig',
	'OnCustomCommandMenuInput(float value, EActionTrigger reason)',
	'keyboard:KC_I',
	'IsLocalOwner',
	'local player menu component ready',
	'input registered',
	'snapshot received',
	'CreateWidgetInWorkspace',
	'FrameWidgetTypeID',
	'CanvasWidgetTypeID',
	'PolygonDrawCommand',
	'm_aCanvasCommandSets',
	'SetDrawCommands',
	'ActivateContext(MENU_INPUT_CONTEXT)',
	'ActivateAction(COMMAND_MENU_CUSTOM_ACTION)',
	'GetActionTriggered(COMMAND_MENU_CUSTOM_ACTION)',
	'Debug.KeyState(KeyCode.KC_I)',
	'Debug.ClearKey(KeyCode.KC_I)',
	'keyState != 0',
	'custom action listener fired',
	'raw KC_I edge seen',
	'ignored duplicate toggle',
	'm_fCommandMenuDebounceRemaining',
	'm_bCommandMenuKeyDownLastFrame',
	'm_bRawIKeyDownLastFrame',
	'PollCommandMenuInput',
	'PollRawCommandMenuKey',
	'TryToggleCommandMenu',
	'MENU_FONT',
	'ApplyTextStyle',
	'SetFont',
	'SetLineSpacing',
	'SetOutline',
	'SetShadow',
	'SetTextWrapping(false)',
	'FrameSlot.SetPos',
	'WidgetFlags.VISIBLE',
	'if (i >= 4)',
	'ShortenText(m_aRowValues[i], 42)',
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
	'ResolveLocalPlayerId',
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
	'inspect_persistence',
	'inspect_mission_runtime',
	'inspect_arsenal',
	'setup_hideout',
	'loot_nearby',
	'vehicle_collect_loot',
	'vehicle_unload_loot',
	'inspect_vehicle_cargo',
	'withdraw_arsenal',
	'garage_capture_nearby',
	'garage_redeploy',
	'TAB_GARAGE',
	'AppendGarageSections',
	'Redeploy: ',
	'move_hq',
	'move_hq_here',
	'recruit_zone',
	'mission_zone',
	'support_qrf',
	'support_fire',
	'support_gbu',
	'support_umpk',
	'cancel_support',
	'capture_zone',
	'award_small',
	'new_campaign',
	'member_accept',
	'member_remove',
	'admin_grant'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCommandMenuEntry)) {
		throw "Missing I-key alpha command menu contract entry: $requiredCommandMenuEntry"
	}
}
if ($scriptText -match "\bCreateWidgets\b") {
	throw "Command menu must remain procedural until HST_CommandMenu.layout is indexed with verified resource metadata"
}
if ($scriptText -match "CreateWidgetInWorkspace\(WidgetType\.CanvasWidgetTypeID") {
	throw "Command menu root must be a child-capable frame/layout container, not a canvas widget"
}
if ($scriptText -match "\bPanelWidgetTypeID\b") {
	throw "Command menu visible fills must use canvas draw commands, not panel widget fills"
}
if ($scriptText -match "\bWidgetFlags\.WRAP_TEXT\b") {
	throw "Command menu fixed-height text must avoid automatic wrapping; shorten or clip instead"
}
$rectFactoryMatch = [regex]::Match($scriptText, "protected Widget CreateRectWidget[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool SetupCanvasRect")
if (!$rectFactoryMatch.Success -or $rectFactoryMatch.Value -notmatch "WidgetType\.CanvasWidgetTypeID") {
	throw "Command menu rectangle factory must use CanvasWidgetTypeID"
}
if ($rectFactoryMatch.Value -match "FrameWidgetTypeID|PanelWidgetTypeID") {
	throw "Command menu rectangle factory must not use frame/panel widgets for visible fills"
}
$contextCommandMatch = [regex]::Match($scriptText, "void RunCommandFromContext[\s\S]*?\r?\n\t}\r?\n\r?\n\tvoid OnServerSnapshot")
if (!$contextCommandMatch.Success) {
	throw "Missing command menu context command path"
}
if ($contextCommandMatch.Value -match "\bOpenMenu\(") {
	throw "Context command path must execute quick actions without opening the command menu"
}
$registerInputMatch = [regex]::Match($scriptText, "protected bool RegisterInputListeners[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void UnregisterInputListeners")
if (!$registerInputMatch.Success) {
	throw "Missing command menu input registration path"
}
if ($registerInputMatch.Value -match "if\s*\(!EnsureIKeyBinding\(inputManager\)\)") {
	throw "Input registration must not block on the custom command menu binding"
}
foreach ($removedMenuInputContract in @(
	'COMMAND_MENU_ACTION = "Inventory"',
	'RegisterExistingIKeyActionListeners',
	'ActionUsesIKey',
	'OnInventoryCommandMenuInput',
	'OnIKeyAliasInput',
	'ActivateAction(COMMAND_MENU_ACTION)',
	'GetActionTriggered(COMMAND_MENU_ACTION)',
	'KEY_PRESSED_MASK',
	'inventory fallback listener fired',
	'I-key alias listener fired'
)) {
	if ($scriptText -match [regex]::Escape($removedMenuInputContract)) {
		throw "Command menu must use custom HST_CommandMenu plus raw KC_I, not the old Inventory/alias fallback: $removedMenuInputContract"
	}
}
$appendTopStatsMatch = [regex]::Match($scriptText, "protected string AppendTopStats[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected string AppendTabSections")
if (!$appendTopStatsMatch.Success) {
	throw "Missing command menu top stat payload builder"
}
foreach ($requiredTopStat in @(
	'AppendStat(payload, "FIA Money"',
	'AppendStat(payload, "HR"',
	'AppendStat(payload, "War Level"',
	'AppendStat(payload, "Training"'
)) {
	if ($appendTopStatsMatch.Value -notmatch [regex]::Escape($requiredTopStat)) {
		throw "Command menu top stat strip is missing reduced numeric badge: $requiredTopStat"
	}
}
foreach ($removedTopStat in @(
	'AppendStat(payload, "Commander"',
	'AppendStat(payload, "Petros"',
	'AppendStat(payload, "Zones"',
	'AppendStat(payload, "Arsenal"',
	'AppendStat(payload, "Support"',
	'AppendStat(payload, "Civilians"'
)) {
	if ($appendTopStatsMatch.Value -match [regex]::Escape($removedTopStat)) {
		throw "Command menu top stat strip must not include long string stat cards: $removedTopStat"
	}
}
Write-Host "I-key alpha command menu OK"

if (!(Test-Path "Configs/HST/Input/HST_Input.conf")) {
	throw "Missing command menu input config"
}

$commandMenuInputText = Get-Content -Raw "Configs/HST/Input/HST_Input.conf"
foreach ($requiredCommandMenuInputEntry in @(
	"ActionManager",
	"Action HST_CommandMenu",
	"InputSourceSum",
	"InputSourceValue",
	'FilterPreset "down"',
	'Input "keyboard:KC_I"'
)) {
	if ($commandMenuInputText -notmatch [regex]::Escape($requiredCommandMenuInputEntry)) {
		throw "Command menu input config is missing entry: $requiredCommandMenuInputEntry"
	}
}
Write-Host "Command menu input config OK"

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
	"vehicleLootEnabled",
	"vehicleLootRadiusMeters",
	"vehicleLootOnlyLockedItems",
	"vehicleLootRemoveSourceItems",
	"vehicleLootMaxItemsPerAction",
	"airSupportEnabled",
	"airSupportCooldownSeconds",
	"civilianPopulationEnabled",
	"civilianMaxActivePerTown",
	"civilianVehicleMinPerTown",
	"civilianVehicleMaxPerTown",
	"occupierVehicleMinPerTown",
	"occupierVehicleMaxPerTown",
	"MigrateSettings",
	"ApplyTo"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSettingsEntry)) {
		throw "Missing runtime settings generated-config contract entry: $requiredSettingsEntry"
	}
}
if ($scriptText -notmatch "SCHEMA_VERSION = 5") {
	throw "Runtime settings schema must be bumped to 5 for the town vehicle range settings migration"
}
if ($scriptText -notmatch "m_iArsenalUnlockThreshold = 25") {
	throw "Runtime/balance defaults must set arsenal unlock threshold to 25"
}
if ($configResourceText -notmatch "m_iArsenalUnlockThreshold 25") {
	throw "Balance config must set arsenal unlock threshold to 25"
}
if ($scriptText -match '"arsenalUnlockThreshold": 15' -or $configResourceText -match "m_iArsenalUnlockThreshold 15") {
	throw "Generated/default balance settings must not keep the old 15-item infinite unlock threshold"
}
foreach ($requiredCivilianDefault in @(
	"m_iCivilianMaxActivePerTown = 12",
	"m_iCivilianVehicleMinPerTown = 1",
	"m_iCivilianVehicleMaxPerTown = 5",
	"m_iOccupierVehicleMinPerTown",
	"m_iOccupierVehicleMaxPerTown = 2"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCivilianDefault)) {
		throw "Runtime settings must keep the updated civilian default: $requiredCivilianDefault"
	}
}
foreach ($requiredCivilianBalanceDefault in @(
	"m_iCivilianMaxActivePerTown 12",
	"m_iCivilianVehicleMinPerTown 1",
	"m_iCivilianVehicleMaxPerTown 5",
	"m_iOccupierVehicleMinPerTown 0",
	"m_iOccupierVehicleMaxPerTown 2"
)) {
	if ($configResourceText -notmatch [regex]::Escape($requiredCivilianBalanceDefault)) {
		throw "Balance config must keep the updated civilian default: $requiredCivilianBalanceDefault"
	}
}
Write-Host "Runtime settings generated-config contract OK"

foreach ($requiredLootEntry in @(
	"HST_LootService",
	"HST_LootResult",
	"HST_DisplayNameService",
	"LootNearbyToArsenal",
	"CollectNearbyLootToVehicle",
	"CollectLooseItemToArsenal",
	"CollectLooseItemToVehicle",
	"IsEligibleLooseLootEntity",
	"RemoveLooseLootItem",
	"ResolveItemDisplayName",
	"ResolveVehicleDisplayName",
	"UnloadNearestVehicleCargoToArsenal",
	"vehicleRuntimeId",
	"FindLootVehicleByRuntimeId",
	"IsPlayerAtVehicleRear",
	"target vehicle not nearby or invalid",
	"stand near the rear/load area",
	"BuildVehicleCargoReport",
	"DepositVehicleCargo",
	"CaptureNearbyVehicleToGarage",
	"IsEligibleVehicleRoot",
	"IsLikelyVehicleRootPrefab",
	"IsRejectedVehicleRootPrefab",
	"IsVehiclePartEntity",
	"IsVehiclePartName",
	"no safe root vehicle nearby",
	"nearest candidate was not a top-level vehicle",
	"RedeployGarageVehicle",
	"SelectGarageVehicle",
	"DistanceSq2D",
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
	"m_bVehicleLootEnabled",
	"m_iVehicleLootRadiusMeters",
	"m_bVehicleLootOnlyLockedItems",
	"m_bVehicleLootRemoveSource",
	"m_iVehicleLootMaxItemsPerAction",
	"HST_HQArsenalOpenAction",
	"HST_HQArsenalLootNearbyAction",
	"HST_VehicleCollectLootAction",
	"HST_VehicleUnloadLootAction",
	"Load loot to vehicle",
	"Unload vehicle loot to arsenal"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredLootEntry)) {
		throw "Missing loot-to-arsenal contract entry: $requiredLootEntry"
	}
}
Write-Host "Loot-to-arsenal contract OK"

$lootServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_LootService.c"
foreach ($requiredVehicleRejectionEntry in @(
	'prefab.Contains("Prefabs/Vehicles/")',
	'prefab.Contains("Supply")',
	'prefab.Contains("Crate")',
	'prefab.Contains("Cache")',
	'prefab.Contains("Box")',
	'prefab.Contains("Cargo")',
	'prefab.Contains("Container")',
	'prefab.Contains("Arsenal")'
)) {
	if ($lootServiceText -notmatch [regex]::Escape($requiredVehicleRejectionEntry)) {
		throw "Loot service must keep strict vehicle-root rejection entry: $requiredVehicleRejectionEntry"
	}
}
if ($lootServiceText -match 'if \(!prefab\.Contains\("Vehicles"\) && !prefab\.Contains\("Vehicle"\)\)') {
	throw "Loot service must not use loose Vehicle/Vehicles substring matching for vehicle roots"
}
Write-Host "Strict vehicle-root eligibility OK"

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
	'ResolveDryPlayerSpawnPosition',
	'TryResolveGroundPosition',
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
	"m_sRouteId",
	"m_vSourcePosition",
	"m_vTargetPosition",
	"m_sRuntimeEntityId",
	"m_sRuntimeStatus",
	"m_iSurvivorInfantryCount",
	"m_iSurvivorVehicleCount",
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

foreach ($requiredCivilianRuntimeEntry in @(
	"UpdatePhysicalTownPopulation",
	"SpawnTownPopulation",
	"CIVILIAN_GROUP_SIZE",
	"CIVILIAN_GROUP_PREFAB",
	"HST_CivilianTownGroup.et",
	"ENTERABLE_AMBIENT_VEHICLE_PREFAB",
	"ResolveTownVehicleSpawnPosition",
	"BuildTownSpawnSeed",
	"IsTownGroundVehicleResource",
	"IsAircraftVehicleResource",
	"CleanupZoneRuntimeEntities",
	"CleanupAllRuntimeEntities",
	"ShouldDetachFromTownCleanup",
	"m_aRuntimeEntityKinds",
	"m_aRuntimeEntitySpawnPositions",
	"IsGuidQualifiedResource",
	'SetAffiliatedFactionByKey("CIV")',
	"m_bCivilianPopulationEnabled",
	"m_iCivilianMaxActivePerTown",
	"m_iCivilianVehicleMinPerTown",
	"m_iCivilianVehicleMaxPerTown",
	"m_iOccupierVehicleMinPerTown",
	"m_iOccupierVehicleMaxPerTown",
	"m_aVehiclePrefabs",
	"civilianRuntimeChanged"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCivilianRuntimeEntry) -and $configResourceText -notmatch [regex]::Escape($requiredCivilianRuntimeEntry)) {
		throw "Missing physical civilian town runtime entry: $requiredCivilianRuntimeEntry"
	}
}
$civilianRuntimeServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CivilianService.c"
if ($civilianRuntimeServiceText -match 'DoSpawn\(SelectCivilianCharacterPrefab' -or $civilianRuntimeServiceText -match 'CIVILIAN_PREFAB = "Prefabs/Characters/Factions/CIV') {
	throw "Civilian runtime must spawn h-istasi civilian AI groups, not direct path-only civilian characters"
}
if ($civilianRuntimeServiceText -match 'ResolveTownSpawnPosition\(zone, civilianCount \+ j' -or $civilianRuntimeServiceText -match 'ResolveTownSpawnPosition\(zone, civilianCount \+ civilianVehicleCount \+ k') {
	throw "Civilian runtime vehicles must use scattered town vehicle placement, not the old grid helper"
}
foreach ($requiredTownVehicleEntry in @(
	'ResolveTownVehicleSpawnPosition(zone, j, false)',
	'ResolveTownVehicleSpawnPosition(zone, civilianVehicleCount + k, true)',
	'IsAircraftVehicleResource',
	'!IsAircraftVehicleResource(prefab)',
	'no GUID-qualified non-aircraft faction vehicle available'
)) {
	if ($civilianRuntimeServiceText -notmatch [regex]::Escape($requiredTownVehicleEntry)) {
		throw "Civilian runtime must keep scattered/non-heli vehicle contract entry: $requiredTownVehicleEntry"
	}
}
foreach ($forbiddenCivilianSpawnResource in @(
	"Prefabs/Vehicles/Wheeled/S105/S105_base.et",
	"Prefabs/Vehicles/Wheeled/S1203/S1203_base.et",
	"Prefabs/Vehicles/Wheeled/M151A2/M151A2.et"
)) {
	if ($civilianRuntimeServiceText -match [regex]::Escape($forbiddenCivilianSpawnResource)) {
		throw "Civilian runtime must not directly DoSpawn path-only vehicle resource: $forbiddenCivilianSpawnResource"
	}
}
Write-Host "Physical civilian town runtime OK"

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
