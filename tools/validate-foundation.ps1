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

function Get-BraceCountsOutsideStrings {
	param([string] $Text)

	$open = 0
	$close = 0
	$inString = $false
	$escaped = $false
	for ($i = 0; $i -lt $Text.Length; $i++) {
		$char = $Text[$i]
		if ($inString) {
			if ($escaped) {
				$escaped = $false
			} elseif ($char -eq "\") {
				$escaped = $true
			} elseif ($char -eq '"') {
				$inString = $false
			}
			continue
		}

		if ($char -eq '"') {
			$inString = $true
			continue
		}

		if ($char -eq "{") {
			$open++
		} elseif ($char -eq "}") {
			$close++
		}
	}

	return [pscustomobject]@{
		Open = $open
		Close = $close
	}
}

$project = Get-Content -Raw "addon.gproj"
foreach ($requiredProjectDependency in @(
	'"58D0FB3206B6F859"'
)) {
	if ($project -notmatch [regex]::Escape($requiredProjectDependency)) {
		throw "addon.gproj is missing required dependency: $requiredProjectDependency"
	}
}

$allowedProjectDependencies = @(
	"58D0FB3206B6F859"
)
if ($project -notmatch 'Dependencies\s*\{([\s\S]*?)\}') {
	throw "addon.gproj is missing a Dependencies block"
}
$projectDependenciesBlock = $Matches[1]
foreach ($projectDependencyMatch in [regex]::Matches($projectDependenciesBlock, '"([0-9A-F]{16})"')) {
	$dependencyGuid = $projectDependencyMatch.Groups[1].Value
	if ($dependencyGuid -notin $allowedProjectDependencies) {
		throw "addon.gproj has non-base-game dependency GUID: $dependencyGuid"
	}
}
Write-Host "Project dependencies OK"

$files = Get-ChildItem -Recurse -File -Include *.c,*.conf,*.ent,*.et,*.meta,*.layer,*.layout,*.gproj,*.md,.gitignore |
	Where-Object { $_.FullName -notlike "*\.git\*" }

foreach ($file in $files) {
	$text = Get-Content -Raw $file.FullName
	$braceCounts = Get-BraceCountsOutsideStrings $text
	if ($braceCounts.Open -ne $braceCounts.Close) {
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
	'm_sFactionKey "USSR"',
	'FactionKey "US"',
	'FactionKey "USSR"',
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

	if ($text -match "Loadout_US_") {
		throw "Runtime layer must not expose US role-selection loadouts in the primary FIA deploy path: $runtimeLayer"
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

	$legacyMarkerAreaName = "HST_" + "Ton" + "kaMapMarkerArea"
	if ($text -match $legacyMarkerAreaName -or $text -match "HST_DevMapMarkerArea" -or $text -match "HST_CallsignMarker_" -or $text -match "GenericEntity\s+HST_MapMarker_") {
		throw "Runtime layer must not keep stale Scenario Framework fallback map markers: $runtimeLayer"
	}

	if ($text -match "SCR_MapMarkerDotCircle\s+HST_NativeMapMarker_") {
		throw "Runtime layer must not use red dot-circle native markers for h-istasi map locations: $runtimeLayer"
	}

	foreach ($forbiddenAddonFactionEntry in @(
		'Configs/Factions/R',
		'Configs/EntityCatalog/U',
		'm_aEntityCatalogs'
	)) {
		if ($text -match [regex]::Escape($forbiddenAddonFactionEntry)) {
			throw "Runtime layer must not retain external faction/entity catalog entries in ${runtimeLayer}: $forbiddenAddonFactionEntry"
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
	if ($text -match '"faction affiliation" "US"') {
		throw "Starting point layer must not expose US as the primary playable deploy affiliation: $startingPointLayer"
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
	"ResolvePrimaryArsenalPosition",
	"ARSENAL_POSITION_TOLERANCE_METERS",
	"ResolveArsenalReadinessFailure",
	"RebuildRuntimeObjects",
	"m_sHQArsenalRuntimeStatus",
	"m_sLastHQArsenalFailure",
	"state.m_sPetrosPrefab = PETROS_PREFAB",
	"state.m_sArsenalPrefab = ARSENAL_PREFAB",
	"MoveHQToPosition",
	"ClearRuntimeObjects",
	"ResolveRuntimeObjectGroundPosition",
	"TryResolveGroundPosition",
	"SCR_EntityHelper.DeleteEntityAndChildren",
	"failed to spawn; using base FIA fallback",
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
foreach ($forbiddenRuntimeArsenalResource in @(
	"ArsenalBox_FIA_Weapons.et",
	"ArsenalBox_FIA_Equipment.et",
	"ArsenalBox_FIA.et",
	"SCR_Arsenal",
	"MSAR"
)) {
	if ($hqServiceText -match [regex]::Escape($forbiddenRuntimeArsenalResource)) {
		throw "HQ service must not spawn stock arsenal/MSAR resources directly: $forbiddenRuntimeArsenalResource"
	}
}
foreach ($removedArsenalRecoveryEntry in @(
	"ARSENAL_FALLBACK_PREFAB",
	"ARSENAL_VISIBLE_LIFT_METERS",
	"m_bArsenalNeedsDelayedVerification",
	"VerifyDelayedArsenalEntity",
	"ResolveArsenalSpawnPosition",
	"HST_HQArsenalFallback.et"
)) {
	if ($hqServiceText -match [regex]::Escape($removedArsenalRecoveryEntry)) {
		throw "HQ service must not keep obsolete fallback/delayed arsenal recovery entry: $removedArsenalRecoveryEntry"
	}
}
if ($hqServiceText -match "using FIA supply-cache fallback") {
	throw "HQ service must not use a supply-cache fallback for the HQ arsenal"
}
foreach ($forbiddenArsenalReadinessGate in @(
	"missing RplComponent",
	"missing ActionsManagerComponent"
)) {
	if ($hqServiceText -match [regex]::Escape($forbiddenArsenalReadinessGate)) {
		throw "HQ arsenal readiness must not reject visible arsenal visuals for wrapper-only component state: $forbiddenArsenalReadinessGate"
	}
}
if ($hqServiceText -notmatch 'm_bHQRuntimeObjectsSpawned && AreRuntimeObjectsTracked\(\) && IsUsableArsenalEntity\(m_ArsenalEntity\)') {
	throw "HQ runtime must not skip arsenal usability just because the entity exists"
}
if ($coordinatorText -notmatch "RequestCommanderRebuildHQAssets" -or $coordinatorText -notmatch "ResolveHQRebuildPlacement") {
	throw "Coordinator must expose a Build Mode guarded HQ runtime rebuild action"
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
	'{39568880CC1F9CED}Prefabs/Props/Military/Arsenal/ArsenalBoxes/FIA/ArsenalBox_FIA.et',
	"ActionsManagerComponent",
	'ActionsManagerComponent "{56F2C6D1431ADB12}"',
	"HST_HQArsenalActionFilterComponent",
	"HST_HQArsenalLoadoutEditorAction",
	"Open Loadout Editor"
)) {
	if ($hqArsenalPrefabText -notmatch [regex]::Escape($requiredArsenalPrefabEntry)) {
		throw "HST HQ arsenal prefab is missing h-istasi-only arsenal entry: $requiredArsenalPrefabEntry"
	}
}
$petrosActionText = Get-Content -Raw "Scripts/Game/HST/Components/HST_PetrosUserActions.c"
foreach ($requiredArsenalActionFilterEntry in @(
	"HST_HQArsenalActionFilterComponent",
	"GetActionsList",
	"SetActionEnabled_S(false)",
	"HST_HQArsenalLoadoutEditorAction.Cast(action)",
	"filtered %1 inherited action"
)) {
	if ($petrosActionText -notmatch [regex]::Escape($requiredArsenalActionFilterEntry)) {
		throw "HST HQ arsenal action filter is missing inherited action cleanup entry: $requiredArsenalActionFilterEntry"
	}
}
if ($hqArsenalPrefabText -match 'ActionsManagerComponent "\{6985327711303401\}"') {
	throw "HST HQ arsenal prefab must override the inherited ArsenalBox action manager, not add a duplicate HST action manager"
}
if ($hqArsenalPrefabText -match "RplComponent") {
	throw "HST HQ arsenal prefab must not add a duplicate RplComponent; the visible FIA arsenal base supplies replication"
}
foreach ($forbiddenArsenalPrefabEntry in @(
	'{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et',
	'{B53B98CEA2D72735}Prefabs/Props/Military/Compositions/FIA/ArsenalBox_FIA.et',
	"HST_HQArsenalOpenAction",
	"HST_HQArsenalLootNearbyAction",
	"Open h-istasi Arsenal",
	"Loot nearby to h-istasi Arsenal",
	"SupplyCache_S_FIA_01.et",
	"Prefabs/Compositions/Slotted/SlotFlatSmall",
	"SupplyDrop/Parts",
	"ArsenalBox_FIA_Weapons.et",
	"ArsenalBox_FIA_Equipment.et",
	"SCR_Arsenal",
	"MSAR",
	"Ural4320_arsenal_box_tan.et"
)) {
	if ($hqArsenalPrefabText -match [regex]::Escape($forbiddenArsenalPrefabEntry)) {
		throw "HST HQ arsenal prefab must not use stock arsenal/MSAR/fake supply-cache entry: $forbiddenArsenalPrefabEntry"
	}
}
Write-Host "HST HQ arsenal custom action-surface contract OK"

$hqArsenalFallbackPrefabPath = "Prefabs/Objects/HST/HST_HQArsenalFallback.et"
if (Test-Path $hqArsenalFallbackPrefabPath) {
	throw "Remove obsolete HST HQ fallback arsenal prefab: $hqArsenalFallbackPrefabPath"
}

$hqArsenalFallbackPrefabMetaPath = "$hqArsenalFallbackPrefabPath.meta"
if (Test-Path $hqArsenalFallbackPrefabMetaPath) {
	throw "Remove obsolete HST HQ fallback arsenal prefab metadata: $hqArsenalFallbackPrefabMetaPath"
}
Write-Host "Obsolete HST HQ fallback arsenal prefab removed"

$balanceConfigTextEarly = Get-Content -Raw "Configs/HST/Balance/HST_CE311_Balance.conf"
$defaultCatalogEarly = Get-Content -Raw "Scripts/Game/HST/Config/HST_DefaultCatalog.c"
if ($balanceConfigTextEarly -match "m_aCivilianGroupPrefabs" -or $defaultCatalogEarly -match "m_aCivilianGroupPrefabs") {
	throw "Civilian runtime config must not use the broken SCR_AIGroup civilian ambience pool"
}
if ($balanceConfigTextEarly -notmatch "m_aCivilianCharacterPrefabs" -or $defaultCatalogEarly -notmatch "m_aCivilianCharacterPrefabs") {
	throw "Civilian runtime config must expose the direct civilian character prefab pool"
}
if ($balanceConfigTextEarly -match '"Prefabs/Characters/Factions/CIV/Character_CIV' -or $defaultCatalogEarly -match '"Prefabs/Characters/Factions/CIV/Character_CIV') {
	throw "Civilian character pools must not contain path-only Character_CIV resources"
}
$civilianCharacterPoolEntriesEarly = [regex]::Matches($balanceConfigTextEarly + "`n" + $defaultCatalogEarly, '"\{[0-9A-F]{16}\}Prefabs/Characters/Factions/CIV/[^"]+Character_CIV_[^"]+\.et"')
if ($civilianCharacterPoolEntriesEarly.Count -lt 6) {
	throw "Civilian character pools must contain at least 6 GUID-qualified stock CIV character resources"
}
Write-Host "Civilian character runtime resource surface OK"

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

if ($defaultCatalog -notmatch 'preset\.m_sResistanceFactionKey = "FIA"' -or $defaultCatalog -notmatch 'preset\.m_sOccupierFactionKey = "US"' -or $defaultCatalog -notmatch 'preset\.m_sInvaderFactionKey = "USSR"') {
	throw "Campaign catalog must keep FIA as resistance, US as occupier, and USSR as invader"
}
Write-Host "Campaign faction identity OK"

$campaignStateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
if ($campaignStateText -notmatch "SCHEMA_VERSION\s*=\s*(\d+)") {
	throw "Unable to parse HST_CampaignState.SCHEMA_VERSION"
}
$campaignSchemaVersion = [int] $Matches[1]
$migrationsPath = "docs/MIGRATIONS.md"
if (!(Test-Path $migrationsPath)) {
	throw "Missing campaign migration notes: $migrationsPath"
}
$migrationsText = Get-Content -Raw $migrationsPath
if ($migrationsText -notmatch "(?im)^##\s+Schema\s+$campaignSchemaVersion\b") {
	throw "docs/MIGRATIONS.md must document HST_CampaignState schema $campaignSchemaVersion"
}
Write-Host "Campaign save migration note OK: schema $campaignSchemaVersion"

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
	"BuildRuntimeReport",
	"TrySpawnMissionRuntimeProp",
	"EnsureMissionRuntimeProp",
	"HST_MissionProp_HVT.et",
	"HST_MissionProp_DestroyTarget.et",
	"HST_MissionProp_Cargo.et",
	"HST_MissionProp_Captives.et",
	"HST_MissionProp_HoldMarker.et"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredMissionPrimitive)) {
		throw "Mission runtime service is missing physical primitive contract: $requiredMissionPrimitive"
	}
}
foreach ($requiredMissionPropPath in @(
	"Prefabs/Objects/HST/HST_MissionProp_HVT.et",
	"Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et",
	"Prefabs/Objects/HST/HST_MissionProp_Cargo.et",
	"Prefabs/Objects/HST/HST_MissionProp_Captives.et",
	"Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et"
)) {
	if (!(Test-Path $requiredMissionPropPath)) {
		throw "Missing mission runtime prop prefab: $requiredMissionPropPath"
	}
	if (!(Test-Path "$requiredMissionPropPath.meta")) {
		throw "Missing mission runtime prop prefab metadata: $requiredMissionPropPath.meta"
	}
}
foreach ($missionPropContract in @(
	@("Prefabs/Objects/HST/HST_MissionProp_HVT.et", "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et"),
	@("Prefabs/Objects/HST/HST_MissionProp_Captives.et", "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et"),
	@("Prefabs/Objects/HST/HST_MissionProp_Cargo.et", "{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et"),
	@("Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et", "{7E2380494811A5FB}Prefabs/Structures/Infrastructure/Towers/TransmitterTower_01/TransmitterTower_01_medium.et"),
	@("Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et", "{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et")
)) {
	$missionPropText = Get-Content -Raw $missionPropContract[0]
	if ($missionPropText -notmatch [regex]::Escape($missionPropContract[1])) {
		throw "Mission runtime prop must inherit a visible GUID-qualified parent: $($missionPropContract[0]) -> $($missionPropContract[1])"
	}

	foreach ($blankMissionPropParent in @(
		"Prefabs/Props/Military/CISS/SupplyDrop/Parts",
		"EquipmentBox_US.et",
		"SupplyCache_S_FIA_01.et",
		"AmmoBoxArsenal_Weapons_FIA.et",
		"CampaignRadioBox.et"
	)) {
		if ($missionPropText -match [regex]::Escape($blankMissionPropParent)) {
			throw "Mission runtime prop must not inherit blank/part/problem prefab $blankMissionPropParent in $($missionPropContract[0])"
		}
	}
}
Write-Host "Mission runtime primitive coverage OK"

$missionConfigResourceText = (Get-ChildItem -Recurse -File "Configs" -Include *.conf |
	ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
foreach ($requiredMissionExpansionEntry in @(
	"m_sBriefingText",
	"m_sRequirementText",
	"m_sFailureText",
	"m_sRuntimeType",
	"m_sTargetSitePreference",
	"m_sRewardText",
	"m_sFailurePenaltyText",
	"m_iCargoCount",
	"m_iCaptiveCount",
	"m_iVehicleCount",
	"m_iRequiredWarLevel",
	"m_aTargetZoneTypes",
	"ApplyMissionContract",
	"BriefingForMission",
	"RequirementForMission",
	"RewardTextForMission"
)) {
	if ($defaultCatalog -notmatch [regex]::Escape($requiredMissionExpansionEntry) -and $missionConfigResourceText -notmatch [regex]::Escape($requiredMissionExpansionEntry)) {
		throw "Mission definitions are missing expanded Antistasi metadata contract: $requiredMissionExpansionEntry"
	}
}
foreach ($requiredExpandedMissionId in @(
	"conquest_town",
	"conquest_factory",
	"conquest_airfield",
	"conquest_seaport",
	"destroy_outpost_cache",
	"destroy_factory_asset",
	"destroy_airfield_asset",
	"destroy_seaport_asset",
	"logistics_resource_cache",
	"logistics_factory_supplies",
	"logistics_airfield_intel",
	"logistics_seaport_supplies",
	"logistics_support_cache"
)) {
	if ($runtimeMissions -notcontains $requiredExpandedMissionId -or $configMissions -notcontains $requiredExpandedMissionId) {
		throw "Expanded mission registry is missing target-aware mission: $requiredExpandedMissionId"
	}
}
$missionExpansionScriptText = (Get-ChildItem -Recurse -File "Scripts/Game/HST" -Include *.c |
	ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
foreach ($requiredMissionStateMachineEntry in @(
	"HST_MISSION_RUNTIME_STATE_MACHINE",
	"HST_MissionRuntimeEntityState",
	"m_aMissionRuntimeEntities",
	"FindFailedActiveMissionId",
	"AdvanceMissionStateMachine",
	"MarkRuntimeMissionFailed",
	"BuildRuntimeReportForMission",
	"CountMissionRuntimeEntities",
	"objective count",
	"complete objectives",
	"failed objectives",
	"mission asset count",
	"runtime entity count",
	"failure reason",
	"ReportTargetZone",
	"ReportSite",
	"missing:",
	"ResolveConvoyArrivalSeconds",
	"EnsureMissionHostileGroup",
	"RegisterRuntimeEntityState",
	"mission_guard"
)) {
	if ($missionExpansionScriptText -notmatch [regex]::Escape($requiredMissionStateMachineEntry)) {
		throw "Mission runtime is missing full Antistasi state-machine entry: $requiredMissionStateMachineEntry"
	}
}
$missionRequestBridgeText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CommandMenuRequestComponent.c"
$missionClientText = Get-Content -Raw "Scripts/Game/HST/Components/HST_MissionClientComponent.c"
$coordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$mapMarkerServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
foreach ($requiredMissionUiEntry in @(
	"HST_MISSION_INTEL|%1|%2",
	"MISSION|",
	"OBJECTIVE|%1|%2|%3|%4|%5|%6|%7",
	"RpcDo_ReceiveMissionEvent",
	"RpcDo_ReceiveMissionIntel",
	"BroadcastMissionEvent",
	"BroadcastMissionIntel",
	"HST_MissionClientComponent",
	"HandleDirectMissionMarkerClick",
	"OpenMissionDetailsForMarker",
	"mission_clickable"
)) {
	if ($missionRequestBridgeText -notmatch [regex]::Escape($requiredMissionUiEntry) -and $missionClientText -notmatch [regex]::Escape($requiredMissionUiEntry) -and $coordinatorText -notmatch [regex]::Escape($requiredMissionUiEntry) -and $mapMarkerServiceText -notmatch [regex]::Escape($requiredMissionUiEntry)) {
		throw "Mission UI/intel contract is missing entry: $requiredMissionUiEntry"
	}
}
$playerControllerPrefabText = Get-Content -Raw "Prefabs/Characters/HST/HST_PlayerController.et"
if ($playerControllerPrefabText -notmatch "HST_MissionClientComponent") {
	throw "HST player controller prefab is missing mission client notification/detail component"
}
Write-Host "Full Antistasi mission expansion contracts OK"

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
	throw "Everon campaign catalog must contain 79 zones in config/runtime, found config=$($configZones.Count) runtime=$($runtimeZones.Count)"
}

$campaignBaseBlocks = @($zoneBlocks | Where-Object { $_ -match 'm_sSourceLayerName "Bases\.layer"' })
$campaignDepotBlocks = @($zoneBlocks | Where-Object { $_ -match 'm_sSourceLayerName "SupplyDepots\.layer"' })
$campaignCallsigns = @($zoneBlocks | ForEach-Object {
	if ($_ -match 'm_sMarkerCallsign "([^"]+)"') {
		$Matches[1]
	}
} | Where-Object { ![string]::IsNullOrWhiteSpace($_) })
if ($campaignBaseBlocks.Count -ne 71) {
	throw "Expected 71 Everon Bases.layer nodes, found $($campaignBaseBlocks.Count)"
}
if ($campaignDepotBlocks.Count -ne 8) {
	throw "Expected 8 Everon SupplyDepots.layer nodes, found $($campaignDepotBlocks.Count)"
}
if ($campaignCallsigns.Count -ne 56) {
	throw "Expected 56 paired legacy callsigns, found $($campaignCallsigns.Count)"
}

foreach ($group in @($campaignCallsigns | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate legacy callsign: $($group.Name)"
}

$zoneDisplayNames = @($zoneBlocks | ForEach-Object {
	if ($_ -match 'm_sDisplayName "([^"]+)"') {
		$Matches[1]
	}
})
foreach ($group in @($zoneDisplayNames | Group-Object | Where-Object Count -gt 1)) {
	throw "Duplicate campaign zone display label: $($group.Name)"
}

foreach ($block in $zoneBlocks) {
	if ($block -notmatch 'm_sZoneId "([^"]+)"') {
		continue
	}

	$zoneId = $Matches[1]
	foreach ($requiredCampaignField in @("m_sSourceLayoutId", "m_sSourceLayerName", "m_sMarkerLabel", "m_sMarkerTextColor", "m_sMarkerStyle", "m_aLinkedZoneIds")) {
		if ($block -notmatch [regex]::Escape($requiredCampaignField)) {
			throw "campaign zone $zoneId is missing $requiredCampaignField"
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
		throw "Old rough alpha placeholder zone remains after campaign import: $oldPlaceholderId"
	}
}

foreach ($requiredNativeMarkerEntry in @(
	"SCR_MapMarkerManagerComponent",
	"Configs/Map/CampaignMapMarkerConfig.conf"
)) {
	if ($runtimeMarkerLayer -notmatch [regex]::Escape($requiredNativeMarkerEntry)) {
		throw "Missing campaign map marker scaffold entry: $requiredNativeMarkerEntry"
	}
}

$legacyMarkerAreaName = "HST_" + "Ton" + "kaMapMarkerArea"
if ($runtimeMarkerLayer -match $legacyMarkerAreaName -or $runtimeMarkerLayer -match "HST_CallsignMarker_" -or $runtimeMarkerLayer -match "GenericEntity\s+HST_MapMarker_") {
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
	"AddMissionConvoyVehicleMarkers",
	"mission.m_sRuntimePhase == ""convoy_moving""",
	"hst_mission_convoy_vehicle_",
	"Convoy vehicle %1 - %2: neutralize crew",
	"m_bRuntimeNative",
	"m_aRuntimeNativeMarkers",
	"m_bNativePublishPending",
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
		throw "Missing runtime campaign marker service entry: $requiredRuntimeMarkerEntry"
	}
}

foreach ($removedRuntimeMarkerEntry in @(
	"hst_zone_callsign_",
	"BuildCallsignMarkerPosition"
)) {
	if ($markerServiceText -match [regex]::Escape($removedRuntimeMarkerEntry)) {
		throw "Runtime map markers must use real place names; found obsolete callsign marker entry: $removedRuntimeMarkerEntry"
	}
}

foreach ($zoneId in $configZones) {
	if ($strategicZonesLayer -notmatch [regex]::Escape("HST_ZoneAnchor_$zoneId")) {
		throw "Missing strategic zone anchor for configured campaign node: $zoneId"
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
	throw "campaign marker layer must not include red dot-circle native marker overlays"
}

Write-Host "Everon campaign coverage OK: zones=$($configZones.Count) bases=$($campaignBaseBlocks.Count) depots=$($campaignDepotBlocks.Count) callsigns=$($campaignCallsigns.Count) towns=$($townZoneIds.Count)"

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

	foreach ($requiredCampaignRuntimeField in @("m_sDisplayName", "m_sResourceKind", "m_iCaptureRadiusMeters", "m_iPriority", "m_sCompositionId", "m_sSpawnProfileId")) {
		if ($block -notmatch [regex]::Escape($requiredCampaignRuntimeField)) {
			throw "campaign zone $zoneId is missing $requiredCampaignRuntimeField"
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
$commandMenuComponentText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CommandMenuComponent.c"
$commandUiText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
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
	throw "Map marker service must not publish question-mark icons for campaign campaign markers"
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
	"HST_HQArsenalLoadoutEditorAction",
	"HST_VehicleCollectLootAction",
	"HST_VehicleUnloadLootAction",
	"HST_LoadoutEditorComponent"
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
	"StrikeKindForSupport",
	"StrikeConfigForSupport",
	"ResolveStrikeSupport",
	"HasResistanceAirSupportCapability",
	"air_support_unlockable",
	"abstract_strike"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSupportStrikeEntry)) {
		throw "Missing base-game abstract support strike contract entry: $requiredSupportStrikeEntry"
	}
}
Write-Host "Base-game abstract support strike contract OK"

$externalOpforGroupCopies = @(Get-ChildItem -Path "Prefabs/Groups/OPFOR" -Directory -ErrorAction SilentlyContinue)
if ($externalOpforGroupCopies.Count -gt 0) {
	throw "External OPFOR group copies must not remain in the base-game preset"
}
Write-Host "External group copy cleanup OK"

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
	"RequestMemberManualCheckpointReport",
	"RequestMemberFoundationStatus",
	"BuildFoundationStatusReport",
	"h-istasi checkpoint | success",
	"h-istasi checkpoint | not available",
	"schema %2/%3",
	"active missions %1 | active groups %2",
	"RequestMemberInspectCampaign",
	"RequestMemberInspectMarkers",
	"RequestMemberInspectEconomy",
	"RequestMemberInspectZones",
	"RequestMemberInspectMissions",
	"RequestMemberInspectActiveMissions",
	"RequestMemberInspectMission",
	"RequestMemberInspectConvoyRuntime",
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
	'CountRowsForSection',
	'CONTENT_PAGE_SIZE',
	'ACTION_PAGE_SIZE',
	'CONTENT_PREV_WIDGET_ID',
	'CONTENT_NEXT_WIDGET_ID',
	'ACTION_PREV_WIDGET_ID',
	'ACTION_NEXT_WIDGET_ID',
	'ScrollContentPage',
	'ScrollActionPage',
	'RenderContentPager',
	'RenderActionPager',
	'CreateWrappedTextWidget',
	'BuildContentItemList',
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
	'foundation_status',
	'inspect_campaign',
	'inspect_markers',
	'inspect_economy',
	'inspect_zones',
	'inspect_missions',
	'inspect_active_missions',
	'inspect_mission',
	'inspect_convoy_runtime',
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
if ($commandMenuComponentText -match "\bCreateWidgets\b") {
	throw "Command menu must remain procedural until HST_CommandMenu.layout is indexed with verified resource metadata"
}
if ($commandMenuComponentText -match "CreateWidgetInWorkspace\(WidgetType\.CanvasWidgetTypeID") {
	throw "Command menu root must be a child-capable frame/layout container, not a canvas widget"
}
if ($commandMenuComponentText -match "\bPanelWidgetTypeID\b") {
	throw "Command menu visible fills must use canvas draw commands, not panel widget fills"
}
if ($commandMenuComponentText -match "\bWidgetFlags\.WRAP_TEXT\b") {
	throw "Command menu fixed-height text must avoid automatic wrapping; shorten or clip instead"
}
$rectFactoryMatch = [regex]::Match($commandMenuComponentText, "protected Widget CreateRectWidget[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool SetupCanvasRect")
if (!$rectFactoryMatch.Success -or $rectFactoryMatch.Value -notmatch "WidgetType\.CanvasWidgetTypeID") {
	throw "Command menu rectangle factory must use CanvasWidgetTypeID"
}
if ($rectFactoryMatch.Value -match "FrameWidgetTypeID|PanelWidgetTypeID") {
	throw "Command menu rectangle factory must not use frame/panel widgets for visible fills"
}
$contextCommandMatch = [regex]::Match($commandMenuComponentText, "void RunCommandFromContext[\s\S]*?\r?\n\t}\r?\n\r?\n\tvoid OnServerSnapshot")
if (!$contextCommandMatch.Success) {
	throw "Missing command menu context command path"
}
if ($contextCommandMatch.Value -match "\bOpenMenu\(") {
	throw "Context command path must execute quick actions without opening the command menu"
}
$registerInputMatch = [regex]::Match($commandMenuComponentText, "protected bool RegisterInputListeners[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void UnregisterInputListeners")
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
	if ($commandMenuComponentText -match [regex]::Escape($removedMenuInputContract)) {
		throw "Command menu must use custom HST_CommandMenu plus raw KC_I, not the old Inventory/alias fallback: $removedMenuInputContract"
	}
}
$appendTopStatsMatch = [regex]::Match($commandUiText, "protected string AppendTopStats[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected string AppendTabSections")
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
	"HST_RuntimeSettingsCapture",
	"startingFactionMoney",
	"startingHR",
	"captureProgressRequired",
	"captureProgressPerSecond",
	"captureDecayPerSecond",
	"captureAggressionBase",
	"captureCounterattackChancePercent",
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
	"hqInteractionRadiusMeters",
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
if ($scriptText -notmatch "SCHEMA_VERSION = 8") {
	throw "Runtime settings schema must be bumped to 8 for capture tuning migration"
}
if ($scriptText -notmatch "m_iArsenalUnlockThreshold = 25") {
	throw "Runtime/balance defaults must set arsenal unlock threshold to 25"
}
if ($scriptText -notmatch "m_iHQInteractionRadiusMeters = 50") {
	throw "Runtime/balance defaults must set HQ interaction radius to 50 meters"
}
if ($configResourceText -notmatch "m_iHQInteractionRadiusMeters 50") {
	throw "Balance config must set HQ interaction radius to 50 meters"
}
if ($configResourceText -notmatch "m_iArsenalUnlockThreshold 25") {
	throw "Balance config must set arsenal unlock threshold to 25"
}
foreach ($requiredCaptureDefault in @(
	"m_iCaptureProgressRequired 100",
	"m_iCaptureProgressPerSecond 2",
	"m_iCaptureDecayPerSecond 1",
	"m_iCaptureAggressionBase 10",
	"m_iCaptureCounterattackChancePercent 45"
)) {
	if ($configResourceText -notmatch $requiredCaptureDefault) {
		throw "Balance config must set capture default: $requiredCaptureDefault"
	}
}
if ($configResourceText -notmatch "m_bLootOnlyLockedItems 0" -or $configResourceText -notmatch "m_bVehicleLootOnlyLockedItems 0") {
	throw "Loot defaults must deposit all recovered gear instead of leaving repeated unlocked items on sources"
}
if ($scriptText -notmatch "settings.m_ArsenalLoot.m_bLootOnlyLockedItems = false" -or $scriptText -notmatch "settings.m_VehicleLoot.m_bOnlyLockedItems = false") {
	throw "Runtime settings migration must switch old profiles to deposit-all loot defaults"
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
	"HST_VehicleRootScanResult",
	"HST_RuntimeVehicleState",
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
	"stand near the vehicle load area",
	"BuildVehicleCargoReport",
	"DepositVehicleCargo",
	"CaptureNearbyVehicleToGarage",
	"FindNearestVehicleRoot",
	"PublishVehicleTargetDiagnostics",
	"ResolveVehicleRoot",
	"TrySelectRuntimeRecordVehicle",
	"ResolveRuntimeVehicleRootFromRecord",
	"FillVehicleScanResult",
	"VehiclePrefabsMatch",
	"ResolveVehicleRuntimeIdFromScan",
	"ResolveVehiclePrefabFromScan",
	"IsEligibleVehicleRoot",
	"HST_VehicleRootPolicy",
	"IsEligibleVehicleRootPrefab",
	"BuildVehicleRootRejectReason",
	"ResolveRuntimeVehicleRecord",
	"selected registered h-istasi runtime vehicle",
	"IsLikelyVehicleRootPrefab",
	"IsRejectedVehicleRootPrefab",
	"IsVehiclePartEntity",
	"IsVehiclePartName",
	"RekeyLegacyVehiclePartCargo",
	"CountVehicleCargoEntriesIncludingLegacy",
	"IsLegacyVehiclePartCargoNear",
	"IsVehicleRootStillPresent",
	"m_iLastVehicleTargetCandidates",
	"m_sLastVehicleTargetStatus",
	"m_sLastVehicleTargetReason",
	"m_sLastVehicleTargetPrefab",
	"m_iLastVehicleTargetCargoEntries",
	"no safe root vehicle nearby",
	"nearest candidate was not a top-level vehicle",
	"deleted verified root vehicle then stored garage record",
	"selected root still present after delete",
	"RedeployGarageVehicle",
	"HST_BuildModeService",
	"HST_BuildModePlacement",
	"ResolveGarageRedeployPlacement",
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
	"HST_VehicleCollectLootAction",
	"HST_VehicleUnloadLootAction",
	"Load loot to vehicle",
	"Unload vehicle loot to arsenal"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredLootEntry)) {
		throw "Missing loot-to-arsenal contract entry: $requiredLootEntry"
	}
}
foreach ($removedVehicleResolverEntry in @(
	"ResolveRegisteredRuntimeVehicleRootFromCandidate",
	"DoesCandidateChainHaveVehicleSignal",
	"runtime fallback rejected top-level scenery/prop near vehicle record"
)) {
	if ($scriptText -match [regex]::Escape($removedVehicleResolverEntry)) {
		throw "Vehicle targeting must use direct root scan plus record fallback, not the old candidate-based resolver: $removedVehicleResolverEntry"
	}
}
Write-Host "Loot-to-arsenal contract OK"

foreach ($requiredHQGateEntry in @(
	"IsPlayerWithinHQInteractionRadius",
	"BuildHQInteractionDenied",
	"m_iHQInteractionRadiusMeters",
	"RequestMemberLootNearby",
	"RequestMemberUnloadVehicleCargo",
	"RequestMemberCaptureNearbyVehicle",
	"RequestMemberRedeployGarageVehicle",
	"RequestMemberWithdrawBestArsenalItem"
)) {
	if ($coordinatorText -notmatch [regex]::Escape($requiredHQGateEntry) -and $scriptText -notmatch [regex]::Escape($requiredHQGateEntry)) {
		throw "Missing HQ-radius interaction gate contract entry: $requiredHQGateEntry"
	}
}
if ($coordinatorText -notmatch 'RequestMemberCollectVehicleLoot[\s\S]*?CollectNearbyLootToVehicle') {
	throw "Field vehicle loot loading must remain usable without the HQ-radius arsenal gate"
}
if (([regex]::Matches($coordinatorText, "IsPlayerWithinHQInteractionRadius\(playerId\)").Count) -lt 5) {
	throw "HQ-only arsenal and garage actions must all call the HQ-radius gate"
}
$buildModeText = Get-Content -Raw "Scripts/Game/HST/Services/HST_BuildModeService.c"
$commandUiText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
foreach ($requiredBuildModeEntry in @(
	"HST_BuildModeService",
	"HST_BuildModePlacement",
	"ResolveGarageRedeployPlacement",
	"ResolveHQRebuildPlacement",
	"BuildPlayerForwardOffset",
	"PublishPlacement",
	"m_sBuildModeStatus",
	"m_sLastBuildModeFailure",
	"m_vLastBuildModePosition",
	"m_fLastBuildModeYaw"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredBuildModeEntry)) {
		throw "Missing Build Mode v1 contract entry: $requiredBuildModeEntry"
	}
}
if ($coordinatorText -notmatch 'RequestMemberRedeployGarageVehicle[\s\S]*?ResolveGarageRedeployPlacement[\s\S]*?RedeployGarageVehicle') {
	throw "Garage redeploy must route through Build Mode placement before spawning"
}
if ($buildModeText -notmatch 'HST_VehicleRootPolicy\.IsEligibleVehicleRootPrefab') {
	throw "Build Mode garage placement must reject non-root stored vehicle prefabs"
}
Write-Host "HQ interaction radius gates OK"

if ($commandUiText -match 'TAB_ARSENAL[\s\S]*?Withdraw first available item') {
	throw "Normal Arsenal/Loot menu must not expose the placeholder withdraw-first item action"
}
foreach ($normalMenuDebugPattern in @(
	'AddMenuAction\(actions, TAB_FORCES, "Apply income tick"',
	'AddMenuAction\(actions, TAB_MISSIONS, "Progress active mission"',
	'AddMenuAction\(actions, TAB_GARAGE, "Manual checkpoint"',
	'AddMenuAction\(actions, TAB_ARSENAL, "Manual checkpoint"',
	'AddMenuAction\(actions, TAB_MEMBERS, "Manual checkpoint"',
	'AddMenuAction\(actions, TAB_PETROS, "Open Arsenal/Loot"'
)) {
	if ($commandUiText -match $normalMenuDebugPattern) {
		throw "Normal command menu still exposes debug/noise action matching: $normalMenuDebugPattern"
	}
}
foreach ($requiredMenuRecoveryEntry in @(
	"Garage/Build",
	"Build mode",
	"Build position",
	"Rebuild HQ assets",
	"rebuild_hq_assets",
	"Build redeploy",
	"Force income tick",
	"Force mission progress"
)) {
	if ($commandUiText -notmatch [regex]::Escape($requiredMenuRecoveryEntry)) {
		throw "Command menu is missing recovery/build-mode entry: $requiredMenuRecoveryEntry"
	}
}
foreach ($forbiddenNormalLoadoutMenuEntry in @(
	'AddMenuAction\(actions, TAB_ARSENAL, "Open Loadout Editor"',
	'AddMenuAction\(actions, TAB_ARSENAL, "Save current loadout draft"',
	'AddMenuAction\(actions, TAB_ARSENAL, "Apply saved loadout"',
	'AddMenuAction\(actions, TAB_ARSENAL, "Close Loadout Editor"',
	'AddMenuAction\(actions, TAB_ARSENAL, "Loadout editor report"',
	'AppendSection\(payload, "loadout_editor"'
)) {
	if ($commandUiText -match $forbiddenNormalLoadoutMenuEntry) {
		throw "Normal command menu must not expose loadout editor entry: $forbiddenNormalLoadoutMenuEntry"
	}
}
Write-Host "Command menu recovery/build cleanup OK"

$loadoutEditorText = Get-Content -Raw "Scripts/Game/HST/Services/HST_LoadoutEditorService.c"
$loadoutEditorComponentText = Get-Content -Raw "Scripts/Game/HST/Components/HST_LoadoutEditorComponent.c"
$displayNameServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_DisplayNameService.c"
$coordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$requestBridgeText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CommandMenuRequestComponent.c"
foreach ($requiredLoadoutEditorEntry in @(
	"HST_LoadoutEditorService",
	"HST_LoadoutEditorComponent",
	'$profile:h-istasi/loadouts',
	"HST_LoadoutSlotState",
	"HST_LoadoutNodeState",
	"HST_SavedLoadoutState",
	"HST_IssuedLoadoutItemState",
	"HST_LoadoutEditorSessionState",
	"m_aSavedLoadouts",
	"m_aIssuedLoadoutItems",
	"OpenEditor",
	"CloseEditor",
	"BuildEditorPayload",
	"HST_LOADOUT_EDITOR|%1|%2|%3|%4|%5|%6|%7",
	"PREVIEW|%1|%2|%3|%4",
	"PREVIEW_PREFAB|%1",
	"CATEGORY|%1|%2|%3",
	"ITEM|%1|%2|%3|%4|%5|%6|%7|%8|%9",
	"SLOT|%1|%2|%3|%4|%5|%6|%7|%8|%9",
	"NODE|%1|%2|%3|%4|%5|%6|%7|%8|%9",
	"CANDIDATE|%1|%2|%3|%4|%5|%6|%7|%8|%9",
	"STORAGE|%1|%2|%3|%4|%5",
	"ATTACH|%1|%2|%3|%4|%5",
	"TEMPLATE|%1|%2|%3",
	"SaveCurrentDraft",
	"ApplySavedLoadout",
	"AddDraftItem",
	"ReplaceDraftSlotItem",
	"SetNodeItem",
	"RemoveNodeItem",
	"RefreshDraftNodes",
	"RemoveDraftSlot",
	"SetDraftSlotQuantity",
	"ClearDraft",
	"SelectSavedLoadout",
	"DeleteSavedLoadout",
	"SpawnPreviewMannequin",
	"DeletePreviewMannequin",
	"FindPreviewMannequin",
	"RefreshPreviewMannequinLoadout",
	"ClearInventoryItems",
	"ResolvePreviewQuantityLimit",
	"ValidateLoadoutTransaction",
	"ValidateAttachmentCompatibility",
	"ApplyLoadoutToPlayerEntity",
	"TryInsertItemIntoPlayerInventory",
	"TryInsertItemIntoInventory",
	"ResolveDraftMaxQuantity",
	"BuildUniqueDraftSlotId",
	"CommitLoadoutTransaction",
	"ReturnUnneededIssuedItems",
	"m_bUnlocked",
	"INF",
	"m_sPreviewStatus",
	"m_iPreviewItemCount",
	"MarkIssuedLoadoutLostOnDeath",
	"RequestMemberOpenLoadoutEditor",
	"RequestMemberCloseLoadoutEditor",
	"RequestMemberSaveLoadoutDraft",
	"RequestMemberApplySavedLoadout",
	"RequestMemberAddLoadoutDraftItem",
	"RequestMemberRemoveLoadoutDraftSlot",
	"RequestMemberSetLoadoutDraftSlotQuantity",
	"RequestMemberReplaceLoadoutDraftSlotItem",
	"RequestMemberSetLoadoutNodeItem",
	"RequestMemberRemoveLoadoutNodeItem",
	"RequestMemberClearLoadoutDraft",
	"RequestMemberSelectSavedLoadout",
	"RequestMemberDeleteSavedLoadout",
	"BuildLoadoutEditorPayload",
	"RequestLoadoutEditorCommand",
	"RequestLoadoutEditorAction",
	"RpcDo_ReceiveLoadoutEditorPayload",
	"loadout_replace_slot",
	"set_node_item",
	"set_attachment",
	"remove_node_item",
	"remove_attachment",
	"add_storage_item",
	"loadout_clear_draft",
	"HST_HQArsenalLoadoutEditorAction"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredLoadoutEditorEntry)) {
		throw "Custom loadout editor contract is missing: $requiredLoadoutEditorEntry"
	}
}
if (!(Test-Path "UI/layouts/HST_LoadoutEditor.layout")) {
	throw "Missing fullscreen loadout editor layout resource"
}
if (!(Test-Path "UI/layouts/HST_LoadoutEditor.layout.meta")) {
	throw "Missing GUID-backed loadout editor layout meta resource"
}
$loadoutEditorLayoutText = Get-Content -Raw "UI/layouts/HST_LoadoutEditor.layout"
$loadoutEditorLayoutMetaText = Get-Content -Raw "UI/layouts/HST_LoadoutEditor.layout.meta"
if ($loadoutEditorComponentText -notmatch [regex]::Escape('EDITOR_LAYOUT = "{5AF2D86E07D44A51}UI/layouts/HST_LoadoutEditor.layout"')) {
	throw "Loadout editor must reference the GUID-backed layout resource"
}
if ($loadoutEditorComponentText -match [regex]::Escape("{0000000000000000}UI/layouts/HST_LoadoutEditor.layout")) {
	throw "Loadout editor must not reference the zero-GUID layout resource"
}
if ($loadoutEditorLayoutMetaText -notmatch [regex]::Escape('Name "{5AF2D86E07D44A51}UI/layouts/HST_LoadoutEditor.layout"')) {
	throw "Loadout editor layout meta must carry the expected non-zero GUID"
}
foreach ($requiredLayoutEntry in @(
	"RenderTargetWidgetClass",
	"HST_LoadoutPreview",
	"Anchor 0 0 1 1"
)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLayoutEntry)) {
		throw "Loadout editor layout is missing stable render-target entry: $requiredLayoutEntry"
	}
}
foreach ($requiredDisplayNameEntry in @(
	"InventoryItemComponent",
	"GetUIInfo().GetName()",
	"WidgetManager.Translate",
	"LooksLikeLocalizationKey",
	"ResolveReadableDisplayName",
	"ResolveShortItemDisplayName",
	"FriendlyPrefabName",
	'if (LooksLikeLocalizationKey(value))',
	'name.Replace("#", "")'
)) {
	if ($displayNameServiceText -notmatch [regex]::Escape($requiredDisplayNameEntry)) {
		throw "Display name service must resolve readable loadout item names: $requiredDisplayNameEntry"
	}
}
if ($displayNameServiceText -match [regex]::Escape('if (!existingName.IsEmpty() && !LooksLikePrefabPath(existingName))')) {
	throw "Display name service must not pass raw existing names through before localization-key resolution"
}
foreach ($requiredLoadoutEditorComponentEntry in @(
	"OpenFromArsenal",
	"CloseMenuFromExternal",
	"RenderEditor",
	"RenderModeTabs",
	"RenderSlotRail",
	"RenderCandidatePanel",
	"RenderTemplatePanel",
	"RenderSettingsPanel",
	"RenderPreviewStage",
	"RenderFooter",
	"RenderTargetWidget",
	"BaseWorld.CreateWorld",
	"SetWorld",
	"ConfigurePreviewWidget",
	"EnsurePreviewWorld",
	"RefreshPreviewWorldLoadout",
	"CreatePreviewEntity",
	"m_PreviewEntity.SetOrigin(vector.Up)",
	"preview direct mannequin fallback",
	"SetPreviewEntityQualityRecursive",
	"BuildStageToast",
	"UpdatePreviewCamera",
	"AnimatePreviewCamera",
	"ApplyPreviewCameraImmediate",
	"GetPreviewCharacterBounds",
	"GetWorldBounds",
	"vector.Distance",
	"Math.Clamp",
	"DeletePreviewWorld",
	"HST_LoadoutPreview",
	"BuildPreviewStatusLabel",
	"BuildDraftHeaderSummary",
	"BuildSwapHeaderText",
	"BuildSwapActionLabel",
	"BuildNodeActionLabel",
	"RenderNodeRow",
	"RenderCandidateRow",
	"RenderSelectedNodeHeader",
	"EnsureSelectedSlotForCategory",
	"m_iEditorWidth",
	"workspace.GetWidth()",
	"ResolvePayloadDisplayText",
	"m_aItemShortDisplays",
	"m_aItemSlotLabels",
	"m_aItemPreviewEligible",
	"m_aSlotShortDisplays",
	"m_aSlotLabels",
	"m_aSlotPreviewEligible",
	"m_aNodeIds",
	"m_aCandidateNodeIds",
	"m_aVisibleNodeIndexes",
	"m_aVisibleCandidateIndexes",
	"ClampPages",
	"ParseEditorPayload",
	"RequestServerAction",
	"OnServerActionResult",
	"loadout_replace_slot",
	"loadout_add_item",
	"loadout_remove_slot",
	"loadout_set_quantity",
	"set_node_item",
	"set_attachment",
	"loadout_clear_draft",
	"loadout_select",
	"loadout_delete",
	"IsRemovedExternalPrefab",
	"Resource.Load(prefab)",
	"ITEM_PAGE_NEXT_WIDGET_ID",
	"SLOT_PAGE_NEXT_WIDGET_ID",
	"TEMPLATE_PAGE_NEXT_WIDGET_ID"
)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutEditorComponentEntry)) {
		throw "Fullscreen loadout editor component is missing: $requiredLoadoutEditorComponentEntry"
	}
}
if ($loadoutEditorComponentText -match "InventoryPreviewWorld10") {
	throw "Loadout editor preview must not rely on the old visible/cropped InventoryPreviewWorld10 stage"
}
if ($loadoutEditorComponentText -match [regex]::Escape("FrameSlot.SetPos(m_PreviewWidget") -or $loadoutEditorComponentText -match [regex]::Escape("FrameSlot.SetSize(m_PreviewWidget")) {
	throw "Loadout editor must not set position/size on the full-anchor render-target layout widget"
}
if ($loadoutEditorComponentText -match "CreateTextWidget\([^\r\n]*m_sLastResult") {
	throw "Loadout editor must not render raw server result/debug text across the preview stage"
}
if ($coordinatorText -match [regex]::Escape("AppendLoadoutEditorPayload")) {
	throw "Normal command menu snapshots must not append loadout-editor payloads"
}
$visiblePayloadMatch = [regex]::Match($coordinatorText, "string BuildVisibleMenuPayload[\s\S]*?\r?\n\t}\r?\n\r?\n\tstring RequestVisibleMenuCommand")
if (!$visiblePayloadMatch.Success -or $visiblePayloadMatch.Value -notmatch "return m_CommandUI\.BuildVisibleMenuPayload" -or $visiblePayloadMatch.Value -match "HST_LOADOUT_EDITOR") {
	throw "Normal command menu payloads must stay separate from HST_LOADOUT_EDITOR blocks"
}
$snapshotRpcMatch = [regex]::Match($requestBridgeText, "protected void RpcDo_ReceiveSnapshot[\s\S]*?\r?\n\t}\r?\n\r?\n\t\[RplRpc\(RplChannel\.Reliable, RplRcver\.Owner\)\]\r?\n\tprotected void RpcDo_ReceiveLoadoutEditorPayload")
if (!$snapshotRpcMatch.Success -or $snapshotRpcMatch.Value -match "HST_LoadoutEditorComponent") {
	throw "Normal I-menu snapshots must not be delivered to HST_LoadoutEditorComponent"
}
if ($requestBridgeText -notmatch "RequestLoadoutEditorAction[\s\S]*?RpcAsk_RequestLoadoutEditorAction[\s\S]*?RpcDo_ReceiveLoadoutEditorPayload") {
	throw "Loadout editor must use a dedicated request/RPC payload path"
}
foreach ($requiredArsenalLootMenuEntry in @(
	'AddMenuAction(actions, TAB_ARSENAL, "Loot nearby to arsenal"',
	'AddMenuAction(actions, TAB_ARSENAL, "Load loot to vehicle"',
	'AddMenuAction(actions, TAB_ARSENAL, "Unload vehicle loot to arsenal"'
)) {
	if ($commandUiText -notmatch [regex]::Escape($requiredArsenalLootMenuEntry)) {
		throw "Arsenal/Loot I-menu action must remain visible: $requiredArsenalLootMenuEntry"
	}
}
foreach ($forbiddenLoadoutDependencyEntry in @(
	"BaconLoadoutEditor",
	"GunBuilder",
	"Bacon_",
	("Ton" + "ka Bean Tools LE")
)) {
	if ($loadoutEditorText -match [regex]::Escape($forbiddenLoadoutDependencyEntry) -or $loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutDependencyEntry)) {
		throw "h-istasi loadout editor must not depend on legacy loadout runtime strings: $forbiddenLoadoutDependencyEntry"
	}
}
foreach ($forbiddenArsenalRuntimeEntry in @(
	"SCR_Arsenal",
	"MSAR"
)) {
	if ($loadoutEditorText -match [regex]::Escape($forbiddenArsenalRuntimeEntry)) {
		throw "Custom loadout editor must not issue gear through stock/MSAR arsenal runtime: $forbiddenArsenalRuntimeEntry"
	}
}
if ($loadoutEditorText -match "\bSCR_PlayerArsenalLoadout\b") {
	foreach ($requiredSerializedLoadoutEntry in @(
		"SCR_PlayerArsenalLoadout.ReadLoadoutString",
		"SCR_PlayerArsenalLoadout.ApplyLoadoutString",
		"JsonSaveContext",
		"JsonLoadContext"
	)) {
		if ($loadoutEditorText -notmatch [regex]::Escape($requiredSerializedLoadoutEntry)) {
			throw "SCR_PlayerArsenalLoadout may only be used by the custom editor for serialized personal preset save/load; missing: $requiredSerializedLoadoutEntry"
		}
	}
}
foreach ($requiredLoadoutEditorServiceCleanupEntry in @(
	"PurgeRemovedExternalLoadoutState",
	"PurgeRemovedExternalDraftSlots",
	"IsAllowedLoadoutSlot",
	"IsRemovedExternalPrefab",
	"IsRemovedExternalItem",
	"IsLoadablePrefabResource",
	"HasUnresolvedDisplayKey",
	"SavePersonalLoadoutsToFile(state, identityId)",
	"purged external"
)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredLoadoutEditorServiceCleanupEntry)) {
		throw "Loadout editor service must purge removed external state from base-game-only sessions: $requiredLoadoutEditorServiceCleanupEntry"
	}
}
foreach ($forbiddenBaseGameOnlyEntry in @(
	"RHS",
	"#RHS",
	"AFRF",
	"MARSOC",
	"FORECON",
	"VKPO",
	"VVRG"
)) {
	if ($loadoutEditorText -match [regex]::Escape($forbiddenBaseGameOnlyEntry) -or $loadoutEditorComponentText -match [regex]::Escape($forbiddenBaseGameOnlyEntry) -or $configResourceText -match [regex]::Escape($forbiddenBaseGameOnlyEntry) -or $defaultCatalog -match [regex]::Escape($forbiddenBaseGameOnlyEntry)) {
		throw "Base-game-only runtime/config files must not contain removed external arsenal content references: $forbiddenBaseGameOnlyEntry"
	}
}
foreach ($requiredWeaponFilterEntry in @(
	"IsPrimaryWeaponCandidate",
	"IsLauncherWeaponCandidate",
	"IsAttachmentCandidateForSlot",
	"IsReplacementCategoryAllowed",
	"ResolveAttachmentSlotKey",
	'node.m_sSlotKey == "weapon"',
	'node.m_sSlotKey == "launcher"',
	'node.m_sKind == "attachment"',
	"attach_bayonet",
	"attach_handguard"
)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredWeaponFilterEntry)) {
		throw "Loadout editor service must enforce weapon-owned attachment and category filtering: $requiredWeaponFilterEntry"
	}
}
if ($loadoutEditorText -match 'sourceCategory == "launcher"\s*\r?\n\s*return "weapon"') {
	throw "Launcher items must remain launcher category, not be collapsed into primary weapons"
}
Write-Host "Custom HST loadout editor contract OK"

$lootServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_LootService.c"
$vehicleRootPolicyText = Get-Content -Raw "Scripts/Game/HST/Services/HST_VehicleRootPolicy.c"
foreach ($requiredVehicleRejectionEntry in @(
	'prefab.Contains("Prefabs/Vehicles/")',
	"GetResourceBasename",
	"BasenameStartsWith",
	"IsRejectedWorldPrefab",
	"Prefabs/Vegetation/",
	'BasenameStartsWith(basename, "b_")',
	"IsKnownVehicleRootName",
	"M998",
	"S1203",
	'prefab.Contains("Supply")',
	'prefab.Contains("Crate")',
	'prefab.Contains("Cache")',
	'prefab.Contains("Box")',
	'prefab.Contains("Cargo")',
	'prefab.Contains("Container")',
	'prefab.Contains("Arsenal")',
	'name.Contains("/VehParts/")',
	'name.Contains("LicensePlate")'
)) {
	if ($vehicleRootPolicyText -notmatch [regex]::Escape($requiredVehicleRejectionEntry)) {
		throw "Loot service must keep strict vehicle-root rejection entry: $requiredVehicleRejectionEntry"
	}
}
if ($vehicleRootPolicyText -match 'Contains\("Bus"\)' -or $vehicleRootPolicyText -match 'Contains\("Car_"\)') {
	throw "Vehicle root policy must not use loose Bus/Car_ substring matching; basename checks only"
}
if ($vehicleRootPolicyText -notmatch 'BasenameStartsWith\(basename, "Bus_"\)' -or $vehicleRootPolicyText -notmatch 'BasenameStartsWith\(basename, "Car_"\)') {
	throw "Vehicle root policy must use normalized basename vehicle token checks"
}
foreach ($removedVehicleGateEntry in @(
	"HasVehicleRootComponent",
	"SCR_BaseCompartmentManagerComponent"
)) {
	if ($lootServiceText -match [regex]::Escape($removedVehicleGateEntry)) {
		throw "Loot service must not require the old compartment-manager-only vehicle root gate: $removedVehicleGateEntry"
	}
}
foreach ($requiredRuntimeVehicleEntry in @(
	"HST_RuntimeVehicleState",
	"m_aRuntimeVehicles",
	"RegisterRuntimeVehicle",
	"ResolveRuntimeVehicleRecord",
	"SCR_EditableEntityComponent",
	"EEditableEntityType.VEHICLE",
	"IsDirectVehicleRootCandidate",
	"resolved direct editor/base-game vehicle root",
	"resolved known base-game vehicle basename root",
	"ResolveVehicleIdentityName",
	"selected registered h-istasi runtime vehicle",
	"MarkRuntimeVehicleDeleted"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredRuntimeVehicleEntry)) {
		throw "Vehicle targeting must support registered h-istasi runtime vehicles: $requiredRuntimeVehicleEntry"
	}
}
foreach ($requiredVehicleDiagnosticEntry in @(
	"m_iDirectRootHits",
	"m_iParentChainRootHits",
	"m_iBoundsRootHits",
	"m_iRuntimeFallbackHits",
	"m_sNearestCandidateDebug",
	"ResolveVehicleRootByNearbyBounds",
	"resolved by nearby vehicle bounds root",
	"TrackNearestVehicleReject",
	"BuildVehicleCandidateDebug",
	"passSummary",
	"direct %1 | parent %2 | bounds %3 | runtime %4"
)) {
	if ($lootServiceText -notmatch [regex]::Escape($requiredVehicleDiagnosticEntry)) {
		throw "Vehicle resolver must emit multi-pass root diagnostics: $requiredVehicleDiagnosticEntry"
	}
}
if ($scriptText -notmatch [regex]::Escape("CleanupInvalidGarageRecords")) {
	throw "Campaign startup must purge invalid garage/cargo vehicle records"
}
if ($lootServiceText -match 'return true;\s*\r?\n\s*}\s*\r?\n\s*protected bool IsRejectedVehicleRootPrefab') {
	throw "Loot service must not accept every Prefabs/Vehicles resource as a root vehicle"
}
if ($vehicleRootPolicyText -match 'if \(!prefab\.Contains\("Prefabs/"\)\)') {
	throw "Vehicle root policy must not reject basename vehicle prefabs"
}
if ($vehicleRootPolicyText -notmatch 'IsVehiclePartPrefab\(prefab\)' -or $vehicleRootPolicyText -notmatch 'LicensePlate') {
	throw "Vehicle root policy must reject vehicle parts and LicensePlate resources"
}
foreach ($requiredProtectedLootEntry in @(
	"IsProtectedHQLootSource",
	"IsProtectedHQEntity",
	"IsHQProtectedPrefab",
	"protectedByProximity",
	"state.m_vArsenalPosition",
	"skipped protected HQ source",
	"ArsenalBox_FIA",
	"SupplyCache",
	"Character_HST_Petros"
)) {
	if ($lootServiceText -notmatch [regex]::Escape($requiredProtectedLootEntry)) {
		throw "Loot service must protect HQ arsenal/cache/tent/Petros from vehicle cargo and area loot: $requiredProtectedLootEntry"
	}
}
foreach ($requiredRecursiveLootEntry in @(
	"GatherInventoryItemsRecursive",
	"GatherStorageItemsRecursive",
	"GatherInventoryItemRecursive",
	"GetOwnedItems",
	"GetParentSlot",
	"GetAttachedEntity",
	"GetSlotsCount",
	"SCR_EntityHelper.DeleteEntityAndChildren(item)"
)) {
	if ($lootServiceText -notmatch [regex]::Escape($requiredRecursiveLootEntry)) {
		throw "Loot service must recursively gather nested corpse/crate/gear inventory: $requiredRecursiveLootEntry"
	}
}
if ($lootServiceText -notmatch 'RekeyLegacyVehiclePartCargo[\s\S]*?m_sVehicleRuntimeId = vehicleId') {
	throw "Vehicle unload must rekey legacy part-bound cargo to the selected root vehicle"
}
if ($lootServiceText -notmatch 'CaptureNearbyVehicleToGarage[\s\S]*?DeleteEntityAndChildren\(selectedVehicle\)[\s\S]*?IsVehicleRootStillPresent[\s\S]*?StoreVehicle\(state, vehicle\)') {
	throw "Garage capture must delete and verify the selected root before keeping a garage record"
}
if ($lootServiceText -match 'if \(!prefab\.Contains\("Vehicles"\) && !prefab\.Contains\("Vehicle"\)\)') {
	throw "Loot service must not use loose Vehicle/Vehicles substring matching for vehicle roots"
}
foreach ($requiredGarageSnapshotEntry in @(
	"IsVehicleOccupied",
	"CaptureVehiclePhysicalCargo",
	"CopyVirtualVehicleCargoToGarage",
	"RemoveVirtualVehicleCargo",
	"m_aStoredCargoItems",
	"m_sDamageState",
	"m_bHadPhysicalCargo",
	"RestoreStoredVehicleCargo",
	"RemoveVehicleCargoByRuntimeId",
	"restored cargo"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredGarageSnapshotEntry)) {
		throw "Garage capture/redeploy must preserve occupancy/cargo snapshot contract: $requiredGarageSnapshotEntry"
	}
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
	"m_aQRFGroupPrefabs",
	"m_aGroupPool",
	"m_aPatrolGroupPool",
	"m_aQRFGroupPool",
	"HST_PrefabPoolEntry",
	"SelectWeightedPrefab"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPhysicalWarEntry) -and $configResourceText -notmatch [regex]::Escape($requiredPhysicalWarEntry)) {
		throw "Missing physical AI war scaffold entry: $requiredPhysicalWarEntry"
	}
}
Write-Host "Physical AI war scaffold OK"

$physicalWarServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PhysicalWarService.c"
foreach ($requiredConvoyRuntimeReportEntry in @(
	"BuildConvoyRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)",
	"convoy mission | instance",
	"source position",
	"target position",
	"route/site ID",
	"vehicle asset count",
	"mission failure reason",
	"convoy vehicle asset | asset",
	"delivered/captured",
	"last interaction",
	"convoy group | group",
	"spawned entity",
	"crew count",
	"alive crew count",
	"fallback mode",
	"spawn failure reason",
	"CountMissionConvoyVehicleAssets",
	"ResolveMissionConvoySourcePosition",
	"ResolveMissionConvoyTargetPosition",
	"travel distance",
	"ReportRouteSite",
	"missing:",
	"SelectMissionConvoyVehiclePrefab",
	"IsAircraftVehicleResource",
	"convoy_vehicle_control_unavailable",
	"convoy_seating_pending",
	"BuildCrewSeatingReport",
	"Convoy crew seating has not confirmed a seated AI driver yet."
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredConvoyRuntimeReportEntry)) {
		throw "Missing Phase 2 convoy runtime report contract entry: $requiredConvoyRuntimeReportEntry"
	}
}
$worldPositionServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_WorldPositionService.c"
foreach ($requiredConvoySpawnSafetyEntry in @(
	"TryResolveLargeVehicleSpawnPosition",
	"MAX_LARGE_VEHICLE_SLOPE_DELTA_METERS",
	"LARGE_VEHICLE_SAMPLE_RADIUS_METERS",
	"IsVehicleFootprintStable",
	"IsVehicleFootprintStableForTravel",
	"IsVehicleFootprintStableWithForward",
	"TryResolveNearestRoadVehiclePosition",
	"RoadNetworkManager",
	"GetRoadsInAABB",
	"BaseRoad",
	"road.GetPoints(points)",
	"road.GetWidth()",
	"MIN_ROAD_VEHICLE_WIDTH_METERS = 3.5",
	"ROAD_VEHICLE_FOOTPRINT_MARGIN_METERS = 0.75",
	"VEHICLE_FOOTPRINT_HALF_WIDTH_METERS",
	"VEHICLE_FOOTPRINT_HALF_LENGTH_METERS",
	"MAX_VEHICLE_FOOTPRINT_ROLL_DELTA_METERS"
)) {
	if ($worldPositionServiceText -notmatch [regex]::Escape($requiredConvoySpawnSafetyEntry)) {
		throw "Missing convoy large vehicle spawn safety entry: $requiredConvoySpawnSafetyEntry"
	}
}
$missionRuntimeServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionRuntimeService.c"
foreach ($requiredConvoyRouteEntry in @(
	"TryResolveConvoySpawnPlan",
	"ResolveConvoyRandomStartDistanceMeters",
	"TryBuildConvoyVehicleStartSlots",
	"ResolveConvoyEndPosition",
	"CONVOY_START_PROBE_ATTEMPTS = 72",
	"CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS = 300.0",
	"CONVOY_START_ROAD_SEARCH_RADIUS_METERS = 250.0",
	"CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS = 40.0",
	"MIN_CONVOY_START_DISTANCE_METERS = 1000.0",
	"MAX_CONVOY_START_DISTANCE_METERS = 2500.0",
	"IsConvoyDistanceInsideBand",
	"planned distance",
	"required band 1000-2500m",
	"band valid",
	"No road-resolved convoy destination",
	"No road-resolved convoy spawn plan found in required 1000-2500m band",
	"convoy vehicle slot is not road-resolved",
	"convoy road vehicle slot failed the flat vehicle footprint check"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredConvoyRouteEntry)) {
		throw "Missing convoy route distance/staging entry: $requiredConvoyRouteEntry"
	}
}
if ($missionRuntimeServiceText -match [regex]::Escape("IsUsableConvoyRouteSegment(fallback, convoyEnd, 120.0)")) {
	throw "Phase 6 convoy route selection must not keep the close-range 120m fallback"
}
if ($missionRuntimeServiceText -match [regex]::Escape("TryResolveConvoyRoadPosition")) {
	throw "Convoy route selection must not depend on the removed road resolver"
}
if ($missionRuntimeServiceText -notmatch [regex]::Escape("float distanceMeters = ResolveConvoyRandomStartDistanceMeters(seed, attempt);")) {
	throw "Convoy spawn planning must sample a seeded random distance for every probe"
}
if ($missionRuntimeServiceText -notmatch [regex]::Escape("convoyStartSlots.Count() != vehicleCount")) {
	throw "Convoy spawn planning must verify the full vehicle column before creating assets"
}
if ($missionRuntimeServiceText -match [regex]::Escape("vector startSlot = OffsetConvoyVehicleStartPosition(convoyRoute, convoyStart, convoyEnd, i, reservedConvoyStarts);")) {
	throw "Convoy asset creation must not commit per-vehicle slots before full-column probing succeeds"
}
$commandUIServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
if ($commandUIServiceText -notmatch [regex]::Escape('AddMenuAction(actions, TAB_MISSIONS, "Convoy Runtime Report", "inspect_convoy_runtime"')) {
	throw "Missions tab must expose Convoy Runtime Report"
}
Write-Host "Phase 2 convoy runtime report contract OK"

$campaignSaveDataText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$generatedContentServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_GeneratedContentService.c"
$coordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
foreach ($requiredPhase3RouteStateEntry in @(
	"HST_RouteWaypointState",
	"m_iRadiusMeters",
	"m_sHint",
	"m_iWaypointCount",
	"m_bValidatedForVehicles",
	"m_sValidationFailureReason",
	"m_aWaypoints"
)) {
	if ($campaignStateText -notmatch [regex]::Escape($requiredPhase3RouteStateEntry)) {
		throw "Missing Phase 3 route state entry: $requiredPhase3RouteStateEntry"
	}
}
foreach ($requiredPhase3SaveEntry in @(
	"CopyRouteWaypoint",
	"BackfillGeneratedRouteWaypoints",
	"CreateRouteWaypoint",
	"CalculateRouteDistanceMeters",
	"foreach (HST_GeneratedRouteState route : m_aGeneratedRoutes)"
)) {
	if ($campaignSaveDataText -notmatch [regex]::Escape($requiredPhase3SaveEntry)) {
		throw "Missing Phase 3 route save/migration entry: $requiredPhase3SaveEntry"
	}
}
foreach ($requiredPhase3GeneratedContentEntry in @(
	"CreateRouteWaypoint",
	"EnsureRouteWaypointMetadata",
	"ValidateRouteForVehicles",
	"BuildGeneratedRouteReport",
	"vehicle-safe routes",
	"min route waypoints",
	"waypoint count",
	"TryResolveLargeVehicleSpawnPosition",
	"route has fewer than three waypoints"
)) {
	if ($generatedContentServiceText -notmatch [regex]::Escape($requiredPhase3GeneratedContentEntry)) {
		throw "Missing Phase 3 generated route entry: $requiredPhase3GeneratedContentEntry"
	}
}
foreach ($requiredPhase3MissionRuntimeEntry in @(
	"BuildConvoyRouteWaypointReport",
	"route waypoint",
	"vehicle-safe",
	"waypoint count",
	"validation"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredPhase3MissionRuntimeEntry)) {
		throw "Missing Phase 3 mission route report entry: $requiredPhase3MissionRuntimeEntry"
	}
}
foreach ($requiredPhase3PhysicalWarEntry in @(
	"ResolveMissionConvoyRouteId",
	"ResolveMissionConvoyRoute",
	"activeGroup.m_sRouteId = ResolveMissionConvoyRouteId(state, mission)",
	"BuildMissionConvoyRouteReport",
	"active route waypoint",
	"BuildGroundVehicleCandidateReport",
	"BuildGroundVehicleCandidateFactionReport",
	"BuildConvoyVehicleCandidates",
	"AppendRuntimeFactionCatalogVehiclePrefabs",
	"SCR_EntityCatalogManagerComponent.GetInstance()",
	"GetAllFactionEntityCatalogs",
	"SCR_EntityCatalogEntry",
	"entry.GetPrefab()",
	"BuildFactionCampaignCatalogSource",
	"catalog source",
	"{ADFDBDA163950168}Configs/Factions/US_Campaign.conf",
	"{15B582F8FA0B0940}Configs/Factions/USSR_Campaign.conf",
	"BuildUsableConvoyVehicleCandidates",
	"IsGuidQualifiedVehicleResource",
	"IsGroundVehicleResource",
	"CountUnverifiedVehicleCandidates",
	"convoy ground vehicle candidates",
	"unverified path-only",
	"rejected non-ground vehicle prefab",
	"rejected unverified path-only vehicle prefab",
	"IsValidVehiclePrefabResource(prefab, factionKey, false)",
	"{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et",
	"{F1FBD0972FA5FE09}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport.et",
	"{81FDAD5EB644CC3D}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport_covered.et"
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase3PhysicalWarEntry)) {
		throw "Missing Phase 3 convoy route/vehicle report entry: $requiredPhase3PhysicalWarEntry"
	}
}
foreach ($forbiddenPhase3ConvoyVehicleEntry in @(
	"Prefabs/Vehicles/Wheeled/M998/M998.et",
	"Prefabs/Vehicles/Wheeled/M923A1/M923A1_covered.et"
)) {
	if ($physicalWarServiceText -match [regex]::Escape($forbiddenPhase3ConvoyVehicleEntry)) {
		throw "Phase 3 convoy vehicle defaults must not retain invalid/path-only resource: $forbiddenPhase3ConvoyVehicleEntry"
	}
}
if ($coordinatorText -notmatch [regex]::Escape("m_PhysicalWar.BuildGroundVehicleCandidateReport()")) {
	throw "Generated content report must append convoy ground vehicle candidate report"
}
if ($commandUIServiceText -notmatch [regex]::Escape('AddMenuAction(actions, TAB_MAP, "Generated content report", "inspect_content"')) {
	throw "Map tab must expose the generated content report action"
}
Write-Host "Phase 3 convoy route state contract OK"

foreach ($requiredPhase4ReadinessEntry in @(
	"HST_ConvoyReadinessStatus",
	"CONVOY_READINESS_GRACE_SECONDS",
	"BuildMissionConvoyReadinessReport",
	"BuildMissionConvoyReadinessStatus",
	"TryAdvanceMissionConvoyFromStaging",
	"SetMissionConvoyMoving",
	"SetMissionConvoyStaticFallback",
	"CanAttemptMissionConvoyWaypointAssignment",
	"IsMissionConvoyReadinessGraceActive",
	"convoy readiness | ready",
	"pending grace",
	"static fallback",
	"convoy readiness vehicle assets",
	"spawned vehicles",
	"convoy readiness crew groups",
	"alive crew count",
	"driver availability",
	"route assignment",
	"waypoint assignment",
	"No convoy vehicle assets exist; convoy cannot move.",
	"No convoy vehicles spawned; convoy cannot move.",
	"Convoy crew groups failed to spawn.",
	"Convoy route assignment failed.",
	"Convoy waypoint assignment failed.",
	"Convoy has no seated living AI driver yet."
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase4ReadinessEntry)) {
		throw "Missing Phase 4 convoy readiness gate entry: $requiredPhase4ReadinessEntry"
	}
}
if ($physicalWarServiceText -notmatch [regex]::Escape("changed = TryAdvanceMissionConvoyFromStaging(state, mission) || changed;")) {
	throw "Phase 4 convoy staging transition must route through readiness gate"
}
if ($campaignStateText -match [regex]::Escape("HST_ConvoyReadinessStatus")) {
	throw "Phase 4 convoy readiness status must remain transient and out of persistent campaign state"
}
Write-Host "Phase 4 convoy readiness gate contract OK"

$convoyVehicleControlAdapterText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ConvoyVehicleControlAdapter.c"
foreach ($requiredPhase6SeatingEntry in @(
	"HST_ConvoyCrewSeatingResult",
	"driver assigned",
	"seated crew",
	"turret seated",
	"cargo seated",
	"seating pending",
	"last seating reason",
	"AIGroup group",
	"GetAgents(agents)",
	"AIAgent",
	"GetControlledEntity",
	"SCR_BaseCompartmentManagerComponent",
	"BaseCompartmentManagerComponent",
	"BaseCompartmentSlot",
	"IsPiloting",
	"ECompartmentType.PILOT",
	"ECompartmentType.TURRET",
	"ECompartmentType.CARGO",
	"SCR_CompartmentAccessComponent",
	"MoveInVehicle(vehicleEntity, compartmentType, false, slot)",
	"HasLivingDriver",
	"AreLivingCrewMounted",
	"CountLivingCrew",
	"BuildCrewSeatingReport"
)) {
	if ($convoyVehicleControlAdapterText -notmatch [regex]::Escape($requiredPhase6SeatingEntry)) {
		throw "Missing Phase 6 convoy seating contract entry: $requiredPhase6SeatingEntry"
	}
}
foreach ($requiredPhase6RuntimeEntry in @(
	"EnsureMissionConvoyCrewSeating",
	"ShouldRetryMissionConvoyCrewSeating",
	"changed = EnsureMissionConvoyCrewSeating(state, mission) || changed;",
	"convoy_seating_pending",
	"convoy_driver_available",
	"preserveWaypointMode",
	"mission.m_sRuntimePhase == MISSION_CONVOY_STAGING",
	"mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && state.m_iElapsedSeconds % CONVOY_PROGRESS_SYNC_SECONDS == 0",
	"readiness.m_iDriverAvailableCount >= required",
	"Convoy has no seated living AI driver yet."
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase6RuntimeEntry)) {
		throw "Missing Phase 6 convoy runtime seating entry: $requiredPhase6RuntimeEntry"
	}
}
foreach ($forbiddenPhase6PlaceholderEntry in @(
	"real seating is planned for Phase 6",
	"Convoy crew seating/movement is planned for later convoy phases; convoy remains a static ambush.",
	"Convoy crew seating is planned for Phase 6; crew remains near vehicle for static ambush."
)) {
	if ($physicalWarServiceText -match [regex]::Escape($forbiddenPhase6PlaceholderEntry) -or $convoyVehicleControlAdapterText -match [regex]::Escape($forbiddenPhase6PlaceholderEntry)) {
		throw "Phase 6 convoy runtime must not report placeholder seating text: $forbiddenPhase6PlaceholderEntry"
	}
}
if ($campaignStateText -match [regex]::Escape("HST_ConvoyCrewSeatingResult")) {
	throw "Phase 6 convoy seating result must remain transient and out of persistent campaign state"
}
Write-Host "Phase 6 convoy seating and distance contract OK"

if ($campaignSchemaVersion -ne 17) {
	throw "Phase 7 convoy waypoint-chain movement requires campaign schema 17"
}
foreach ($requiredPhase7StateEntry in @(
	"m_iAssignedWaypointCount",
	"target.m_iAssignedWaypointCount = source.m_iAssignedWaypointCount",
	"FindGeneratedRouteForMigration",
	"ResolveGeneratedRouteWaypointCount",
	"group.m_iAssignedWaypointCount <= 0 && group.m_sSpawnFallbackMode == ""convoy_waypoints"""
)) {
	if ($campaignStateText -notmatch [regex]::Escape($requiredPhase7StateEntry) -and $campaignSaveDataText -notmatch [regex]::Escape($requiredPhase7StateEntry)) {
		throw "Missing Phase 7 convoy waypoint state/migration entry: $requiredPhase7StateEntry"
	}
}
foreach ($requiredPhase7AdapterEntry in @(
	"out int assignedWaypointCount",
	"assignedWaypointCount = 0",
	"waypoint-chain route needs at least two waypoint positions",
	"array<IEntity> spawnedWaypoints",
	"SpawnRouteWaypoint",
	"DeleteSpawnedWaypoints",
	"assignedWaypointCount++",
	"Convoy adapter assigned route waypoint chain",
	"m_iRemovedOverflowCrew",
	"RemoveUnseatedOverflowCrew",
	"overflow removed",
	"surplus convoy crew without vehicle seats",
	"AreAllLivingCrewDismounted"
)) {
	if ($convoyVehicleControlAdapterText -notmatch [regex]::Escape($requiredPhase7AdapterEntry)) {
		throw "Missing Phase 7 convoy adapter waypoint-chain entry: $requiredPhase7AdapterEntry"
	}
}
foreach ($requiredPhase7RuntimeEntry in @(
	"assigned waypoint count",
	"activeGroup.m_iAssignedWaypointCount > 1",
	"BuildMissionConvoyWaypointPositions",
	"lastIndex",
	"selectedWaypoint",
	"AppendConvoyRoadWaypoint(waypoints, route.m_vEndPosition, route.m_vEndPosition)",
	"BuildMissionConvoyGroupWaypointPositions",
	"AppendConvoyLeadInWaypoints",
	"activeGroup.m_vSourcePosition, routeWaypoints[0]",
	"TryAssignVehicleRoute(activeGroup, crewEntity, vehicle, groupWaypoints, assignedWaypointCount, adapterReason)",
	"activeGroup.m_iAssignedWaypointCount = assignedWaypointCount",
	"activeGroup.m_iAssignedWaypointCount = 0",
	"CONVOY_VEHICLE_SPAWN_LIFT_METERS = 1.25",
	"CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS = 18.0",
	"TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, false)",
	"TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, true)",
	"TryResolveMissionConvoyVehicleSpawnCandidate",
	"CONVOY_PHYSICAL_ROAD_SEARCH_RADIUS_METERS = 40.0",
	"TryResolveNearestRoadVehiclePosition(position, CONVOY_PHYSICAL_ROAD_SEARCH_RADIUS_METERS",
	"if (IsMissionConvoyGroup(activeGroup))",
	"aliveCount = CountAliveRuntimeCrewAgents(activeGroup)",
	"IsMissionConvoyMovementInterrupted",
	"IsMissionConvoyGroupFullyDismounted",
	"AreAllLivingCrewDismounted(crewEntity, vehicleEntity, reason)",
	"Convoy movement interrupted because every moving convoy group lost vehicle control or waypoint assignment.",
	"RefreshMissionConvoyCrewCount",
	"return prefab.Contains(""SentryTeam"");"
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase7RuntimeEntry)) {
		throw "Missing Phase 7 convoy runtime waypoint-chain entry: $requiredPhase7RuntimeEntry"
	}
}
$largeConvoySpawnPassIndex = $physicalWarServiceText.IndexOf("TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, false)")
$heavyConvoySpawnPassIndex = $physicalWarServiceText.IndexOf("TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, true)")
if ($largeConvoySpawnPassIndex -lt 0 -or $heavyConvoySpawnPassIndex -lt 0 -or $largeConvoySpawnPassIndex -gt $heavyConvoySpawnPassIndex) {
	throw "Phase 8 convoy compact staging must prefer the large-vehicle route slot before the wider heavy-vehicle fallback"
}
foreach ($requiredPhase7WorldPositionEntry in @(
	"BuildEntitySetAnglesFromYawVector",
	"BuildEntitySetAnglesFromYaw",
	"genericEntity.SetAngles(BuildEntitySetAnglesFromYawVector(angles))",
	"entity.SetAngles(BuildEntitySetAnglesFromYawVector(uprightAngles))",
	"angles[1] = NormalizeYaw(yawDegrees)"
)) {
	if ($worldPositionServiceText -notmatch [regex]::Escape($requiredPhase7WorldPositionEntry)) {
		throw "Missing Phase 7 convoy world-position angle entry: $requiredPhase7WorldPositionEntry"
	}
}
foreach ($forbiddenPhase7VehicleBiasEntry in @(
	"BuildPreferredMissionConvoyVehicleCandidates",
	"IsPreferredMissionConvoyVehiclePrefab",
	"IsMediumConvoyVehicleResource"
)) {
	if ($physicalWarServiceText -match [regex]::Escape($forbiddenPhase7VehicleBiasEntry)) {
		throw "Phase 7 convoy vehicle selection must not include mission-specific vehicle prefab bias: $forbiddenPhase7VehicleBiasEntry"
	}
}
foreach ($requiredPhase7MissionRuntimeEntry in @(
	"BuildConvoyVehicleCountSeed",
	"ResolveMissionInstanceNumericSeed",
	"CONVOY_VEHICLE_START_SPACING_METERS = 24.0",
	"MIN_CONVOY_VEHICLE_START_SEPARATION_METERS = 18.0",
	"ResolveConvoyRouteStagingPosition",
	"BuildConvoyRouteStagingPoints",
	"MIN_CONVOY_VEHICLES + HST_DefaultCatalog.PositiveMod(seed, MAX_CONVOY_VEHICLES - MIN_CONVOY_VEHICLES + 1)",
	"ResolveMissionInstanceNumericSeed(mission.m_sInstanceId) * 127"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredPhase7MissionRuntimeEntry)) {
		throw "Missing Phase 7 convoy vehicle-count contract entry: $requiredPhase7MissionRuntimeEntry"
	}
}
foreach ($forbiddenPhase7MissionRuntimeEntry in @(
	"return Math.Max(MIN_CONVOY_VEHICLES, Math.Min(MAX_CONVOY_VEHICLES, definition.m_iVehicleCount));",
	"resolved = Math.Max(resolved, definition.m_iVehicleCount);"
)) {
	if ($missionRuntimeServiceText -match [regex]::Escape($forbiddenPhase7MissionRuntimeEntry)) {
		throw "Phase 7 convoy vehicle count must roll 3-6 regardless of convoy type: $forbiddenPhase7MissionRuntimeEntry"
	}
}
if ($physicalWarServiceText -match [regex]::Escape("return activeGroup.m_sSpawnFallbackMode == ""convoy_waypoints"";")) {
	throw "Phase 7 waypoint readiness must require assigned waypoint count, not only convoy_waypoints fallback mode"
}
Write-Host "Phase 7 convoy waypoint-chain contract OK"

if ($campaignSchemaVersion -ne 17) {
	throw "Phase 8 convoy progress must keep campaign schema 17 because progress samples are transient diagnostics"
}
foreach ($requiredPhase8RuntimeEntry in @(
	"HST_ConvoyProgressStatus",
	"m_aConvoyProgressStatuses",
	"CONVOY_PROGRESS_SYNC_SECONDS = 5",
	"CONVOY_MARKER_REFRESH_SECONDS = 30",
	"CONVOY_PROGRESS_THRESHOLD_METERS = 8.0",
	"CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS = 45",
	"CONVOY_HARD_STUCK_THRESHOLD_SECONDS = 120",
	"CONVOY_UNSTUCK_MIN_PLAYER_DISTANCE_METERS = 300.0",
	"CONVOY_ROUTE_SNAP_SEARCH_RADIUS_METERS = 180.0",
	"CONVOY_ROUTE_SNAP_MIN_DESTINATION_DISTANCE_METERS = 250.0",
	"CONVOY_ROUTE_SNAP_MAX_ADVANCE_METERS = 160.0",
	"UpdateMissionConvoyProgress",
	"UpdateConvoyVehicleProgressStatus",
	"TryReissueMissionConvoyRouteForProgress",
	"TrySnapMissionConvoyVehicleToRoute",
	"TryResolveNearestConvoyRouteSnapPosition",
	"BuildConvoyVehicleAnglesFromForward",
	"ResolveNearestLivingPlayerDistanceMeters",
	"progress.m_fNearestPlayerDistanceMeters >= 0 && progress.m_fNearestPlayerDistanceMeters < CONVOY_UNSTUCK_MIN_PLAYER_DISTANCE_METERS",
	"activeGroup.m_vSourcePosition = activeGroup.m_vPosition",
	"SetRuntimeGroupEntitiesOrigin(activeGroup.m_sGroupId, snapPosition)",
	"MoveUnseatedLivingCrewNearVehicle",
	"AreLivingCrewMounted",
	"route snap succeeded to road point",
	"no road-resolved snap point near stuck vehicle",
	"road snap point would advance convoy",
	"ClosestPointOnSegment2D",
	"CONVOY_DESTINATION_RADIUS_METERS = 50.0",
	"CONVOY_ROUTE_WAYPOINT_ROAD_SEARCH_RADIUS_METERS = 250.0",
	"AppendConvoyRoadSegmentWaypoints",
	"AppendConvoyRoadWaypoint",
	"route reissue waiting for reseated driver",
	"TryEnsureMissionConvoyDriverForRouteReissue",
	"routeWaypoints.Count() <= 1",
	"TryResolveMissionConvoyDestinationArrival",
	"CanResolveMissionConvoyDestinationArrival",
	"Convoy reached its destination with living crew.",
	"Convoy moving recovery pending",
	"CleanupInactiveMissionConvoyRuntime"
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase8RuntimeEntry)) {
		throw "Missing Phase 8 convoy progress/stuck entry: $requiredPhase8RuntimeEntry"
	}
}
foreach ($requiredPhase8MissionRuntimeEntry in @(
	"ResolveConvoyEndPosition",
	"TryResolveConvoySpawnPlan",
	"BuildConvoySpawnPlanSeed",
	"ResolveConvoyRandomStartDistanceMeters",
	"BuildConvoySpawnPlanCandidate",
	"BuildConvoyColumnSlotPosition",
	"TryBuildConvoyVehicleStartSlots",
	"No road-resolved convoy spawn plan found in required 1000-2500m band",
	"vector startSlot = convoyStartSlots[i]",
	"TryResolveConvoyVehicleStartSlot",
	"convoy vehicle slot is outside the required 1000-2500m destination band",
	"convoy road vehicle slot failed the flat vehicle footprint check"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredPhase8MissionRuntimeEntry)) {
		throw "Missing Phase 8 convoy dry-route mission runtime entry: $requiredPhase8MissionRuntimeEntry"
	}
}
foreach ($requiredPhase8AdapterEntry in @(
	"MoveUnseatedLivingCrewNearVehicle",
	"IsCrewSeatedInVehicle(crewEntity, vehicleEntity)"
)) {
	if ($convoyVehicleControlAdapterText -notmatch [regex]::Escape($requiredPhase8AdapterEntry)) {
		throw "Missing Phase 8 convoy reseat adapter entry: $requiredPhase8AdapterEntry"
	}
}
foreach ($requiredPhase8RoadResolverEntry in @(
	"TryResolveNearestRoadVehiclePosition",
	"RoadNetworkManager available",
	"road-resolved yes",
	"planned source road",
	"planned target road",
	"source road",
	"current road",
	"target road",
	"assigned road waypoint chain",
	"road check assigned-runtime-chain",
	"road manager"
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase8RoadResolverEntry) -and $missionRuntimeServiceText -notmatch [regex]::Escape($requiredPhase8RoadResolverEntry) -and $worldPositionServiceText -notmatch [regex]::Escape($requiredPhase8RoadResolverEntry)) {
		throw "Missing Phase 8 road-endpoint convoy resolver entry: $requiredPhase8RoadResolverEntry"
	}
}
$phase8ArrivalIndex = $physicalWarServiceText.IndexOf("if (TryResolveMissionConvoyDestinationArrival(state, mission))")
$phase8MovementInterruptIndex = $physicalWarServiceText.IndexOf("if (mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && IsMissionConvoyMovementInterrupted(state, mission))")
if ($phase8ArrivalIndex -lt 0 -or $phase8MovementInterruptIndex -lt 0 -or $phase8ArrivalIndex -gt $phase8MovementInterruptIndex) {
	throw "Phase 8 convoy arrival must be resolved before movement interruption fallback"
}
if ($physicalWarServiceText -match [regex]::Escape("mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && IsMissionConvoyAtDestination")) {
	throw "Phase 8 convoy arrival must not be gated by the old moving-phase-only objective condition"
}
if ($physicalWarServiceText -match [regex]::Escape("Convoy movement interrupted because every living crew member in a moving convoy group dismounted.")) {
	throw "Phase 8 smoke fix must not let one fully dismounted convoy group force static/contact fallback"
}
foreach ($requiredPhase8ReportEntry in @(
	"BuildConvoyVehicleProgressReport",
	"distance to destination",
	"sampled distance",
	"Math.Round(distanceToDestination), Math.Round(progress.m_fDistanceToDestinationMeters)",
	"last progress age",
	"no-progress",
	"hard stuck",
	"route reissue",
	"route snap",
	"nearest player",
	"snap gate"
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase8ReportEntry)) {
		throw "Missing Phase 8 convoy report entry: $requiredPhase8ReportEntry"
	}
}
if ($campaignStateText -match [regex]::Escape("HST_ConvoyProgressStatus")) {
	throw "Phase 8 convoy progress status must remain transient and out of persistent campaign state"
}
foreach ($forbiddenPhase8RoadPersistentEntry in @(
	"RoadNetworkManager",
	"BaseRoad"
)) {
	if ($campaignStateText -match [regex]::Escape($forbiddenPhase8RoadPersistentEntry) -or $campaignSaveDataText -match [regex]::Escape($forbiddenPhase8RoadPersistentEntry)) {
		throw "Phase 8 road resolver handles must remain runtime-only and out of persistent campaign state: $forbiddenPhase8RoadPersistentEntry"
	}
}
Write-Host "Phase 8 convoy progress/stuck contract OK"

if ($campaignSchemaVersion -ne 17) {
	throw "Phase 9 convoy contact must keep campaign schema 17 because contact uses existing mission runtime phase state"
}
foreach ($requiredPhase9RuntimeEntry in @(
	"CONVOY_CONTACT_RADIUS_METERS = 120.0",
	"UpdateMissionConvoyContact",
	"TryResolveMissionConvoyContactReason",
	"SetMissionConvoyContact",
	"BuildMissionConvoyContactReport",
	"Convoy contact: living player within",
	"Convoy contact: crew count decreased",
	"asset.m_bDelivered",
	"asset.m_bDestroyed",
	"IsVehicleMobile(vehicleEntity, mobileReason)",
	"MarkMissionConvoyVehicleDestroyed",
	"mission.m_sRuntimePhase = MISSION_CONVOY_CONTACT",
	"ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_CONTACT)",
	"convoy contact | active",
	"radius %2m"
)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase9RuntimeEntry)) {
		throw "Missing Phase 9 convoy contact runtime entry: $requiredPhase9RuntimeEntry"
	}
}
$phase9ContactIndex = $physicalWarServiceText.IndexOf("changed = UpdateMissionConvoyContact(state, mission) || changed;")
$phase9SurvivorIndex = $physicalWarServiceText.IndexOf("changed = UpdateRuntimeGroupSurvivors(state) || changed;")
if ($phase9ContactIndex -lt 0 -or $phase9SurvivorIndex -lt 0 -or $phase9ContactIndex -gt $phase9SurvivorIndex) {
	throw "Phase 9 convoy contact must run before survivor counts are refreshed"
}
foreach ($requiredPhase9MissionRuntimeEntry in @(
	"ShouldKeepConvoyContactPhase",
	"mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT",
	"activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iLastSeenAliveCount > 0"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredPhase9MissionRuntimeEntry)) {
		throw "Missing Phase 9 convoy contact mission-runtime entry: $requiredPhase9MissionRuntimeEntry"
	}
}
if ($missionRuntimeServiceText -match [regex]::Escape("mission.m_sRuntimePhase = PHASE_CAPTURED;`r`n`t`tresult = `"h-istasi mission | captured `" + BuildAssetShortLabel(asset);")) {
	throw "Phase 9 convoy vehicle capture must not unconditionally replace convoy_contact with generic captured phase"
}
if ($missionRuntimeServiceText -match [regex]::Escape("if (mission)`r`n`t`t`tmission.m_sRuntimePhase = PHASE_DESTROYED;")) {
	throw "Phase 9 convoy vehicle destruction must not unconditionally replace convoy_contact with generic destroyed phase"
}
if ($commandUIServiceText -notmatch [regex]::Escape('return "convoy contact";')) {
	throw "Missing Phase 9 command UI convoy contact label"
}
if ($physicalWarServiceText -match [regex]::Escape("HST_ConvoyContactStatus") -or $campaignStateText -match [regex]::Escape("HST_ConvoyContactStatus") -or $campaignSaveDataText -match [regex]::Escape("HST_ConvoyContactStatus")) {
	throw "Phase 9 contact status must not add a persistent convoy contact state object"
}
Write-Host "Phase 9 convoy contact contract OK"

foreach ($requiredCivilianRuntimeEntry in @(
	"UpdatePhysicalTownPopulation",
	"SpawnActiveZoneRuntime",
	"m_aCivilianCharacterPrefabs",
	"SelectCivilianCharacterPrefab",
	"ResolveTownCharacterSpawnPosition",
	"BuildSpawnAngles",
	"RecordSpawnFailure",
	"PublishRuntimeDiagnostics",
	"IsGuidQualifiedResource",
	"MIN_CIVILIAN_CHARACTER_PREFABS",
	"CountGuidQualifiedCivilianCharacterPrefabs",
	"fewer than",
	"SelectCivilianVehiclePrefab",
	"ResolveTownVehicleSpawnPosition",
	"BuildZoneSeed",
	"ModInt",
	"IsTownGroundVehicleResource",
	"IsAircraftVehicleResource",
	"CleanupZoneRuntimeEntities",
	"CleanupAllRuntimeEntities",
	"ShouldDetachFromTownCleanup",
	"m_aRuntimeEntityKinds",
	"m_aRuntimeEntitySpawnPositions",
	'SetAffiliatedFactionByKey("CIV")',
	"m_bCivilianPopulationEnabled",
	"m_iCivilianMaxActivePerTown",
	"m_iCivilianVehicleMinPerTown",
	"m_iCivilianVehicleMaxPerTown",
	"m_iOccupierVehicleMinPerTown",
	"m_iOccupierVehicleMaxPerTown",
	"m_aVehiclePrefabs",
	"m_aCivilianVehiclePrefabs",
	"CIV_CHARACTER",
	"MILITARY_VEHICLE",
	"m_iRuntimeCivilianCharacterCount",
	"m_iRuntimeCivilianVehicleCount",
	"m_iRuntimeMilitaryVehicleCount",
	"m_iRuntimeSpawnFailureCount",
	"m_sLastRuntimeSpawnFailurePrefab",
	"civilianRuntimeChanged"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCivilianRuntimeEntry) -and $configResourceText -notmatch [regex]::Escape($requiredCivilianRuntimeEntry)) {
		throw "Missing physical civilian town runtime entry: $requiredCivilianRuntimeEntry"
	}
}
$civilianRuntimeServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CivilianService.c"
if ($civilianRuntimeServiceText -match 'HST_CivilianTownGroup' -or $civilianRuntimeServiceText -match 'm_aCivilianGroupPrefabs' -or $configResourceText -match 'm_aCivilianGroupPrefabs') {
	throw "Civilian runtime must not spawn the broken HST civilian SCR_AIGroup prefabs"
}
if ($civilianRuntimeServiceText -match '"Prefabs/Characters/Factions/CIV/Character_CIV' -or $configResourceText -match '"Prefabs/Characters/Factions/CIV/Character_CIV' -or $defaultCatalog -match '"Prefabs/Characters/Factions/CIV/Character_CIV') {
	throw "Civilian character runtime must not use path-only Character_CIV resources"
}
$civilianCharacterPoolEntries = [regex]::Matches($configResourceText + "`n" + $defaultCatalog, '"\{[0-9A-F]{16}\}Prefabs/Characters/Factions/CIV/[^"]+Character_CIV_[^"]+\.et"')
if ($civilianCharacterPoolEntries.Count -lt 6) {
	throw "Civilian character runtime must ship at least 6 GUID-qualified default CIV character resources"
}
if ($civilianRuntimeServiceText -match 'DoSpawn\(prefab, position, "0 0 0"\)') {
	throw "Ambient civilian/military runtime spawns must pass deterministic angles instead of hard-coded zero angles"
}
if ($missionRuntimeServiceText -match 'DoSpawn\([^;\r\n]*"0 0 0"\)') {
	throw "Mission runtime props must pass deterministic angles instead of hard-coded zero angles"
}
foreach ($requiredMissionUprightEntry in @(
	"ShouldApplyUprightMissionAssetTransform",
	"ApplyUprightEntityTransform(entity, position, angles)"
)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredMissionUprightEntry)) {
		throw "Mission runtime props must keep upright transform guard: $requiredMissionUprightEntry"
	}
}
$zoneCompositionServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ZoneCompositionService.c"
foreach ($requiredZoneCompositionRadioEntry in @(
	"ShouldSkipRadioTowerStaticForMissionTarget",
	"HasLiveMissionDestroyTarget",
	"active mission target owns radio tower",
	"RecordSkip",
	"ApplyUprightEntityTransform(entity, slot.m_vPosition, slot.m_vAngles)"
)) {
	if ($zoneCompositionServiceText -notmatch [regex]::Escape($requiredZoneCompositionRadioEntry)) {
		throw "Zone composition must keep radio mission duplicate-tower guard: $requiredZoneCompositionRadioEntry"
	}
}
$arsenalServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ArsenalService.c"
if ($lootServiceText -match 'm_vAngles = "0 0 0"') {
	throw "Garage capture must not store all-zero vehicle angles"
}
if ($arsenalServiceText -notmatch "ResolveRedeployAngles" -or $arsenalServiceText -notmatch "BuildFallbackRedeployAngles") {
	throw "Garage redeploy must use stored angles with deterministic fallback for legacy zero-angle records"
}
if ($civilianRuntimeServiceText -match 'ResolveTownSpawnPosition\(zone, civilianCount \+ j' -or $civilianRuntimeServiceText -match 'ResolveTownSpawnPosition\(zone, civilianCount \+ civilianVehicleCount \+ k') {
	throw "Civilian runtime vehicles must use scattered town vehicle placement, not the old grid helper"
}
foreach ($requiredTownVehicleEntry in @(
	'ResolveTownVehicleSpawnPosition(zone, j, false)',
	'ResolveTownVehicleSpawnPosition(zone, civilianVehicleCount + k, true)',
	'IsAircraftVehicleResource',
	'!IsAircraftVehicleResource(prefab)',
	'no non-aircraft faction vehicle available'
)) {
	if ($civilianRuntimeServiceText -notmatch [regex]::Escape($requiredTownVehicleEntry)) {
		throw "Civilian runtime must keep scattered/non-heli vehicle contract entry: $requiredTownVehicleEntry"
	}
}
$balanceConfigText = Get-Content -Raw "Configs/HST/Balance/HST_CE311_Balance.conf"
$civilianPoolDefaultText = [regex]::Match($defaultCatalog, "EnsureCivilianPools[\s\S]*?static HST_PrefabPoolEntry").Value
foreach ($requiredCivilianVehiclePoolResource in @(
	"Prefabs/Vehicles/Wheeled/S105/S105_base.et",
	"Prefabs/Vehicles/Wheeled/S1203/S1203_base.et"
)) {
	if ($configResourceText -notmatch [regex]::Escape($requiredCivilianVehiclePoolResource) -and $scriptText -notmatch [regex]::Escape($requiredCivilianVehiclePoolResource)) {
		throw "Civilian vehicle pool must include stock civilian ground vehicle resource: $requiredCivilianVehiclePoolResource"
	}
}
foreach ($forbiddenCivilianSpawnResource in @(
	"Prefabs/Vehicles/Wheeled/M151A2/M151A2.et"
)) {
	if ($civilianRuntimeServiceText -match [regex]::Escape($forbiddenCivilianSpawnResource) -or $configResourceText -match [regex]::Escape($forbiddenCivilianSpawnResource) -or $defaultCatalog -match [regex]::Escape($forbiddenCivilianSpawnResource)) {
		throw "Configured vehicle pools must not use invalid plain M151 resources: $forbiddenCivilianSpawnResource"
	}
}
if ($civilianPoolDefaultText -match "M1025" -or $civilianPoolDefaultText -match "Humvee" -or $civilianPoolDefaultText -match "M151A2" -or $balanceConfigText -match "M1025" -or $balanceConfigText -match "Humvee" -or $balanceConfigText -match "M151A2") {
	throw "Civilian vehicle pools must not retain Humvee/M1025/M151 military fallback resources"
}
$usFactionConfigText = Get-Content -Raw "Configs/HST/Factions/HST_US.conf"
if ($defaultCatalog -match 'm_aVehiclePrefabs\.Insert\("Prefabs/Vehicles/Wheeled/M11xx/Car_M1151\.et"\)' -or $usFactionConfigText -match "Car_M1151.et") {
	throw "Known invalid Car_M1151.et resource must not remain in US faction vehicle pools"
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
