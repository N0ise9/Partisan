$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$layoutMetaResources = @()
foreach ($layoutMeta in Get-ChildItem -Path "UI/layouts" -Recurse -Filter "*.layout.meta") {
	$layoutMetaText = Get-Content -Raw $layoutMeta.FullName
	if ($layoutMetaText -match 'Name "\{([^}]+)\}([^"]+)"') {
		$layoutMetaResources += [pscustomobject]@{
			Guid = $Matches[1]
			Path = $Matches[2]
			File = $layoutMeta.FullName
		}
	}
}
foreach ($layoutGuidGroup in ($layoutMetaResources | Group-Object Guid | Where-Object { $_.Count -gt 1 })) {
	$paths = ($layoutGuidGroup.Group | ForEach-Object { $_.Path }) -join ", "
	throw "Duplicate layout resource GUID $($layoutGuidGroup.Name): $paths"
}
Write-Host "Layout resource GUID uniqueness OK: $($layoutMetaResources.Count)"

$orphanedEddsMetas = @()
foreach ($eddsMeta in Get-ChildItem -Path "." -Recurse -File -Filter "*.edds.meta") {
	$resourcePath = $eddsMeta.FullName.Substring(0, $eddsMeta.FullName.Length - ".meta".Length)
	if (-not (Test-Path -LiteralPath $resourcePath -PathType Leaf)) {
		$orphanedEddsMetas += $eddsMeta.FullName.Substring($root.Length + 1)
	}
}
if ($orphanedEddsMetas.Count -gt 0) {
	throw "EDDS metafile(s) without matching resource: $($orphanedEddsMetas -join ', ')"
}
Write-Host "EDDS metafile resource pairing OK"

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
			}
			elseif ($Text[$i] -eq "}") {
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
			}
			elseif ($char -eq "\") {
				$escaped = $true
			}
			elseif ($char -eq '"') {
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
		}
		elseif ($char -eq "}") {
			$close++
		}
	}

	return [pscustomobject]@{
		Open  = $open
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

$files = Get-ChildItem -Recurse -File -Include *.c, *.conf, *.ent, *.et, *.meta, *.layer, *.layout, *.gproj, *.md, .gitignore |
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
	'Configs/Map/HST_PlayerMapMarkerConfig.conf',
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
			"Prefabs/Systems/MilitaryBase/HST_ConflictMarkerBase.et",
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

$petrosGroupPrefabPath = "Prefabs/Groups/HST/HST_PetrosGroup.et"
if (!(Test-Path $petrosGroupPrefabPath)) {
	throw "Missing dedicated Petros AIGroup prefab: $petrosGroupPrefabPath"
}

$petrosGroupPrefabMetaPath = "$petrosGroupPrefabPath.meta"
if (!(Test-Path $petrosGroupPrefabMetaPath)) {
	throw "Missing dedicated Petros AIGroup prefab metadata: $petrosGroupPrefabMetaPath"
}

if ((Get-Content -Raw $petrosGroupPrefabMetaPath) -notmatch '\{6985327711303900\}Prefabs/Groups/HST/HST_PetrosGroup\.et') {
	throw "Dedicated Petros AIGroup prefab metadata must expose the GUID-qualified resource name"
}

$petrosGroupPrefabText = Get-Content -Raw $petrosGroupPrefabPath
foreach ($requiredPetrosGroupPrefabEntry in @(
		'SCR_AIGroup HST_PetrosGroup : "{242BC3C6BCE96EA5}Prefabs/Groups/INDFOR/Group_FIA_Base.et"',
		"m_bSpawnImmediately 0",
		"m_bDeleteWhenEmpty 0",
		'm_faction "FIA"',
		"m_aUnitPrefabSlots",
		'"{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et"'
	)) {
	if ($petrosGroupPrefabText -notmatch [regex]::Escape($requiredPetrosGroupPrefabEntry)) {
		throw "Dedicated Petros AIGroup prefab must be a non-empty FIA group-owned Petros spawn root: $requiredPetrosGroupPrefabEntry"
	}
}

$runtimeEmptyGroupPrefabs = @(
	@{
		Path = "Prefabs/Groups/HST/HST_RuntimeEmptyGroup.et"
		MetaPattern = '\{6985327711303910\}Prefabs/Groups/HST/HST_RuntimeEmptyGroup\.et'
		Header = 'SCR_AIGroup HST_RuntimeEmptyGroup : "{242BC3C6BCE96EA5}Prefabs/Groups/INDFOR/Group_FIA_Base.et"'
		Faction = 'm_faction "FIA"'
	},
	@{
		Path = "Prefabs/Groups/HST/HST_RuntimeEmptyGroup_US.et"
		MetaPattern = '\{2E3755F24A57D1A0\}Prefabs/Groups/HST/HST_RuntimeEmptyGroup_US\.et'
		Header = 'SCR_AIGroup HST_RuntimeEmptyGroup_US : "{EACD97CF4A702FAE}Prefabs/Groups/BLUFOR/Group_US_Base.et"'
		Faction = 'm_faction "US"'
	},
	@{
		Path = "Prefabs/Groups/HST/HST_RuntimeEmptyGroup_USSR.et"
		MetaPattern = '\{94AA122B0CFB7E40\}Prefabs/Groups/HST/HST_RuntimeEmptyGroup_USSR\.et'
		Header = 'SCR_AIGroup HST_RuntimeEmptyGroup_USSR : "{8DE0C0830FE0C33D}Prefabs/Groups/OPFOR/Group_USSR_Base.et"'
		Faction = 'm_faction "USSR"'
	}
)

foreach ($runtimeEmptyGroupPrefab in $runtimeEmptyGroupPrefabs) {
	$runtimeEmptyGroupPrefabPath = $runtimeEmptyGroupPrefab.Path
	if (!(Test-Path $runtimeEmptyGroupPrefabPath)) {
		throw "Missing faction-specific non-deleting runtime AIGroup prefab: $runtimeEmptyGroupPrefabPath"
	}

	$runtimeEmptyGroupPrefabMetaPath = "$runtimeEmptyGroupPrefabPath.meta"
	if (!(Test-Path $runtimeEmptyGroupPrefabMetaPath)) {
		throw "Missing faction-specific non-deleting runtime AIGroup prefab metadata: $runtimeEmptyGroupPrefabMetaPath"
	}

	if ((Get-Content -Raw $runtimeEmptyGroupPrefabMetaPath) -notmatch $runtimeEmptyGroupPrefab.MetaPattern) {
		throw "Runtime AIGroup prefab metadata must expose the GUID-qualified resource name: $runtimeEmptyGroupPrefabPath"
	}

	$runtimeEmptyGroupPrefabText = Get-Content -Raw $runtimeEmptyGroupPrefabPath
	foreach ($requiredRuntimeEmptyGroupPrefabEntry in @(
			$runtimeEmptyGroupPrefab.Header,
			"m_bSpawnImmediately 0",
			"m_bDeleteWhenEmpty 0",
			$runtimeEmptyGroupPrefab.Faction
		)) {
		if ($runtimeEmptyGroupPrefabText -notmatch [regex]::Escape($requiredRuntimeEmptyGroupPrefabEntry)) {
			throw "Runtime AIGroup fallback prefab must inherit a faction base, start empty, and not queue native empty-group deletion: $requiredRuntimeEmptyGroupPrefabEntry"
		}
	}

	if ($runtimeEmptyGroupPrefabText -notmatch 'm_aUnitPrefabSlots\s*\{\s*\}') {
		throw "Runtime AIGroup fallback prefab must explicitly clear inherited unit slots: $runtimeEmptyGroupPrefabPath"
	}
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
		"SpawnPetrosViaGroupPrefab",
		"TryResolvePetrosFromTrackedGroup",
		"IsPetrosGroupSpawnPending",
		"SpawnPetrosCharacterPrefab",
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
if ($hqServiceText -notmatch "SpawnPetrosViaGroupPrefab\(petrosPosition, `"dedicated Petros group spawn`"\)" -or $hqServiceText -notmatch "TryResolvePetrosFromTrackedGroup\(position, source\)") {
	throw "HQ runtime Petros primary spawn must use the dedicated Petros AIGroup slot path"
}
if ($hqServiceText -notmatch "SpawnPetrosCharacterPrefab\(PETROS_BASE_PREFAB, petrosPosition\)") {
	throw "HQ runtime Petros base fallback must keep the forced character prefab helper for diagnostics"
}
$spawnPetrosMethodMatch = [regex]::Match($hqServiceText, "protected GenericEntity SpawnPetros[\s\S]*?protected GenericEntity SpawnPetrosCharacterPrefab")
if ($spawnPetrosMethodMatch.Success -and $spawnPetrosMethodMatch.Value -match "HST_WorldPositionService\.SpawnPrefab") {
	throw "HQ runtime Petros must not use generic non-forced prefab spawning; dedicated server tests remove that character before HQ runtime stabilizes"
}
$spawnPetrosCharacterMethodMatch = [regex]::Match($hqServiceText, "protected GenericEntity SpawnPetrosCharacterPrefab[\s\S]*?protected bool PreparePetrosRuntimeEntity")
if (!$spawnPetrosCharacterMethodMatch.Success -or $spawnPetrosCharacterMethodMatch.Value -notmatch "SpawnEntityPrefabEx\(resourceName, true, world, params\)") {
	throw "HQ runtime Petros forced character spawn helper must call SpawnEntityPrefabEx with forced runtime spawning"
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
if ($hqServiceText -notmatch 'm_bHQRuntimeObjectsSpawned && AreRuntimeObjectsTracked\(state\) && IsUsableArsenalEntity\(m_ArsenalEntity\)') {
	throw "HQ runtime must not skip state-aware Petros uniqueness or arsenal usability just because cached entity handles exist"
}
foreach ($requiredPetrosGroupRuntimeEntry in @(
		'ReattachUniqueLivingWorldPetros(state, "reattach")',
		'ReattachUniqueLivingWorldPetros(state, "ready check")',
		'ReattachUniqueLivingWorldPetros(state, "final runtime proof")',
		'ReattachUniqueLivingWorldPetros(state, "final ready check")',
		'CountLivingPetrosWorldRuntimeEntities(state) == 1',
		'bool petrosRuntimeReady = IsPetrosRuntimeTracked(state);',
		'GetTrackedRuntimeObjectCount(HST_CampaignState state = null)',
		'TryResolvePetrosFromTrackedGroup(state.m_vPetrosPosition, "tracked group")',
		'SpawnPetrosViaGroupPrefab(petrosPosition, "dedicated Petros group spawn")',
		'TryResolvePetrosFromTrackedGroup(position, source)',
		'PreparePetrosRuntimeEntity(petros, petrosPosition, "base FIA Petros fallback")',
		'PETROS_GROUP_PREFAB = "{6985327711303900}Prefabs/Groups/HST/HST_PetrosGroup.et"',
		"IsPetrosGroupSpawnPending",
		"WarnPetrosAIGroupFallback",
		"HasPetrosRuntimeAIGroup",
		"BuildPetrosAIGroupDebugSummary",
		"AttachPetrosToAIGroup",
		"group.AddAgentFromControlledEntity(petros)",
		"group.ActivateAI()",
		"agent.ActivateAI()",
		"group.SetMaxUnitsToSpawn(1)",
		"group.SpawnUnits()",
		"ResetPetrosRespawnState",
		"m_iPetrosLastSpawnSecond = -999999",
		"AIGroup parentGroup",
		"parentGroup = agent.GetParentGroup();",
		"m_PetrosEntity && !IsLivingRuntimeEntity(m_PetrosEntity)",
		"return IsLivingRuntimeEntity(m_PetrosEntity);",
		'CampaignDebugStatus(m_HQ.HasPetrosRuntimeAIGroup(), "WARN")'
	)) {
	$petrosRuntimeProofText = $hqServiceText + "`n" + $coordinatorText
	if ($petrosRuntimeProofText -notmatch [regex]::Escape($requiredPetrosGroupRuntimeEntry)) {
		throw "HQ runtime must preserve a living Petros character and report AIGroup attachment separately: $requiredPetrosGroupRuntimeEntry"
	}
}
if ($hqServiceText -match 'SCR_AIGroup\.Cast\(agent\.GetParentGroup\(\)\)') {
	throw "HQ Petros runtime must keep AIAgent.GetParentGroup() as base AIGroup; Workbench rejects casting that native return to SCR_AIGroup"
}
$petrosLivenessMatch = [regex]::Match($hqServiceText, "protected bool IsLivingRuntimeEntity[\s\S]*?protected IEntity ResolvePetrosRuntimeEntity")
if (!$petrosLivenessMatch.Success) {
	throw "Could not locate HQ Petros liveness helper"
}
if ($petrosLivenessMatch.Value -match 'if \(!controller\)\s+return false;') {
	throw "HQ Petros liveness must not treat an initializing character with no controller as dead; fall through to damage-state proof"
}
if ($petrosLivenessMatch.Value -notmatch 'if \(controller\)\s+return controller\.GetLifeState\(\) != ECharacterLifeState\.DEAD;') {
	throw "HQ Petros liveness must use controller life state only when the controller is available"
}
if ($hqServiceText -notmatch 'else if \(m_PetrosEntity\)[\s\S]*?PreparePetrosEntity\(m_PetrosEntity, state\.m_vPetrosPosition\);') {
	throw "HQ Petros runtime refresh must re-prepare the tracked character so delayed controller initialization still gets stationary HQ controls"
}
if ($hqServiceText -match 'agent\.GetParentGroup\(\)\s*[!=]=\s*group' -or $hqServiceText -match 'group\s*[!=]=\s*agent\.GetParentGroup\(\)') {
	throw "HQ Petros runtime must store AIAgent.GetParentGroup() in a base AIGroup variable before comparing it with tracked SCR_AIGroup"
}
if ($hqServiceText -match '\{000CD338713F2B5A\}Prefabs/AI/Groups/Group_Base\.et') {
	throw "HQ Petros runtime must use the HST-owned non-deleting Petros group prefab instead of raw Group_Base"
}
if ($coordinatorText -notmatch "RequestCommanderRebuildHQAssets" -or $coordinatorText -notmatch "ResolveHQRebuildPlacement") {
	throw "Coordinator must expose a Build Mode guarded HQ runtime rebuild action"
}
if ($coordinatorText -match "BootstrapInitialHideout\(m_State, HST_DefaultCatalog\.GetDefaultHideoutId\(\)\)") {
	throw "Coordinator must not bootstrap a starter HQ during setup; commander map placement owns initial HQ selection"
}
if ($coordinatorText -notmatch "ResetInitialHQSelection\(m_State\)") {
	throw "Coordinator must reset legacy setup saves with bootstrapped HQ state"
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
if ($civilianCharacterPoolEntriesEarly.Count -lt 2) {
	throw "Civilian character pools must expose the randomized GUID-qualified stock CIV character resource in config and runtime defaults"
}
if ($balanceConfigTextEarly -notmatch "Character_CIV_Randomized.et" -or $defaultCatalogEarly -notmatch "Character_CIV_Randomized.et") {
	throw "Civilian character pools must use the randomized CIV character resource"
}
if ($balanceConfigTextEarly -match "Character_CIV_CottonShirt|Character_CIV_DenimJacket|Character_CIV_Turtleneck" -or $defaultCatalogEarly -match "Character_CIV_CottonShirt|Character_CIV_DenimJacket|Character_CIV_Turtleneck") {
	throw "Civilian character pools must not list explicit clothing variants; use Character_CIV_Randomized.et only"
}
$civilianServiceTextEarly = Get-Content -Raw "Scripts/Game/HST/Services/HST_CivilianService.c"
if ($civilianServiceTextEarly -notmatch "MIN_CIVILIAN_CHARACTER_PREFABS = 1") {
	throw "Civilian character prefab minimum must stay at 1 for randomized-only civilian spawning"
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
if ($playerControllerPrefabText -notmatch '(?m)^\s*ID "6985327711303201"\s*$') {
	throw "HST player controller prefab must have a stable root entity ID for replication"
}

foreach ($requiredPlayerControllerEntry in @(
		"SCR_PlayerController HST_PlayerController",
		"DefaultPlayerControllerMP_ScenarioFramework.et",
		"HST_CommandMenuRequestComponent",
		"HST_CommandMenuComponent",
		"HST_SetupMapComponent"
	)) {
	if ($playerControllerPrefabText -notmatch [regex]::Escape($requiredPlayerControllerEntry)) {
		throw "HST player controller prefab is missing request/RPC bridge entry: $requiredPlayerControllerEntry"
	}
}
Write-Host "HST player controller request bridge OK"

$defaultCatalog = Get-Content -Raw "Scripts/Game/HST/Config/HST_DefaultCatalog.c"
$missionConfig = Get-Content -Raw "Configs/HST/Missions/HST_CE311_Missions.conf"
$mapConfig = Get-Content -Raw "Configs/HST/Maps/HST_Everon.conf"
$setupMapComponentText = Get-Content -Raw "Scripts/Game/HST/Components/HST_SetupMapComponent.c"
$setupMapZoneOverlayText = Get-Content -Raw "Scripts/Game/HST/Map/HST_MapZoneOverlayUIComponent.c"
$setupMapLayoutText = Get-Content -Raw "UI/layouts/HST_SetupHQMap.layout"
$setupPromptBannerLayoutText = Get-Content -Raw "UI/layouts/HST_SetupPromptBanner.layout"
$setupConfirmModalLayoutText = Get-Content -Raw "UI/layouts/HST_SetupConfirmModal.layout"
$notificationToastLayoutText = Get-Content -Raw "UI/layouts/HST_NotificationToast.layout"
$notificationToastLayoutMetaText = Get-Content -Raw "UI/layouts/HST_NotificationToast.layout.meta"
$setupNativeMapConfigText = Get-Content -Raw "Configs/Map/HST_SetupHQMap.conf"
if (!(Test-Path "Configs/Map/HST_GameplayMap.conf")) {
	throw "Gameplay map config is missing: Configs/Map/HST_GameplayMap.conf"
}
if (!(Test-Path "Configs/Map/HST_GameplayMap.conf.meta")) {
	throw "Gameplay map config meta is missing: Configs/Map/HST_GameplayMap.conf.meta"
}
if (Test-Path "UI/layouts/HST_SetupConfirmBlocker.layout") {
	throw "Setup confirmation blocker must remain folded into HST_SetupConfirmModal.layout"
}
if (Test-Path "UI/layouts/HST_SetupConfirmBlocker.layout.meta") {
	throw "Obsolete setup confirmation blocker meta file must be deleted"
}
$gameplayMapConfigText = Get-Content -Raw "Configs/Map/HST_GameplayMap.conf"
$gameplayMapConfigMetaText = Get-Content -Raw "Configs/Map/HST_GameplayMap.conf.meta"
$everonDefaultLayerText = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/default.layer"
$devDefaultLayerText = Get-Content -Raw "Worlds/HST_Dev/HST_Dev_Layers/default.layer"
$requestBridgeSetupText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CommandMenuRequestComponent.c"
$playerSpawnSetupText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PlayerSpawnService.c"
$uiWorkspaceMetricsText = Get-Content -Raw "Scripts/Game/HST/Services/HST_UIWorkspaceMetrics.c"

foreach ($requiredSetupEntry in @(
		"HST_SetupMapComponent",
		"RequestSetupValidatePosition",
		"RequestSetupConfirmPosition",
		"Are you sure you want to place the HQ here?",
		"Select a location on the map to place the HQ",
		"Please wait, the commander is selecting the HQ location..."
	)) {
	if ($setupMapComponentText -notmatch [regex]::Escape($requiredSetupEntry) -and $requestBridgeSetupText -notmatch [regex]::Escape($requiredSetupEntry)) {
		throw "Initial setup HQ placement flow is missing entry: $requiredSetupEntry"
	}
}
foreach ($requiredSetupServerEntry in @(
		"BuildSetupMapPayload",
		"RequestSetupValidateHQPosition",
		"RequestSetupConfirmHQPosition",
		"SelectInitialHQPosition",
		"ValidateInitialHQPosition",
		"RegisterConnectedPlayersOnly"
	)) {
	if (($coordinatorText + "`n" + $hqServiceText + "`n" + $playerSpawnSetupText) -notmatch [regex]::Escape($requiredSetupServerEntry)) {
		throw "Initial setup HQ server flow is missing entry: $requiredSetupServerEntry"
	}
}
if ($mapConfig -notmatch "m_vWorldMin\s+0\s+0\s+0" -or $mapConfig -notmatch "m_vWorldMax\s+12800\s+0\s+12800") {
	throw "Everon map definition must expose setup map world bounds"
}
foreach ($requiredSetupMapProjectionEntry in @(
		"WorldToScreen",
		"HST_UIWorkspaceMetrics.RawToLayoutPx(workspace, sx - radiusPx)",
		"HST_UIWorkspaceMetrics.RawToLayoutPx(workspace, radiusPx * 2)",
		"HST_UIWorkspaceMetrics.RawToLayoutPx(workspace, sx)",
		"FrameSlot.SetPos",
		"CreateCircle",
		"ResolveRadiusPixels",
		"BuildCircleVertices"
	)) {
	if ($setupMapZoneOverlayText -notmatch [regex]::Escape($requiredSetupMapProjectionEntry)) {
		throw "Setup zone overlay must render invalid areas through native map projection: $requiredSetupMapProjectionEntry"
	}
}

if ($setupMapZoneOverlayText -match [regex]::Escape("workspace.DPIUnscale")) {
	throw "Setup zone overlay must use HST_UIWorkspaceMetrics.RawToLayoutPx for projected raw map coordinates"
}
foreach ($requiredSetupMapOverlayRedrawEntry in @(
		"VIEWPORT_PAN_EPSILON",
		"VIEWPORT_ZOOM_EPSILON",
		"m_bViewportStateValid",
		"MarkViewportDirty",
		"AbsFloat(panX - m_fLastPanX)",
		"AbsFloat(zoom - m_fLastZoom)",
		"if (!CanProject())",
		'HST_UIDebug.LogPopulation("map_zone_overlay"',
		"widgetsBefore",
		"m_iAppliedRevision == s_iRevision"
	)) {
	if ($setupMapZoneOverlayText -notmatch [regex]::Escape($requiredSetupMapOverlayRedrawEntry)) {
		throw "Setup zone overlay must coalesce pan/zoom callbacks and avoid projection-not-ready redraw churn: $requiredSetupMapOverlayRedrawEntry"
	}
}
foreach ($forbiddenSetupMapProjectionEntry in @(
		"NativeScreenToParentLocal",
		"GetScreenPos(",
		"SCR_MapConstants.DRAWING_WIDGET_NAME",
		"TessellateCircle",
		"SetSizeInUnits",
		"SetOffsetPx"
	)) {
	if ($setupMapZoneOverlayText -match [regex]::Escape($forbiddenSetupMapProjectionEntry)) {
		throw "Setup zone overlay must not use stale screen-parent or drawing-canvas projection paths: $forbiddenSetupMapProjectionEntry"
	}
}
foreach ($requiredSetupLayoutEntry in @(
		'Name "HST_SetupPromptBannerRoot"',
		'Name "HST_SetupPromptPanel"',
		'Name "HST_SetupPromptRule"',
		'Name "HST_SetupPromptText"',
		"Anchor 0 0 1 0",
		"SizeY 76",
		"OffsetBottom -76",
		"SizeY 42",
		"OffsetBottom -60"
	)) {
	if ($setupPromptBannerLayoutText -notmatch [regex]::Escape($requiredSetupLayoutEntry)) {
		throw "Setup prompt banner layout is missing anchored prompt entry: $requiredSetupLayoutEntry"
	}
}
foreach ($forbiddenSetupPromptLayoutEntry in @(
		"OffsetBottom 76",
		"OffsetBottom 60"
	)) {
	if ($setupPromptBannerLayoutText -match [regex]::Escape($forbiddenSetupPromptLayoutEntry)) {
		throw "Setup prompt banner fixed-height widgets must use negative bottom-edge offsets: $forbiddenSetupPromptLayoutEntry"
	}
}
foreach ($forbiddenSetupMapLayoutEntry in @(
		'Name "HST_SetupOverlayRoot"',
		'Name "HST_SetupPromptPanel"',
		'Name "HST_SetupModalRoot"'
	)) {
	if ($setupMapLayoutText -match [regex]::Escape($forbiddenSetupMapLayoutEntry)) {
		throw "Setup map layout must not own setup prompt/modal chrome anymore: $forbiddenSetupMapLayoutEntry"
	}
}
foreach ($requiredConfirmModalLayoutEntry in @(
		'FrameWidgetClass "{B55C6FB34BF94001}"',
		'Name "HST_SetupConfirmModalRoot"',
		'Name "ModalDimmer"',
		'Name "Dialog"',
		'Name "Message"',
		'Name "NoButton"',
		'Name "YesButton"',
		"Anchor 0 0 1 1",
		"Color 0 0 0 0.16",
		'"Ignore Cursor" 0',
		"Anchor 0.5 0.5 0.5 0.5",
		"SizeX 620",
		"OffsetRight -620",
		"SizeY 280",
		"OffsetBottom -280",
		"Alignment 0.5 0.5",
		"SizeX 160",
		"SizeY 50",
		"OffsetRight -298",
		"OffsetRight -482",
		"OffsetBottom -256",
		"OffsetLeft 42",
		"SizeX -84",
		"OffsetRight 42",
		"SizeY 86",
		"OffsetBottom -138"
	)) {
	if ($setupConfirmModalLayoutText -notmatch [regex]::Escape($requiredConfirmModalLayoutEntry)) {
		throw "Setup confirmation modal must be a centered real-button layout: $requiredConfirmModalLayoutEntry"
	}
}
if ($setupConfirmModalLayoutText -notmatch '(?s)Name "Message".*?OffsetLeft 42.*?SizeX -84.*?OffsetRight 42.*?"Horizontal Alignment" Center.*?"Vertical Alignment" Center') {
	throw "Setup confirmation modal message must use symmetric centered bounds inside the dialog."
}
foreach ($forbiddenConfirmModalLayoutEntry in @(
		'PanelWidgetClass "{B55C6FB34BF94001}"',
		'Name "ModalCursorProxy"',
		'Name "ModalCursorProxyShadow"',
		'Name "ModalCursorProxyV"',
		'Name "ModalCursorProxyH"',
		"OffsetLeft -310",
		"OffsetTop -140",
		"OffsetRight 310",
		"OffsetBottom 140",
		"OffsetBottom 132",
		"OffsetRight 298",
		"OffsetRight 482",
		"OffsetBottom 256"
	)) {
	if ($setupConfirmModalLayoutText -match [regex]::Escape($forbiddenConfirmModalLayoutEntry)) {
		throw "Setup confirmation modal must use the centered size/alignment slot pattern and negative fixed bounds: $forbiddenConfirmModalLayoutEntry"
	}
}
foreach ($requiredSetupChromeEntry in @(
		"workspace.CreateWidgets(SETUP_NATIVE_MAP_LAYOUT, workspace)",
		"SETUP_PROMPT_BANNER_LAYOUT",
		'SETUP_CONFIRM_MODAL_OWNER = "HST_SetupConfirmModal"',
		"CONFIRM_BLOCKER_WIDGET_ID",
		"SETUP_ZONE_OVERLAY_ENABLED = true",
		"PublishSetupZoneOverlay();",
		"PublishSetupCandidateOverlay();",
		"BuildSetupZoneOverlaySignature",
		"m_sLastSetupCandidateOverlaySignature",
		"HST_MapZoneOverlayUIComponent.SetSetupZones",
		"HST_MapZoneOverlayUIComponent.SetSetupCandidate",
		"workspace.CreateWidgets(SETUP_PROMPT_BANNER_LAYOUT, workspace)",
		"workspace.CreateWidgets(SETUP_CONFIRM_MODAL_LAYOUT, workspace)",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.SETUP_MAP, SETUP_CONFIRM_MODAL_OWNER, modal, false, true, true)",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.SETUP_MAP, SETUP_CONFIRM_MODAL_OWNER)",
		"m_wConfirmBlockerRoot = modal",
		"OnSetupOverlayMouseWheel",
		"m_iFrameSerial",
		"m_Component.OnSetupOverlayClicked(w, w.GetUserID(), x, y, button)",
		"OnSetupOverlayClicked(Widget widget, int widgetId, int x = -1, int y = -1, int button = 0)",
		"IsDuplicateWidgetActivation(resolvedWidgetId, button)",
		"BindConfirmModalButton(modal, `"NoButton`", CONFIRM_NO_WIDGET_ID)",
		"BindConfirmModalButton(modal, `"YesButton`", CONFIRM_YES_WIDGET_ID)"
	)) {
	if ($setupMapComponentText -notmatch [regex]::Escape($requiredSetupChromeEntry)) {
		throw "Setup map component must create top-level prompt/modal chrome above the native setup map: $requiredSetupChromeEntry"
	}
}
foreach ($forbiddenSetupChromeEntry in @(
		"workspace.CreateWidgets(SETUP_PROMPT_BANNER_LAYOUT, m_wSetupRoot)",
		"workspace.CreateWidgets(SETUP_CONFIRM_MODAL_LAYOUT, m_wSetupRoot)",
		'QueueOverlayRedraw("pan")',
		'QueueOverlayRedraw("zoom")',
		"QueueOverlayRedraw",
		"FlushOverlayRedraw",
		"RedrawNativeMapOverlay",
		"m_bOverlayDirty",
		"m_bOverlayRedrawQueued",
		"overlay render #",
		"m_wConfirmCursorProxy",
		"ModalCursorProxy",
		"UpdateConfirmModalCursorProxy",
		"FrameSlot.SetPos(m_wConfirmCursorProxy",
		"WidgetManager.SetCursor(0)",
		"SETUP_CURSOR_CONTEXT",
		"SETUP_INTERACTABLE_DIALOG_CONTEXT",
		'"DialogContext"',
		'"InteractableDialogContext"'
	)) {
	if ($setupMapComponentText -match [regex]::Escape($forbiddenSetupChromeEntry)) {
		throw "Setup chrome must not be hidden under the native map root or redraw on every map pan/zoom tick: $forbiddenSetupChromeEntry"
	}
}
if ($setupMapComponentText -match [regex]::Escape("OnSetupOverlayClicked(w, w.GetUserID(), x, y);")) {
	throw "Setup modal widget handler must pass button state through the duplicate-activation guard"
}
$setupFinalizeMatch = [regex]::Match($setupMapComponentText, "protected void FinalizeSetupMap[\s\S]*?\r?\n\t}\r?\n\r?\n\tvoid OnServerSetupResult")
if (!$setupFinalizeMatch.Success) {
	throw "Setup map finalization method is missing"
}
if ($setupFinalizeMatch.Value -match [regex]::Escape('Debug.ClearKey(KeyCode.KC_I)')) {
	throw "Setup finalization must not clear KC_I after HQ placement; command menu recovery owns that input latch"
}
foreach ($requiredSetupMapLayerEntry in @(
		"HST_UIConstants.Z_SETUP_MAP",
		"HST_UIConstants.Z_SETUP_PROMPT",
		"SETUP_CONFIRM_MODAL_ROOT_Z",
		"SETUP_CONFIRM_MODAL_LABEL_Z",
		"ApplySetupLayerOrder",
		"ApplyConfirmModalLayerOrder",
		"SetWidgetLayer(m_wPromptPanel",
		"SetWidgetLayer(m_wPromptRule",
		"m_wPromptText.SetZOrder",
		"ApplySetupMapDialogState",
		"m_bSetupMapDialogCursorActive",
		"m_bSetupSelectionSuppressedForModal",
		"cursorModule.ToggleLocationSelection(false)",
		"cursorModule.HandleDialog(true)",
		"cursorModule.HandleDialog(false)",
		"ReleaseSetupMapDialogState",
		"inputManager.ActivateContext(SETUP_MAP_CONTEXT)"
	)) {
	if ($setupMapComponentText -notmatch [regex]::Escape($requiredSetupMapLayerEntry)) {
		throw "Setup map UI must explicitly layer prompt/modal widgets and block native map input during confirmation: $requiredSetupMapLayerEntry"
	}
}
foreach ($forbiddenSetupMagicLayerEntry in @(
		"SETUP_Z_ORDER",
		"SETUP_PROMPT_Z_ORDER",
		"SETUP_MODAL_Z_ORDER",
		"51000",
		"52000"
	)) {
	if ($setupMapComponentText -match [regex]::Escape($forbiddenSetupMagicLayerEntry)) {
		throw "Setup map UI must use centralized HST_UIConstants layer values: $forbiddenSetupMagicLayerEntry"
	}
}
foreach ($setupLayoutPath in @(
		"UI/layouts/HST_SetupHQMap.layout",
		"UI/layouts/HST_SetupPromptBanner.layout",
		"UI/layouts/HST_SetupConfirmModal.layout"
	)) {
	$setupLayoutNoWrappingText = Get-Content -Raw $setupLayoutPath
	if ($setupLayoutNoWrappingText -match '"Text Wrapping"') {
		throw "$setupLayoutPath must not use unsupported layout Text Wrapping keywords; script should call SetTextWrapping"
	}
}
foreach ($forbiddenSetupMapHitTest in @(
		"m_iConfirmNoLeft",
		"m_iConfirmYesLeft",
		"ResolveConfirmWidgetId",
		"ResolveModalRoot",
		"HandleConfirmModalMapSelection",
		"IsWidgetPointInsideRect",
		"HST_SetupMapDrawCommandSet",
		"m_wModalRoot",
		"SETUP_OVERLAY_Z_ORDER",
		"HST_SetupChromeDrawCommandSet",
		"m_aOverlayWidgets",
		"m_aModalDrawCommandSets",
		"CreateRectWidgetAtZ",
		"CreateWrappedTextWidgetAtZ",
		"CreateModalRect",
		"BuildModalRectVertices"
	)) {
	if ($setupMapComponentText -match [regex]::Escape($forbiddenSetupMapHitTest)) {
		throw "Setup map UI must not use stale script-created overlay or fake hit-testing paths: $forbiddenSetupMapHitTest"
	}
}
foreach ($requiredWorkspaceMetricEntry in @(
		"GetRawWorkspaceSize",
		"workspace.GetWidth()",
		"workspace.GetHeight()",
		"GetLayoutSize",
		"workspace.DPIUnscale(workspace.GetWidth())",
		"workspace.DPIUnscale(workspace.GetHeight())",
		"LayoutToRawPx",
		"workspace.DPIScale(value)",
		"RawToLayoutPx",
		"workspace.DPIUnscale(value)",
		"DebugWorkspaceMetrics",
		"raw=%2x%3",
		"layout=%4x%5",
		"dpi=%6"
	)) {
	if ($uiWorkspaceMetricsText -notmatch [regex]::Escape($requiredWorkspaceMetricEntry)) {
		throw "Workspace metrics must keep raw/layout coordinate spaces explicit: $requiredWorkspaceMetricEntry"
	}
}
foreach ($requiredWorkspaceMetricCaller in @(
		"HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, `"HST_SetupMap`")",
		"HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, `"HST_CommandMenu`")",
		"HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, `"HST_LoadoutEditor`")",
		"HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, `"HST_NotificationToast`")",
		"HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, data.m_sDebugOwner)"
	)) {
	$foundWorkspaceMetricCaller = $false
	foreach ($uiText in @(
			$setupMapComponentText,
			(Get-Content -Raw "Scripts/Game/HST/Components/HST_CommandMenuComponent.c"),
			(Get-Content -Raw "Scripts/Game/HST/Components/HST_LoadoutEditorComponent.c"),
			(Get-Content -Raw "Scripts/Game/HST/Components/HST_MissionClientComponent.c"),
			(Get-Content -Raw "Scripts/Game/HST/UI/HST_ReportDialogController.c"),
			(Get-Content -Raw "Scripts/Game/HST/UI/HST_NotificationToastController.c")
		)) {
		if ($uiText -and $uiText -match [regex]::Escape($requiredWorkspaceMetricCaller)) {
			$foundWorkspaceMetricCaller = $true
			break
		}
	}
	if (!$foundWorkspaceMetricCaller) {
		throw "UI entry points must log workspace coordinate metrics through the shared helper: $requiredWorkspaceMetricCaller"
	}
}
$allowedDirectDpiFiles = @(
	"Scripts/Game/HST/Services/HST_UIWorkspaceMetrics.c",
	"Scripts/Game/HST/Map/HST_MapZoneOverlayUIComponent.c"
)
foreach ($scriptFile in Get-ChildItem -Path "Scripts/Game/HST" -Recurse -Filter "*.c") {
	$relativeScriptFile = ($scriptFile.FullName.Substring($root.Length + 1)).Replace("\", "/")
	if ($allowedDirectDpiFiles -contains $relativeScriptFile) {
		continue
	}
	$scriptText = Get-Content -Raw $scriptFile.FullName
	if ($scriptText -match "workspace\.DPI(Unscale|Scale)\(") {
		throw "Direct workspace DPI conversion is only allowed in HST_UIWorkspaceMetrics or map overlay projection code: $relativeScriptFile"
	}
}
if (!(Test-Path "docs/HST_UI_REWRITE_AUDIT.md")) {
	throw "UI rewrite audit document is missing: docs/HST_UI_REWRITE_AUDIT.md"
}
foreach ($requiredUIRootFile in @(
		"Scripts/Game/HST/UI/HST_UIConstants.c",
		"Scripts/Game/HST/UI/HST_UIScreenBase.c",
		"Scripts/Game/HST/UI/HST_UIRootService.c",
		"Scripts/Game/HST/UI/HST_UIDebug.c",
		"Scripts/Game/HST/UI/HST_ActionDialogController.c",
		"Scripts/Game/HST/UI/HST_ReportDialogController.c",
		"Scripts/Game/HST/UI/HST_NotificationToastController.c"
	)) {
	if (!(Test-Path $requiredUIRootFile)) {
		throw "UI root coordinator file is missing: $requiredUIRootFile"
	}
}
$uiConstantsText = Get-Content -Raw "Scripts/Game/HST/UI/HST_UIConstants.c"
$uiScreenBaseText = Get-Content -Raw "Scripts/Game/HST/UI/HST_UIScreenBase.c"
$uiRootServiceText = Get-Content -Raw "Scripts/Game/HST/UI/HST_UIRootService.c"
$uiDebugText = Get-Content -Raw "Scripts/Game/HST/UI/HST_UIDebug.c"
$actionDialogControllerText = Get-Content -Raw "Scripts/Game/HST/UI/HST_ActionDialogController.c"
$reportDialogControllerText = Get-Content -Raw "Scripts/Game/HST/UI/HST_ReportDialogController.c"
$notificationToastControllerText = Get-Content -Raw "Scripts/Game/HST/UI/HST_NotificationToastController.c"
foreach ($requiredUIDebugEntry in @(
		"class HST_UIDebug",
		"LAYOUT_DEBUG_ENABLED",
		"LAYOUT_WIDGET_DEBUG_ENABLED",
		"LAYOUT_GEOMETRY_DEBUG_ENABLED",
		"LAYOUT_POPULATION_DEBUG_ENABLED",
		"LAYOUT_ROW_SAMPLE_DEBUG_ENABLED",
		"LAYOUT_ROW_SAMPLE_LIMIT",
		"SetRuntimeDebugEnabled",
		"IsRuntimeDebugEnabled",
		"HST_RuntimeSettingsService.LoadDebugLoggingEnabledQuiet",
		"CanLogLayout",
		"CanLogWidget",
		"CanLogGeometry",
		"CanLogPopulation",
		"CanLogRowSample",
		"LogLayoutCreate",
		"LogLayoutRejected",
		"LogExpectedWidgetsCsv",
		"LogWidgetBound",
		"LogWidgetGeometryCsv",
		"LogReadyWidgetsCsv",
		"LogWidgetGeometry",
		"LogPopulation",
		"LogRowSample",
		"LogRowSummary",
		"LogNamedChildSummaryCsv",
		"LogChildSummary",
		"WidgetSummary",
		"WidgetHasNegativeBounds",
		"WidgetHasZeroBounds",
		"WidgetIsOutsideRoot",
		"GetChildren",
		"GetSibling",
		"GetScreenPos",
		"GetScreenSize",
		"screen=%1,%2 size=%3x%4",
		"negative=%8 offscreen=%9",
		"h-istasi ui layout debug"
	)) {
	if ($uiDebugText -notmatch [regex]::Escape($requiredUIDebugEntry)) {
		throw "UI debug helper must expose layout diagnostics: $requiredUIDebugEntry"
	}
}
foreach ($requiredUIRootEntry in @(
		"HST_EUIScreenMode",
		"SETUP_MAP",
		"COMMAND_MENU",
		"LOADOUT_EDITOR",
		"ACTION_DIALOG",
		"MISSION_DIALOG",
		"GAMEPLAY_MAP_OVERLAY",
		"Z_SETUP_MAP",
		"Z_SETUP_PROMPT",
		"Z_SETUP_MODAL",
		"Z_COMMAND_MENU",
		"Z_NOTIFICATION",
		"Z_LOADOUT_EDITOR",
		"Z_ACTION_DIALOG",
		"Z_MISSION_DIALOG"
	)) {
	if ($uiConstantsText -notmatch [regex]::Escape($requiredUIRootEntry)) {
		throw "UI constants must define shared screen modes and z-order entries: $requiredUIRootEntry"
	}
}
foreach ($requiredSetupZBandEntry in @(
		"static const int Z_SETUP_MAP = 0;",
		"static const int Z_SETUP_MODAL = 0;"
	)) {
	if ($uiConstantsText -notmatch [regex]::Escape($requiredSetupZBandEntry)) {
		throw "Setup map/modal z-order must stay below the native map cursor band: $requiredSetupZBandEntry"
	}
}
foreach ($requiredUIScreenBaseEntry in @(
		"Widget m_wRoot",
		"m_bBlocksGameplay",
		"m_bBlocksMap",
		"m_bModal",
		"void Configure",
		"bool Matches"
	)) {
	if ($uiScreenBaseText -notmatch [regex]::Escape($requiredUIScreenBaseEntry)) {
		throw "UI screen descriptor must track root ownership and blocking behavior: $requiredUIScreenBaseEntry"
	}
}
foreach ($requiredUIRootServiceEntry in @(
		"UI_ROOT_DEBUG_ENABLED",
		"static HST_UIRootService Get()",
		"RequestOpen",
		"NotifyClosed",
		"NotifyNotificationShown",
		"NotifyNotificationHidden",
		"CanOpen",
		'CanOpen(HST_EUIScreenMode mode, string owner = "", bool modal = false)',
		"CanHandleScreenInput",
		'CanHandleScreenInput(HST_EUIScreenMode mode, string owner = "")',
		"CanHandleModalInput",
		'CanHandleModalInput(HST_EUIScreenMode mode, string owner = "")',
		"bool wantsModal = modal || mode == HST_EUIScreenMode.ACTION_DIALOG || mode == HST_EUIScreenMode.MISSION_DIALOG",
		"if (m_ModalScreen)",
		"m_ModalScreen.Matches(mode, owner)",
		"m_CurrentScreen.Matches(mode, owner)",
		"owner.IsEmpty() || (m_CurrentScreen && m_CurrentScreen.Matches(mode, owner))",
		"if (mode == HST_EUIScreenMode.SETUP_MAP)",
		"return current == HST_EUIScreenMode.SETUP_MAP",
		"GetModalMode",
		"GetTopmostMode",
		"GetTopmostOwner",
		"IsTopmost",
		"m_CurrentScreen",
		"m_ModalScreen",
		"m_ModalScreen.m_bBlocksGameplay",
		"m_ModalScreen.m_bBlocksMap",
		"IsGameplayBlocked",
		"IsMapBlocked",
		"IsModalOpen",
		"IsNotificationVisible",
		"DebugState",
		"DescribeScreen",
		"HST_UIDebug.WidgetSummary",
		"opened modal",
		"opened screen",
		"closed modal",
		"closed screen",
		"refused open",
		"ignored close",
		"notification shown",
		"notification hidden",
		"current == HST_EUIScreenMode.SETUP_MAP",
		"mode == HST_EUIScreenMode.COMMAND_MENU && current == HST_EUIScreenMode.LOADOUT_EDITOR"
	)) {
	if ($uiRootServiceText -notmatch [regex]::Escape($requiredUIRootServiceEntry)) {
		throw "UI root service must centralize screen state and input blocking policy: $requiredUIRootServiceEntry"
	}
}
foreach ($widgetHandlerContract in @(
		@{ Path = "Scripts/Game/HST/Components/HST_SetupMapComponent.c"; Handler = "HST_SetupMapWidgetHandler" },
		@{ Path = "Scripts/Game/HST/Components/HST_CommandMenuComponent.c"; Handler = "HST_CommandMenuWidgetHandler" },
		@{ Path = "Scripts/Game/HST/Components/HST_LoadoutEditorComponent.c"; Handler = "HST_LoadoutEditorWidgetHandler" },
		@{ Path = "Scripts/Game/HST/Components/HST_MissionClientComponent.c"; Handler = "HST_MissionClientWidgetHandler" }
	)) {
	$widgetHandlerText = Get-Content -Raw $widgetHandlerContract.Path
	if ($widgetHandlerText -notmatch [regex]::Escape("class $($widgetHandlerContract.Handler)")) {
		throw "Missing HST widget handler contract: $($widgetHandlerContract.Handler)"
	}
	if ($widgetHandlerText -match "GetUserID\(\)" -and $widgetHandlerText -notmatch [regex]::Escape("!w")) {
		throw "$($widgetHandlerContract.Handler) must guard null widget callbacks before GetUserID"
	}
}
foreach ($requiredReportDialogControllerEntry in @(
		"class HST_ReportDialogData",
		"class HST_ReportDialogController",
		"REPORT_DIALOG_LAYOUT =",
		"REPORT_OBJECTIVE_ROW_LAYOUT =",
		"workspace.CreateWidgets(REPORT_DIALOG_LAYOUT, workspace)",
		"workspace.CreateWidgets(REPORT_OBJECTIVE_ROW_LAYOUT, items)",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.MISSION_DIALOG",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.MISSION_DIALOG",
		"HST_UIDebug.LogLayoutCreate(data.m_sDebugOwner",
		"HST_UIDebug.LogLayoutRejected(data.m_sDebugOwner",
		"HST_UIDebug.LogExpectedWidgetsCsv(data.m_sDebugOwner",
		"HST_UIDebug.LogPopulation(data.m_sDebugOwner",
		'HST_UIDebug.LogRowSample("mission_report_objectives"',
		'HST_UIDebug.LogRowSummary("mission_report_objectives"',
		"static void Close(string owner)",
		"protected static void QueueReadyGeometry",
		"protected static void LogReadyGeometry",
		"protected static void BindClick",
		"protected static void SetText",
		"protected static void AddObjectiveRow"
	)) {
	if ($reportDialogControllerText -notmatch [regex]::Escape($requiredReportDialogControllerEntry)) {
		throw "Report dialog controller must own layout creation, diagnostics, and row population: $requiredReportDialogControllerEntry"
	}
}
foreach ($requiredActionDialogControllerEntry in @(
		"class HST_ActionDialogData",
		"class HST_ActionDialogController",
		'ACTION_DIALOG_LAYOUT = "{D66CFA01E5AA4200}UI/layouts/HST_ActionDialog.layout"',
		"workspace.CreateWidgets(ACTION_DIALOG_LAYOUT, workspace)",
		"root.SetZOrder(HST_UIConstants.Z_ACTION_DIALOG)",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.ACTION_DIALOG",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.ACTION_DIALOG",
		"HST_UIDebug.LogLayoutCreate(data.m_sDebugOwner",
		"HST_UIDebug.LogLayoutRejected(data.m_sDebugOwner",
		"HST_UIDebug.LogExpectedWidgetsCsv(data.m_sDebugOwner",
		"HST_UIDebug.LogPopulation(data.m_sDebugOwner",
		"BindClick(data.m_sDebugOwner, root, `"CancelButton`", data.m_iCancelWidgetId, handler)",
		"BindClick(data.m_sDebugOwner, root, `"ConfirmButton`", data.m_iConfirmWidgetId, handler)",
		'SetText(root, "Title"',
		'SetText(root, "Message"',
		"static void Close(string owner)",
		"protected static void QueueReadyGeometry",
		"protected static void LogReadyGeometry",
		"protected static void BindClick",
		"protected static void SetText"
	)) {
	if ($actionDialogControllerText -notmatch [regex]::Escape($requiredActionDialogControllerEntry)) {
		throw "Action dialog controller must own layout creation, diagnostics, text population, and button binding: $requiredActionDialogControllerEntry"
	}
}
foreach ($requiredUIRootCaller in @(
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.SETUP_MAP",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.SETUP_MAP",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.COMMAND_MENU",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.COMMAND_MENU",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.LOADOUT_EDITOR",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.LOADOUT_EDITOR",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.ACTION_DIALOG",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.ACTION_DIALOG",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.MISSION_DIALOG",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.MISSION_DIALOG",
		"HST_UIRootService.Get().NotifyNotificationShown()",
		"HST_UIRootService.Get().NotifyNotificationHidden()"
	)) {
	$foundUIRootCaller = $false
	foreach ($uiText in @(
			$setupMapComponentText,
			(Get-Content -Raw "Scripts/Game/HST/Components/HST_CommandMenuComponent.c"),
			(Get-Content -Raw "Scripts/Game/HST/Components/HST_LoadoutEditorComponent.c"),
			(Get-Content -Raw "Scripts/Game/HST/Components/HST_MissionClientComponent.c"),
			$actionDialogControllerText,
			$reportDialogControllerText,
			$notificationToastControllerText
		)) {
		if ($uiText -and $uiText -match [regex]::Escape($requiredUIRootCaller)) {
			$foundUIRootCaller = $true
			break
		}
	}
	if (!$foundUIRootCaller) {
		throw "UI screens must register open/close/passive notification state with HST_UIRootService: $requiredUIRootCaller"
	}
}
foreach ($requiredNotificationToastLayoutEntry in @(
		'Name "HST_NotificationRoot"',
		'Name "Toast"',
		'Name "Background"',
		'Name "AccentLine"',
		'Name "Title"',
		'Name "Message"',
		"Anchor 0 0 1 1",
		"Anchor 0.5 0 0.5 0",
		"SizeX 840",
		"OffsetRight -840",
		"Alignment 0.5 0",
		"SizeY 24",
		"OffsetBottom -34",
		'"Ignore Cursor" 1'
	)) {
	if ($notificationToastLayoutText -notmatch [regex]::Escape($requiredNotificationToastLayoutEntry)) {
		throw "Notification toast must use a passive anchored layout entry: $requiredNotificationToastLayoutEntry"
	}
}
foreach ($forbiddenNotificationToastLayoutEntry in @(
		"OffsetBottom 34"
	)) {
	if ($notificationToastLayoutText -match [regex]::Escape($forbiddenNotificationToastLayoutEntry)) {
		throw "Notification toast fixed-height text must use negative bottom-edge offsets: $forbiddenNotificationToastLayoutEntry"
	}
}
if ($notificationToastLayoutMetaText -notmatch [regex]::Escape('Name "{A34F448C7E830600}UI/layouts/HST_NotificationToast.layout"')) {
	throw "Notification toast layout meta must carry the expected non-zero GUID"
}
foreach ($requiredSetupHoldingEntry in @(
		"PrepareSetupHoldingEntity",
		"ClearFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true)",
		"SetDisableMovementControls(true)",
		"SetDisableViewControls(true)",
		"SetDisableWeaponControls(true)",
		"EnableSimulation(false)",
		"ClearSetupHoldingPlayer(disconnectedPlayerId)",
		"DeleteSetupHoldingEntity"
	)) {
	if ($playerSpawnSetupText -notmatch [regex]::Escape($requiredSetupHoldingEntry)) {
		throw "Setup holding spawn must be hidden/frozen and cleaned up before HQ spawn: $requiredSetupHoldingEntry"
	}
}
foreach ($requiredSetupNativeMapConfigEntry in @(
		"m_iMapMode PLAIN",
		"HST_MapZoneOverlayUIComponent",
		"SCR_MapCursorModule",
		"SCR_MapMarkersUI"
	)) {
	if ($setupNativeMapConfigText -notmatch [regex]::Escape($requiredSetupNativeMapConfigEntry)) {
		throw "Setup map config is missing setup-only map component: $requiredSetupNativeMapConfigEntry"
	}
}
if ($setupNativeMapConfigText -match [regex]::Escape("m_iMapMode FULLSCREEN")) {
	throw "Setup map config must use a distinct non-fullscreen mode so it cannot poison the normal fullscreen map config cache"
}
foreach ($requiredGameplayMapConfigEntry in @(
		'SCR_MapConfig : "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf"',
		'Name "{6985327711306211}Configs/Map/HST_GameplayMap.conf"'
	)) {
	if (($gameplayMapConfigText + "`n" + $gameplayMapConfigMetaText) -notmatch [regex]::Escape($requiredGameplayMapConfigEntry)) {
		throw "Gameplay map config must inherit the vanilla fullscreen map resource and have its own resource id: $requiredGameplayMapConfigEntry"
	}
}
foreach ($forbiddenGameplayMapConfigEntry in @(
		"HST_SetupHQMap",
		"HST_MapZoneOverlayUIComponent",
		"MapCursorModuleDef",
		"m_bEnableCursorEdgePan 0",
		"m_aUIComponents"
	)) {
	if ($gameplayMapConfigText -match [regex]::Escape($forbiddenGameplayMapConfigEntry)) {
		throw "Gameplay map config must not copy setup-only map behavior or override the vanilla tool stack: $forbiddenGameplayMapConfigEntry"
	}
}
foreach ($requiredGameplayLayerEntry in @(
		'm_sGadgetMapConfigPath "{6985327711306211}Configs/Map/HST_GameplayMap.conf"',
		'm_sSpawnMapConfigPath "{901F9ED2088BBCA4}Configs/Map/MapSpawnConflict.conf"'
	)) {
	if (($everonDefaultLayerText + "`n" + $devDefaultLayerText) -notmatch [regex]::Escape($requiredGameplayLayerEntry)) {
		throw "Game mode map config component must explicitly separate gameplay and spawn map configs: $requiredGameplayLayerEntry"
	}
}
$setupConfigReferences = @()
foreach ($setupReferenceFile in (Get-ChildItem -Recurse -File "Scripts", "Configs", "UI")) {
	if ($setupReferenceFile.Extension -notin @(".c", ".conf", ".layout", ".meta")) {
		continue
	}

	$setupReferenceText = Get-Content -Raw $setupReferenceFile.FullName
	if ($setupReferenceText -match "HST_SetupHQMap\.conf") {
		$relativeSetupReferencePath = (Resolve-Path -Relative $setupReferenceFile.FullName) -replace '^[.\\/]+', ''
		$setupConfigReferences += $relativeSetupReferencePath
	}
}
$allowedSetupConfigReferences = @(
	"Scripts\Game\HST\Components\HST_SetupMapComponent.c",
	"Configs\Map\HST_SetupHQMap.conf.meta"
)
foreach ($setupConfigReference in $setupConfigReferences) {
	if ($allowedSetupConfigReferences -notcontains $setupConfigReference) {
		throw "Setup map config must not be referenced by normal gameplay map flows: $setupConfigReference"
	}
}
Write-Host "Initial HQ setup placement flow OK"
Write-Host "Setup/gameplay map config separation OK"

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
$missionServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionService.c"
$commandUIServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
$missionCompletionContractText = $missionRuntimeServiceText + "`n" + $missionServiceText + "`n" + $coordinatorText + "`n" + $commandUIServiceText
$missionCaptiveFollowComponentPath = "Scripts/Game/HST/Components/HST_MissionCaptiveFollowComponent.c"
$missionCaptiveFollowComponentText = ""
if (Test-Path $missionCaptiveFollowComponentPath) {
	$missionCaptiveFollowComponentText = Get-Content -Raw $missionCaptiveFollowComponentPath
}
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
		"POST_EXPIRY_PLAYER_ASSET_BUBBLE_METERS = 1800.0",
		"ShouldContinueExpiredPlayerBoundMissionRuntime",
		"TickExpiredPlayerBoundMissionRuntime",
		"CanCompleteExpiredPlayerBoundMission",
		"IsExpiredPlayerBoundMissionInteractionAllowed",
		"HasDeliveredPlayerBoundMissionAssetAfterExpiry",
		"EnsureRestoredMissionCarrierVehicles(state, mission)",
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
foreach ($requiredCaptiveBoardingEntry in @(
		"RefreshCaptiveBoardingDebugState",
		"rescue.captive.boarding.state_samples",
		"rescue.captive.boarding.transport_state_samples",
		"access.GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true)",
		"access.MoveInVehicle(vehicleEntity, ECompartmentType.CARGO, true, slot)",
		"server-authoritative captive cargo move-in completed",
		"boardingAssociated",
		"transportAssociated",
		"captive boarding command did not produce a seated/getting-in state or authoritative loaded association after bounded rescan",
		"CaptiveBoardingDebugStatus(captiveInCarrier || captiveGettingIn || boardingAssociated)",
		"CaptiveBoardingDebugStatus(carrierMoved && (captiveStillInCarrier || captiveStillGettingIn || transportAssociated))"
	)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredCaptiveBoardingEntry)) {
		throw "Captive boarding debug probe must use authoritative move-in plus bounded seat/association proof: $requiredCaptiveBoardingEntry"
	}
}
if ($missionRuntimeServiceText -match [regex]::Escape('CaptiveBoardingDebugStatus(captiveInCarrier || captiveGettingIn, "WARN")') -or $missionRuntimeServiceText -match [regex]::Escape('MoveInVehicle(vehicleEntity, ECompartmentType.CARGO, false, slot)')) {
	throw "Captive boarding proof must not keep immediate WARN-only seat state or unpaused animated-only boarding"
}
foreach ($requiredExpiredMissionCompletionEntry in @(
		"allowExpired = false",
		"CanCompleteExpiredPlayerBoundMission(m_State, activeMission)",
		"m_Missions.Complete(m_State, m_Economy, instanceId, false, allowExpiredCompletion)",
		"ApplyMissionOutcomeEvent(m_State, m_Preset, m_Economy, m_Balance, m_Towns, m_ZoneCapture, m_Garrisons, m_EnemyCommander, m_EnemyDirector, m_SupportRequests, m_HQ, definition, activeMission, true, applyDefinitionRewards)",
		"IsExpiredPlayerBoundMissionActionCandidate",
		"SelectExpiredPlayerBoundMissionAsset",
		"IsExpiredPlayerBoundMissionAsset"
	)) {
	if ($missionCompletionContractText -notmatch [regex]::Escape($requiredExpiredMissionCompletionEntry)) {
		throw "Missing expired player-bound mission completion entry: $requiredExpiredMissionCompletionEntry"
	}
}
foreach ($requiredCaptiveRuntimeContract in @(
		'SetAffiliatedFactionByKey("CIV")',
		"EnsureMissionCaptivesNeutralized",
		"StripMissionCaptiveWeapons",
		"RemoveMissionCaptiveWeaponItem",
		"BaseWeaponComponent",
		"BaseMagazineComponent",
		"MagazineComponent",
		"StartCaptiveFollowController",
		"StopCaptiveFollowController",
		"TryIssueCaptiveFollowWaypoint",
		"ResolveCaptiveAIGroup",
		"GetParentGroup",
		"StopCaptiveFollowing",
		"CAPTIVE_FOLLOW_BREAK_DISTANCE_METERS",
		"CAPTIVE_DISEMBARK_RADIUS_METERS",
		"LogMissionCaptiveProjection",
		"captive projection live"
	)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredCaptiveRuntimeContract)) {
		throw "Mission runtime service is missing captive neutralization contract: $requiredCaptiveRuntimeContract"
	}
}
if ($missionRuntimeServiceText -match "StepCaptiveToward") {
	throw "Mission runtime service must not use transform-stepped captive follow fallback"
}
foreach ($requiredCaptiveFollowComponentContract in @(
		"HST_MissionCaptiveFollowComponent",
		"CAPTIVE_AI_GROUP_PREFAB",
		"{6985327711303910}Prefabs/Groups/HST/HST_RuntimeEmptyGroup.et",
		"CAPTIVE_FOLLOW_WAYPOINT_PREFAB",
		"IssueFollowWaypoint",
		"ClearFollowWaypoint",
		"SetCompletionRadius",
		"AddAgentFromControlledEntity",
		"RequestFollowPathOfEntity",
		"AIBaseMovementComponent",
		"AIControlComponent",
		"GetControlAIAgent",
		"GetMovementComponent",
		"SetGroupCharactersWantedMovementType",
		"EMovementType.RUN",
		"EMovementType.SPRINT",
		"BuildResponsiveFollowPosition",
		"TARGET_LEAD_MULTIPLIER",
		"WAYPOINT_REFRESH_DISTANCE_METERS = 14.0",
		"WAYPOINT_REACHED_REFRESH_DISTANCE_METERS",
		"FOLLOW_UPDATE_MS = 500",
		"m_bDirectFollowUnavailable"
	)) {
	if ($missionCaptiveFollowComponentText -notmatch [regex]::Escape($requiredCaptiveFollowComponentContract)) {
		throw "Captive follow component is missing AI movement contract: $requiredCaptiveFollowComponentContract"
	}
}
if ($missionCaptiveFollowComponentText -match 'CAPTIVE_AI_GROUP_PREFAB = "\{000CD338713F2B5A\}Prefabs/AI/Groups/Group_Base\.et"') {
	throw "Captive follow must use the HST-owned non-deleting runtime group prefab instead of raw Group_Base"
}
if ($missionCaptiveFollowComponentText -match "EMovementType.WALK" -or $missionCaptiveFollowComponentText -match "TryForceWalkSpeed") {
	throw "Captive follow component must not force freed POWs to walk while following"
}
if ($missionCaptiveFollowComponentText -match "AddWaypointAt" -or $missionCaptiveFollowComponentText -match "CompleteWaypoint") {
	throw "Captive follow component must use the proven moving AddWaypoint fallback, not front-inserted/completed waypoint replacement"
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
		@("Prefabs/Objects/HST/HST_MissionProp_Captives.et", "{6985327711303300}Prefabs/Characters/HST/Character_HST_Petros.et"),
		@("Prefabs/Objects/HST/HST_MissionProp_Cargo.et", "{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et"),
		@("Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et", "{7E2380494811A5FB}Prefabs/Structures/Infrastructure/Towers/TransmitterTower_01/TransmitterTower_01_medium.et"),
		@("Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et", "{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et")
	)) {
	$missionPropText = Get-Content -Raw $missionPropContract[0]
	if ($missionPropText -notmatch [regex]::Escape($missionPropContract[1])) {
		throw "Mission runtime prop must inherit the expected visible parent: $($missionPropContract[0]) -> $($missionPropContract[1])"
	}
	if ($missionPropContract[0] -eq "Prefabs/Objects/HST/HST_MissionProp_Captives.et" -and $missionPropText -notmatch "ActionsManagerComponent") {
		throw "Captive mission prop must expose a POW action surface for direct context interactions"
	}
	if ($missionPropContract[0] -eq "Prefabs/Objects/HST/HST_MissionProp_Captives.et" -and $missionPropText -notmatch "HST_MissionCaptiveFollowComponent") {
		throw "Captive mission prop must include the POW AI follow component"
	}
	if ($missionPropContract[0] -eq "Prefabs/Objects/HST/HST_MissionProp_Captives.et" -and $missionPropText -notmatch 'ActionsManagerComponent "\{520EA1D2F659CE02\}"') {
		throw "Captive mission prop must override the inherited FIA action manager, matching the Petros runtime-action pattern"
	}
	if ($missionPropContract[0] -eq "Prefabs/Objects/HST/HST_MissionProp_Captives.et" -and $missionPropText -match 'ActionsManagerComponent "\{6985327711303733\}"') {
		throw "Captive mission prop must not add a duplicate HST-owned action manager"
	}
	foreach ($requiredCaptiveAction in @("HST_MissionCaptiveFreeAction", "HST_MissionCaptiveFollowAction", "HST_MissionCaptiveExtractAction", "Free captive", "Order POWs to follow", "Extract captive")) {
		if ($missionPropContract[0] -eq "Prefabs/Objects/HST/HST_MissionProp_Captives.et" -and $missionPropText -notmatch [regex]::Escape($requiredCaptiveAction)) {
			throw "Captive mission prop is missing POW context action: $requiredCaptiveAction"
		}
	}
	if ($missionPropContract[0] -eq "Prefabs/Objects/HST/HST_MissionProp_Captives.et" -and $missionPropText -match "RplComponent") {
		throw "Captive mission prop must inherit replication from the action-capable character base, not a hand-authored RplComponent block"
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
$devRuntimeMarkerLayer = Get-Content -Raw "Worlds/HST_Dev/HST_Dev_Layers/default.layer"
$townLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/Towns.layer"
$strategicZonesLayer = Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/StrategicZones.layer"
$markerServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
$coordinatorMarkerText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$campaignMarkerDirectorText = Get-Content -Raw "Scripts/Game/HST/Map/HST_CampaignMapMarkerDirector.c"
$nativeMarkerReconcilerText = Get-Content -Raw "Scripts/Game/HST/Map/HST_NativeMapMarkerReconciler.c"
$playerMarkerServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PlayerMapMarkerService.c"
$playerMarkerEntryText = Get-Content -Raw "Scripts/Game/HST/Map/HST_PlayerMapMarkerEntry.c"
$playerMarkerEntityPatchText = Get-Content -Raw "Scripts/Game/HST/Map/HST_PlayerMapMarkerEntityConfigPatch.c"
$mapMarkerManagerPatchText = Get-Content -Raw "Scripts/Game/HST/Map/HST_MapMarkerManagerConfigPatch.c"
$playerMarkerTypesText = Get-Content -Raw "Scripts/Game/HST/Map/HST_MapMarkerTypes.c"
$playerMarkerConfigText = Get-Content -Raw "Configs/Map/HST_PlayerMapMarkerConfig.conf"
$runtimeMarkerPipelineText = $markerServiceText + "`n" + $coordinatorMarkerText + "`n" + $campaignMarkerDirectorText + "`n" + $nativeMarkerReconcilerText
$zoneBlocks = @(Get-ConfigBlocks $mapConfig "HST_ZoneDefinition")

foreach ($requiredCuratedLocationEntry in @(
		"ApplyEveronLocationPlanOverrides",
		"UpsertEveronLocationPlanZone",
		"GetEveronLocationPlanSpawnProfile",
		"town_lamentin",
		"town_morton",
		"town_saint_philippe",
		"resource_logistics_warehouse",
		"factory_concrete_plant",
		"factory_montignac",
		"radio_lamentin_tower",
		"mission_radar_airport",
		"HST_ZONE_MISSION_SITE",
		"preflight.zone_graph.curated_location_categories",
		"preflight.zone_graph.curated_location_counts"
	)) {
	if ($defaultCatalog -notmatch [regex]::Escape($requiredCuratedLocationEntry) -and $coordinatorMarkerText -notmatch [regex]::Escape($requiredCuratedLocationEntry)) {
		throw "Curated Everon location taxonomy contract is missing entry: $requiredCuratedLocationEntry"
	}
}

$upsertDeclaration = [regex]::Match($defaultCatalog, 'private static void UpsertEveronLocationPlanZone\(([^\r\n]+)\)')
if (-not $upsertDeclaration.Success) {
	throw "Curated Everon location taxonomy upsert helper declaration is missing"
}
$upsertParameterCount = @($upsertDeclaration.Groups[1].Value.ToCharArray() | Where-Object { $_ -eq ',' }).Count + 1
if ($upsertParameterCount -gt 16) {
	throw "Curated Everon location taxonomy upsert helper exceeds Enforce's 16-argument cap: $upsertParameterCount"
}
if ($defaultCatalog -match 'UpsertEveronLocationPlanZone\([^\r\n]+, "comp_[^"]+", "spawn_[^"]+"\);') {
	throw "Curated Everon location taxonomy upsert calls must not pass composition/spawn metadata positionally"
}
if ($defaultCatalog -notmatch 'return "spawn_none";') {
	throw "Curated Everon mission-site taxonomy anchors must preserve no-spawn metadata"
}

foreach ($requiredSupportMarkerEntry in @(
		"trackResistanceSupportGroupsOnMap",
		"SetTrackResistanceSupportGroupsOnMap",
		"AddResistanceSupportGroupMarkers",
		"ShouldShowResistanceSupportGroupMarker",
		"support_group_live",
		"support.physical_live_marker",
		"support.physical_live_marker_terminal",
		"support.physical_map_destination",
		"support.physical_spawn_offset",
		"support.physical_spawn_clearance",
		"hst_defend_petros_attackers",
		"IsTerminalActiveGroupStatus",
		"HasCampaignDebugLiveActiveGroup",
		"phase22.marker.attackers_backing"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSupportMarkerEntry) -and $runtimeMarkerPipelineText -notmatch [regex]::Escape($requiredSupportMarkerEntry)) {
		throw "Resistance support live-marker contract is missing entry: $requiredSupportMarkerEntry"
	}
}
if ($coordinatorMarkerText -notmatch [regex]::Escape('return qrf != null || HasCampaignDebugLiveActiveGroup(marker.m_sLinkedId);')) {
	throw "QRF-style marker backing audit must accept live active-group backing for Defend Petros attacker markers"
}
foreach ($requiredMarkerIconDeconflictEntry in @(
		'if (zone.m_sMarkerStyle == "town")',
		'return "POINT_OF_INTEREST";',
		'if (zone.m_sMarkerStyle == "enemy_base" || zone.m_sMarkerStyle == "stronghold")',
		'return "OBSERVATION_POST";',
		'if (zone.m_sMarkerStyle == "mission_site")',
		'return "POINT_SPECIAL";',
		'phase23.marker.location_qrf_icon_deconflict',
		'CountCampaignDebugStaticLocationQRFIconCollisions'
	)) {
	if ($runtimeMarkerPipelineText -notmatch [regex]::Escape($requiredMarkerIconDeconflictEntry)) {
		throw "Static location marker/QRF icon deconflict contract is missing entry: $requiredMarkerIconDeconflictEntry"
	}
}

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

$runtimeMarkerLayerChecks = @(
	@{ Name = "Everon"; Text = $runtimeMarkerLayer },
	@{ Name = "Dev"; Text = $devRuntimeMarkerLayer }
)
foreach ($markerLayerCheck in $runtimeMarkerLayerChecks) {
	foreach ($requiredNativeMarkerEntry in @(
			"SCR_MapMarkerManagerComponent",
			'm_sMarkerCfgPath "{6985327711306212}Configs/Map/HST_PlayerMapMarkerConfig.conf"'
		)) {
		if ($markerLayerCheck["Text"] -notmatch [regex]::Escape($requiredNativeMarkerEntry)) {
			throw "$($markerLayerCheck["Name"]) runtime layer is missing campaign/player map marker scaffold entry: $requiredNativeMarkerEntry"
		}
	}
}

foreach ($requiredPlayerMarkerEntry in @(
		"HST_PlayerMapMarkerService",
		"HST_NativeMapMarkerReconciler",
		"HST_EMapMarkerRenderMode.DYNAMIC_ENTITY",
		"SCR_EMapMarkerType.HST_PLAYER",
		"RequestRefresh",
		"IsEnabled",
		"ResolveControlledPlayerEntity",
		"ResolvePlayerName",
		"SetEnabled",
		"ClearAll",
		"ReportReconcileFailure",
		"ShouldPublishPlayerMarkers",
		"HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE",
		"HST_ECampaignPhase.HST_CAMPAIGN_WON",
		"HST_ECampaignPhase.HST_CAMPAIGN_LOST"
	)) {
	if (($playerMarkerServiceText + "`n" + $coordinatorMarkerText) -notmatch [regex]::Escape($requiredPlayerMarkerEntry)) {
		throw "Player map marker implementation is missing: $requiredPlayerMarkerEntry"
	}
}
foreach ($requiredPlayerMarkerEntryConfig in @(
		"[BaseContainerProps(), SCR_MapMarkerTitle()]",
		"class HST_PlayerMapMarkerEntry : SCR_MapMarkerEntryDynamic",
		"SCR_EMapMarkerType.HST_PLAYER",
		'PLAYER_MARKER_IMAGESET = "{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset"',
		"PLAYER_MARKER_ICON = `"whisper`"",
		"SetImage(PLAYER_MARKER_IMAGESET, PLAYER_MARKER_ICON)",
		"PLAYER_MARKER_LABEL_RETRY_COUNT = 12",
		"ApplyPlayerMarkerLabel(marker, widgetComp, 0)",
		"GetMarkerConfigID()",
		"ResolvePlayerMarkerLabel",
		"ResolvePlayerDisplayName",
		"SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerId)",
		"GetGame().GetPlayerManager()",
		"CallLater(ApplyPlayerMarkerLabel",
		"SetTextVisible(true)",
		"ResolvePlayerMarkerColor",
		"SCR_FactionManager.SGetPlayerFaction(playerId)",
		'GetFactionByKey("FIA")'
	)) {
	if ($playerMarkerEntryText -notmatch [regex]::Escape($requiredPlayerMarkerEntryConfig)) {
		throw "Player map marker entry must keep config-safe visible dynamic marker visuals: $requiredPlayerMarkerEntryConfig"
	}
}
if ($playerMarkerServiceText -notmatch [regex]::Escape("record.m_iConfigId = playerId;")) {
	throw "Player map marker service must pass player id through replicated marker config id for client-side label resolution"
}
if ($playerMarkerServiceText -notmatch "if \(!ShouldPublishPlayerMarkers\(state\)\)[\s\S]{0,180}?ClearAll\(\)") {
	throw "Player map marker service must clear and suppress markers before gameplay marker publication is available"
}
if ($playerMarkerServiceText -notmatch "protected bool ShouldPublishPlayerMarkers\(HST_CampaignState state\)[\s\S]*?HST_CAMPAIGN_ACTIVE[\s\S]*?HST_CAMPAIGN_WON[\s\S]*?HST_CAMPAIGN_LOST") {
	throw "Player map marker service must publish during active and terminal gameplay campaign phases"
}
foreach ($requiredDynamicCleanupContract in @(
		"if (!m_mDynamicDomainIdToMarkerEntity.Contains(id))",
		"if (manager && markerEntity)",
		"m_mDynamicDomainIdToMarkerEntity.Remove(id)"
	)) {
	if ($nativeMarkerReconcilerText -notmatch [regex]::Escape($requiredDynamicCleanupContract)) {
		throw "Native dynamic marker cleanup must drop stale tracked ids even when marker entity pointer is already null: $requiredDynamicCleanupContract"
	}
}
foreach ($requiredDynamicLivenessContract in @(
		"CountTrackedDynamicLive()",
		"CountTrackedDynamicLive(SCR_MapMarkerManagerComponent manager)",
		"IsDynamicMarkerLive(manager, markerEntity)",
		"array<SCR_MapMarkerEntity> dynamicMarkers = manager.GetDynamicMarkers()",
		"return dynamicMarkers.Contains(markerEntity)",
		"if (markerEntity && !IsDynamicMarkerLive(manager, markerEntity))"
	)) {
	if ($nativeMarkerReconcilerText -notmatch [regex]::Escape($requiredDynamicLivenessContract)) {
		throw "Native dynamic marker reconciliation must verify tracked handles are still registered with the marker manager: $requiredDynamicLivenessContract"
	}
}
if ($playerMarkerServiceText -notmatch [regex]::Escape("m_Reconciler.CountTrackedDynamicLive() == m_mDesiredPlayerMarkers.Count()")) {
	throw "Player map marker refresh skip must require live native dynamic handles, not just matching tracked counts"
}
foreach ($requiredPlayerMarkerReportContract in @(
		"string BuildRuntimeReport()",
		"h-istasi player marker report",
		"m_Reconciler.GetTrackedDynamicHandleCount()",
		"m_Reconciler.CountTrackedDynamicLive()",
		"SCR_MapMarkerManagerComponent.GetInstance()",
		"markerManager.GetDynamicMarkers().Count()",
		"markerConfig.GetMarkerEntryConfigByType(SCR_EMapMarkerType.HST_PLAYER)",
		"native marker manager",
		"last reconcile",
		"player marker | id"
	)) {
	if ($playerMarkerServiceText -notmatch [regex]::Escape($requiredPlayerMarkerReportContract)) {
		throw "Player map marker service must expose runtime marker diagnostics: $requiredPlayerMarkerReportContract"
	}
}
if ($coordinatorMarkerText -notmatch [regex]::Escape("m_PlayerMapMarkers.BuildRuntimeReport()")) {
	throw "Native marker admin report must include player marker service diagnostics"
}
foreach ($requiredPlayerMarkerManagerPatchEntry in @(
		"bool EnsureHSTMarkerConfig(ResourceName markerConfigPath)",
		"m_MarkerCfg.GetMarkerEntryConfigByType(SCR_EMapMarkerType.HST_PLAYER)",
		"BaseContainerTools.LoadContainer(markerConfigPath)",
		"BaseContainerTools.CreateInstanceFromContainer",
		"m_MarkerCfg = markerConfig",
		"InitHSTMarkerConfigEntries()",
		"dynamicEntry.InitServerLogic()",
		"dynamicEntry.InitClientLogic()"
	)) {
	if ($mapMarkerManagerPatchText -notmatch [regex]::Escape($requiredPlayerMarkerManagerPatchEntry)) {
		throw "Player marker manager config patch must load and initialize the HST_PLAYER marker config: $requiredPlayerMarkerManagerPatchEntry"
	}
}
foreach ($requiredPlayerMarkerEntityPatchEntry in @(
		"modded class SCR_MapMarkerEntity",
		"override protected void EOnInit(IEntity owner)",
		"markerManager.RegisterDynamicMarker(this)",
		"BaseRplComponent rplComp = BaseRplComponent.Cast(FindComponent(BaseRplComponent))",
		"if (!rplComp || rplComp.IsOwner())",
		"SetFlags(EntityFlags.ACTIVE, true)",
		"SetEventMask(EntityEvent.FRAME)",
		"m_fUpdateDelay = markerEntityClass.GetUpdateDelay()",
		"m_fTimeTracker = m_fUpdateDelay",
		"override void OnCreateMarker()",
		"GetType() == SCR_EMapMarkerType.HST_PLAYER",
		"markerManager.EnsureHSTMarkerConfig(PLAYER_MARKER_CONFIG)",
		"super.OnCreateMarker()"
	)) {
	if ($playerMarkerEntityPatchText -notmatch [regex]::Escape($requiredPlayerMarkerEntityPatchEntry)) {
		throw "Player marker entity patch must guard vanilla dynamic marker init/config creation path: $requiredPlayerMarkerEntityPatchEntry"
	}
}
if ($playerMarkerEntityPatchText -match [regex]::Escape("super.EOnInit(owner)")) {
	throw "Player marker entity patch must not call vanilla EOnInit; vanilla dereferences rplComp without a null guard"
}
$nativeMarkerPurgeMatch = [regex]::Match($coordinatorMarkerText, "string RequestAdminPurgeNativeHSTMarkers[\s\S]*?\r?\n\t}")
if (!$nativeMarkerPurgeMatch.Success -or $nativeMarkerPurgeMatch.Value -notmatch [regex]::Escape("m_MapMarkers.AdminPurgeNativeHSTMarkers()") -or $nativeMarkerPurgeMatch.Value -notmatch [regex]::Escape("m_PlayerMapMarkers.ClearAll()") -or $nativeMarkerPurgeMatch.Value -notmatch [regex]::Escape('m_PlayerMapMarkers.RequestRefresh("admin native marker purge")')) {
	throw "Native marker admin purge must clear both campaign marker and separate player marker reconcilers"
}
foreach ($forbiddenPlayerMarkerEntryConfig in @(
		"PLAYER_MARKER_ICON = `"dot`"",
		"SCR_MapMarkerEntryDynamicExample",
		"SCR_EMapMarkerType.DYNAMIC_EXAMPLE"
	)) {
	if (($playerMarkerEntryText + "`n" + $playerMarkerConfigText + "`n" + $playerMarkerServiceText) -match [regex]::Escape($forbiddenPlayerMarkerEntryConfig)) {
		throw "Player map marker entry must not use known-bad marker config/visual entry: $forbiddenPlayerMarkerEntryConfig"
	}
}
foreach ($requiredPlayerMarkerTypeEntry in @(
		"modded enum SCR_EMapMarkerType",
		"HST_PLAYER"
	)) {
	if ($playerMarkerTypesText -notmatch [regex]::Escape($requiredPlayerMarkerTypeEntry)) {
		throw "Player markers need a dedicated map marker enum value, missing: $requiredPlayerMarkerTypeEntry"
	}
}
if ($playerMarkerConfigText -notmatch "SCR_MapMarkerConfig[\s\S]*?HST_PlayerMapMarkerEntry" -or $playerMarkerConfigText -notmatch [regex]::Escape('{6985327711306230}Prefabs/Markers/HST_PlayerMapMarker.et')) {
	throw "Player map marker config must inherit map marker config and register HST_PlayerMapMarkerEntry with the HST marker entity prefab"
}
if (!(Test-Path "Prefabs/Markers/HST_PlayerMapMarker.et")) {
	throw "Player marker prefab is missing: Prefabs/Markers/HST_PlayerMapMarker.et"
}
$playerMarkerPrefabText = Get-Content -Raw "Prefabs/Markers/HST_PlayerMapMarker.et"
if ($playerMarkerPrefabText -match [regex]::Escape('Prefabs/Markers/MapMarkerEntityBase.et')) {
	throw "Player marker prefab must be standalone when declaring its non-streaming RplComponent; inheriting MapMarkerEntityBase.et and adding RPL creates duplicate components"
}
$playerMarkerRplComponentCount = ([regex]::Matches($playerMarkerPrefabText, "\bRplComponent\b")).Count
if ($playerMarkerRplComponentCount -ne 1) {
	throw "Player marker prefab must declare exactly one RplComponent; duplicate RPL leaves SCR_MapMarkerEntity rplComp null at runtime, found $playerMarkerRplComponentCount"
}
foreach ($requiredPlayerMarkerPrefabEntry in @(
		'SCR_MapMarkerEntity',
		'RplComponent',
		'SpatialRelevancy 0',
		'Streamable Disabled'
	)) {
	if ($playerMarkerPrefabText -notmatch [regex]::Escape($requiredPlayerMarkerPrefabEntry)) {
		throw "Player marker prefab must declare exactly one non-streaming local RplComponent like native squad marker prefabs: $requiredPlayerMarkerPrefabEntry"
	}
}
if ($playerMarkerConfigText -notmatch [regex]::Escape('m_sMarkerLayout "{6985327711306214}UI/layouts/HST/Map/HST_PlayerMapMarkerDynamic.layout"')) {
	throw "Player map marker config must use the HST dynamic marker layout"
}
if (!(Test-Path "UI/layouts/HST/Map/HST_PlayerMapMarkerDynamic.layout")) {
	throw "Player marker layout is missing: UI/layouts/HST/Map/HST_PlayerMapMarkerDynamic.layout"
}
$playerMarkerLayoutText = Get-Content -Raw "UI/layouts/HST/Map/HST_PlayerMapMarkerDynamic.layout"
$playerMarkerDynamicWidgetText = Get-Content -Raw "Scripts/Game/HST/Map/HST_PlayerMapMarkerDynamicWComponent.c"
foreach ($requiredPlayerMarkerLayoutEntry in @(
		"Name `"HST_PlayerMapMarkerDynamic`"",
		"HST_PlayerMapMarkerDynamicWComponent",
		"Name `"MarkerIcon`"",
		"Name `"MarkerText`""
	)) {
	if ($playerMarkerLayoutText -notmatch [regex]::Escape($requiredPlayerMarkerLayoutEntry)) {
		throw "Player marker layout is missing dynamic marker widget contract: $requiredPlayerMarkerLayoutEntry"
	}
}
if ($playerMarkerLayoutText -match "SCR_MapMarkerSquadMemberComponent") {
	throw "Player marker layout must not use the squad-member component for SCR_MapMarkerEntity dynamic markers"
}
foreach ($requiredPlayerMarkerFacingEntry in @(
		"WHISPER_ICON_FORWARD_OFFSET_DEGREES = -50.0",
		"GetYawPitchRoll()",
		"yawPitchRoll[0] + WHISPER_ICON_FORWARD_OFFSET_DEGREES",
		"m_wMarkerIcon.SetRotation(rotation)",
		"GetGame().GetCallqueue().CallLater(UpdateFacingRotation, FACING_UPDATE_INTERVAL_MS, true)",
		"GetGame().GetCallqueue().Remove(UpdateFacingRotation)",
		"return m_MarkerEnt.GetTarget()"
	)) {
	if ($playerMarkerDynamicWidgetText -notmatch [regex]::Escape($requiredPlayerMarkerFacingEntry)) {
		throw "Player marker widget must rotate the whisper icon from player facing and stop its update loop on detach: $requiredPlayerMarkerFacingEntry"
	}
}
if ($playerMarkerDynamicWidgetText -match "m_wMarkerText\.SetRotation") {
	throw "Player marker facing must rotate only the icon, not the label text"
}
if ($playerMarkerConfigText -notmatch [regex]::Escape('SCR_MapMarkerConfig : "{3583D42139D9A10B}Configs/Map/CampaignMapMarkerConfig.conf"')) {
	throw "Player map marker config must inherit CampaignMapMarkerConfig.conf so existing campaign markers stay registered"
}
if ($playerMarkerConfigText -match [regex]::Escape('HST_PlayerMapMarkerEntry "{6985327711306212}"')) {
	throw "Player map marker entry must not reuse the config resource GUID as its inner entry id"
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
		"m_mDesiredNativeMarkers",
		"m_MarkerDirector",
		"m_NativeReconciler",
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
		"OBSERVATION_POST",
		"MAX_NATIVE_MARKERS = 192",
		"MAX_NATIVE_TACTICAL_MARKERS = 48"
	)) {
	if ($runtimeMarkerPipelineText -notmatch [regex]::Escape($requiredRuntimeMarkerEntry)) {
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

$hstConflictMarkerPrefab = "Prefabs/Systems/MilitaryBase/HST_ConflictMarkerBase.et"
$hstConflictMarkerMeta = "$hstConflictMarkerPrefab.meta"
if (!(Test-Path $hstConflictMarkerPrefab) -or !(Test-Path $hstConflictMarkerMeta)) {
	throw "Missing stripped HST conflict marker prefab or metadata"
}

$hstConflictMarkerText = Get-Content -Raw $hstConflictMarkerPrefab
if ($hstConflictMarkerText -match "AmbientPatrolSpawnpoint" -or $hstConflictMarkerText -match "SCR_AmbientPatrolSpawnPointComponent") {
	throw "Stripped HST conflict marker prefab must not include ambient patrol spawnpoints"
}

foreach ($layerToCheck in @($runtimeMarkerLayer, (Get-Content -Raw "Worlds/HST_Everon/HST_Everon_Layers/StartingPoints.layer"), (Get-Content -Raw "Worlds/HST_Dev/HST_Dev_Layers/StartingPoints.layer"))) {
	if ($layerToCheck -match "Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et") {
		throw "HST marker/hideout layers must use HST_ConflictMarkerBase instead of stock ConflictMilitaryBase"
	}

	if ($layerToCheck -match "AmbientPatrolSpawnpoint_FIA" -or $layerToCheck -match "5CCED514A5908822" -or $layerToCheck -match "606078EE23DE48AC") {
		throw "HST marker/hideout layers must not carry inherited FIA ambient patrol spawnpoint references"
	}
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
	"hideout_north_forest"  = "6332.167 75.926 8446.294"
	"hideout_central_hills" = "4280.766 14.317 3468.06"
	"hideout_south_woods"   = "8355.991 237.817 4765.673"
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
$devStartingPointsLayer = Get-Content -Raw "Worlds/HST_Dev/HST_Dev_Layers/StartingPoints.layer"
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

foreach ($startingPointMarkerLayer in @(
		@{ Label = "Everon starting points"; Text = $everonStartingPointsLayer },
		@{ Label = "Dev starting points"; Text = $devStartingPointsLayer }
)) {
	if ($startingPointMarkerLayer.Text -match "Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et" -or $startingPointMarkerLayer.Text -match "AmbientPatrolSpawnpoint_FIA") {
		throw "$($startingPointMarkerLayer.Label) must use stripped HST conflict markers without FIA ambient patrol references"
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

foreach ($requiredNotificationControllerEntry in @(
		"class HST_NotificationToastController",
		'NOTIFICATION_TOAST_LAYOUT = "{A34F448C7E830600}UI/layouts/HST_NotificationToast.layout"',
		"static HST_NotificationToastController Get()",
		"m_aQueue",
		"void Show(string eventId, string category, string severity, string title, string message, float durationSeconds)",
		"workspace.CreateWidgets(NOTIFICATION_TOAST_LAYOUT, workspace)",
		'root.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS)',
		'FindAnyWidget("Title")',
		'FindAnyWidget("Message")',
		'FindAnyWidget("AccentLine")',
		"HST_UIRootService.Get().NotifyNotificationShown()",
		"HST_UIRootService.Get().NotifyNotificationHidden()",
		"QueueReadyGeometry(generation)",
		"protected void LogReadyGeometry(int generation)",
		"GetGame().GetCallqueue().CallLater(DismissCurrent",
		"m_iToastGeneration",
		"protected void DismissCurrent(int generation)",
		"generation != m_iToastGeneration"
	)) {
	if ($notificationToastControllerText -notmatch [regex]::Escape($requiredNotificationControllerEntry)) {
		throw "Notification toast controller must own passive anchored toast rendering and queueing: $requiredNotificationControllerEntry"
	}
}
foreach ($requiredNotificationCallerEntry in @(
		"HST_NotificationToastController.Get().Show"
	)) {
	if ($commandMenuComponentText -notmatch [regex]::Escape($requiredNotificationCallerEntry)) {
		throw "Command menu external notifications must enqueue through HST_NotificationToastController: $requiredNotificationCallerEntry"
	}
	if ($missionClientText -notmatch [regex]::Escape($requiredNotificationCallerEntry)) {
		throw "Mission notifications must enqueue through HST_NotificationToastController: $requiredNotificationCallerEntry"
	}
}
foreach ($forbiddenNotificationComponentRenderer in @(
		"NOTIFICATION_TOAST_LAYOUT",
		"RenderExternalNotification",
		"QueueExternalNotification",
		"TickExternalNotification",
		"ClearExternalNotificationWidgets",
		"RenderTopMissionNotification",
		"QueueTopMissionNotification",
		"ShowNextQueuedNotification",
		"m_bNotificationVisible",
		"m_bExternalNotificationVisible",
		"m_fNotificationRemaining",
		"m_fExternalNotificationRemaining"
	)) {
	if ($commandMenuComponentText -match [regex]::Escape($forbiddenNotificationComponentRenderer)) {
		throw "Command menu must not own notification toast widgets or timers after controller extraction: $forbiddenNotificationComponentRenderer"
	}
	if ($missionClientText -match [regex]::Escape($forbiddenNotificationComponentRenderer)) {
		throw "Mission client must not own notification toast widgets or timers after controller extraction: $forbiddenNotificationComponentRenderer"
	}
}
foreach ($forbiddenCommandNotificationHelper in @(
		"SCRIPTED_PANEL_ROOT_LAYOUT",
		"CreateExternalRectWidget",
		"CreateExternalTextWidget",
		"SetupExternalCanvasRect",
		"m_aExternalNotificationCommandSets"
	)) {
	if ($commandMenuComponentText -match [regex]::Escape($forbiddenCommandNotificationHelper)) {
		throw "Command menu external notification must not keep script-created toast geometry: $forbiddenCommandNotificationHelper"
	}
}
foreach ($forbiddenNotificationGeometry in @(
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"CreateRectWidget(workspace, root, 0, 0",
		"CreateTextWidget(workspace, root, ShortenText(title",
		"CreateTextWidget(workspace, root, ShortenText(message"
	)) {
	if ($notificationToastControllerText -match [regex]::Escape($forbiddenNotificationGeometry)) {
		throw "Notification toast controller must not keep script-created toast geometry: $forbiddenNotificationGeometry"
	}
}
Write-Host "Anchored notification toast layout OK"

foreach ($missionDialogLayoutPath in @(
		"UI/layouts/HST_ReportDialog.layout",
		"UI/layouts/HST_ReportDialog.layout.meta",
		"UI/layouts/HST_ActionDialog.layout",
		"UI/layouts/HST_ActionDialog.layout.meta",
		"UI/layouts/HST/Rows/HST_ReportObjectiveRow.layout",
		"UI/layouts/HST/Rows/HST_ReportObjectiveRow.layout.meta"
	)) {
	if (!(Test-Path $missionDialogLayoutPath)) {
		throw "Mission dialog layout resource is missing: $missionDialogLayoutPath"
	}
}
foreach ($removedScriptedPanelLayoutPath in @(
		"UI/layouts/HST_ScriptedPanelRoot.layout",
		"UI/layouts/HST_ScriptedPanelRoot.layout.meta"
	)) {
	if (Test-Path $removedScriptedPanelLayoutPath) {
		throw "Legacy scripted panel layout must be deleted after report/action dialogs moved to named layouts: $removedScriptedPanelLayoutPath"
	}
}
$reportDialogLayoutText = Get-Content -Raw "UI/layouts/HST_ReportDialog.layout"
$actionDialogLayoutText = Get-Content -Raw "UI/layouts/HST_ActionDialog.layout"
$reportObjectiveRowLayoutText = Get-Content -Raw "UI/layouts/HST/Rows/HST_ReportObjectiveRow.layout"
foreach ($requiredReportDialogLayoutEntry in @(
		'Name "HST_ReportDialogRoot"',
		'Name "Dialog"',
		'Name "Title"',
		'Name "Subtitle"',
		'Name "CloseButton"',
		'Name "CloseLabel"',
		'Name "LocationLabel"',
		'Name "LocationValue"',
		'Name "MapPositionLabel"',
		'Name "MapPositionValue"',
		'Name "RequirementLabel"',
		'Name "RequirementValue"',
		'Name "ProgressLabel"',
		'Name "ProgressValue"',
		'Name "RewardLabel"',
		'Name "RewardValue"',
		'Name "FailureLabel"',
		'Name "FailureValue"',
		'Name "ObjectivesTitle"',
		'Name "ObjectiveScroll"',
		'Name "ObjectiveItems"',
		"SizeY 5",
		"OffsetBottom -5",
		"SizeY 36",
		"OffsetBottom -58",
		"SizeY 32",
		"OffsetBottom -94",
		"SizeY 42",
		"OffsetBottom -62"
	)) {
	if ($reportDialogLayoutText -notmatch [regex]::Escape($requiredReportDialogLayoutEntry)) {
		throw "Mission report dialog layout is missing named widget: $requiredReportDialogLayoutEntry"
	}
}
foreach ($forbiddenReportDialogLayoutEntry in @(
		"CloseBody",
		"CloseBackground",
		"ButtonWidgetSlot",
		"OffsetBottom 5",
		"OffsetBottom 58",
		"OffsetBottom 94",
		"OffsetBottom 62"
	)) {
	if ($reportDialogLayoutText -match [regex]::Escape($forbiddenReportDialogLayoutEntry)) {
		throw "Mission report dialog layout must use passive sibling button labels and fixed-height negative bottom offsets: $forbiddenReportDialogLayoutEntry"
	}
}
if ($reportDialogLayoutText -notmatch 'Name "HST_ReportDialogRoot"[\s\S]*?"Ignore Cursor" 0[\s\S]*?\{') {
	throw "Mission report dialog root must be cursor-active so the modal blocks clicks behind it"
}
foreach ($requiredActionDialogLayoutEntry in @(
		'Name "HST_ActionDialogRoot"',
		'Name "Dialog"',
		'Name "Title"',
		'Name "Message"',
		'Name "CancelButton"',
		'Name "CancelLabel"',
		'Name "ConfirmButton"',
		'Name "ConfirmLabel"',
		"SizeY 5",
		"OffsetBottom -5",
		"SizeY 38",
		"OffsetBottom -66",
		"SizeY 46",
		"OffsetBottom -316"
	)) {
	if ($actionDialogLayoutText -notmatch [regex]::Escape($requiredActionDialogLayoutEntry)) {
		throw "Mission action dialog layout is missing named widget: $requiredActionDialogLayoutEntry"
	}
}
foreach ($forbiddenActionDialogLayoutEntry in @(
		"CancelBody",
		"ConfirmBody",
		"ButtonWidgetSlot",
		"OffsetBottom 5",
		"OffsetBottom 66",
		"OffsetBottom -24"
	)) {
	if ($actionDialogLayoutText -match [regex]::Escape($forbiddenActionDialogLayoutEntry)) {
		throw "Mission action dialog layout must use passive sibling button labels and fixed-height negative bottom offsets: $forbiddenActionDialogLayoutEntry"
	}
}
if ($actionDialogLayoutText -notmatch 'Name "HST_ActionDialogRoot"[\s\S]*?"Ignore Cursor" 0[\s\S]*?\{') {
	throw "Mission action dialog root must be cursor-active so the modal blocks clicks behind it"
}
foreach ($requiredReportObjectiveRowLayoutEntry in @(
		'Name "HST_ReportObjectiveRow"',
		'Name "Label"',
		'Name "Value"',
		"OffsetRight -12",
		"OffsetBottom 28",
		"OffsetBottom -7"
	)) {
	if ($reportObjectiveRowLayoutText -notmatch [regex]::Escape($requiredReportObjectiveRowLayoutEntry)) {
		throw "Mission report objective row layout is missing named widget: $requiredReportObjectiveRowLayoutEntry"
	}
}
foreach ($forbiddenReportObjectiveRowLayoutEntry in @(
		"OffsetRight 12",
		"OffsetBottom -28",
		"OffsetBottom 7"
	)) {
	if ($reportObjectiveRowLayoutText -match [regex]::Escape($forbiddenReportObjectiveRowLayoutEntry)) {
		throw "Mission report objective row layout must use Enfusion offset signs for anchored row children: $forbiddenReportObjectiveRowLayoutEntry"
	}
}
foreach ($requiredMissionDialogComponentEntry in @(
		"HST_ReportDialogData data = new HST_ReportDialogData()",
		'data.m_sOwner = "HST_MissionClientComponent"',
		'data.m_sDebugOwner = "mission_report_dialog"',
		"data.m_iCloseWidgetId = DETAIL_CLOSE_WIDGET_ID",
		"m_iFrameSerial",
		"m_Client.OnWidgetClicked(w.GetUserID(), button)",
		"OnWidgetClicked(int widgetId, int button = 0)",
		"IsDuplicateWidgetActivation(widgetId, button)",
		"CanHandleMissionDialogInput",
		'HST_UIRootService.Get().CanHandleModalInput(HST_EUIScreenMode.MISSION_DIALOG, "HST_MissionClientComponent")',
		"data.m_sReportId = m_sSelectedMissionId",
		"data.m_aObjectiveLabels.Insert",
		"data.m_aObjectiveValues.Insert",
		"HST_ReportDialogController.Render(workspace, data, m_WidgetHandler)",
		'HST_ReportDialogController.Close("HST_MissionClientComponent")',
		"protected bool OpenMissionDetailsAtIndex",
		"protected bool RenderMissionDetailPanel",
		"if (RenderMissionDetailPanel())",
		"protected void CloseMissionDetails"
	)) {
	if ($missionClientText -notmatch [regex]::Escape($requiredMissionDialogComponentEntry)) {
		throw "Mission client must delegate report dialog layout population to HST_ReportDialogController: $requiredMissionDialogComponentEntry"
	}
}
if ($missionClientText -match [regex]::Escape("m_Client.OnWidgetClicked(w.GetUserID());")) {
	throw "Mission report widget handler must pass button state through the duplicate-activation guard"
}
foreach ($requiredMissionDialogControllerEntry in @(
		'REPORT_DIALOG_LAYOUT = "{D66CFA01E5AA4100}UI/layouts/HST_ReportDialog.layout"',
		'REPORT_OBJECTIVE_ROW_LAYOUT = "{D66CFA01E5AA4300}UI/layouts/HST/Rows/HST_ReportObjectiveRow.layout"',
		"workspace.CreateWidgets(REPORT_DIALOG_LAYOUT, workspace)",
		"workspace.CreateWidgets(REPORT_OBJECTIVE_ROW_LAYOUT, items)",
		'HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.MISSION_DIALOG, data.m_sOwner, root',
		'BindClick(data.m_sDebugOwner, root, "CloseButton", data.m_iCloseWidgetId, handler)',
		'SetText(root, "Title"',
		'root.FindAnyWidget("ObjectiveItems")',
		"root.RemoveFromHierarchy()",
		"protected static void QueueReadyGeometry",
		"protected static void LogReadyGeometry",
		"protected static void BindClick",
		"protected static void SetText",
		"protected static void AddObjectiveRow"
	)) {
	if ($reportDialogControllerText -notmatch [regex]::Escape($requiredMissionDialogControllerEntry)) {
		throw "Mission report controller must populate named layout widgets: $requiredMissionDialogControllerEntry"
	}
}
$missionDialogScriptText = $missionClientText + "`n" + $reportDialogControllerText
if ($missionClientText -match [regex]::Escape('RequestOpen(HST_EUIScreenMode.MISSION_DIALOG, "HST_MissionClientComponent", null')) {
	throw "Mission detail modal must not register with UI root before a report dialog root exists"
}
foreach ($forbiddenMissionDialogScriptGeometry in @(
		"SCRIPTED_PANEL_ROOT_LAYOUT",
		"DETAIL_ROOT_Z",
		"HST_MissionClientDrawCommandSet",
		"m_aCanvasCommandSets",
		"CreatePanelRow",
		"CreateRectWidget(workspace, root",
		"CreateTextWidget(workspace, root",
		"FrameSlot.SetPos(root",
		"FrameSlot.SetSize(root"
	)) {
	if ($missionDialogScriptText -match [regex]::Escape($forbiddenMissionDialogScriptGeometry)) {
		throw "Mission detail/report UI must not keep scripted panel geometry: $forbiddenMissionDialogScriptGeometry"
	}
}
foreach ($requiredNotificationVisibilityGuard in @(
		"m_bVisible",
		"ClearCurrent",
		"HST_UIRootService.Get().NotifyNotificationHidden()"
	)) {
	if ($notificationToastControllerText -notmatch [regex]::Escape($requiredNotificationVisibilityGuard)) {
		throw "Notification cleanup must guard UI-root notification state: $requiredNotificationVisibilityGuard"
	}
}
Write-Host "Mission dialog layout contract OK"

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
$nativeMarkerPublishPipelineText = $mapMarkerServiceText + "`n" + $campaignMarkerDirectorText + "`n" + $nativeMarkerReconcilerText
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
		"manager.InsertStaticMarker(nativeMarker, record.m_bLocalOnly, record.m_bServerMarker)",
		"manager.RemoveStaticMarker(marker)"
	)) {
	if ($nativeMarkerPublishPipelineText -notmatch [regex]::Escape($requiredNativeMarkerPublishContract)) {
		throw "Map marker service is missing native marker publish contract: $requiredNativeMarkerPublishContract"
	}
}
Write-Host "Faction marker color contract OK"

$definedSymbols = @([regex]::Matches($scriptText, "(?:class|enum)\s+(HST_[A-Za-z0-9_]+)") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
foreach ($enumBlockMatch in [regex]::Matches($scriptText, "(?s)(?:modded\s+)?enum\s+[A-Za-z0-9_]+\s*(?::\s*[A-Za-z0-9_]+)?\s*\{(?<body>.*?)\}")) {
	$definedSymbols = @($definedSymbols + @([regex]::Matches($enumBlockMatch.Groups["body"].Value, "(?m)^\s*(HST_[A-Za-z0-9_]+)\b") |
			ForEach-Object { $_.Groups[1].Value }))
}
$definedSymbols = @($definedSymbols | Sort-Object -Unique)
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

if (($coordinatorText -match "\bReportBool\s*\(") -and ($coordinatorText -notmatch "protected\s+string\s+ReportBool\s*\(\s*bool\s+value\s*\)")) {
	throw "Campaign coordinator calls ReportBool but does not define a class-local helper; Enforce does not share protected helpers across services"
}

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

foreach ($requiredCampaignDebugSummaryEntry in @(
		"critical failures %1",
		"CountCampaignDebugCriticalFailures",
		"AppendCampaignDebugFailureDetails",
		"failure details",
		"FindCampaignDebugPrimaryFailureAssertion",
		"BuildCampaignDebugFailureReference",
		"BuildCampaignDebugFailurePosition",
		"ResolveCampaignDebugFailureReason",
		"BuildCampaignDebugSuggestedInspectionCommand",
		"Suggested next inspection:"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugSummaryEntry)) {
		throw "Campaign debug summary must include critical failure details for forensic triage: $requiredCampaignDebugSummaryEntry"
	}
}
Write-Host "Campaign debug critical failure summary OK"

foreach ($requiredPersistenceSmokeEntry in @(
		"persistence.restore.report_exact",
		"restoredReportHealthy",
		"CampaignDebugPersistenceReportHealthy(restoredReport)",
		"restored persistence smoke report failed or reported missing data"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPersistenceSmokeEntry)) {
		throw "Missing persistence smoke restored-report drift contract entry: $requiredPersistenceSmokeEntry"
	}
}
Write-Host "Persistence smoke restored-report drift contract OK"

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

foreach ($requiredSupportRuntimeProbeEntry in @(
		"support.physical_population",
		"convoy.crew.population",
		"physical_combat.population",
		".physical_population",
		"CampaignDebugResolvePendingActiveGroupPopulation",
		"ResolveCampaignDebugMissionConvoyPopulation",
		"m_bRuntimeSpawnProbeRanBeforePopulation",
		"m_bRuntimeSpawnProbeChangedBeforePopulation",
		"support.physical_spawn_probe_ran",
		"support.physical_spawn_probe_changed",
		"m_bPendingPopulationResolvedBeforeRoute",
		"CaptureCampaignDebugSupportRequestMarkerSnapshot",
		"m_bMarkerVisibleAfterRequest",
		"linked support marker published immediately after request before runtime resolution",
		"support.physical_terminal_resolution",
		"support.physical_map_destination",
		"support.physical_spawn_offset",
		"support.physical_spawn_clearance",
		"spawn_placement.support_clear_offset",
		"m_bAvoidLivingPlayers",
		"m_bAvoidActiveGroups",
		"SUPPORT_MIN_PLAYER_CLEARANCE_METERS",
		"SUPPORT_MIN_ACTIVE_GROUP_CLEARANCE_METERS",
		"ResolveNearestActiveGroupDistanceMeters"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSupportRuntimeProbeEntry)) {
		throw "Missing physical support runtime debug proof entry: $requiredSupportRuntimeProbeEntry"
	}
}
Write-Host "Physical support runtime debug proof OK"

foreach ($requiredPhysicalCombatStrictEntry in @(
		"CAMPAIGN_DEBUG_COMBAT_PROBE_SAMPLE_SECONDS = 45",
		"physical_combat.faction_hostility",
		"physical_combat.contact_distance",
		"AreRuntimeFactionKeysHostile",
		"IsFactionEnemy",
		"ConvoyDebugStatus(contactObserved)",
		'ConvoyDebugStatus(casualtyObserved, "WARN")'
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPhysicalCombatStrictEntry)) {
		throw "Missing strict physical-combat runtime proof entry: $requiredPhysicalCombatStrictEntry"
	}
}
Write-Host "Physical combat contact proof OK"

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

foreach ($requiredCampaignDebugBuildEntry in @(
		"HST_BuildInfo",
		"BUILD_SHA",
		"BUILD_UTC",
		"BUILD_LABEL",
		"preflight.build_provenance",
		"m_sBuildSha",
		"m_sBuildUtc",
		"m_sBuildLabel",
		"run.build.sha",
		"BuildSummary()",
		'lines.Insert("build " + HST_BuildInfo.BuildSummary())'
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugBuildEntry)) {
		throw "Campaign debug build provenance contract missing: $requiredCampaignDebugBuildEntry"
	}
}
foreach ($requiredEnemyTargetScoringEntry in @(
		"HST_FactionRelationService",
		"ResolveRelation",
		"RELATION_RIVAL",
		"HST_EnemyTargetScoreCandidate",
		"HST_EnemyTargetScoreResult",
		"BuildTargetScoreResult",
		"BuildEnemyTargetScoreReport",
		"ResolveOrderTypeForDebug",
		"weighted_top_band",
		"rival_enemy_pressure",
		"IsEligibleTargetZone",
		"HST_ZONE_HIDEOUT",
		"HST_ZONE_MISSION_SITE",
		"enemy_target_scoring.contract.runtime",
		"enemy_target_scoring.high_value_selection",
		"enemy_target_scoring.excludes_bookkeeping_zones",
		"enemy_target_scoring.relation_owner_scores",
		"enemy_target_scoring.relation_order_types",
		"RecordCampaignDebugCase(BuildCampaignDebugEnemyTargetScoringCase())"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredEnemyTargetScoringEntry)) {
		throw "Enemy commander target scoring proof contract missing: $requiredEnemyTargetScoringEntry"
	}
}
foreach ($requiredRuntimeVehicleUnclaimEntry in @(
		"ClearVehicleFactionAffiliationRecursive",
		"ClearVehicleFactionAffiliationRecursiveCount",
		"CountVehicleFactionClaimsRecursive",
		"IsVehicleRootLikeEntity",
		"vehicle claimed faction %1",
		"vehicles are unclaimed",
		"runtime vehicle faction cleared"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredRuntimeVehicleUnclaimEntry)) {
		throw "Runtime vehicle unclaimed ownership contract missing: $requiredRuntimeVehicleUnclaimEntry"
	}
}
foreach ($requiredCampaignDebugProofEntry in @(
		"m_sProofLevel",
		"m_sObservedPath",
		"m_sRequiredPath",
		"m_bCountsTowardCertification",
		"ShouldCampaignDebugAssertionCountTowardCertification",
		"FinalizeCampaignDebugCertificationSummary",
		"certification proven %1/%2",
		"EXTERNAL_PROCESS",
		"CONTROLLED_RUNTIME",
		"PHYSICAL_RUNTIME"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugProofEntry)) {
		throw "Campaign debug certification proof metadata missing: $requiredCampaignDebugProofEntry"
	}
}
foreach ($requiredCampaignDebugHQRebuildEntry in @(
		"rebuildPlacementBlocked",
		"runtimeObjectsPhysicallyProven",
		"hq.rebuild.existing_runtime_preserved",
		"placement-blocked rebuild leaves existing HQ runtime proof to the existing-runtime case",
		"m_bCampaignDebugHQRebuildAwaitingSettle",
		"hq.rebuild.settled_world_scan",
		"waiting one runtime tick before world-proof assertions",
		"if (!rebuildPlacementBlocked)",
		"CampaignDebugStatus(runtimeObjectsProven || runtimeObjectsPhysicallyProven)"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugHQRebuildEntry)) {
		throw "Campaign debug HQ rebuild proof must not convert placement-blocked rebuilds into existing-runtime failures: $requiredCampaignDebugHQRebuildEntry"
	}
}
foreach ($requiredCampaignDebugExternalBlockEntry in @(
		'AddCampaignDebugAssertion(gapCase, "phase25.real_restart", "real restart-after-primitive explicitly reported as not executed", "manual external gap", "BLOCKED"',
		'AddCampaignDebugAssertion(gapCase, "phase25.second_client", "second-client join/reconnect explicitly reported as not executed", "manual external gap", "BLOCKED"',
		'AddCampaignDebugAssertion(gapCase, "phase25.two_hour_soak", "two-hour endurance soak explicitly reported as not executed", "manual external gap", "BLOCKED"',
		'AddCampaignDebugAssertion(persistenceCase, "persistence.real_restart", "external process restart / reconnect is not executed by this one-button in-memory probe", "not executed", "BLOCKED"'
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugExternalBlockEntry)) {
		throw "Campaign debug external gaps must be BLOCKED instead of WARN/PASS: $requiredCampaignDebugExternalBlockEntry"
	}
}
foreach ($requiredCampaignDebugProfileEntry in @(
		"admin_smoke",
		"foundation",
		"faction_physical",
		"support_physical",
		"mission_matrix_state",
		"mission_matrix_physical",
		"civilian_undercover",
		"arsenal_garage_build",
		"persistence_inprocess",
		"full_certification",
		"persistence_restart_external",
		"background_soak",
		"external_required",
		"IsCampaignDebugExternalProfile",
		"IsCampaignDebugFoundationOnlyProfile",
		"ShouldCampaignDebugPreservePersistenceSmokeState",
		"preflight.external_required",
		"Run Full Campaign Debug",
		"Debug Smoke Profile",
		"Debug Physical Profile",
		"Persistence Restart External",
		"Background Soak External"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugProfileEntry)) {
		throw "Campaign debug profile vocabulary missing: $requiredCampaignDebugProfileEntry"
	}
}
foreach ($requiredCampaignDebugSmokeCleanupEntry in @(
		'CleanupCampaignDebugPrefixedState(PERSISTENCE_SMOKE_PREFIX, "run completion persistence smoke cleanup")',
		"cleanup.smoke_prefixed_records",
		"normal non-restart debug profile",
		"external restart profile preserves hst_smoke sentinels",
		"post_restart_verify before cleanup can be certified"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugSmokeCleanupEntry)) {
		throw "Campaign debug cleanup must explicitly remove or externally preserve persistence smoke sentinels: $requiredCampaignDebugSmokeCleanupEntry"
	}
}
foreach ($requiredCampaignDebugMissionProofEntry in @(
		"IsCampaignDebugInstantOrAbstractPrimitive",
		"ShouldCampaignDebugHoldRuntimeCompletion",
		"HandleRuntimeMissionCompletionCandidate",
		"ReleaseCampaignDebugRuntimeCompletionHoldForPrimitiveProof",
		"runtime completion hold released after active primitive snapshot",
		"mission completion held before primitive proof",
		"mission.runtime.debug_hold",
		"mission runtime record missing, inactive, or completed before runtime proof",
		"primitive mission record disappeared before runtime action proof",
		"explicit abstract fallback is reported but does not certify primary runtime",
		"runtime spawned through the primary runtime path without fallback/failure",
		"mission runtime did not spawn cleanly through the primary path",
		'CampaignDebugStatus(runtimeFallbackCount == 0 && runtimeFailureCount == 0)',
		"primitive probe mission is not active before runtime action proof",
		"rescue.captive.active_before_action"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugMissionProofEntry)) {
		throw "Campaign debug mission proof must keep fallback separate from primary runtime proof: $requiredCampaignDebugMissionProofEntry"
	}
}
if ($scriptText -match [regex]::Escape('CampaignDebugStatus(runtimeFallbackCount == 0 && runtimeFailureCount == 0, "WARN")')) {
	throw "Mission runtime fallback/failure must fail certification-required active health instead of downgrading to WARN"
}
if ($scriptText -match 'bool\s+fallbackOk\s*=') {
	throw "Mission runtime proof must not hide fallback behind a fallbackOk helper"
}
foreach ($requiredCampaignDebugAreaProofEntry in @(
		"primitive.area.physical_combat_observed",
		"mission-owned or target-zone hostiles are observed in natural combat before objective pass",
		'"BLOCKED", areaCombatFailure',
		"area primitive physical combat was not observed before controlled objective setup",
		"area primitive had no hostile population to prove natural combat before objective tick"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugAreaProofEntry)) {
		throw "Campaign debug area primitive physical combat must stay blocked until naturally observed: $requiredCampaignDebugAreaProofEntry"
	}
}
foreach ($requiredRenderedCommandMenuProofEntry in @(
		"RunCampaignDebugRenderedCommandMenuProbeStep",
		"command_ui.rendered_command_menu",
		"SendCampaignDebugCommandMenuProofOwner",
		"RpcDo_CampaignDebugCommandMenuProof",
		"RpcAsk_ReportCampaignDebugCommandMenuProof",
		"RunCampaignDebugRenderedProof",
		"BuildCampaignDebugRenderedProofReport",
		"ReportCampaignDebugCommandMenuProof",
		"owner_client_widget_report",
		"owner client rendered widget visibility and geometry proof"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredRenderedCommandMenuProofEntry)) {
		throw "Campaign debug command-menu rendered proof must stay owner-client reported: $requiredRenderedCommandMenuProofEntry"
	}
}
Write-Host "Campaign debug build/proof/profile contract OK"

foreach ($requiredCommandMenuEntry in @(
		'ScriptComponentClass',
		'COMMAND_MENU_CUSTOM_ACTION = "HST_CommandMenu"',
		'MENU_INPUT_CONTEXT = "InGameMenuContext"',
		'INPUT_CONFIG = "Configs/HST/Input/HST_Input.conf"',
		'COMMAND_MENU_LAYOUT = "{A7B8C9D001234550}UI/layouts/HST_CommandMenu.layout"',
		'AddActionListener',
		'RemoveActionListener',
		'SetCustomConfigs',
		'EnsureInputConfig',
		'OnCustomCommandMenuInput(float value, EActionTrigger reason)',
		'keyboard:KC_I',
		'IsLocalOwner',
		'local player menu component ready',
		'CreateMenuRoot',
		'workspace.CreateWidgets(COMMAND_MENU_LAYOUT, workspace)',
		'COMMAND_SECTION_ROW_LAYOUT',
		'COMMAND_DATA_ROW_LAYOUT',
		'COMMAND_DATA_ROW_COMPACT_LAYOUT',
		'COMMAND_ACTION_ROW_LAYOUT',
		'COMMAND_PANEL_ACTION_ROW_LAYOUT',
		'COMMAND_FEED_ROW_LAYOUT',
		'COMMAND_ACTION_ROW_STRIDE',
		'FindRowText',
		'SetRowText',
		'SetRowImageColor',
		'BindRowClick',
		'SaveCommandMenuScrollOffsets',
		'RestoreScrollPixels',
		'm_ContentScroll',
		'm_ActionScroll',
		'm_FeedScroll',
		'ActivateContext(MENU_INPUT_CONTEXT)',
		'ActivateAction(COMMAND_MENU_CUSTOM_ACTION)',
		'GetActionTriggered(COMMAND_MENU_CUSTOM_ACTION)',
		'Debug.KeyState(KeyCode.KC_I)',
		'Debug.ClearKey(KeyCode.KC_I)',
		'keyState != 0',
		'HST_SetupMapComponent.IsSetupBlocking()',
		'm_fCommandMenuDebounceRemaining',
		'm_bCommandMenuKeyDownLastFrame',
		'm_bRawIKeyDownLastFrame',
		'm_iFrameSerial',
		'm_iLastActivatedFrame',
		'OnWidgetClicked(int widgetId, int button = 0)',
		'm_Menu.OnWidgetClicked(w.GetUserID(), button)',
		'IsDuplicateWidgetActivation(widgetId, button)',
		'PollCommandMenuInput',
		'PollRawCommandMenuKey',
		'TryToggleCommandMenu',
		'ApplyTextStyle',
		'SetLineSpacing',
		'SetOutline',
		'SetShadow',
		'SetTextWrapping(false)',
		'WidgetFlags.VISIBLE',
		'if (i >= 4)',
		'CountRowsForSection',
		'AddCommandContentRow',
		'AddCommandActionRow',
		'EnsureSelectedActionVisible',
		'ScrollToView',
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
		'HST_UIRootService.Get().CanOpen(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent")',
		'protected bool RenderMenu',
		'if (!RenderMenu())',
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
		'PlayerHasMapInInventory',
		'GetGadgetByType(EGadgetType.MAP)',
		'MAP_TARGET_OVERLAY_OWNER',
		'ChimeraMenuPreset.MapMenu',
		'GetOnSelection().Insert(OnCommandMapTargetSelection)',
		'ToggleLocationSelection',
		'HandleDialog(true)',
		'map_target:',
		'MapTargetDisabledReason',
		'map required',
		'recruit_zone',
		'RequestCommanderRecruitGarrisonAtMapTargetReport',
		'RequestCommanderRemoveGarrisonAtMapTargetReport',
		'mission_zone',
		'RequestCommanderCallSupplyDropAtMapTargetReport',
		'RequestCommanderCallPlayerSupportAtMapTargetReport',
		'support_qrf',
		'support_fire',
		'support_search',
		'support_gbu',
		'support_umpk',
		'support_kh55',
		'cancel_support',
		'phase23.ui.map_required_gate',
		'phase23.ui.map_target_support_actions',
		'phase23.ui.map_target_garrison_actions',
		'phase23.ui.no_hq_move_menu_actions',
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
foreach ($forbiddenCommandMenuPathBRegression in @(
		"CreateScrollList",
		"protected Widget CreateScrollContainer",
		"protected Widget CreateRectWidget",
		"protected bool SetupCanvasRect",
		"protected ref array<float> BuildRectVertices",
		"protected TextWidget CreateTextWidget",
		"protected TextWidget CreateWrappedTextWidget",
		"HST_CommandMenuDrawCommandSet",
		"m_aCanvasCommandSets",
		"VERTICAL_SCROLL_LIST_LAYOUT",
		"WRAP_SCROLL_GRID_LAYOUT",
		"CONTENT_PAGE_SIZE",
		"ACTION_PAGE_SIZE",
		"CONTENT_PREV_WIDGET_ID",
		"CONTENT_NEXT_WIDGET_ID",
		"ACTION_PREV_WIDGET_ID",
		"ACTION_NEXT_WIDGET_ID",
		"ScrollContentPage",
		"ScrollActionPage",
		"RenderContentPager",
		"RenderActionPager",
		'FindAnyWidget("Content")',
		"missing Content"
	)) {
	if ($commandMenuComponentText -match [regex]::Escape($forbiddenCommandMenuPathBRegression)) {
		throw "Command menu must not keep old manual scroll/pager pattern: $forbiddenCommandMenuPathBRegression"
	}
}
if ($commandMenuComponentText -match "(?<![A-Za-z0-9_])SCROLL_LIST_LAYOUT(?![A-Za-z0-9_])") {
	throw "Command menu must not keep old manual scroll/pager pattern: SCROLL_LIST_LAYOUT"
}
if ($commandMenuComponentText -match "FrameSlot\.Set(Pos|Size)\((scroll|items|content|row|tile)\b") {
	throw "Command menu Path B code must not position scroll/items/content/row/tile widgets with FrameSlot"
}
if ($commandMenuComponentText -match "CreateWidgetInWorkspace\(WidgetType\.CanvasWidgetTypeID") {
	throw "Command menu root must be a child-capable frame/layout container, not a canvas widget"
}
if ($commandMenuComponentText -match [regex]::Escape("m_Menu.OnWidgetClicked(w.GetUserID());")) {
	throw "Command menu widget handler must pass button state through the duplicate-activation guard"
}
if ($commandUiText -match [regex]::Escape('"Move base to my position"') -or $commandUiText -match [regex]::Escape('"Move HQ:')) {
	throw "Normal command menu must not expose HQ relocation actions after setup-map HQ placement"
}
if ($commandMenuComponentText -match [regex]::Escape('RequestOpen(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent", null')) {
	throw "Command menu must not register as open before its layout root exists"
}
if ($commandMenuComponentText -notmatch [regex]::Escape('root.RemoveFromHierarchy()')) {
	throw "Command menu must remove a rejected layout root when UI root registration fails"
}
foreach ($requiredCommandMenuRootReuseEntry in @(
		"protected Widget m_wMenuRoot",
		"protected Widget EnsureMenuRoot(WorkspaceWidget workspace)",
		"Widget root = EnsureMenuRoot(workspace)",
		"ClearDynamicMenuRows(root)",
		"protected void ClearDynamicMenuRows(Widget root)",
		"protected void ClearMenuContainerChildren(Widget root, string name)",
		"while (container.GetChildren())",
		"container.GetChildren().RemoveFromHierarchy()",
		"m_wMenuRoot = root",
		"m_wMenuRoot = null"
	)) {
	if ($commandMenuComponentText -notmatch [regex]::Escape($requiredCommandMenuRootReuseEntry)) {
		throw "Command menu must reuse its layout root and clear only dynamic row containers during data refresh: $requiredCommandMenuRootReuseEntry"
	}
}
$commandMenuRenderMatch = [regex]::Match($commandMenuComponentText, "protected bool RenderMenu\(\)[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected Widget EnsureMenuRoot")
if (!$commandMenuRenderMatch.Success) {
	throw "Command menu RenderMenu/EnsureMenuRoot boundary missing"
}
if ($commandMenuRenderMatch.Value -match [regex]::Escape("ClearWidgets();")) {
	throw "Command menu RenderMenu must not remove and recreate the whole layout root during data refresh"
}
if ($commandMenuComponentText -match "\bWidgetFlags\.WRAP_TEXT\b") {
	throw "Command menu fixed-height text must avoid automatic wrapping; shorten or clip instead"
}
foreach ($requiredCommandMenuActionDialogEntry in @(
		"ACTION_MODAL_CANCEL_WIDGET_ID",
		"ACTION_MODAL_CONFIRM_WIDGET_ID",
		"m_bActionDialogOpen",
		"m_sPendingActionCommand",
		"ShouldConfirmAction",
		'commandId == "new_campaign"',
		'commandId == "admin_purge_hst_native_markers"',
		'commandId == "move_hq_here"',
		'commandId == "mission_asset_sabotage"',
		'commandId == "mission_vehicle_capture"',
		'commandId == "activate_zone"',
		'commandId.Contains("admin_")',
		"BuildContextActionLabel",
		"ShowActionConfirmDialog",
		'ACTION_DIALOG_OWNER = "HST_CommandMenuActionDialog"',
		"HST_ActionDialogData data = new HST_ActionDialogData()",
		"data.m_sOwner = ACTION_DIALOG_OWNER",
		'data.m_sDebugOwner = "command_action_dialog"',
		"data.m_iCancelWidgetId = ACTION_MODAL_CANCEL_WIDGET_ID",
		"data.m_iConfirmWidgetId = ACTION_MODAL_CONFIRM_WIDGET_ID",
		"data.m_sMessage = BuildActionConfirmMessage(label, commandId, argument)",
		"HST_ActionDialogController.Render(workspace, data, m_WidgetHandler)",
		"HST_ActionDialogController.Close(ACTION_DIALOG_OWNER)",
		"CancelPendingActionDialog",
		"ConfirmPendingActionDialog",
		"ClearActionDialog",
		"RequestConfirmedAction",
		"CancelPendingActionDialog();",
		"IsCommandMenuTopmost",
		"CanHandleCommandMenuInput",
		"CanHandleActionDialogInput",
		'!IsCommandMenuTopmost()',
		'HST_UIRootService.Get().IsTopmost(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent")',
		'HST_UIRootService.Get().CanHandleScreenInput(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent")',
		'HST_UIRootService.Get().CanHandleModalInput(HST_EUIScreenMode.ACTION_DIALOG, ACTION_DIALOG_OWNER)'
	)) {
	if ($commandMenuComponentText -notmatch [regex]::Escape($requiredCommandMenuActionDialogEntry)) {
		throw "Command menu destructive/admin actions must use the named action-dialog modal: $requiredCommandMenuActionDialogEntry"
	}
}
foreach ($forbiddenCommandMenuLocalActionDialogEntry in @(
		"ACTION_DIALOG_LAYOUT",
		"workspace.CreateWidgets(ACTION_DIALOG_LAYOUT)",
		"HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.ACTION_DIALOG, ACTION_DIALOG_OWNER, root",
		"HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.ACTION_DIALOG, ACTION_DIALOG_OWNER)",
		'BindMenuClick(root, "CancelButton", ACTION_MODAL_CANCEL_WIDGET_ID)',
		'BindMenuClick(root, "ConfirmButton", ACTION_MODAL_CONFIRM_WIDGET_ID)'
	)) {
	if ($commandMenuComponentText -match [regex]::Escape($forbiddenCommandMenuLocalActionDialogEntry)) {
		throw "Command menu action modal must delegate layout ownership to HST_ActionDialogController: $forbiddenCommandMenuLocalActionDialogEntry"
	}
}
foreach ($forbiddenCommandMenuPanelMetric in @(
		"m_iRootLeft",
		"m_iRootTop",
		"m_iRootWidth",
		"m_iRootHeight",
		"m_iNavLeft",
		"m_iStatsLeft",
		"m_iMainLeft",
		"m_iRightLeft",
		"m_iActivityTop",
		"m_iActionsTop",
		"m_iActionsTextWidth",
		"ClampLayoutInt"
	)) {
	if ($commandMenuComponentText -match [regex]::Escape($forbiddenCommandMenuPanelMetric)) {
		throw "Command menu must not restore script-owned panel placement metrics: $forbiddenCommandMenuPanelMetric"
	}
}
foreach ($requiredCommandMenuLayerEntry in @(
		"protected void ApplyCommandMenuLayerOrder",
		"ApplyCommandMenuLayerOrder(root)",
		"SetMenuLayer(root, `"ScreenDimmer`", COMMAND_DIMMER_Z, true)",
		"SetMenuLayer(root, `"CommandSurface`", COMMAND_SURFACE_Z, true)",
		"SetMenuLayer(root, `"CloseLabel`", COMMAND_CLOSE_Z + 1, true)",
		'HST_UIDebug.LogReadyWidgetsCsv("command_menu_ready"',
		"RecoverStaleSetupRootStateForCommandOpen",
		'HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.SETUP_MAP, "HST_SetupMapComponent")',
		"open refused by UI root"
	)) {
	if ($commandMenuComponentText -notmatch [regex]::Escape($requiredCommandMenuLayerEntry)) {
		throw "Command menu must keep deterministic layout-layer ordering and ready diagnostics: $requiredCommandMenuLayerEntry"
	}
}
foreach ($requiredCommandMenuRenderedProofEntry in @(
		"CampaignDebugReportBool(report, `"menuOpen`")",
		"CampaignDebugReportBool(report, `"root`")",
		"CampaignDebugReportBool(report, `"readyOk`")",
		"return report.Contains(fieldName + `" true`") || report.Contains(fieldName + `" 1`")"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCommandMenuRenderedProofEntry)) {
		throw "Command menu rendered proof must parse Enfusion bool fields from client reports: $requiredCommandMenuRenderedProofEntry"
	}
}
$renderedProofWidgetListMatch = [regex]::Match($commandMenuComponentText, "protected void BuildCampaignDebugRenderedProofWidgetList[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool CampaignDebugWidgetHasUsableBounds")
if (!$renderedProofWidgetListMatch.Success) {
	throw "Missing command menu rendered proof widget-list boundary"
}
foreach ($requiredRenderedProofWidget in @(
		'HST_CommandMenuRoot',
		'CommandSurface',
		'Header',
		'NavigationPanel',
		'TabItems',
		'StatsPanel',
		'MainItems',
		'ActivityPanel',
		'ActionsItems'
	)) {
	if ($renderedProofWidgetListMatch.Value -notmatch [regex]::Escape($requiredRenderedProofWidget)) {
		throw "Command menu rendered proof is missing required visible widget: $requiredRenderedProofWidget"
	}
}
foreach ($optionalHiddenRenderedProofWidget in @(
		'HeaderTabTitle',
		'ActivityScroll',
		'ActivityItems'
	)) {
	if ($renderedProofWidgetListMatch.Value -match [regex]::Escape($optionalHiddenRenderedProofWidget)) {
		throw "Command menu rendered proof must not require optional or hidden-by-design widget: $optionalHiddenRenderedProofWidget"
	}
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
		"CommandSurface",
		'Name "Header"',
		"OffsetBottom -82",
		'Name "CloseButton"',
		"OffsetRight 24",
		"OffsetBottom -63",
		"NavigationPanel",
		"OffsetRight -220",
		'Name "NavigationTitle"',
		"OffsetBottom -44",
		"TabScroll",
		"OffsetRight 12",
		"OffsetBottom 12",
		"TabItems",
		"StatsPanel",
		"OffsetRight 524",
		"OffsetBottom -168",
		'Name "Stat0Label"',
		"OffsetBottom -28",
		"MainPanel",
		"OffsetRight 524",
		'Name "MainAccent"',
		"OffsetBottom -4",
		"MainScroll",
		"OffsetRight 24",
		"OffsetBottom 24",
		"MainItems",
		"ActivityPanel",
		"OffsetRight 20",
		"OffsetBottom 740",
		'Name "ActivityTitle"',
		'Text "Last Activity"',
		"OffsetBottom -48",
		'Name "ActivityResult"',
		"OffsetBottom 18",
		'Name "ActivityFeedTitle"',
		"OffsetBottom -130",
		'"Is Visible" 0',
		"ActivityScroll",
		"OffsetRight 20",
		'"Scrollbar Always Visible" 0',
		"ActionsPanel",
		"OffsetTop -710",
		"OffsetBottom 20",
		"ActionsScroll",
		"OffsetBottom 16",
		'Name "ActionsTitle"'
	)) {
	if ($commandMenuLayoutText -notmatch [regex]::Escape($requiredCommandMenuLayoutEntry)) {
		throw "Command menu layout is missing widget entry: $requiredCommandMenuLayoutEntry"
	}
}
foreach ($forbiddenCommandMenuLayoutEntry in @(
		"HST_CommandMenuDynamicCanvas",
		"CanvasWidgetClass",
		"OffsetBottom 63",
		"OffsetBottom 44",
		"OffsetBottom 168",
		"OffsetBottom 28",
		"OffsetBottom 4",
		"OffsetBottom 48",
		"OffsetBottom 126",
		"OffsetBottom 166",
		"OffsetRight 220",
		"OffsetRight -524",
		"OffsetBottom -520",
		"OffsetTop 500"
	)) {
	if ($commandMenuLayoutText -match [regex]::Escape($forbiddenCommandMenuLayoutEntry)) {
		throw "Command menu layout must not keep scripted-canvas placeholders or positive fixed-height bounds: $forbiddenCommandMenuLayoutEntry"
	}
}
$commandMenuResolvedGeometryContracts = @(
	@{
		Widget    = "NavigationPanel"
		Required  = @("Anchor 0 0 0 1", "OffsetRight -220", "OffsetBottom -20")
		Forbidden = @("OffsetRight 220")
	},
	@{
		Widget    = "StatsPanel"
		Required  = @("Anchor 0 0 1 0", "OffsetLeft 240", "OffsetRight 524", "OffsetBottom -168")
		Forbidden = @("OffsetRight -524")
	},
	@{
		Widget    = "MainPanel"
		Required  = @("Anchor 0 0 1 1", "OffsetLeft 240", "OffsetRight 524", "OffsetBottom 20")
		Forbidden = @("OffsetRight -524", "OffsetBottom -20")
	},
	@{
		Widget    = "ActivityPanel"
		Required  = @("Anchor 1 0 1 1", "OffsetLeft -500", "OffsetRight 20", "OffsetBottom 740")
		Forbidden = @("OffsetRight -20", "OffsetBottom -740")
	},
	@{
		Widget    = "ActionsPanel"
		Required  = @("Anchor 1 1 1 1", "OffsetLeft -500", "OffsetTop -710", "OffsetRight 20", "OffsetBottom 20")
		Forbidden = @("OffsetRight -20", "OffsetBottom -20")
	}
)
foreach ($commandMenuResolvedGeometryContract in $commandMenuResolvedGeometryContracts) {
	foreach ($requiredResolvedGeometryEntry in $commandMenuResolvedGeometryContract.Required) {
		$resolvedGeometryPattern = [regex]::Escape("Name `"$($commandMenuResolvedGeometryContract.Widget)`"") + "[\s\S]{0,320}?" + [regex]::Escape($requiredResolvedGeometryEntry)
		if ($commandMenuLayoutText -notmatch $resolvedGeometryPattern) {
			throw "Command menu layout has an invalid resolved geometry contract for $($commandMenuResolvedGeometryContract.Widget): missing $requiredResolvedGeometryEntry"
		}
	}
	foreach ($forbiddenResolvedGeometryEntry in $commandMenuResolvedGeometryContract.Forbidden) {
		$resolvedGeometryPattern = [regex]::Escape("Name `"$($commandMenuResolvedGeometryContract.Widget)`"") + "[\s\S]{0,320}?" + [regex]::Escape($forbiddenResolvedGeometryEntry)
		if ($commandMenuLayoutText -match $resolvedGeometryPattern) {
			throw "Command menu layout must not use runtime-distorting offset signs for $($commandMenuResolvedGeometryContract.Widget): $forbiddenResolvedGeometryEntry"
		}
	}
}

$scrollLayoutContracts = @(
	@{
		Path = "UI/layouts/HST_VerticalScrollList.layout"
		Guid = "{A7B8C9D001234560}"
		Name = "HST_VerticalScrollList"
		Flow = "VerticalLayoutWidgetClass"
	},
	@{
		Path = "UI/layouts/HST_WrapScrollGrid.layout"
		Guid = "{A7B8C9D001234570}"
		Name = "HST_WrapScrollGrid"
		Flow = "WrapLayoutWidgetClass"
	}
)
foreach ($scrollLayoutContract in $scrollLayoutContracts) {
	$scrollLayoutPath = $scrollLayoutContract.Path
	$scrollLayoutMetaPath = "$scrollLayoutPath.meta"
	if (!(Test-Path $scrollLayoutPath)) {
		throw "Missing Path B scroll layout resource: $scrollLayoutPath"
	}
	if (!(Test-Path $scrollLayoutMetaPath)) {
		throw "Missing GUID-backed Path B scroll layout meta resource: $scrollLayoutMetaPath"
	}

	$scrollLayoutText = Get-Content -Raw $scrollLayoutPath
	$scrollLayoutMetaText = Get-Content -Raw $scrollLayoutMetaPath
	foreach ($requiredPathBScrollLayoutEntry in @(
			$scrollLayoutContract.Name,
			"OverlayWidgetClass",
			'Name "ScrollHost"',
			"ScrollLayoutWidgetClass",
			$scrollLayoutContract.Flow,
			'Name "Scroll"',
			'Name "Items"',
			'"Scrollbar Always Visible" 1',
			"Anchor 0 0 0 0",
			"Anchor 0 0 1 1",
			"Slot OverlayWidgetSlot",
			"Slot AlignableSlot"
		)) {
		if ($scrollLayoutText -notmatch [regex]::Escape($requiredPathBScrollLayoutEntry)) {
			throw "$scrollLayoutPath is missing Path B scroll layout entry: $requiredPathBScrollLayoutEntry"
		}
	}
	if ($scrollLayoutText -match 'Name "ContentSize"' -or $scrollLayoutText -match "SizeLayoutWidgetClass") {
		throw "$scrollLayoutPath must use direct Items under Scroll, not the old ContentSize wrapper"
	}
	if ($scrollLayoutText -notmatch '(?s)ScrollLayoutWidgetClass\s+"\{[0-9A-F]+\}"\s+\{\s+Name "Scroll"[\s\S]*?Slot OverlayWidgetSlot') {
		throw "$scrollLayoutPath Scroll must use an OverlayWidgetSlot"
	}
	if ($scrollLayoutText -match "\bCanvasWidget(Class)?\b" -or $scrollLayoutText -match "\bPanelWidgetClass\b" -or $scrollLayoutText -match "\bImageWidgetClass\b") {
		throw "$scrollLayoutPath must stay transparent and must not include visible panel/canvas/image backgrounds"
	}
	if ($scrollLayoutText -match 'Name "Content"') {
		throw "$scrollLayoutPath must expose Items, not the old manual Content frame"
	}
	if ($scrollLayoutMetaText -notmatch [regex]::Escape("Name `"$($scrollLayoutContract.Guid)$scrollLayoutPath`"")) {
		throw "$scrollLayoutMetaPath must carry the expected non-zero GUID-qualified resource name"
	}
}

$rowLayoutContracts = @(
	@{ Path = "UI/layouts/HST/Rows/HST_CommandSectionRow.layout"; Guid = "{A7B8C9D001234580}"; Required = @('Name "HST_CommandSectionRow"', 'Slot AlignableSlot', 'Name "Background"', 'Name "Title"') },
	@{ Path = "UI/layouts/HST/Rows/HST_CommandDataRow.layout"; Guid = "{A7B8C9D001234590}"; Required = @('Name "HST_CommandDataRow"', 'Slot AlignableSlot', 'Name "Background"', 'Name "Label"', 'Name "Value"') },
	@{ Path = "UI/layouts/HST/Rows/HST_CommandDataRowCompact.layout"; Guid = "{A7B8C9D0012345A0}"; Required = @('Name "HST_CommandDataRowCompact"', 'Slot AlignableSlot', 'Name "Background"', 'Name "Label"', 'Name "Value"') },
	@{ Path = "UI/layouts/HST/Rows/HST_CommandActionRow.layout"; Guid = "{A7B8C9D0012345B0}"; Required = @('FrameWidgetClass', 'Name "HST_CommandActionRow"', 'Slot AlignableSlot', 'Name "Background"', 'Name "Label"', '"Ignore Cursor" 1') },
	@{ Path = "UI/layouts/HST/Rows/HST_CommandPanelActionRow.layout"; Guid = "{A7B8C9D0012365B0}"; Required = @('FrameWidgetClass', 'Name "HST_CommandPanelActionRow"', 'Slot AlignableSlot', 'WidthOverride 420', 'HeightOverride 56', 'Name "Background"', 'Name "Label"', '"Ignore Cursor" 1') },
	@{ Path = "UI/layouts/HST/Rows/HST_CommandFeedRow.layout"; Guid = "{A7B8C9D0012345C0}"; Required = @('Name "HST_CommandFeedRow"', 'Slot AlignableSlot', 'Name "Text"') },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutNodeRow.layout"; Guid = "{A7B8C9D0012345D0}"; Required = @('FrameWidgetClass', 'Name "HST_LoadoutNodeRow"', 'Slot AlignableSlot', 'Padding 0 0 0 8', 'Name "Background"', 'Name "PreviewBack"', 'Name "PreviewLine"', 'Name "PreviewAnchor"', 'Name "PreviewFallback"', 'Name "Primary"', 'Name "Secondary"', 'Name "OpenMarker"', '"Ignore Cursor" 1') },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutStorageRow.layout"; Guid = "{A7B8C9D0012345E0}"; Required = @('FrameWidgetClass', 'Name "HST_LoadoutStorageRow"', 'Slot AlignableSlot', 'Padding 0 0 0 8', 'Name "Background"', 'Name "PreviewBack"', 'Name "PreviewLine"', 'Name "PreviewAnchor"', 'Name "Primary"', 'Name "Secondary"', 'Name "Meta"', 'Name "VolumeBack"', 'ProgressBarWidgetClass', 'Name "VolumeFill"', 'style SimpleWithBackground', 'Maximum 1', 'Current 0', '"Ignore Cursor" 1') },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutStorageItemRow.layout"; Guid = "{A7B8C9D0012345F0}"; Required = @('FrameWidgetClass', 'Name "HST_LoadoutStorageItemRow"', 'Slot AlignableSlot', 'Padding 0 0 0 6', 'Name "Background"', 'Name "PreviewBack"', 'Name "PreviewLine"', 'Name "PreviewAnchor"', 'Name "Name"', 'Name "Count"', '"Ignore Cursor" 1') },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutCandidateTile.layout"; Guid = "{A7B8C9D001234600}"; Required = @('FrameWidgetClass', 'Name "HST_LoadoutCandidateTile"', 'Slot AlignableSlot', 'Padding 0 0 8 8', 'Name "Background"', 'Name "PreviewBack"', 'Name "PreviewLine"', 'Name "PreviewAnchor"', 'Name "Name"', 'Name "Count"', 'Name "EmptyText"', '"Ignore Cursor" 1') },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutTemplateSlotRow.layout"; Guid = "{A7B8C9D001234620}"; Required = @('ButtonWidgetClass', 'Name "HST_LoadoutTemplateSlotRow"', 'Slot AlignableSlot', 'Padding 0 0 0 8', 'HeightOverride 110', 'Name "Background"', 'Name "Accent"', 'Name "Primary"', 'Name "Secondary"', 'Name "Meta"', 'Name "SaveButton"', 'Name "SaveLabel"', 'Name "LoadButton"', 'Name "LoadLabel"', '"Ignore Cursor" 1') }
)
foreach ($rowLayoutContract in $rowLayoutContracts) {
	$rowLayoutPath = $rowLayoutContract.Path
	$rowLayoutMetaPath = "$rowLayoutPath.meta"
	if (!(Test-Path $rowLayoutPath)) {
		throw "Missing Path B row/tile layout resource: $rowLayoutPath"
	}
	if (!(Test-Path $rowLayoutMetaPath)) {
		throw "Missing GUID-backed Path B row/tile layout meta resource: $rowLayoutMetaPath"
	}

	$rowLayoutText = Get-Content -Raw $rowLayoutPath
	$rowLayoutMetaText = Get-Content -Raw $rowLayoutMetaPath
	foreach ($requiredRowLayoutEntry in $rowLayoutContract.Required) {
		if ($rowLayoutText -notmatch [regex]::Escape($requiredRowLayoutEntry)) {
			throw "$rowLayoutPath is missing row/tile prefab entry: $requiredRowLayoutEntry"
		}
	}
	if ($rowLayoutMetaText -notmatch [regex]::Escape("Name `"$($rowLayoutContract.Guid)$rowLayoutPath`"")) {
		throw "$rowLayoutMetaPath must carry the expected non-zero GUID-qualified resource name"
	}
	foreach ($requiredFlatRowEntry in @(
			"ButtonWidgetClass",
			"Color 1 1 1 0",
			"SizeLayoutWidgetClass",
			'Name "SizeLayout"',
			"Slot ButtonWidgetSlot",
			"AllowHeightOverride 1",
			"OverlayWidgetClass",
			'Name "Overlay"',
			'Name "Body"',
			"Slot AlignableSlot",
			"Slot OverlayWidgetSlot",
			"FrameWidgetClass"
		)) {
		if ($rowLayoutText -notmatch [regex]::Escape($requiredFlatRowEntry)) {
			throw "$rowLayoutPath must use Button/SizeLayout/Overlay row prefab shape: $requiredFlatRowEntry"
		}
	}
	if ($rowLayoutText -match 'Name "Content"') {
		throw "$rowLayoutPath must not use the old nested Content wrapper name"
	}
	if ($rowLayoutText -match '"Text Wrapping"') {
		throw "$rowLayoutPath must not use layout Text Wrapping keywords; wrap behavior belongs in script SetRowText"
	}
	if ($rowLayoutPath -match "HST_Command(SectionRow|DataRow|DataRowCompact|FeedRow|PanelActionRow)\.layout") {
		if ($rowLayoutText -notmatch "Slot AlignableSlot[\s\S]{0,140}?HorizontalAlign 0") {
			throw "$rowLayoutPath command row roots must be left aligned in scroll containers"
		}
		if ($rowLayoutText -match '"Horizontal Alignment" Center') {
			throw "$rowLayoutPath command row text must stay left aligned"
		}
	}
}

$loadoutStorageCategoryTabPath = "UI/layouts/HST/Rows/HST_LoadoutStorageCategoryTab.layout"
$loadoutStorageCategoryTabMetaPath = "$loadoutStorageCategoryTabPath.meta"
if (!(Test-Path $loadoutStorageCategoryTabPath)) {
	throw "Missing loadout storage category tab layout resource: $loadoutStorageCategoryTabPath"
}
if (!(Test-Path $loadoutStorageCategoryTabMetaPath)) {
	throw "Missing GUID-backed loadout storage category tab layout meta resource: $loadoutStorageCategoryTabMetaPath"
}
$loadoutStorageCategoryTabText = Get-Content -Raw $loadoutStorageCategoryTabPath
$loadoutStorageCategoryTabMetaText = Get-Content -Raw $loadoutStorageCategoryTabMetaPath
foreach ($requiredStorageCategoryTabEntry in @(
		"ButtonWidgetClass",
		'Name "HST_LoadoutStorageCategoryTab"',
		"Slot LayoutSlot",
		"Padding 0 0 8 0",
		"SizeLayoutWidgetClass",
		"WidthOverride 84",
		"HeightOverride 78",
		'Name "Background"',
		'Name "Accent"',
		'Name "Icon"',
		'Name "Fallback"',
		'"Ignore Cursor" 1',
		"OffsetBottom 3",
		"OffsetRight 11",
		"OffsetBottom 10",
		"OffsetRight 4",
		"OffsetBottom 4"
	)) {
	if ($loadoutStorageCategoryTabText -notmatch [regex]::Escape($requiredStorageCategoryTabEntry)) {
		throw "$loadoutStorageCategoryTabPath is missing storage category tab layout entry: $requiredStorageCategoryTabEntry"
	}
}
foreach ($forbiddenStorageCategoryTabEntry in @(
		"OffsetBottom -3",
		"OffsetRight -11",
		"OffsetBottom -10",
		"OffsetRight -4",
		"OffsetBottom -4"
	)) {
	if ($loadoutStorageCategoryTabText -match [regex]::Escape($forbiddenStorageCategoryTabEntry)) {
		throw "$loadoutStorageCategoryTabPath must use Enfusion offset signs for fixed and stretched tab children: $forbiddenStorageCategoryTabEntry"
	}
}
if ($loadoutStorageCategoryTabMetaText -notmatch [regex]::Escape('Name "{A7B8C9D001234610}UI/layouts/HST/Rows/HST_LoadoutStorageCategoryTab.layout"')) {
	throw "$loadoutStorageCategoryTabMetaPath must carry the expected non-zero GUID-qualified resource name"
}

$loadoutRowFontContracts = @(
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutNodeRow.layout"; FontGuids = @("{C1A9E1A2092846E0}", "{C1A9E1A2092846E1}", "{C1A9E1A2092846E2}", "{C1A9E1A2092846EA}") },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutStorageRow.layout"; FontGuids = @("{C1A9E1A2092846E3}", "{C1A9E1A2092846E4}", "{C1A9E1A2092846E5}") },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutStorageItemRow.layout"; FontGuids = @("{C1A9E1A2092846E6}", "{C1A9E1A2092846E7}") },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutCandidateTile.layout"; FontGuids = @("{C1A9E1A2092846E8}", "{C1A9E1A2092846E9}", "{C1A9E1A2092846EB}") },
	@{ Path = "UI/layouts/HST/Rows/HST_LoadoutTemplateSlotRow.layout"; FontGuids = @("{C1A9E1A2092846F0}", "{C1A9E1A2092846F1}", "{C1A9E1A2092846F2}", "{C1A9E1A2092846F3}", "{C1A9E1A2092846F4}") }
)
foreach ($loadoutRowFontContract in $loadoutRowFontContracts) {
	$loadoutRowFontText = Get-Content -Raw $loadoutRowFontContract.Path
	foreach ($loadoutRowFontGuid in $loadoutRowFontContract.FontGuids) {
		if ($loadoutRowFontText -notmatch [regex]::Escape("FontProperties FontProperties `"$loadoutRowFontGuid`"")) {
			throw "$($loadoutRowFontContract.Path) is missing explicit text font properties: $loadoutRowFontGuid"
		}
	}
	foreach ($requiredLoadoutRowFontEntry in @(
			'Font "{E2CBA6F76AAA42AF}UI/Fonts/Roboto/Roboto_Regular.fnt"',
			"DisabledColor 0.38 0.38 0.38 1"
		)) {
		if ($loadoutRowFontText -notmatch [regex]::Escape($requiredLoadoutRowFontEntry)) {
			throw "$($loadoutRowFontContract.Path) is missing explicit Roboto text font entry: $requiredLoadoutRowFontEntry"
		}
	}
}

$loadoutVerticalRowLayoutContracts = @(
	"UI/layouts/HST/Rows/HST_LoadoutNodeRow.layout",
	"UI/layouts/HST/Rows/HST_LoadoutStorageRow.layout",
	"UI/layouts/HST/Rows/HST_LoadoutStorageItemRow.layout"
)
foreach ($loadoutVerticalRowLayoutPath in $loadoutVerticalRowLayoutContracts) {
	$loadoutVerticalRowLayoutText = Get-Content -Raw $loadoutVerticalRowLayoutPath
	foreach ($requiredLoadoutVerticalRowEntry in @(
			"AllowWidthOverride 0",
			"AllowMinDesiredWidth 1",
			"MinDesiredWidth 300",
			"AllowMaxDesiredWidth 0"
		)) {
		if ($loadoutVerticalRowLayoutText -notmatch [regex]::Escape($requiredLoadoutVerticalRowEntry)) {
			throw "$loadoutVerticalRowLayoutPath must use responsive vertical row width behavior: $requiredLoadoutVerticalRowEntry"
		}
	}
	foreach ($forbiddenLoadoutVerticalRowEntry in @(
			"WidthOverride 360",
			"MinDesiredWidth 360",
			"MaxDesiredWidth 360"
		)) {
		if ($loadoutVerticalRowLayoutText -match [regex]::Escape($forbiddenLoadoutVerticalRowEntry)) {
			throw "$loadoutVerticalRowLayoutPath must not keep fixed 360px row width: $forbiddenLoadoutVerticalRowEntry"
		}
	}
}
foreach ($requiredLoadoutStorageItemRowEntry in @(
		"HeightOverride 76",
		"MinDesiredHeight 76",
		"MaxDesiredHeight 76",
		"OffsetRight -72",
		"OffsetBottom -72",
		"OffsetLeft 84",
		"OffsetTop 22"
	)) {
	$loadoutStorageItemRowText = Get-Content -Raw "UI/layouts/HST/Rows/HST_LoadoutStorageItemRow.layout"
	if ($loadoutStorageItemRowText -notmatch [regex]::Escape($requiredLoadoutStorageItemRowEntry)) {
		throw "Loadout storage item row is missing preview/count entry: $requiredLoadoutStorageItemRowEntry"
	}
}
$loadoutStorageRowText = Get-Content -Raw "UI/layouts/HST/Rows/HST_LoadoutStorageRow.layout"
foreach ($requiredLoadoutStorageRowProgressEntry in @(
		"ProgressBarWidgetClass",
		'Name "VolumeFill"',
		"Anchor 0 0 1 0",
		"OffsetLeft 86",
		"OffsetTop 74",
		"OffsetRight 10",
		"OffsetBottom -80",
		"style SimpleWithBackground",
		"Maximum 1",
		"Current 0"
	)) {
	if ($loadoutStorageRowText -notmatch [regex]::Escape($requiredLoadoutStorageRowProgressEntry)) {
		throw "Loadout storage row volume fill must be layout-owned progress bar: $requiredLoadoutStorageRowProgressEntry"
	}
}
foreach ($requiredLoadoutCandidateTileEntry in @(
		"WidthOverride 354",
		"HeightOverride 140",
		"MinDesiredWidth 354",
		"MinDesiredHeight 140",
		"MaxDesiredWidth 354",
		"MaxDesiredHeight 140",
		"OffsetRight -82",
		"OffsetBottom -82",
		"OffsetLeft 112",
		"SizeY 72",
		"OffsetRight 18",
		"OffsetLeft -72",
		"OffsetTop 88",
		"SizeY 28"
	)) {
	$loadoutCandidateTileText = Get-Content -Raw "UI/layouts/HST/Rows/HST_LoadoutCandidateTile.layout"
	if ($loadoutCandidateTileText -notmatch [regex]::Escape($requiredLoadoutCandidateTileEntry)) {
		throw "Loadout candidate tile is missing layout-owned tile entry: $requiredLoadoutCandidateTileEntry"
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
		"startingTrainingLevel",
		"captureProgressRequired",
		"captureProgressPerSecond",
		"captureDecayPerSecond",
		"captureAggressionBase",
		"captureCounterattackChancePercent",
		"autosaveIntervalSeconds",
		"activationRadiusMeters",
		"deactivationRadiusMeters",
		"playerRenderBubbleRadiusMeters",
		"missionSelectionRadiusMeters",
		"debugLoggingEnabled",
		"gameMasterBudgetsEnabled",
		"HST_GameMasterBudgetService",
		"SetHistasiGameMasterBudgetsEnabled",
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
		"warLevel2Score",
		"warLevel3Score",
		"warLevel4Score",
		"warLevel5Score",
		"warLevel6Score",
		"warLevel7Score",
		"warLevel8Score",
		"warLevel9Score",
		"warLevel10Score",
		"victoryControlPercent",
		"victoryRequiresAirfields",
		"victoryRequiresSeaports",
		"lossConditionEnabled",
		"lossHRThreshold",
		"lossMoneyThreshold",
		"lossPetrosDeathLimit",
		"lossGraceSeconds",
		"enemyAttackIncomeWarPercent",
		"enemySupportIncomeWarPercent",
		"aggressionDecayIntervalSeconds",
		"aggressionDecayAmount",
		"showPlayerMapMarkers",
		"infiniteStaminaEnabled",
		"trackResistanceSupportGroupsOnMap",
		"SetPlayerEventBubbleRadiusMeters",
		"mission_category",
		"populationOutcomeEnabled",
		"victoryPopulationSupportPercent",
		"legacyControlVictoryEnabled",
		"MigrateSettings",
		"ApplyTo"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredSettingsEntry)) {
		throw "Missing runtime settings generated-config contract entry: $requiredSettingsEntry"
	}
}
if ($scriptText -notmatch "SCHEMA_VERSION = 17") {
	throw "Runtime settings schema must be bumped to 17 for generated settings hideout-key removal"
}
if ($scriptText -match "m_sDefaultHideoutId" -or $scriptText -match '"defaultHideoutId"') {
	throw "Runtime settings JSON must not expose defaultHideoutId after map-based HQ selection"
}
if ($scriptText -notmatch "m_bGameMasterBudgetsEnabled" -or $scriptText -notmatch '\\"gameMasterBudgetsEnabled\\": %1') {
	throw "Runtime settings must expose gameMasterBudgetsEnabled"
}
if ($scriptText -notmatch "m_bShowPlayerMapMarkers" -or $scriptText -notmatch '\\"showPlayerMapMarkers\\": %1') {
	throw "Runtime settings must expose showPlayerMapMarkers"
}
if ($scriptText -notmatch "m_bInfiniteStaminaEnabled" -or $scriptText -notmatch '\\"infiniteStaminaEnabled\\": %1') {
	throw "Runtime settings must expose infiniteStaminaEnabled"
}
if ($scriptText -notmatch "m_bTrackResistanceSupportGroupsOnMap" -or $scriptText -notmatch '\\"trackResistanceSupportGroupsOnMap\\": %1') {
	throw "Runtime settings must expose trackResistanceSupportGroupsOnMap"
}
if ($scriptText -notmatch "m_iPlayerRenderBubbleRadiusMeters" -or $scriptText -notmatch '\\"playerRenderBubbleRadiusMeters\\": %1') {
	throw "Runtime settings must expose playerRenderBubbleRadiusMeters"
}
if ($scriptText -notmatch "m_iMissionSelectionRadiusMeters" -or $scriptText -notmatch '\\"missionSelectionRadiusMeters\\": %1') {
	throw "Runtime settings must expose missionSelectionRadiusMeters"
}
if ($scriptText -notmatch "m_bPopulationOutcomeEnabled" -or $scriptText -notmatch '\\"populationOutcomeEnabled\\": %1') {
	throw "Runtime settings must expose populationOutcomeEnabled"
}
if ($scriptText -notmatch "m_iVictoryPopulationSupportPercent" -or $scriptText -notmatch '\\"victoryPopulationSupportPercent\\": %1') {
	throw "Runtime settings must expose victoryPopulationSupportPercent"
}
if ($scriptText -notmatch "m_bLegacyControlVictoryEnabled" -or $scriptText -notmatch '\\"legacyControlVictoryEnabled\\": %1') {
	throw "Runtime settings must expose legacyControlVictoryEnabled"
}
if ($scriptText -notmatch "settings.m_Features.m_bGameMasterBudgetsEnabled = false") {
	throw "Runtime settings migration must default Game Master budgets to disabled"
}
foreach ($requiredGameMasterBudgetDiagnostic in @(
		"preflight.gm_budget.state",
		"preflight.gm_budget.policy",
		"preflight.gm_budget.editor_diagnostics",
		"preflight.gm_budget.disabled_shim",
		"preflight.gm_budget.disabled_spawn_probe",
		"RunCampaignDebugGameMasterBudgetDisabledSpawnProbe",
		"gm_budget_spawn_probe",
		'CampaignDebugStatus(editorReady, "BLOCKED")',
		'assertion.m_sAssertionId == "preflight.gm_budget.editor_diagnostics"',
		"diagnostic_unavailable",
		"SCR_EntityHelper.DeleteEntityAndChildren(probeEntity)",
		"HistasiBuildGameMasterBudgetDiagnostics",
		"HistasiIsBudgetDeficitHandlerRegistered",
		"HistasiIsBudgetCapEnabledForDiagnostics"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredGameMasterBudgetDiagnostic)) {
		throw "Campaign debug preflight must prove Game Master budget policy: $requiredGameMasterBudgetDiagnostic"
	}
}
foreach ($requiredGameMasterBudgetShimEntry in @(
		"ResolveDisabledBudgetHeadroom",
		"ResolveDisabledBudgetRepairHeadroom",
		"budgetSettings.SetCurrentBudget(disabledBudget)",
		"modded class SCR_EditableEntityCoreBudgetSetting",
		"HistasiEnsureDisabledBudgetHeadroomBeforeSubtract",
		"restored disabled-budget headroom before native subtract",
		"HistasiCountManagedCurrentBudgetsAtDisabledHeadroom",
		"HistasiCountBudgetDeficitCorrections",
		"trackedHeadroom",
		"preSubtractRepairs",
		"deficitCorrections"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredGameMasterBudgetShimEntry)) {
		throw "Game Master budget shim must pre-balance disabled-budget accounting before native clamp paths: $requiredGameMasterBudgetShimEntry"
	}
}
if ($scriptText -notmatch "settings.m_Features.m_bShowPlayerMapMarkers = true") {
	throw "Runtime settings migration must default player map markers to enabled"
}
if ($scriptText -notmatch "settings.m_Features.m_bInfiniteStaminaEnabled = true") {
	throw "Runtime settings migration must default infinite stamina to enabled"
}
if ($scriptText -notmatch "settings.m_Features.m_bTrackResistanceSupportGroupsOnMap = true") {
	throw "Runtime settings migration must default resistance support tracking to enabled"
}
foreach ($requiredInfiniteStaminaEntry in @(
		"FEATURE|infiniteStamina|%1",
		"RpcAsk_RequestRuntimeFeatureSettings",
		"RpcDo_ReceiveRuntimeFeatureSettings",
		"CharacterStaminaComponent.Cast",
		"stamina.AddStamina",
		"SetHistasiInfiniteStaminaVisualSuppressed",
		"modded class SCR_StaminaBlurEffect",
		"ClearHistasiStaminaVisuals",
		"super.DisplayOnSuspended"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredInfiniteStaminaEntry)) {
		throw "Infinite stamina runtime path must expose refill and sprint-vignette suppression: $requiredInfiniteStaminaEntry"
	}
}
if ($scriptText -notmatch "m_iArsenalUnlockThreshold = 18") {
	throw "Runtime/balance defaults must set arsenal unlock threshold to 18"
}
if ($scriptText -notmatch "m_iHQInteractionRadiusMeters = 50") {
	throw "Runtime/balance defaults must set HQ interaction radius to 50 meters"
}
if ($configResourceText -notmatch "m_iHQInteractionRadiusMeters 50") {
	throw "Balance config must set HQ interaction radius to 50 meters"
}
if ($configResourceText -notmatch "m_iArsenalUnlockThreshold 18") {
	throw "Balance config must set arsenal unlock threshold to 18"
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

$runtimeSettingsText = Get-Content -Raw "Scripts/Game/HST/Config/HST_RuntimeSettings.c"
$runtimeSettingsServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_RuntimeSettingsService.c"
$configModelsText = Get-Content -Raw "Scripts/Game/HST/Config/HST_ConfigModels.c"
$arsenalServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ArsenalService.c"
$lootServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_LootService.c"
$loadoutEditorText = Get-Content -Raw "Scripts/Game/HST/Services/HST_LoadoutEditorService.c"
$campaignStateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
$arsenalEquivalenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ArsenalItemEquivalence.c"
$campaignSaveDataText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$loadoutPreviewWorldText = Get-Content -Raw "Prefabs/HST/HST_LoadoutPreviewWorld.et"
$loadoutPreviewLightsText = Get-Content -Raw "Prefabs/HST/HST_LoadoutPreviewLights.et"
$loadoutPreviewSkyText = Get-Content -Raw "Prefabs/HST/HST_LoadoutPreviewSky.emat"
$loadoutPreviewSkySphereText = Get-Content -Raw "Prefabs/HST/HST_LoadoutPreviewSkySphere.et"
$loadoutPreviewVisualText = $loadoutPreviewWorldText + "`n" + $loadoutPreviewSkyText + "`n" + $loadoutPreviewSkySphereText
foreach ($requiredPaperMapEquivalenceEntry in @(
		"AreEquivalentPrefabs",
		"IsPaperMapPrefab",
		'prefab.Contains("prefabs/items/equipment/maps/")',
		'prefab.Contains("papermap_")',
		'prefab.Contains("paper_map_")',
		'prefab.Contains("map_paper_")'
	)) {
	if ($arsenalEquivalenceText -notmatch [regex]::Escape($requiredPaperMapEquivalenceEntry)) {
		throw "Paper map arsenal equivalence helper is missing: $requiredPaperMapEquivalenceEntry"
	}
}
foreach ($requiredPaperMapUsageEntry in @(
		"FindArsenalItem",
		"FindIssuedLoadoutItem",
		"HST_ArsenalItemEquivalence.IsPaperMapPrefab",
		"HST_ArsenalItemEquivalence.AreEquivalentPrefabs"
	)) {
	if ($campaignStateText -notmatch [regex]::Escape($requiredPaperMapUsageEntry)) {
		throw "Campaign state must resolve paper-map equivalent arsenal and issued-loadout items: $requiredPaperMapUsageEntry"
	}
}
foreach ($requiredPaperMapLoadoutEntry in @(
		"FindCostEntry",
		"CountLoadoutItem",
		"HST_ArsenalItemEquivalence.AreEquivalentPrefabs"
	)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredPaperMapLoadoutEntry)) {
		throw "Loadout editor accounting must group paper-map equivalent prefabs: $requiredPaperMapLoadoutEntry"
	}
}
foreach ($requiredPhase14ConfigEntry in @(
		"HST_ArsenalItemRule",
		"m_sPrefabContains",
		"m_sCategory",
		"m_sPolicy",
		"m_iUnlockThresholdOverride",
		"m_bAppliesToAreaLoot",
		"m_bAppliesToVehicleLoot",
		"m_aArsenalItemRules",
		"finite_only",
		"blocked",
		"MissionProp_CitySupplies",
		"MissionProp_ConvoyPayload",
		"MissionProp_DestroyTarget"
	)) {
	if ($configModelsText -notmatch [regex]::Escape($requiredPhase14ConfigEntry) -and $configResourceText -notmatch [regex]::Escape($requiredPhase14ConfigEntry) -and $defaultCatalog -notmatch [regex]::Escape($requiredPhase14ConfigEntry)) {
		throw "Phase 14 arsenal item policy config is missing: $requiredPhase14ConfigEntry"
	}
}
if ($runtimeSettingsText -match "m_aArsenalItemRules" -or $runtimeSettingsServiceText -match "m_aArsenalItemRules") {
	throw "Runtime settings JSON must remain scalar-only; arsenal item rules belong in typed balance config"
}
foreach ($requiredPhase14ArsenalEntry in @(
		"CanDepositItem",
		"RefundItem",
		"FindItemRule",
		"ResolveUnlockPolicy",
		"ResolveUnlockThreshold",
		"policy %4",
		"threshold %5",
		"raw visual/support asset is not loot",
		"finite_only",
		"blocked"
	)) {
	if ($arsenalServiceText -notmatch [regex]::Escape($requiredPhase14ArsenalEntry)) {
		throw "Phase 14 arsenal service is missing policy/accounting entry: $requiredPhase14ArsenalEntry"
	}
}
foreach ($requiredPhase14LootEntry in @(
		"IsRawNonLootAsset",
		"Assets/Images/",
		"Assets/Objects/",
		".png",
		".edds",
		".xob",
		".fbx",
		".txo",
		"vehicleLoot",
		"m_bVehicleLootOnlyLockedItems",
		"m_bLootOnlyLockedItems",
		"arsenal.CanDepositItem"
	)) {
	if ($lootServiceText -notmatch [regex]::Escape($requiredPhase14LootEntry)) {
		throw "Phase 14 loot service is missing policy-aware/raw-asset entry: $requiredPhase14LootEntry"
	}
}
foreach ($requiredPhase14VehiclePersistenceEntry in @(
		"SnapshotNearbyPersistentVehicles",
		"RestorePersistentFieldVehicles",
		"ShouldRestorePersistentFieldVehicle",
		"PERSISTENT_FIELD_VEHICLE_SNAPSHOT_RADIUS_METERS",
		"PERSISTENT_FIELD_VEHICLE_RESTORE_RADIUS_METERS",
		'record.m_sRuntimeKind == "loot_vehicle"',
		'record.m_sRuntimeKind == "field_vehicle"',
		"m_Loot.SnapshotNearbyPersistentVehicles(m_State)",
		"m_Loot.RestorePersistentFieldVehicles(m_State)"
	)) {
	if ($lootServiceText -notmatch [regex]::Escape($requiredPhase14VehiclePersistenceEntry) -and $coordinatorText -notmatch [regex]::Escape($requiredPhase14VehiclePersistenceEntry)) {
		throw "Phase 14 vehicle persistence contract is missing: $requiredPhase14VehiclePersistenceEntry"
	}
}
foreach ($requiredPhase14LoadoutEntry in @(
		"HST_LoadoutCostEntry",
		"BuildLoadoutCostLedger",
		"RollbackLoadoutWithdrawals",
		"m_iAdditionalFiniteRequired",
		"arsenal.RefundItem",
		"CommitLoadoutTransaction(state, arsenal, loadout, identityId, costLedger",
		"ValidateLoadoutTransaction(state, loadout, identityId, costLedger"
	)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredPhase14LoadoutEntry) -and $scriptText -notmatch [regex]::Escape($requiredPhase14LoadoutEntry)) {
		throw "Phase 14 loadout apply accounting is missing: $requiredPhase14LoadoutEntry"
	}
}
foreach ($requiredPhase14SaveCopyEntry in @(
		"target.m_sSerializedLoadout = source.m_sSerializedLoadout",
		"target.m_sClothingSummary = source.m_sClothingSummary",
		"target.m_sWeaponSummary = source.m_sWeaponSummary",
		"target.m_sRequiredItemsSummary = source.m_sRequiredItemsSummary",
		"target.m_iSlotIndex = source.m_iSlotIndex",
		"target.m_sParentSlotId = source.m_sParentSlotId",
		"target.m_sStorageId = source.m_sStorageId",
		"target.m_sSlotKind = source.m_sSlotKind"
	)) {
	if ($campaignSaveDataText -notmatch [regex]::Escape($requiredPhase14SaveCopyEntry)) {
		throw "Campaign save copy must preserve Phase 14 loadout field: $requiredPhase14SaveCopyEntry"
	}
}
foreach ($requiredPhase14CommandEntry in @(
		"admin_phase14_seed_finite",
		"admin_phase14_seed_threshold",
		"admin_phase14_seed_blocked",
		"admin_phase14_report",
		"RequestAdminPhase14SeedFinite",
		"RequestAdminPhase14SeedThreshold",
		"RequestAdminPhase14SeedBlocked",
		"RequestAdminPhase14Report"
	)) {
	if ($commandUIServiceText -notmatch [regex]::Escape($requiredPhase14CommandEntry) -and $coordinatorText -notmatch [regex]::Escape($requiredPhase14CommandEntry)) {
		throw "Phase 14 command surface is missing: $requiredPhase14CommandEntry"
	}
}
foreach ($requiredPhase14PreviewEntry in @(
		"SkyPreset",
		"Assets/Objects/Plane.xob",
		"HST_LoadoutPreviewGround.emat",
		"HST_LoadoutPreviewSkySphere.et",
		"Assets/Objects/sphere.xob",
		"GameEnvironmentProbeEntity"
	)) {
	if ($loadoutPreviewVisualText -notmatch [regex]::Escape($requiredPhase14PreviewEntry)) {
		throw "Loadout preview world is missing Phase 14 visual/support asset: $requiredPhase14PreviewEntry"
	}
}
foreach ($requiredPhase14PreviewLightEntry in @(
		"LightEntity",
		"Hierarchy",
		"Dynamic 1",
		"CastShadow 0",
		"SourceSize"
	)) {
	if ($loadoutPreviewLightsText -notmatch [regex]::Escape($requiredPhase14PreviewLightEntry)) {
		throw "Loadout preview light rig is missing Phase 14 visual/support asset: $requiredPhase14PreviewLightEntry"
	}
}
foreach ($requiredLoadoutPreviewDarkEntry in @(
		"ColorBottom 0.10 0.13 0.15 0",
		"ColorZenith 0.015 0.025 0.035 0",
		"ColorUp 0.06 0.08 0.09 0",
		"IntensityLV -8.0",
		"SunSpotIntensityLV 0.75",
		"DirectLightLV 0.25",
		"DirectLightColor 0.70 0.76 0.76 0",
		"IndirectLightLV -0.25",
		"IndirectLightColor 0.22 0.25 0.25 0",
		"ManualHDRBrightnessLV -0.25",
		"Color 0.78 0.82 0.78 0",
		"LV 5.6",
		"Radius 24",
		"SourceSize 2.5"
	)) {
	if ($loadoutPreviewVisualText -notmatch [regex]::Escape($requiredLoadoutPreviewDarkEntry) -and $loadoutPreviewLightsText -notmatch [regex]::Escape($requiredLoadoutPreviewDarkEntry)) {
		throw "Loadout preview world is missing dark-style visual entry: $requiredLoadoutPreviewDarkEntry"
	}
}
Write-Host "Phase 14 arsenal/loot/loadout contracts OK"

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
		"registered vehicle prefab mismatch",
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
		"stored garage record before delete; deleted verified root vehicle",
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
if ($loadoutEditorComponentText -notmatch "protected void BindEditorHandler[\s\S]*?FindHandler\(HST_LoadoutEditorWidgetHandler\)[\s\S]*?widget\.AddHandler\(m_WidgetHandler\)") {
	throw "Loadout editor reused layout widgets must bind handlers through idempotent BindEditorHandler"
}
$loadoutEditorDirectHandlerAdds = @([regex]::Matches($loadoutEditorComponentText, "\.AddHandler\(m_WidgetHandler\)"))
if ($loadoutEditorDirectHandlerAdds.Count -ne 1) {
	throw "Loadout editor must not repeatedly AddHandler(m_WidgetHandler) outside BindEditorHandler"
}
if ($loadoutEditorComponentText -notmatch 'LOADOUT_TEXT_FONT = "\{E2CBA6F76AAA42AF\}UI/Fonts/Roboto/Roboto_Regular\.fnt"' -or $loadoutEditorComponentText -notmatch "protected void SetLoadoutText[\s\S]*?textWidget\.SetFont\(LOADOUT_TEXT_FONT\)[\s\S]*?textWidget\.SetForceFont\(true\)") {
	throw "Loadout editor common text setter must force a known font for layout-owned labels"
}
$loadoutMouseUpHandlerMatch = [regex]::Match($loadoutEditorComponentText, "override bool OnMouseButtonUp[\s\S]*?\r?\n\t}\r?\n\r?\n\toverride bool OnMouseEnter")
if (!$loadoutMouseUpHandlerMatch.Success -or $loadoutMouseUpHandlerMatch.Value -match [regex]::Escape("OnWidgetClickedWithButton(widgetId, button)")) {
	throw "Loadout editor mouse-up handler must not also dispatch normal button clicks; OnClick owns activation"
}
$loadoutMouseDownHandlerMatch = [regex]::Match($loadoutEditorComponentText, "override bool OnMouseButtonDown[\s\S]*?\r?\n\t}\r?\n\r?\n\toverride bool OnMouseButtonUp")
if (!$loadoutMouseDownHandlerMatch.Success -or $loadoutMouseDownHandlerMatch.Value -notmatch [regex]::Escape("return m_Editor.OnWidgetMouseButtonDown(widgetId, x, y, button)")) {
	throw "Loadout editor mouse-down handler must route to component feedback handling"
}
$loadoutPressedFeedbackMatch = [regex]::Match($loadoutEditorComponentText, "protected bool ApplyPressedButtonFeedback[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool ShouldApplyPressedFeedback[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected Widget ResolveButtonFeedbackBackground")
if (!$loadoutPressedFeedbackMatch.Success) {
	throw "Loadout editor pressed-button feedback contract is missing"
}
foreach ($requiredLoadoutPressedFeedbackEntry in @(
		"if (widgetId != PREVIEW_DRAG_WIDGET_ID)",
		"if (button == 0)",
		"ApplyPressedButtonFeedback(widgetId, true)",
		"if (m_iPressedWidgetId >= 0)",
		"ApplyPressedButtonFeedback(m_iPressedWidgetId, false)",
		"m_iPressedWidgetId = widgetId",
		"color = GetAccentColor()",
		"m_iPressedWidgetId = -1",
		"background.SetColorInt(color)",
		"widgetId >= SETTINGS_LIGHT_MINUS_WIDGET_ID && widgetId <= SETTINGS_WORLD_PRESET_WIDGET_ID",
		"widgetId == SETTINGS_RESET_WIDGET_ID"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutPressedFeedbackEntry)) {
		throw "Loadout editor buttons must show accent-color feedback on mouse down and reset on mouse up: $requiredLoadoutPressedFeedbackEntry"
	}
}
foreach ($requiredSetupUIDebugEntry in @(
		'HST_UIDebug.LogLayoutCreate("setup_map"',
		'HST_UIDebug.LogExpectedWidgetsCsv("setup_map"',
		'HST_UIDebug.LogLayoutCreate("setup_prompt"',
		'HST_UIDebug.LogExpectedWidgetsCsv("setup_prompt"',
		'HST_UIDebug.LogPopulation("setup_prompt"',
		'HST_UIDebug.LogLayoutCreate("setup_confirm_modal"',
		'HST_UIDebug.LogLayoutRejected("setup_confirm_modal"',
		'HST_UIDebug.LogExpectedWidgetsCsv("setup_confirm_modal"',
		'HST_UIDebug.LogPopulation("setup_confirm_modal"'
	)) {
	if ($setupMapComponentText -notmatch [regex]::Escape($requiredSetupUIDebugEntry)) {
		throw "Setup UI layouts must emit debug diagnostics: $requiredSetupUIDebugEntry"
	}
}
foreach ($requiredNotificationUIDebugEntry in @(
		'HST_UIDebug.LogLayoutCreate("notification_toast"',
		'HST_UIDebug.LogExpectedWidgetsCsv("notification_toast"',
		'HST_UIDebug.LogPopulation("notification_toast"'
	)) {
	if ($notificationToastControllerText -notmatch [regex]::Escape($requiredNotificationUIDebugEntry)) {
		throw "Notification toast layout must emit debug diagnostics: $requiredNotificationUIDebugEntry"
	}
}
$missionReportDebugText = $missionClientText + "`n" + $reportDialogControllerText
foreach ($requiredMissionUIDebugEntry in @(
		'data.m_sDebugOwner = "mission_report_dialog"',
		'HST_UIDebug.LogLayoutCreate(data.m_sDebugOwner',
		'HST_UIDebug.LogLayoutRejected(data.m_sDebugOwner',
		'HST_UIDebug.LogExpectedWidgetsCsv(data.m_sDebugOwner',
		'HST_UIDebug.LogPopulation(data.m_sDebugOwner',
		'HST_UIDebug.LogWidgetBound(debugOwner',
		'HST_UIDebug.LogRowSample("mission_report_objectives"',
		'HST_UIDebug.LogRowSummary("mission_report_objectives"'
	)) {
	if ($missionReportDebugText -notmatch [regex]::Escape($requiredMissionUIDebugEntry)) {
		throw "Mission report layouts must emit debug diagnostics: $requiredMissionUIDebugEntry"
	}
}
$commandUIDebugText = $commandMenuComponentText + "`n" + $actionDialogControllerText
foreach ($requiredCommandUIDebugEntry in @(
		'HST_UIDebug.LogLayoutCreate("command_menu"',
		'HST_UIDebug.LogExpectedWidgetsCsv("command_menu"',
		'HST_UIDebug.LogPopulation("command_menu"',
		'HST_UIDebug.LogRowSample("command_menu_tabs"',
		'HST_UIDebug.LogRowSummary("command_menu_tabs"',
		'HST_UIDebug.LogRowSample("command_menu_main"',
		'HST_UIDebug.LogRowSummary("command_menu_main"',
		'HST_UIDebug.LogRowSample("command_menu_activity"',
		'HST_UIDebug.LogRowSummary("command_menu_activity"',
		'HST_UIDebug.LogRowSample("command_menu_actions"',
		'HST_UIDebug.LogRowSummary("command_menu_actions"',
		'HST_UIDebug.LogNamedChildSummaryCsv("command_menu_ready"',
		'data.m_sDebugOwner = "command_action_dialog"',
		'HST_UIDebug.LogLayoutCreate(data.m_sDebugOwner',
		'HST_UIDebug.LogLayoutRejected(data.m_sDebugOwner',
		'HST_UIDebug.LogExpectedWidgetsCsv(data.m_sDebugOwner',
		'HST_UIDebug.LogWidgetBound(debugOwner',
		'HST_UIDebug.LogPopulation("command_action_dialog"'
	)) {
	if ($commandUIDebugText -notmatch [regex]::Escape($requiredCommandUIDebugEntry)) {
		throw "Command UI layouts must emit debug diagnostics: $requiredCommandUIDebugEntry"
	}
}
foreach ($requiredLoadoutUIDebugEntry in @(
		'HST_UIDebug.LogLayoutCreate("loadout_editor"',
		'HST_UIDebug.LogLayoutRejected("loadout_editor"',
		'HST_UIDebug.LogExpectedWidgetsCsv("loadout_editor"',
		'HST_UIDebug.LogPopulation("loadout_editor"',
		'HST_UIDebug.LogRowSample("loadout_mode_tabs"',
		'HST_UIDebug.LogRowSummary("loadout_mode_tabs"',
		'HST_UIDebug.LogRowSample("loadout_slot_rows"',
		'HST_UIDebug.LogRowSummary("loadout_slot_rows"',
		'HST_UIDebug.LogRowSample("loadout_storage_containers"',
		'HST_UIDebug.LogRowSummary("loadout_storage_containers"',
		'HST_UIDebug.LogRowSample("loadout_storage_items"',
		'HST_UIDebug.LogRowSummary("loadout_storage_items"',
		'HST_UIDebug.LogRowSample("loadout_candidate_rows"',
		'HST_UIDebug.LogRowSummary("loadout_candidate_rows"',
		'HST_UIDebug.LogRowSample("loadout_storage_candidates"',
		'HST_UIDebug.LogRowSummary("loadout_storage_candidates"',
		'HST_UIDebug.LogRowSample("loadout_templates"',
		'HST_UIDebug.LogRowSummary("loadout_templates"',
		'HST_UIDebug.LogRowSample("loadout_storage_category_tabs"',
		'HST_UIDebug.LogRowSummary("loadout_storage_category_tabs"',
		'HST_UIDebug.LogPopulation("loadout_storage_filter_controls"',
		'HST_UIDebug.LogRowSample("loadout_preview_cells"',
		'HST_UIDebug.LogNamedChildSummaryCsv("loadout_editor_ready"',
		"DebugLoadoutNodeRow",
		"DebugLoadoutCandidateRow",
		"DebugLoadoutTemplateRow",
		"DebugLoadoutPanelButton"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutUIDebugEntry)) {
		throw "Loadout editor layouts must emit debug diagnostics: $requiredLoadoutUIDebugEntry"
	}
}
Write-Host "UI layout debug instrumentation OK"
foreach ($requiredLoadoutEditorToastEntry in @(
		"EDITOR_TOAST_DURATION_MS = 2600",
		"protected string m_sEditorToastText",
		"protected int m_iEditorToastGeneration",
		"protected string BuildStageToast()",
		"return m_sEditorToastText",
		"protected void ShowHint(string text)",
		"m_iEditorToastGeneration++",
		"m_sEditorToastText = text",
		"CallLater(ClearEditorToast, EDITOR_TOAST_DURATION_MS, false, m_iEditorToastGeneration)",
		"protected void ClearEditorToast(int generation)",
		"generation != m_iEditorToastGeneration",
		"Render Preview Reset",
		"Loadout Editor Settings Reset",
		"Saved to Loadout Slot %1",
		"Loaded Loadout Slot %1"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutEditorToastEntry)) {
		throw "Loadout editor action notifications must use editor-local toast text: $requiredLoadoutEditorToastEntry"
	}
}
if ($loadoutEditorComponentText -notmatch "CloseEditorInternal[\s\S]*?GetGame\(\)\.GetCallqueue\(\)\.Remove\(ClearEditorToast\)") {
	throw "Loadout editor must clear pending local toast timers when closing"
}
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
		"PREVIEW_FALLBACK_PREFAB",
		"client render preview",
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
		"loadout_editor_refresh",
		"set_node_item",
		"set_attachment",
		"remove_node_item",
		"remove_attachment",
		"add_storage_item",
		"loadout_clear_draft",
		"FindUsableDepositStorages",
		"ResolveLiveStorageTargets",
		"ClearSpawnedCargoStorageContents",
		"FindCargoDepositStorages",
		"FindCargoDepositStoragesRecursive",
		"FindRefundableContentStorages",
		"IsStructuralAttachmentStorage",
		"Webbing",
		"IsWebbingText",
		"GatherStorageContentEntitiesFromStorages",
		"HST_HQArsenalLoadoutEditorAction"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredLoadoutEditorEntry)) {
		throw "Custom loadout editor contract is missing: $requiredLoadoutEditorEntry"
	}
}
foreach ($forbiddenLoadoutStorageShortcut in @(
		"slot.GetAttachedEntity().FindComponent(BaseInventoryStorageComponent)",
		"containerEntity.FindComponent(BaseInventoryStorageComponent)"
	)) {
	if ($loadoutEditorText -match [regex]::Escape($forbiddenLoadoutStorageShortcut)) {
		throw "Loadout editor service must resolve all usable storage components instead of direct storage shortcuts: $forbiddenLoadoutStorageShortcut"
	}
}
if (!(Test-Path "UI/layouts/HST_LoadoutEditor.layout")) {
	throw "Missing fullscreen loadout editor layout resource"
}
if (!(Test-Path "UI/layouts/HST_LoadoutEditor.layout.meta")) {
	throw "Missing GUID-backed loadout editor layout meta resource"
}
if (!(Test-Path "UI/layouts/HST_LoadoutItemPreviewCell.layout")) {
	throw "Missing loadout item preview cell layout resource"
}
if (!(Test-Path "UI/layouts/HST_LoadoutItemPreviewCell.layout.meta")) {
	throw "Missing GUID-backed loadout item preview cell layout meta resource"
}
if (!(Test-Path "UI/layouts/HST_LoadoutEditor_TabButton.layout")) {
	throw "Missing loadout editor tab button layout resource"
}
if (!(Test-Path "UI/layouts/HST_LoadoutEditor_TabButton.layout.meta")) {
	throw "Missing GUID-backed loadout editor tab button layout meta resource"
}
$loadoutEditorLayoutText = Get-Content -Raw "UI/layouts/HST_LoadoutEditor.layout"
$loadoutEditorLayoutMetaText = Get-Content -Raw "UI/layouts/HST_LoadoutEditor.layout.meta"
$loadoutPreviewCellLayoutText = Get-Content -Raw "UI/layouts/HST_LoadoutItemPreviewCell.layout"
$loadoutPreviewCellLayoutMetaText = Get-Content -Raw "UI/layouts/HST_LoadoutItemPreviewCell.layout.meta"
$loadoutTabButtonLayoutText = Get-Content -Raw "UI/layouts/HST_LoadoutEditor_TabButton.layout"
$loadoutTabButtonLayoutMetaText = Get-Content -Raw "UI/layouts/HST_LoadoutEditor_TabButton.layout.meta"
if ($loadoutEditorComponentText -notmatch [regex]::Escape('EDITOR_LAYOUT = "{5AF2D86E07D44A51}UI/layouts/HST_LoadoutEditor.layout"')) {
	throw "Loadout editor must reference the GUID-backed layout resource"
}
if ($loadoutEditorComponentText -notmatch [regex]::Escape('ITEM_PREVIEW_CELL_LAYOUT = "{6B43C4A98B4F47F2}UI/layouts/HST_LoadoutItemPreviewCell.layout"')) {
	throw "Loadout editor must reference the GUID-backed item preview cell layout resource"
}
foreach ($requiredLoadoutPathBResource in @(
		'VERTICAL_SCROLL_LIST_LAYOUT = "{A7B8C9D001234560}UI/layouts/HST_VerticalScrollList.layout"',
		'WRAP_SCROLL_GRID_LAYOUT = "{A7B8C9D001234570}UI/layouts/HST_WrapScrollGrid.layout"',
		'LOADOUT_NODE_ROW_LAYOUT = "{A7B8C9D0012345D0}UI/layouts/HST/Rows/HST_LoadoutNodeRow.layout"',
		'LOADOUT_STORAGE_ROW_LAYOUT = "{A7B8C9D0012345E0}UI/layouts/HST/Rows/HST_LoadoutStorageRow.layout"',
		'LOADOUT_STORAGE_ITEM_ROW_LAYOUT = "{A7B8C9D0012345F0}UI/layouts/HST/Rows/HST_LoadoutStorageItemRow.layout"',
		'LOADOUT_CANDIDATE_TILE_LAYOUT = "{A7B8C9D001234600}UI/layouts/HST/Rows/HST_LoadoutCandidateTile.layout"',
		'LOADOUT_STORAGE_CATEGORY_TAB_LAYOUT = "{A7B8C9D001234610}UI/layouts/HST/Rows/HST_LoadoutStorageCategoryTab.layout"',
		'LOADOUT_TAB_BUTTON_LAYOUT = "{D66CFA01E5AA4400}UI/layouts/HST_LoadoutEditor_TabButton.layout"'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutPathBResource)) {
		throw "Loadout editor must reference Path B layout resource: $requiredLoadoutPathBResource"
	}
}
if ($loadoutEditorComponentText -match [regex]::Escape("{0000000000000000}UI/layouts/HST_LoadoutEditor.layout")) {
	throw "Loadout editor must not reference the zero-GUID layout resource"
}
if ($loadoutEditorComponentText -notmatch "m_RootWidget\s*=\s*workspace\.CreateWidgets\(EDITOR_LAYOUT,\s*workspace\)") {
	throw "Loadout editor must create the anchored layout resource directly as its root"
}
foreach ($requiredLoadoutRootLifecycleEntry in @(
		'HST_UIRootService.Get().CanOpen(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent")',
		"protected bool RenderEditor",
		"if (!RenderEditor())",
		"CloseEditorInternal(false)",
		"protected void CloseEditorInternal",
		"if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.LOADOUT_EDITOR, `"HST_LoadoutEditorComponent`", root, true, true, false))",
		"DeleteEditorRoot()",
		"IsLoadoutEditorTopmost",
		'HST_UIRootService.Get().IsTopmost(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent")',
		"CanHandleLoadoutEditorInput",
		'HST_UIRootService.Get().CanHandleScreenInput(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent")',
		"LOADOUT_PREVIEW_Z",
		"LOADOUT_UI_LAYER_Z",
		"LOADOUT_PREVIEW_INPUT_Z",
		"protected void ApplyLoadoutLayerOrder",
		"ApplyLoadoutLayerOrder(root)",
		"ApplyLoadoutLayerOrder(m_RootWidget)",
		'HST_UIDebug.LogWidgetGeometryCsv("loadout_editor_ready"',
		'HST_UIDebug.LogPopulation("loadout_editor_ready"'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutRootLifecycleEntry)) {
		throw "Loadout editor must register blocking UI only after its layout root exists: $requiredLoadoutRootLifecycleEntry"
	}
}
if ($loadoutEditorComponentText -match [regex]::Escape('RequestOpen(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent", null')) {
	throw "Loadout editor must not register as open before its layout root exists"
}
foreach ($forbiddenLoadoutNativeHintEntry in @(
		"SCR_HintManagerComponent",
		"ShowCustomHint",
		"Loadout editor opened",
		"Loadout editor closed"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutNativeHintEntry)) {
		throw "Loadout editor action feedback must use the shared passive toast controller: $forbiddenLoadoutNativeHintEntry"
	}
}
foreach ($forbiddenLoadoutRootMaskEntry in @(
		"CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID",
		"CreateWidgets(EDITOR_LAYOUT, m_RootWidget)",
		"CreateFullscreenShield(workspace)",
		"protected Widget CreateFullscreenShield(WorkspaceWidget workspace)",
		"FrameSlot.SetPos(layoutRoot, 0, 0)",
		"FrameSlot.SetSize(layoutRoot, m_iEditorWidth, m_iEditorHeight)",
		"int screenBleed = ScalePx(96)",
		"FrameSlot.SetPos(root, -screenBleed, -screenBleed)",
		"hardMask = CreateRectWidget(workspace, root"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutRootMaskEntry)) {
		throw "Loadout editor must not use procedural root/shield or resize stretched layout roots: $forbiddenLoadoutRootMaskEntry"
	}
}
if ($loadoutEditorLayoutMetaText -notmatch [regex]::Escape('Name "{5AF2D86E07D44A51}UI/layouts/HST_LoadoutEditor.layout"')) {
	throw "Loadout editor layout meta must carry the expected non-zero GUID"
}
if ($loadoutPreviewCellLayoutMetaText -notmatch [regex]::Escape('Name "{6B43C4A98B4F47F2}UI/layouts/HST_LoadoutItemPreviewCell.layout"')) {
	throw "Loadout item preview cell layout meta must carry the expected non-zero GUID"
}
if ($loadoutTabButtonLayoutMetaText -notmatch [regex]::Escape('Name "{D66CFA01E5AA4400}UI/layouts/HST_LoadoutEditor_TabButton.layout"')) {
	throw "Loadout tab button layout meta must carry the expected non-zero GUID"
}
foreach ($requiredLayoutEntry in @(
		"HST_LoadoutEditorRoot",
		'Slot FrameWidgetSlot "{7B2FD986A4D3410F}"',
		"RenderTargetWidgetClass",
		"HST_LoadoutBackdrop",
		"HST_LoadoutPreviewContainer",
		"HST_LoadoutPreview",
		"OverlayWidgetSlot",
		"Padding 0 0 0 0",
		"Color 0.12 0.16 0.18 1",
		"Anchor 0 0 1 1"
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLayoutEntry)) {
		throw "Loadout editor layout is missing stable render-target entry: $requiredLayoutEntry"
	}
}
foreach ($requiredLoadoutCoreVisibleWidget in @(
		"HST_LoadoutEditorRoot",
		"HST_LoadoutPreviewContainer",
		"HST_LoadoutUILayer",
		"PreviewDragSurface",
		"LeftButtons",
		"TopTabs",
		"LeftRail",
		"Footer"
	)) {
	$visibilityPattern = [regex]::Escape("Name `"$requiredLoadoutCoreVisibleWidget`"") + "[\s\S]{0,900}?" + [regex]::Escape('"Is Visible" 1')
	if ($loadoutEditorLayoutText -notmatch $visibilityPattern) {
		throw "Loadout editor core chrome must be explicitly visible by default: $requiredLoadoutCoreVisibleWidget"
	}
}
foreach ($requiredLoadoutModeHiddenWidget in @(
		"CandidateList",
		"StorageBrowser",
		"SavePanel"
	)) {
	$visibilityPattern = [regex]::Escape("Name `"$requiredLoadoutModeHiddenWidget`"") + "[\s\S]{0,900}?" + [regex]::Escape('"Is Visible" 0')
	if ($loadoutEditorLayoutText -notmatch $visibilityPattern) {
		throw "Loadout editor mode-specific panels must be hidden by default and shown by script: $requiredLoadoutModeHiddenWidget"
	}
}
$loadoutLayoutLines = Get-Content "UI/layouts/HST_LoadoutEditor.layout"
$loadoutFrameSlotAlignmentFindings = @()
foreach ($loadoutFrameSlotAlignmentMatch in [regex]::Matches($loadoutEditorLayoutText, 'Slot FrameWidgetSlot "[^"]+"\s*\{(?<slot>[^{}]*\b(?:HorizontalAlign|VerticalAlign)\b[^{}]*)\}')) {
	$loadoutFrameSlotAlignmentFindings += (($loadoutFrameSlotAlignmentMatch.Value -split "`r?`n") | Select-Object -First 1).Trim()
}
if ($loadoutFrameSlotAlignmentFindings.Count -gt 0) {
	throw "Loadout editor FrameWidgetSlot blocks must not use HorizontalAlign/VerticalAlign; use Frame offsets or an Alignable/Overlay slot instead: $($loadoutFrameSlotAlignmentFindings -join '; ')"
}
$loadoutSlotStack = @()
$loadoutSameAnchorNegativeFindings = @()
for ($i = 0; $i -lt $loadoutLayoutLines.Count; $i++) {
	$line = $loadoutLayoutLines[$i]
	if ($line -match 'Slot .*\{') {
		$loadoutSlotStack += [pscustomobject]@{
			Start        = $i + 1
			Anchor       = $null
			SizeX        = $false
			SizeY        = $false
			OffsetRight  = $null
			OffsetBottom = $null
		}
	}
	if ($loadoutSlotStack.Count -gt 0) {
		$slot = $loadoutSlotStack[$loadoutSlotStack.Count - 1]
		if ($line -match 'Anchor\s+([-0-9\.]+)\s+([-0-9\.]+)\s+([-0-9\.]+)\s+([-0-9\.]+)') {
			$slot.Anchor = @([double]$matches[1], [double]$matches[2], [double]$matches[3], [double]$matches[4])
		}
		if ($line -match '\bSizeX\b') {
			$slot.SizeX = $true
		}
		if ($line -match '\bSizeY\b') {
			$slot.SizeY = $true
		}
		if ($line -match 'OffsetRight\s+(-\d+)') {
			$slot.OffsetRight = [pscustomobject]@{ Line = $i + 1; Text = $line.Trim() }
		}
		if ($line -match 'OffsetBottom\s+(-\d+)') {
			$slot.OffsetBottom = [pscustomobject]@{ Line = $i + 1; Text = $line.Trim() }
		}
	}
	if ($line -match '^\s*\}') {
		if ($loadoutSlotStack.Count -gt 0) {
			$slot = $loadoutSlotStack[$loadoutSlotStack.Count - 1]
			if ($slot.Anchor) {
				if ($slot.Anchor[0] -eq $slot.Anchor[2] -and !$slot.SizeX -and $slot.OffsetRight) {
					$loadoutSameAnchorNegativeFindings += "line $($slot.OffsetRight.Line): $($slot.OffsetRight.Text)"
				}
				if ($slot.Anchor[1] -eq $slot.Anchor[3] -and !$slot.SizeY -and $slot.OffsetBottom) {
					$loadoutSameAnchorNegativeFindings += "line $($slot.OffsetBottom.Line): $($slot.OffsetBottom.Text)"
				}
			}
			if ($loadoutSlotStack.Count -eq 1) {
				$loadoutSlotStack = @()
			}
			else {
				$loadoutSlotStack = $loadoutSlotStack[0..($loadoutSlotStack.Count - 2)]
			}
		}
	}
}
if ($loadoutSameAnchorNegativeFindings.Count -gt 0) {
	Write-Host "Loadout editor same-anchor fixed bounds use signed far edges OK"
}
$loadoutEditorResolvedGeometryContracts = @(
	@{
		Widget   = "LeftButtons"
		Required = @("SizeX 78", "OffsetRight -102", "SizeY 128", "OffsetBottom -64")
	},
	@{
		Widget   = "LoadoutBackButton"
		Required = @("OffsetBottom -34")
	},
	@{
		Widget   = "LoadoutCloseButton"
		Required = @("OffsetBottom -108")
	},
	@{
		Widget   = "TopTabs"
		Required = @("OffsetRight -560", "OffsetBottom -126")
	},
	@{
		Widget   = "LeftRail"
		Required = @("OffsetRight -560", "OffsetBottom 92")
	},
	@{
		Widget   = "SlotRailList"
		Required = @("OffsetRight 20", "OffsetBottom 22")
	},
	@{
		Widget   = "CandidateList"
		Required = @("OffsetRight -560", "OffsetBottom 92")
	},
	@{
		Widget   = "StorageBrowser"
		Required = @("OffsetRight 116", "OffsetBottom 92")
	},
	@{
		Widget   = "SavePanel"
		Required = @("OffsetRight -560", "OffsetBottom 92")
	},
	@{
		Widget   = "SettingsContent"
		Required = @("OffsetRight 34", "OffsetBottom 28")
	},
	@{
		Widget   = "Footer"
		Required = @("OffsetRight -920", "OffsetBottom 24")
	},
	@{
		Widget   = "Toast"
		Required = @("SizeX 480", "OffsetRight -480", "SizeY 34", "OffsetBottom -34", "Alignment 0.5 0")
	}
)
foreach ($loadoutEditorResolvedGeometryContract in $loadoutEditorResolvedGeometryContracts) {
	foreach ($requiredResolvedGeometryEntry in $loadoutEditorResolvedGeometryContract.Required) {
		$resolvedGeometryPattern = [regex]::Escape("Name `"$($loadoutEditorResolvedGeometryContract.Widget)`"") + "[\s\S]{0,700}?" + [regex]::Escape($requiredResolvedGeometryEntry)
		if ($loadoutEditorLayoutText -notmatch $resolvedGeometryPattern) {
			throw "Loadout editor layout has an invalid resolved geometry contract for $($loadoutEditorResolvedGeometryContract.Widget): missing $requiredResolvedGeometryEntry"
		}
	}
}
foreach ($requiredLoadoutLeftButtonLayoutEntry in @(
		'Name "LeftButtons"',
		'Name "LoadoutBackButton"',
		'Name "BackBackground"',
		'Name "BackAccent"',
		'Name "BackLabel"',
		'Name "LoadoutCloseButton"',
		'Name "CloseBackground"',
		'Name "CloseAccent"',
		'Name "CloseLabel"',
		'Name "BackVisibleLabel"',
		'Name "CloseVisibleLabel"',
		"Anchor 0 0.5 0 0.5",
		"OffsetLeft 24",
		"Text `"Exit`""
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutLeftButtonLayoutEntry)) {
		throw "Loadout editor layout is missing layout-owned left button chrome: $requiredLoadoutLeftButtonLayoutEntry"
	}
}
foreach ($requiredLoadoutLeftButtonScriptEntry in @(
		"RenderLeftButtons(workspace, uiRoot)",
		'Widget leftButtons = ResolveLoadoutRegion(root, "LeftButtons")',
		'BindLoadoutClick(leftButtons, "LoadoutBackButton", BACK_WIDGET_ID)',
		'BindLoadoutClick(leftButtons, "LoadoutCloseButton", CLOSE_WIDGET_ID)',
		'SetLoadoutText(leftButtons, "BackLabel"',
		'SetLoadoutText(leftButtons, "CloseLabel"',
		"protected void BindLoadoutClick",
		"protected void SetLoadoutWidgetColor",
		"protected void SetLoadoutText"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutLeftButtonScriptEntry)) {
		throw "Loadout editor must bind layout-owned left button chrome: $requiredLoadoutLeftButtonScriptEntry"
	}
}
foreach ($forbiddenLoadoutLeftButtonScriptGeometry in @(
		'CreateButton(workspace, uiRoot, "Back"',
		'CreateButton(workspace, uiRoot, "ESC"',
		"int closeLeft = ScalePx(24)",
		"int centerY = m_iEditorHeight / 2"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutLeftButtonScriptGeometry)) {
		throw "Loadout editor left Back/ESC controls must be layout-owned: $forbiddenLoadoutLeftButtonScriptGeometry"
	}
}
foreach ($requiredLoadoutTabLayoutEntry in @(
		'Name "TopTabs"',
		'Name "TopTabsBackground"',
		'Name "TopTabItems"',
		'Name "HST_LoadoutEditor_TabButton"',
		'Name "SizeLayout"',
		'Name "Background"',
		'Name "Accent"',
		'Name "Icon"',
		'Name "Fallback"',
		"HorizontalLayoutWidgetClass",
		"WidthOverride 58",
		"HeightOverride 58"
	)) {
	if (($loadoutEditorLayoutText + "`n" + $loadoutTabButtonLayoutText) -notmatch [regex]::Escape($requiredLoadoutTabLayoutEntry)) {
		throw "Loadout editor mode tabs must be layout-owned: $requiredLoadoutTabLayoutEntry"
	}
}
if ($loadoutEditorLayoutText -notmatch 'Name "TopTabItems"[\s\S]*?OffsetLeft 24[\s\S]*?OffsetRight 24') {
	throw "Loadout editor mode tabs must keep the generated tab strip centered in its layout host"
}
foreach ($requiredLoadoutTabScriptEntry in @(
		'Widget items = target.FindAnyWidget("TopTabItems")',
		"AddLoadoutTabButton(workspace, items",
		"workspace.CreateWidgets(LOADOUT_TAB_BUTTON_LAYOUT, parent)",
		"SetLoadoutImageTexture(tab, `"Icon`"",
		"SetLoadoutText(tab, `"Fallback`"",
		"protected void AddLoadoutTabButton",
		"protected bool SetLoadoutImageTexture"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutTabScriptEntry)) {
		throw "Loadout editor mode tabs must populate layout-owned tab buttons: $requiredLoadoutTabScriptEntry"
	}
}
$loadoutModeTabsMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderModeTabs[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void AddLoadoutTabButton")
if (!$loadoutModeTabsMatch.Success) {
	throw "Loadout editor mode tab renderer is missing"
}
foreach ($forbiddenLoadoutModeTabGeometry in @(
		"CreateButton",
		"CreateRectWidget",
		"CreateTextWidget",
		"CreateIconWidget",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize"
	)) {
	if ($loadoutModeTabsMatch.Value -match [regex]::Escape($forbiddenLoadoutModeTabGeometry)) {
		throw "Loadout editor mode tabs must not use scripted geometry: $forbiddenLoadoutModeTabGeometry"
	}
}
foreach ($requiredLoadoutStorageBrowserLayoutEntry in @(
		'Name "StorageBrowser"',
		'Name "StorageBrowserBackground"',
		'Name "StorageBrowserAccent"',
		'Name "StorageBrowserTitle"',
		'Name "StorageBrowserSubtitle"',
		'Name "StorageCategoryTabs"',
		"HorizontalLayoutWidgetClass",
		'Name "StorageCandidateGrid"',
		'Name "StorageCandidateScroll"',
		'Name "StorageCandidateItems"',
		'Name "StorageCandidateEmpty"',
		"ScrollLayoutWidgetClass",
		"WrapLayoutWidgetClass",
		"OffsetLeft -820",
		"SizeY 78",
		"OffsetBottom -150",
		"OffsetTop 164",
		"OffsetBottom 46"
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutStorageBrowserLayoutEntry)) {
		throw "Loadout editor storage add-items panel must be layout-owned: $requiredLoadoutStorageBrowserLayoutEntry"
	}
}
foreach ($requiredLoadoutStorageBrowserScriptEntry in @(
		'SetLoadoutWidgetColor(panelRoot, "StorageBrowserBackground"',
		'SetLoadoutWidgetColor(panelRoot, "StorageBrowserAccent"',
		'SetLoadoutText(panelRoot, "StorageBrowserTitle"',
		'SetLoadoutText(panelRoot, "StorageBrowserSubtitle"',
		'Widget categoryRoot = panelRoot.FindAnyWidget("StorageCategoryTabs")',
		"RenderStorageCategoryTabs(workspace, categoryRoot)",
		'm_StorageCandidateScroll = ScrollLayoutWidget.Cast(root.FindAnyWidget("StorageCandidateScroll"))',
		'Widget items = root.FindAnyWidget("StorageCandidateItems")',
		'SetLoadoutText(root, "StorageCandidateEmpty"'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutStorageBrowserScriptEntry)) {
		throw "Loadout editor storage add-items panel must populate named layout widgets: $requiredLoadoutStorageBrowserScriptEntry"
	}
}
foreach ($requiredLoadoutStorageFeatureLayoutEntry in @(
		'Name "StorageFilterSortBar"',
		'Name "StorageFilterFitButton"',
		'Name "StorageFilterFitAccent"',
		'Name "StorageFilterFitLabel"',
		'Name "StorageFilterAmmoButton"',
		'Name "StorageFilterAmmoAccent"',
		'Name "StorageFilterAmmoLabel"',
		'Name "StorageFilterInfiniteButton"',
		'Name "StorageFilterInfiniteAccent"',
		'Name "StorageFilterInfiniteLabel"',
		'Name "StorageFilterShowAllButton"',
		'Name "StorageFilterShowAllAccent"',
		'Name "StorageFilterShowAllLabel"',
		'Name "StorageFilterNotInfiniteButton"',
		'Name "StorageFilterNotInfiniteAccent"',
		'Name "StorageFilterNotInfiniteLabel"',
		'Name "StorageSortAZButton"',
		'Name "StorageSortAZAccent"',
		'Name "StorageSortAZLabel"',
		'Name "StorageSortZAButton"',
		'Name "StorageSortZAAccent"',
		'Name "StorageSortZALabel"',
		'Name "StorageSortCountDescButton"',
		'Name "StorageSortCountDescAccent"',
		'Name "StorageSortCountDescLabel"',
		'Name "StorageSortCountAscButton"',
		'Name "StorageSortCountAscAccent"',
		'Name "StorageSortCountAscLabel"',
		'Name "StorageSearchPanel"',
		'Name "StorageSearchInput"',
		'Name "StorageSearchClearButton"',
		'Name "StorageSearchClearAccent"',
		'Name "StorageSearchClearLabel"',
		'Name "StorageSearchResultMeta"',
		'Name "StorageSearchScroll"',
		'Name "StorageSearchItems"',
		'Name "StorageSearchEmpty"'
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutStorageFeatureLayoutEntry)) {
		throw "Loadout editor storage filter/sort/search layout is missing: $requiredLoadoutStorageFeatureLayoutEntry"
	}
}
foreach ($forbiddenLoadoutStorageFeatureLayoutEntry in @(
		"CandidateRemoveBody",
		"StorageFilterFitBody",
		"StorageFilterAmmoBody",
		"StorageFilterInfiniteBody",
		"StorageFilterShowAllBody",
		"StorageFilterNotInfiniteBody",
		"StorageSortAZBody",
		"StorageSortZABody",
		"StorageSortCountDescBody",
		"StorageSortCountAscBody",
		"StorageSearchClearBody",
		"HorizontalAlign 1`r`n       VerticalAlign 1"
	)) {
	if ($loadoutEditorLayoutText -match [regex]::Escape($forbiddenLoadoutStorageFeatureLayoutEntry)) {
		throw "Loadout editor storage controls must avoid known runtime-bad layout shape: $forbiddenLoadoutStorageFeatureLayoutEntry"
	}
}
foreach ($requiredLoadoutStorageFeatureScriptEntry in @(
		"STORAGE_FILTER_WIDGET_ID_BASE = 48000",
		"STORAGE_SORT_WIDGET_ID_BASE = 48100",
		"STORAGE_SEARCH_RESULT_WIDGET_ID_BASE = 50000",
		"STORAGE_SEARCH_CLEAR_WIDGET_ID = 69000",
		"STORAGE_SEARCH_INPUT_WIDGET_ID = 69001",
		'ICON_SEARCH = "{4EE10E8893136EF3}Assets/512/search_icon.edds"',
		"RenderStorageFilterSortControls",
		"ToggleStorageFilterByIndex",
		"SetStorageSortMode",
		"IsStorageAmmoFilterApplicable",
		"QueueStorageBrowserRender",
		"FlushStorageBrowserRender",
		"PassesStorageCandidateFilters",
		"SortStorageVisibleCandidateIndexes",
		"CompareCandidateNamesForConfiguredDirection",
		"m_iStorageNameSortMode",
		"m_iStorageCountSortMode",
		"GetStorageBrowserCategoryFallback",
		'SetLoadoutText(tab, "Fallback", GetStorageBrowserCategoryFallback(category)',
		"RenderStorageSearchPanel",
		"BuildStorageSearchResults",
		"IsArsenalItemSearchMatch",
		"SortStorageSearchResultsAZ",
		"m_fStorageSearchScrollY",
		"RestoreScrollPixels(m_StorageSearchScroll, m_fStorageSearchScrollY)",
		"workspace.SetFocusedWidget(input, true)",
		"m_bSyncingStorageSearchInput",
		"RequestServerAction(`"add_storage_item`""
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutStorageFeatureScriptEntry)) {
		throw "Loadout editor storage filter/sort/search script is missing: $requiredLoadoutStorageFeatureScriptEntry"
	}
}
$loadoutStorageButtonBindingContracts = @(
	@("StorageFilterFitButton", "StorageFilterFitAccent", "StorageFilterFitLabel", "Fits", "STORAGE_FILTER_WIDGET_ID_BASE + 0"),
	@("StorageFilterAmmoButton", "StorageFilterAmmoAccent", "StorageFilterAmmoLabel", "Ammo", "STORAGE_FILTER_WIDGET_ID_BASE + 1"),
	@("StorageFilterInfiniteButton", "StorageFilterInfiniteAccent", "StorageFilterInfiniteLabel", "INF", "STORAGE_FILTER_WIDGET_ID_BASE + 2"),
	@("StorageFilterShowAllButton", "StorageFilterShowAllAccent", "StorageFilterShowAllLabel", "All", "STORAGE_FILTER_WIDGET_ID_BASE + 3"),
	@("StorageFilterNotInfiniteButton", "StorageFilterNotInfiniteAccent", "StorageFilterNotInfiniteLabel", "Not INF", "STORAGE_FILTER_WIDGET_ID_BASE + 4"),
	@("StorageSortAZButton", "StorageSortAZAccent", "StorageSortAZLabel", "A-Z", "STORAGE_SORT_WIDGET_ID_BASE + 0"),
	@("StorageSortZAButton", "StorageSortZAAccent", "StorageSortZALabel", "Z-A", "STORAGE_SORT_WIDGET_ID_BASE + 1"),
	@("StorageSortCountDescButton", "StorageSortCountDescAccent", "StorageSortCountDescLabel", "INF-1", "STORAGE_SORT_WIDGET_ID_BASE + 2"),
	@("StorageSortCountAscButton", "StorageSortCountAscAccent", "StorageSortCountAscLabel", "1-INF", "STORAGE_SORT_WIDGET_ID_BASE + 3")
)
foreach ($loadoutStorageButtonBindingContract in $loadoutStorageButtonBindingContracts) {
	$expectedStorageButtonBinding = "ConfigureStorageBrowserButton(panelRoot, `"$($loadoutStorageButtonBindingContract[0])`", `"$($loadoutStorageButtonBindingContract[0])`", `"$($loadoutStorageButtonBindingContract[1])`", `"$($loadoutStorageButtonBindingContract[2])`", `"$($loadoutStorageButtonBindingContract[3])`", $($loadoutStorageButtonBindingContract[4])"
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($expectedStorageButtonBinding)) {
		throw "Loadout editor storage filter/sort button must bind and populate its layout-owned label: $expectedStorageButtonBinding"
	}
}
if ($loadoutEditorLayoutText -notmatch 'Name "StorageFilterNotInfiniteButton"[\s\S]*?OffsetLeft 204' -or $loadoutEditorLayoutText -notmatch 'Name "StorageFilterShowAllButton"[\s\S]*?OffsetLeft 302') {
	throw "Loadout editor storage filters must render Not INF before All"
}
if ($loadoutEditorComponentText -notmatch [regex]::Escape('ConfigureStorageBrowserButton(searchRoot, "StorageSearchClearButton", "StorageSearchClearButton", "StorageSearchClearAccent", "StorageSearchClearLabel", "Clear", STORAGE_SEARCH_CLEAR_WIDGET_ID')) {
	throw "Loadout editor storage search clear button must bind and populate its layout-owned label"
}
if ($loadoutEditorComponentText -notmatch "if \(\s*m_bSyncingStorageSearchInput\s*\)\s*\r?\n\s*return true;" -or $loadoutEditorComponentText -notmatch "m_bSyncingStorageSearchInput = true;[\s\S]*?input\.SetText\(m_sStorageSearchQuery\);[\s\S]*?m_bSyncingStorageSearchInput = false;") {
	throw "Loadout editor search input must guard programmatic SetText sync from re-entering OnChange render"
}
if ($loadoutEditorComponentText -notmatch "m_StorageSearchScroll\.GetSliderPosPixels\(x, y\);[\s\S]*?m_fStorageSearchScrollY = y;" -or $loadoutEditorComponentText -match "m_StorageSearchScroll\.GetSliderPosPixels\(x, y\);[\s\S]{0,120}?m_fStorageCandidateScrollY = y;") {
	throw "Loadout editor storage search scroll must persist independently from the normal storage candidate grid scroll"
}
if ($loadoutEditorComponentText -notmatch 'if \(key == "search"\)\s*\r?\n\s*return ICON_SEARCH;') {
	throw "Loadout editor storage search tab must resolve the search icon resource"
}
if ($loadoutEditorComponentText -notmatch 'if \(m_sSelectedCategory != "search"\)\s*\r?\n\s*EnsureCandidatePayloadForStorageContainer\(\);') {
	throw "Loadout editor search tab must not request selected-storage candidate payloads when selected"
}
$loadoutSearchImplementationMatch = [regex]::Match($loadoutEditorComponentText, "protected void BuildStorageSearchResults[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool IsArsenalItemSearchMatch[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void SortStorageSearchResultsAZ")
if (!$loadoutSearchImplementationMatch.Success) {
	throw "Loadout editor storage search implementation block is missing"
}
foreach ($requiredSearchImplementationEntry in @(
		"m_aItemPrefabs.Count()",
		"GetArsenalItemDisplayName(itemIndex)",
		"m_aItemShortDisplays",
		"m_aItemCategories",
		"BuildStorageSearchCategoryText",
		"IsBlockedStorageSearchItem(itemIndex)",
		"NormalizeStorageSearchText(query)",
		"NormalizeStorageSearchText(haystack)",
		"ShouldStorageSearchIncludePrefabPath(query)"
	)) {
	if ($loadoutSearchImplementationMatch.Value -notmatch [regex]::Escape($requiredSearchImplementationEntry)) {
		throw "Loadout editor storage search must query full recovered arsenal item fields: $requiredSearchImplementationEntry"
	}
}
$loadoutSearchCategoryTextMatch = [regex]::Match($loadoutEditorComponentText, "protected string BuildStorageSearchCategoryText[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected string ResolveStorageBrowserCategoryForItem[\s\S]*?\r?\n\t}")
if (!$loadoutSearchCategoryTextMatch.Success -or $loadoutSearchCategoryTextMatch.Value -notmatch "BuildSlotCategoryLabel" -or $loadoutSearchCategoryTextMatch.Value -notmatch "GetStorageBrowserCategoryLabel" -or $loadoutSearchCategoryTextMatch.Value -notmatch "IsCategoryInStorageBrowserTab") {
	throw "Loadout editor storage search must match raw, slot, and visible storage browser category labels"
}
foreach ($forbiddenSearchImplementationEntry in @(
		"m_aCandidate",
		"PassesStorageCandidateFilters",
		"m_iStorageFilterMask",
		"m_iStorageSortMode",
		"CompareStorageCandidates"
	)) {
	if ($loadoutSearchImplementationMatch.Value -match [regex]::Escape($forbiddenSearchImplementationEntry)) {
		throw "Loadout editor storage search must stay independent from selected-storage candidates/filter/sort: $forbiddenSearchImplementationEntry"
	}
}
$loadoutSearchDisplayHelperMatch = [regex]::Match($loadoutEditorComponentText, "protected string GetArsenalItemDisplayName[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderStorageFilterSortControls")
if (!$loadoutSearchDisplayHelperMatch.Success -or $loadoutSearchDisplayHelperMatch.Value -notmatch "m_aItemDisplays" -or $loadoutSearchDisplayHelperMatch.Value -notmatch "m_aItemShortDisplays") {
	throw "Loadout editor search display helper must use full item display arrays"
}
$loadoutSearchResultTileMatch = [regex]::Match($loadoutEditorComponentText, "protected void AddStorageSearchResultTile[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void AddStorageSearchPreviewToRow")
if (!$loadoutSearchResultTileMatch.Success -or $loadoutSearchResultTileMatch.Value -notmatch [regex]::Escape("BuildCountLabel(itemIndex)")) {
	throw "Loadout editor search result tiles must render recovered arsenal count badges"
}
$loadoutSearchCountHelperMatch = [regex]::Match($loadoutEditorComponentText, "protected string BuildCountLabel[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected string BuildPreviewStatusLabel")
if (!$loadoutSearchCountHelperMatch.Success -or $loadoutSearchCountHelperMatch.Value -notmatch "m_aItemCounts" -or $loadoutSearchCountHelperMatch.Value -notmatch "m_aItemInfinite") {
	throw "Loadout editor search count badges must use full item count/infinite arrays"
}
$loadoutSearchClickMatch = [regex]::Match($loadoutEditorComponentText, "int storageSearchVisibleIndex = widgetId - STORAGE_SEARCH_RESULT_WIDGET_ID_BASE;[\s\S]*?\r?\n\t\tint itemVisibleIndex = widgetId - ITEM_WIDGET_ID_BASE;")
if (!$loadoutSearchClickMatch.Success -or $loadoutSearchClickMatch.Value -notmatch [regex]::Escape('RequestServerAction("add_storage_item", targetNodeId + ":" + m_aItemPrefabs[itemIndex]);') -or $loadoutSearchClickMatch.Value -notmatch [regex]::Escape('select storage before adding search result')) {
	throw "Loadout editor search result clicks must require selected storage and route through add_storage_item"
}
$loadoutEditorLayoutWidgetNames = @{}
foreach ($layoutWidgetNameMatch in [regex]::Matches($loadoutEditorLayoutText, 'Name "([^"]+)"')) {
	$loadoutEditorLayoutWidgetNames[$layoutWidgetNameMatch.Groups[1].Value] = $true
}
foreach ($storageBrowserButtonCall in [regex]::Matches($loadoutEditorComponentText, 'ConfigureStorageBrowserButton\([^;]+\);')) {
	$storageBrowserButtonNames = @([regex]::Matches($storageBrowserButtonCall.Value, '"([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
	for ($i = 0; $i -lt [Math]::Min(4, $storageBrowserButtonNames.Count); $i++) {
		$storageBrowserButtonWidgetName = $storageBrowserButtonNames[$i]
		if (!$loadoutEditorLayoutWidgetNames.ContainsKey($storageBrowserButtonWidgetName)) {
			throw "Loadout storage browser button call references missing layout widget: $storageBrowserButtonWidgetName"
		}
	}
}
$loadoutVisualSettingsText = Get-Content -Raw "Scripts/Game/HST/Config/HST_LoadoutEditorVisualSettings.c"
foreach ($requiredLoadoutVisualSettingsEntry in @(
		"SCHEMA_VERSION = 5",
		"STORAGE_FILTER_FIT_ONLY",
		"STORAGE_FILTER_AMMO_USABLE",
		"STORAGE_FILTER_INFINITE_ONLY",
		"STORAGE_FILTER_SHOW_ALL",
		"STORAGE_FILTER_NOT_INFINITE",
		"STORAGE_SORT_AZ",
		"STORAGE_SORT_ZA",
		"STORAGE_SORT_COUNT_DESC",
		"STORAGE_SORT_COUNT_ASC",
		"STORAGE_COUNT_SORT_NONE",
		"STORAGE_COUNT_SORT_DESC",
		"STORAGE_COUNT_SORT_ASC",
		"m_iStorageFilterMask",
		"m_iStorageSortMode",
		"m_iStorageNameSortMode",
		"m_iStorageCountSortMode",
		"storageFilterMask",
		"storageSortMode",
		"storageNameSortMode",
		"storageCountSortMode",
		"validStorageFilterMask",
		"ApplyLegacyStorageSortMode",
		"shouldSaveNormalized"
	)) {
	if ($loadoutVisualSettingsText -notmatch [regex]::Escape($requiredLoadoutVisualSettingsEntry)) {
		throw "Loadout editor visual settings must persist storage filter/sort setting: $requiredLoadoutVisualSettingsEntry"
	}
}
foreach ($requiredLoadoutStoragePayloadEntry in @(
		"BuildCandidatePayloadsForNode",
		"bool isStorageBrowserNode = node.m_sKind == `"storage`" || node.m_sKind == `"storage_item`"",
		"IsBlockedStorageBrowserContainerCandidate(session.m_iPlayerId, item.m_sPrefab, display, shortDisplay, category)",
		"ContainsStructuralStorageCandidateToken",
		"if (!isStorageBrowserNode && !compatible)",
		'payload = payload + string.Format("\nCANDIDATE|%1|%2|%3|%4|%5|%6|%7|%8|%9"',
		"payload = payload + string.Format(`"|%1`", ammoMatch)",
		"if (category == `"magazine`")",
		"ammoMatch = IsMagazineCompatibleWithEquippedWeapons"
	)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredLoadoutStoragePayloadEntry)) {
		throw "Loadout editor service must emit storage candidates with fit/ammo metadata: $requiredLoadoutStoragePayloadEntry"
	}
}
foreach ($requiredCandidatePreviewHintEntry in @(
		"if (category == `"medical`")",
		"return `"medical`";",
		"if (category == `"utility`")",
		"return `"utility`";",
		"return `"equipment`";"
	)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredCandidatePreviewHintEntry)) {
		throw "Loadout editor service must emit preview-capable candidate icon hints: $requiredCandidatePreviewHintEntry"
	}
}
$candidateIconHintMatch = [regex]::Match($loadoutEditorText, "protected string BuildCandidateIconHint[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected int ResolveDisplayCapacityForCategory")
if (!$candidateIconHintMatch.Success) {
	throw "Loadout editor service candidate icon hint function is missing"
}
if ($candidateIconHintMatch.Value -match "return `"gear`";") {
	throw "Loadout editor candidate icon hints must not fall back to gear, because the preview renderer treats that as icon-only"
}
$loadoutStorageCandidatePanelMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderStorageCandidatePanel[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderStorageCandidateGrid[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void AddLoadoutNodeRow")
if (!$loadoutStorageCandidatePanelMatch.Success) {
	throw "Loadout editor storage add-items renderer is missing"
}
$loadoutCandidateTileMatch = [regex]::Match($loadoutEditorComponentText, "protected void AddLoadoutCandidateTile[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void AddCandidateTileOverlayText")
if (!$loadoutCandidateTileMatch.Success -or $loadoutCandidateTileMatch.Value -notmatch [regex]::Escape("BuildCandidateCountLabel(candidateIndex, true)") -or $loadoutCandidateTileMatch.Value -notmatch [regex]::Escape("color = 0xAA5B2C2C")) {
	throw "Loadout storage non-fitting candidates must keep their count badge and use a light red row highlight"
}
if ($loadoutCandidateTileMatch.Value -match [regex]::Escape('"NO FIT"')) {
	throw "Loadout storage non-fitting candidates must not overwrite the arsenal count label with NO FIT"
}
foreach ($forbiddenLoadoutStorageBrowserGeometry in @(
		"CreateRectWidget",
		"CreateTextWidget",
		"CreateScrollContainer",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"WRAP_SCROLL_GRID_LAYOUT"
	)) {
	if ($loadoutStorageCandidatePanelMatch.Value -match [regex]::Escape($forbiddenLoadoutStorageBrowserGeometry)) {
		throw "Loadout editor storage add-items panel must not create panel/grid geometry in script: $forbiddenLoadoutStorageBrowserGeometry"
	}
}
foreach ($requiredLoadoutLeftRailLayoutEntry in @(
		'Name "LeftRail"',
		'Name "LeftRailBackground"',
		'Name "LeftRailAccent"',
		'Name "SlotRailList"',
		'Name "SlotRailScroll"',
		'Name "SlotRailItems"',
		'Name "SlotRailEmpty"',
		'Name "StorageRailTitle"',
		'Name "StorageContainerList"',
		'Name "StorageContainerScroll"',
		'Name "StorageContainerItems"',
		'Name "StorageContainerEmpty"',
		'Name "StorageContentRule"',
		'Name "StorageContentTitle"',
		'Name "StorageContentList"',
		'Name "StorageContentScroll"',
		'Name "StorageContentItems"',
		'Name "StorageContentEmpty"'
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutLeftRailLayoutEntry)) {
		throw "Loadout editor left rail must be layout-owned: $requiredLoadoutLeftRailLayoutEntry"
	}
}
foreach ($requiredLoadoutLeftRailScriptEntry in @(
		"ResetLoadoutModeRegions(uiRoot)",
		"protected void ShowSlotRailWidgets",
		"protected void ShowStorageRailWidgets",
		'SetLoadoutWidgetColor(railRoot, "LeftRailBackground"',
		'SetLoadoutWidgetColor(railRoot, "LeftRailAccent"',
		'm_SlotScroll = ScrollLayoutWidget.Cast(railRoot.FindAnyWidget("SlotRailScroll"))',
		'Widget items = railRoot.FindAnyWidget("SlotRailItems")',
		'SetLoadoutText(railRoot, "SlotRailEmpty"',
		'SetLoadoutText(railRoot, "StorageRailTitle"',
		'SetLoadoutWidgetColor(railRoot, "StorageContentRule"',
		'SetLoadoutText(railRoot, "StorageContentTitle"',
		'm_StorageContainerScroll = ScrollLayoutWidget.Cast(railRoot.FindAnyWidget("StorageContainerScroll"))',
		'Widget containerItems = railRoot.FindAnyWidget("StorageContainerItems")',
		'SetLoadoutText(railRoot, "StorageContainerEmpty"',
		'm_StorageContentScroll = ScrollLayoutWidget.Cast(railRoot.FindAnyWidget("StorageContentScroll"))',
		'Widget contentItems = railRoot.FindAnyWidget("StorageContentItems")',
		'SetLoadoutText(railRoot, "StorageContentEmpty"'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutLeftRailScriptEntry)) {
		throw "Loadout editor left rail must populate named layout widgets: $requiredLoadoutLeftRailScriptEntry"
	}
}
$loadoutLeftRailMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderSlotRail[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderStorageRail[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderStorageCandidatePanel")
if (!$loadoutLeftRailMatch.Success) {
	throw "Loadout editor left rail renderer is missing"
}
foreach ($forbiddenLoadoutLeftRailGeometry in @(
		"CreateRectWidget",
		"CreateTextWidget",
		"CreateScrollContainer",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"VERTICAL_SCROLL_LIST_LAYOUT",
		"railLeft",
		"railTop",
		"containerTop",
		"contentTop",
		"contentsHeaderTop"
	)) {
	if ($loadoutLeftRailMatch.Value -match [regex]::Escape($forbiddenLoadoutLeftRailGeometry)) {
		throw "Loadout editor left rail must not create panel/scroll geometry in script: $forbiddenLoadoutLeftRailGeometry"
	}
}
foreach ($requiredLoadoutCandidateTemplateLayoutEntry in @(
		'Name "CandidateList"',
		'Name "CandidatePanelBackground"',
		'Name "CandidateHeaderRule"',
		'Name "CandidateHeaderPreviewBack"',
		'Name "CandidateHeaderPreviewLine"',
		'Name "CandidateHeaderPreviewAnchor"',
		'Name "CandidateHeaderSlot"',
		'Name "CandidateHeaderName"',
		'Name "CandidateHeaderMarker"',
		'Name "CandidateEditButton"',
		'Name "CandidateEditBackground"',
		'Name "CandidateEditAccent"',
		'Name "CandidateEditIcon"',
		'Name "CandidateEditLabel"',
		'Name "CandidateRemoveButton"',
		'Name "CandidateRemoveBackground"',
		'Name "CandidateRemoveAccent"',
		'Name "CandidateRemoveIcon"',
		'Name "CandidateRemoveLabel"',
		'Name "CandidateListFrame"',
		'Name "CandidateScroll"',
		'Name "CandidateItems"',
		'Name "CandidateEmpty"',
		'Name "SavePanel"',
		'Name "SavePanelBackground"',
		'Name "SavePanelAccent"',
		'Name "SavePanelTitle"',
		'Name "TemplateSaveButton"',
		'Name "TemplateSaveBackground"',
		'Name "TemplateSaveAccent"',
		'Name "TemplateSaveLabel"',
		'Name "TemplateClearButton"',
		'Name "TemplateClearBackground"',
		'Name "TemplateClearAccent"',
		'Name "TemplateClearLabel"',
		'Name "TemplateList"',
		'Name "TemplateScroll"',
		'Name "TemplateItems"',
		'Name "TemplateEmpty"'
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutCandidateTemplateLayoutEntry)) {
		throw "Loadout editor candidate/template panel must be layout-owned: $requiredLoadoutCandidateTemplateLayoutEntry"
	}
}
if ($loadoutEditorComponentText -notmatch [regex]::Escape('ConfigureLoadoutPanelButton(panelRoot, "CandidateRemoveButton", "CandidateRemoveBackground", "CandidateRemoveAccent", "CandidateRemoveLabel", "Remove", REMOVE_SELECTED_NODE_WIDGET_ID)')) {
	throw "Loadout editor candidate remove button must bind and populate its layout-owned Remove label"
}
if ($loadoutEditorComponentText -notmatch [regex]::Escape('SetLoadoutText(panelRoot, "CandidateRemoveIcon", "X"')) {
	throw "Loadout editor candidate remove button must populate its layout-owned remove icon text"
}
if ($loadoutEditorLayoutText -match 'Name "CandidateEditIcon"[\s\S]*?Text ">"') {
	throw "Loadout editor candidate edit button must not carry a visible or fallback > icon"
}
if ($loadoutEditorLayoutText -notmatch 'Name "CandidateEditLabel"[\s\S]*?OffsetLeft -128[\s\S]*?OffsetRight 20[\s\S]*?SizeX 108[\s\S]*?"Horizontal Alignment" Center') {
	throw "Loadout editor candidate edit label must span and center inside the full Edit button"
}
$loadoutEditorCombinedText = $loadoutEditorComponentText + "`n" + $loadoutEditorText + "`n" + $loadoutEditorLayoutText
foreach ($requiredArmoredVestLabelEntry in @(
		'InsertCategory("vest", "Armored Vest"',
		'AddLoadoutNodeForCategory(session, "vest", "ArmoredVest", "Armored Vest", "torso")'
	)) {
	if ($loadoutEditorCombinedText -notmatch [regex]::Escape($requiredArmoredVestLabelEntry)) {
		throw "Loadout editor armor vest-category fallback UI must resolve to Armored Vest: $requiredArmoredVestLabelEntry"
	}
}
foreach ($requiredVestLabelPattern in @(
		'if \(category == "vest"\)[\s\S]{0,220}?IsLoadoutSlotWebbing[\s\S]{0,120}?return "Chest Rig";[\s\S]{0,120}?return "Armored Vest";',
		'if \(category == "vest"\)[\s\S]{0,220}?m_aNodeLabels\[nodeIndex\][\s\S]{0,120}?return m_aNodeLabels\[nodeIndex\];[\s\S]{0,120}?return "Armored Vest";',
		'if \(category == "vest"\)[\s\S]{0,80}?return "Armored Vest";'
	)) {
	if ($loadoutEditorCombinedText -notmatch $requiredVestLabelPattern) {
		throw "Loadout editor vest-category UI must distinguish Armored Vest from webbing/Chest Rig: $requiredVestLabelPattern"
	}
}
$loadoutClothingEditMatch = [regex]::Match($loadoutEditorComponentText, "protected bool RequestEditSelectedWornStorage\(\)[\s\S]*?\r?\n\t}")
if (!$loadoutClothingEditMatch.Success) {
	throw "Loadout editor clothing Edit action is missing"
}
foreach ($requiredClothingEditEntry in @(
		'm_sEditorMode = EDITOR_MODE_CLOTHING_EDIT',
		'ResolveSelectedWornAttachmentParentNodeId()',
		'm_sSelectedStorageContainerNodeId = ""',
		'm_sSelectedStoredItemNodeId = ""',
		'm_bCandidateMode = false'
	)) {
	if ($loadoutClothingEditMatch.Value -notmatch [regex]::Escape($requiredClothingEditEntry)) {
		throw "Loadout editor clothing Edit must route to clothing child-slot editing, not storage browsing: $requiredClothingEditEntry"
	}
}
if ($loadoutClothingEditMatch.Value -match [regex]::Escape('m_sEditorMode = "attachments"')) {
	throw "Loadout editor clothing Edit must not switch to the weapon attachments mode"
}
if ($loadoutClothingEditMatch.Value -match [regex]::Escape('m_sEditorMode = "storage"')) {
	throw "Loadout editor clothing Edit must not switch to storage mode"
}
if ($loadoutClothingEditMatch.Value -match [regex]::Escape('EnsureCandidatePayloadForSelectedNode()')) {
	throw "Loadout editor clothing Edit must not immediately open replacement candidates"
}
foreach ($requiredFixedCandidateHeaderLayoutPattern in @(
		'Name "CandidateHeaderPreviewBack"[\s\S]*?OffsetRight -94[\s\S]*?OffsetBottom -82',
		'Name "CandidateHeaderPreviewLine"[\s\S]*?OffsetRight -94[\s\S]*?OffsetBottom -16',
		'Name "CandidateHeaderPreviewAnchor"[\s\S]*?OffsetRight -92[\s\S]*?OffsetBottom -80'
	)) {
	if ($loadoutEditorLayoutText -notmatch $requiredFixedCandidateHeaderLayoutPattern) {
		throw "Loadout editor candidate header preview boxes must use negative far offsets for same-anchor fixed bounds: $requiredFixedCandidateHeaderLayoutPattern"
	}
}
foreach ($requiredLoadoutCandidateTemplateScriptEntry in @(
		"protected void ShowCandidatePanelWidgets",
		"protected void ShowTemplatePanelWidgets",
		"protected void HideTemplatePanelWidgets",
		"protected void ConfigureLoadoutPanelShell",
		"protected void ConfigureLoadoutPanelButton",
		"protected void ConfigureTemplateSlotButton",
		'SetLoadoutWidgetColor(panelRoot, "CandidatePanelBackground"',
		'SetLoadoutWidgetColor(panelRoot, "CandidateHeaderRule"',
		'ConfigureLoadoutPanelButton(panelRoot, "CandidateRemoveButton"',
		'm_CandidateScroll = ScrollLayoutWidget.Cast(panelRoot.FindAnyWidget("CandidateScroll"))',
		'Widget items = panelRoot.FindAnyWidget("CandidateItems")',
		'SetLoadoutText(panelRoot, "CandidateEmpty"',
		'ConfigureLoadoutPanelShell(panelRoot, "Save Loadout")',
		'ConfigureLoadoutPanelButton(panelRoot, "TemplateSaveButton"',
		'ConfigureLoadoutPanelButton(panelRoot, "TemplateClearButton"',
		'm_TemplateScroll = ScrollLayoutWidget.Cast(panelRoot.FindAnyWidget("TemplateScroll"))',
		'Widget items = panelRoot.FindAnyWidget("TemplateItems")',
		'SetLoadoutText(panelRoot, "TemplateEmpty"',
		'workspace.CreateWidgets(LOADOUT_TEMPLATE_SLOT_ROW_LAYOUT, items)',
		'ConfigureTemplateSlotButton(row, "SaveButton", "SaveLabel", "Save", TEMPLATE_SAVE_WIDGET_ID_BASE + templateIndex, true)',
		'ConfigureTemplateSlotButton(row, "LoadButton", "LoadLabel", "Load", TEMPLATE_LOAD_WIDGET_ID_BASE + templateIndex, !IsTemplateSlotEmpty(templateIndex))',
		'SetLoadoutWidgetColor(root, "CandidateHeaderPreviewBack"',
		'Widget anchor = root.FindAnyWidget("CandidateHeaderPreviewAnchor")',
		'SetLoadoutText(root, "CandidateHeaderSlot"',
		'SetLoadoutText(root, "CandidateHeaderName"',
		'SetLoadoutText(root, "CandidateHeaderMarker"'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutCandidateTemplateScriptEntry)) {
		throw "Loadout editor candidate/template panel must populate named layout widgets: $requiredLoadoutCandidateTemplateScriptEntry"
	}
}
$loadoutCandidatePanelMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderCandidatePanel[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderTemplatePanel")
if (!$loadoutCandidatePanelMatch.Success) {
	throw "Loadout editor candidate panel renderer is missing"
}
$loadoutTemplatePanelMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderTemplatePanel[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderSettingsPanel")
if (!$loadoutTemplatePanelMatch.Success) {
	throw "Loadout editor template panel renderer is missing"
}
$loadoutTemplateSlotActionMatch = [regex]::Match($loadoutEditorComponentText, "int templateSaveIndex = widgetId - TEMPLATE_SAVE_WIDGET_ID_BASE;[\s\S]*?int templateLoadIndex = widgetId - TEMPLATE_LOAD_WIDGET_ID_BASE;[\s\S]*?int templateIndex = widgetId - TEMPLATE_WIDGET_ID_BASE;")
if (!$loadoutTemplateSlotActionMatch.Success -or $loadoutTemplateSlotActionMatch.Value -notmatch [regex]::Escape('RequestServerAction("loadout_save", m_sSelectedTemplateId);') -or $loadoutTemplateSlotActionMatch.Value -notmatch [regex]::Escape('RequestServerAction("loadout_apply", m_sSelectedTemplateId);') -or $loadoutTemplateSlotActionMatch.Value -notmatch [regex]::Escape('if (IsTemplateSlotEmpty(templateLoadIndex))')) {
	throw "Loadout editor save panel must route per-slot Save/Load buttons through fixed saved loadout ids and keep empty Load disabled"
}
foreach ($forbiddenLoadoutCandidateTemplateGeometry in @(
		"CreateRectWidget",
		"CreateTextWidget",
		"CreateScrollContainer",
		"CreateButton",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"VERTICAL_SCROLL_LIST_LAYOUT",
		"panelLeft",
		"panelTop",
		"listLeft",
		"rowTop",
		"firstRowTop"
	)) {
	if ($loadoutCandidatePanelMatch.Value -match [regex]::Escape($forbiddenLoadoutCandidateTemplateGeometry)) {
		throw "Loadout editor candidate panel must not create panel/scroll geometry in script: $forbiddenLoadoutCandidateTemplateGeometry"
	}
	if ($loadoutTemplatePanelMatch.Value -match [regex]::Escape($forbiddenLoadoutCandidateTemplateGeometry)) {
		throw "Loadout editor template panel must not create panel/scroll geometry in script: $forbiddenLoadoutCandidateTemplateGeometry"
	}
}
$loadoutSelectedNodeHeaderMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderSelectedNodeHeader[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected int FindSelectedNodeIndex")
if (!$loadoutSelectedNodeHeaderMatch.Success) {
	throw "Loadout editor selected-node header renderer is missing"
}
foreach ($forbiddenLoadoutSelectedHeaderGeometry in @(
		"CreateRectWidget",
		"CreateTextWidget",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize"
	)) {
	if ($loadoutSelectedNodeHeaderMatch.Value -match [regex]::Escape($forbiddenLoadoutSelectedHeaderGeometry)) {
		throw "Loadout editor selected-node header must use named layout widgets: $forbiddenLoadoutSelectedHeaderGeometry"
	}
}
foreach ($requiredLoadoutSettingsLayoutEntry in @(
		'Name "SettingsContent"',
		'Name "SettingsLightRow"',
		'Name "SettingsLightLabel"',
		'Name "SettingsLightValue"',
		'Name "SettingsLightMinusButton"',
		'Name "SettingsLightMinusLabel"',
		'Name "SettingsLightPlusButton"',
		'Name "SettingsLightPlusLabel"',
		'Name "SettingsPanelPresetRow"',
		'Name "SettingsPanelPresetLabel"',
		'Name "SettingsPanelPresetValue"',
		'Name "SettingsPanelPresetButton"',
		'Name "SettingsPanelPresetButtonLabel"',
		'Name "SettingsAccentPresetRow"',
		'Name "SettingsAccentPresetLabel"',
		'Name "SettingsAccentPresetValue"',
		'Name "SettingsAccentPresetButton"',
		'Name "SettingsAccentPresetButtonLabel"',
		'Name "SettingsRowPresetRow"',
		'Name "SettingsRowPresetLabel"',
		'Name "SettingsRowPresetValue"',
		'Name "SettingsRowPresetButton"',
		'Name "SettingsRowPresetButtonLabel"',
		'Name "SettingsWorldPresetRow"',
		'Name "SettingsWorldPresetLabel"',
		'Name "SettingsWorldPresetValue"',
		'Name "SettingsWorldPresetButton"',
		'Name "SettingsWorldPresetButtonLabel"',
		'Name "SettingsActionsRow"',
		'Name "SettingsDefaultsButton"',
		'Name "SettingsDefaultsLabel"',
		'Name "SettingsResetPreviewButton"',
		'Name "SettingsResetPreviewLabel"'
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutSettingsLayoutEntry)) {
		throw "Loadout editor settings panel must be layout-owned: $requiredLoadoutSettingsLayoutEntry"
	}
}
foreach ($requiredFilledButtonBodyName in @(
		"TemplateSaveBody",
		"TemplateClearBody",
		"SettingsLightMinusBody",
		"SettingsLightPlusBody",
		"SettingsPanelPresetButtonBody",
		"SettingsAccentPresetButtonBody",
		"SettingsRowPresetButtonBody",
		"SettingsWorldPresetButtonBody",
		"SettingsDefaultsBody",
		"SettingsResetPreviewBody"
	)) {
	$filledButtonBodyPattern = 'Name "' + [regex]::Escape($requiredFilledButtonBodyName) + '"[\s\S]{0,220}?Slot ButtonWidgetSlot[\s\S]{0,120}?HorizontalAlign 3[\s\S]{0,120}?VerticalAlign 3'
	if ($loadoutEditorLayoutText -notmatch $filledButtonBodyPattern) {
		throw "Loadout editor button body must fill its ButtonWidgetSlot so labels are not zero-sized: $requiredFilledButtonBodyName"
	}
}
foreach ($requiredLoadoutSettingsScriptEntry in @(
		"protected void ShowSettingsPanelWidgets",
		'SetLoadoutWidgetVisible(panelRoot, "SettingsContent", false)',
		'SetLoadoutWidgetVisible(panelRoot, "SettingsContent", true)',
		'ConfigureLoadoutPanelShell(panelRoot, "Settings")',
		'SetLoadoutText(panelRoot, "SettingsLightLabel"',
		'SetLoadoutText(panelRoot, "SettingsLightValue"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsLightMinusButton"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsLightPlusButton"',
		'SetLoadoutText(panelRoot, "SettingsPanelPresetLabel"',
		'SetLoadoutText(panelRoot, "SettingsPanelPresetValue"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsPanelPresetButton"',
		'SetLoadoutText(panelRoot, "SettingsAccentPresetLabel"',
		'SetLoadoutText(panelRoot, "SettingsAccentPresetValue"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsAccentPresetButton"',
		'SetLoadoutText(panelRoot, "SettingsRowPresetLabel"',
		'SetLoadoutText(panelRoot, "SettingsRowPresetValue"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsRowPresetButton"',
		'SetLoadoutText(panelRoot, "SettingsWorldPresetLabel"',
		'SetLoadoutText(panelRoot, "SettingsWorldPresetValue"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsWorldPresetButton"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsDefaultsButton"',
		'ConfigureLoadoutPanelButton(panelRoot, "SettingsResetPreviewButton"'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutSettingsScriptEntry)) {
		throw "Loadout editor settings panel must populate named layout widgets: $requiredLoadoutSettingsScriptEntry"
	}
}
$loadoutSettingsPanelMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderSettingsPanel[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderFooter")
if (!$loadoutSettingsPanelMatch.Success) {
	throw "Loadout editor settings panel renderer is missing"
}
foreach ($forbiddenLoadoutSettingsGeometry in @(
		"CreateRectWidget",
		"CreateTextWidget",
		"CreateWrappedTextWidget",
		"CreateScrollContainer",
		"CreateButton",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"RenderSettingsLightRow",
		"RenderSettingsPresetRow",
		"panelLeft",
		"panelTop",
		"optionLeft",
		"optionTop",
		"optionWidth"
	)) {
	if ($loadoutSettingsPanelMatch.Value -match [regex]::Escape($forbiddenLoadoutSettingsGeometry)) {
		throw "Loadout editor settings panel must not create row/control geometry in script: $forbiddenLoadoutSettingsGeometry"
	}
}
foreach ($requiredLoadoutFooterLayoutEntry in @(
		'Name "Footer"',
		'Name "FooterHintItems"',
		'Name "FooterPrevTabHint"',
		'Name "FooterPrevTabKeyBack"',
		'Name "FooterPrevTabKey"',
		'Name "FooterPrevTabLabel"',
		'Name "FooterNextTabHint"',
		'Name "FooterNextTabKeyBack"',
		'Name "FooterNextTabKey"',
		'Name "FooterNextTabLabel"',
		'Name "FooterBackHint"',
		'Name "FooterBackKeyBack"',
		'Name "FooterBackKey"',
		'Name "FooterBackLabel"',
		'Name "FooterPrimaryHint"',
		'Name "FooterPrimaryKeyBack"',
		'Name "FooterPrimaryKey"',
		'Name "FooterPrimaryLabel"',
		'Name "FooterSecondaryHint"',
		'Name "FooterSecondaryKeyBack"',
		'Name "FooterSecondaryKey"',
		'Name "FooterSecondaryLabel"'
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutFooterLayoutEntry)) {
		throw "Loadout editor footer hints must be layout-owned: $requiredLoadoutFooterLayoutEntry"
	}
}
foreach ($requiredLoadoutFooterScriptEntry in @(
		"LOADOUT_INPUT_BUTTON_LAYOUT",
		"protected bool RenderNativeFooterHints",
		"protected bool AddNativeFooterHint",
		"SCR_InputButtonComponent.FindComponent",
		"inputButton.SetAction(actionName)",
		"MakeWidgetTreePassive(hint)",
		'ClearLoadoutContainerChildren(root, "FooterHintItems")',
		"protected void HideLoadoutFooterHints",
		"protected void SetLoadoutFooterHint",
		'SetLoadoutFooterHint(footerRoot, "FooterPrevTabHint"',
		'SetLoadoutFooterHint(footerRoot, "FooterNextTabHint"',
		'SetLoadoutFooterHint(footerRoot, "FooterBackHint"',
		'SetLoadoutFooterHint(footerRoot, "FooterPrimaryHint"',
		'SetLoadoutFooterHint(footerRoot, "FooterSecondaryHint"',
		'SetLoadoutWidgetVisible(footerRoot, "FooterPrevTabHint", false)',
		'SetLoadoutWidgetColor(footerRoot, keyBackName',
		'SetLoadoutText(footerRoot, keyName',
		'SetLoadoutText(footerRoot, labelName'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutFooterScriptEntry)) {
		throw "Loadout editor footer hints must populate named layout widgets: $requiredLoadoutFooterScriptEntry"
	}
}
$loadoutFooterMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderContextHints[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void HideLoadoutFooterHints[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void SetLoadoutFooterHint[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void ConfigurePreviewDragSurface")
if (!$loadoutFooterMatch.Success) {
	throw "Loadout editor footer hint renderer is missing"
}
foreach ($forbiddenLoadoutFooterGeometry in @(
		"CreateTextWidget",
		"CreateRichTextWidget",
		"CreateRectWidget",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"RenderActionHint",
		"BuildActionGlyphMarkup",
		"cursorLeft",
		"hintLeft",
		"hintTop"
	)) {
	if ($loadoutFooterMatch.Value -match [regex]::Escape($forbiddenLoadoutFooterGeometry)) {
		throw "Loadout editor footer hints must use named layout widgets: $forbiddenLoadoutFooterGeometry"
	}
}
foreach ($forbiddenLoadoutFooterLegacyEntry in @(
		"protected int RenderActionHint",
		"protected string BuildActionGlyphMarkup",
		"protected RichTextWidget CreateRichTextWidget"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutFooterLegacyEntry)) {
		throw "Loadout editor must not keep legacy footer hint geometry helper: $forbiddenLoadoutFooterLegacyEntry"
	}
}
foreach ($requiredLoadoutPreviewDragLayoutEntry in @(
		'Name "PreviewDragSurface"',
		'OffsetLeft 560',
		'Color 0 0 0 0.01',
		'"Ignore Cursor" 0'
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutPreviewDragLayoutEntry)) {
		throw "Loadout editor preview drag surface must be layout-owned: $requiredLoadoutPreviewDragLayoutEntry"
	}
}
foreach ($requiredLoadoutPreviewDragScriptEntry in @(
		"ConfigurePreviewDragSurface(uiRoot)",
		"protected void ConfigurePreviewDragSurface",
		'Widget surface = root.FindAnyWidget("PreviewDragSurface")',
		"surface.SetUserID(PREVIEW_DRAG_WIDGET_ID)",
		"BindEditorHandler(surface)",
		"surface.SetZOrder(LOADOUT_PREVIEW_INPUT_Z)"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutPreviewDragScriptEntry)) {
		throw "Loadout editor preview drag surface must bind the named layout widget: $requiredLoadoutPreviewDragScriptEntry"
	}
}
$loadoutPreviewDragMatch = [regex]::Match($loadoutEditorComponentText, "protected void ConfigurePreviewDragSurface[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void ParseEditorPayload")
if (!$loadoutPreviewDragMatch.Success) {
	throw "Loadout editor preview drag surface binder is missing"
}
foreach ($forbiddenLoadoutPreviewDragGeometry in @(
		"CreateRectWidget",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"m_Layout.m_iMainLeft",
		"m_iEditorWidth - left",
		"int width",
		"int height"
	)) {
	if ($loadoutPreviewDragMatch.Value -match [regex]::Escape($forbiddenLoadoutPreviewDragGeometry)) {
		throw "Loadout editor preview drag surface must not calculate geometry in script: $forbiddenLoadoutPreviewDragGeometry"
	}
}
foreach ($forbiddenLoadoutPreviewDragLegacyEntry in @(
		"protected void CreatePreviewDragSurface",
		"CreateRectWidget(workspace, root, left, top, width, height, 0x00111111"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutPreviewDragLegacyEntry)) {
		throw "Loadout editor must not keep legacy preview drag geometry helper: $forbiddenLoadoutPreviewDragLegacyEntry"
	}
}
if ($loadoutEditorComponentText -match [regex]::Escape("surface.SetZOrder(-10)")) {
	throw "Loadout editor preview drag surface must use the named UI-layer z-order constant"
}
foreach ($requiredLoadoutPreviewStatusLayoutEntry in @(
		'Name "Toast"',
		'Name "ToastBackground"',
		'Name "ToastAccent"',
		'Name "ToastText"',
		'Name "PreviewUnavailableText"'
	)) {
	if ($loadoutEditorLayoutText -notmatch [regex]::Escape($requiredLoadoutPreviewStatusLayoutEntry)) {
		throw "Loadout editor preview status must be layout-owned: $requiredLoadoutPreviewStatusLayoutEntry"
	}
}
foreach ($requiredLoadoutPreviewStatusScriptEntry in @(
		"protected void RenderPreviewStage",
		'SetLoadoutWidgetVisible(root, "Toast"',
		'SetLoadoutWidgetColor(root, "ToastBackground"',
		'SetLoadoutWidgetColor(root, "ToastAccent"',
		'SetLoadoutText(root, "ToastText"',
		'SetLoadoutWidgetVisible(root, "PreviewUnavailableText"',
		'SetLoadoutText(root, "PreviewUnavailableText"'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutPreviewStatusScriptEntry)) {
		throw "Loadout editor preview status must populate named layout widgets: $requiredLoadoutPreviewStatusScriptEntry"
	}
}
$loadoutPreviewStatusMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderPreviewStage[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected string BuildStageToast")
if (!$loadoutPreviewStatusMatch.Success) {
	throw "Loadout editor preview status renderer is missing"
}
foreach ($forbiddenLoadoutPreviewStatusGeometry in @(
		"CreateTextWidget",
		"CreateRectWidget",
		"CreateWrappedTextWidget",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"GetRegionLayoutSize",
		"m_iEditorWidth",
		"m_Layout.m_iMainLeft"
	)) {
	if ($loadoutPreviewStatusMatch.Value -match [regex]::Escape($forbiddenLoadoutPreviewStatusGeometry)) {
		throw "Loadout editor preview status must use named layout widgets: $forbiddenLoadoutPreviewStatusGeometry"
	}
}
if ($loadoutEditorComponentText -match [regex]::Escape("protected void RenderCandidateRow")) {
	throw "Loadout editor must not keep unused legacy candidate row geometry renderer"
}
foreach ($requiredPreviewCellLayoutEntry in @(
		"HST_LoadoutItemPreviewCell",
		'Slot FrameWidgetSlot "{7B2FD986A4D3420F}"',
		"Anchor 0 0 1 1",
		"SlotImage",
		"SlotPreview",
		'"Is Visible" 0',
		'"Use clear color" 0',
		'"Ignore Cursor" 1',
		"ItemPreviewWidgetClass"
	)) {
	if ($loadoutPreviewCellLayoutText -notmatch [regex]::Escape($requiredPreviewCellLayoutEntry)) {
		throw "Loadout item preview cell layout is missing stable entry: $requiredPreviewCellLayoutEntry"
	}
}
$loadoutPreviewCellFactoryMatch = [regex]::Match($loadoutEditorComponentText, "protected Widget CreateItemPreviewCell[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void BindItemPreviewCellChild")
if (!$loadoutPreviewCellFactoryMatch.Success) {
	throw "Loadout item preview cell factory is missing"
}
foreach ($forbiddenPreviewCellFactoryGeometry in @(
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"GetLayoutSquareSize",
		"int left",
		"int top",
		"int size"
	)) {
	if ($loadoutPreviewCellFactoryMatch.Value -match [regex]::Escape($forbiddenPreviewCellFactoryGeometry)) {
		throw "Loadout item preview cell factory must rely on the stretched preview-cell layout: $forbiddenPreviewCellFactoryGeometry"
	}
}
foreach ($forbiddenPreviewCellGeometryContract in @(
		"GetLayoutSquareSize",
		"CreateItemPreviewCell(WorkspaceWidget workspace, Widget parent, int left",
		"CreateNodePreviewCell(WorkspaceWidget workspace, Widget root, int nodeIndex, int left",
		"CreateCandidatePreviewCell(WorkspaceWidget workspace, Widget root, int candidateIndex, int left"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenPreviewCellGeometryContract)) {
		throw "Loadout preview cells must be anchor-filled layout children, not script-sized widgets: $forbiddenPreviewCellGeometryContract"
	}
}
foreach ($requiredPreviewFallbackUnderlayEntry in @(
		"PREVIEW_FALLBACK_UNDERLAY_OPACITY",
		"protected void SetPreviewCellFallbackUnderlay",
		"SetPreviewCellFallbackUnderlay(cell)",
		"imageWidget.SetOpacity(PREVIEW_FALLBACK_UNDERLAY_OPACITY)",
		"nativePrefab=true",
		"nativeEntity=true"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredPreviewFallbackUnderlayEntry)) {
		throw "Loadout preview cells must keep a fallback underlay while native item previews are attempted: $requiredPreviewFallbackUnderlayEntry"
	}
}
if ($loadoutEditorComponentText -match [regex]::Escape("FrameSlot.SetPos(m_UILayerWidget") -or $loadoutEditorComponentText -match [regex]::Escape("FrameSlot.SetSize(m_UILayerWidget")) {
	throw "Loadout editor must not manually size the stretched UI layer from script"
}
foreach ($forbiddenLoadoutPanelPlacementEntry in @(
		"BuildEditorSafeRect",
		"BuildStorageModeLayout",
		"ApplyEditorSafeRectOffset",
		"m_iSafeLeft",
		"m_iSafeTop",
		"m_iSafeWidth",
		"m_iSafeHeight",
		"m_iTabsLeft",
		"m_iTabsTop",
		"m_iContentTop",
		"m_iContentBottom",
		"m_iContentHeight",
		"m_iRailLeft",
		"m_iRailTop",
		"m_iRailBottom",
		"m_iMainLeft",
		"m_iMainTop",
		"m_iMainBottom",
		"m_iCategoryTop",
		"m_iListTop",
		"panelLeft",
		"panelOverrun",
		"groupTop",
		"sideMargin"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutPanelPlacementEntry)) {
		throw "Loadout editor must not restore script-owned panel placement metrics: $forbiddenLoadoutPanelPlacementEntry"
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
		"PREVIEW_LIGHTS_PREFAB",
		"QueuePreviewLightSpawn",
		"SpawnPreviewLight",
		"RefreshPreviewLightState",
		"DeletePreviewLight",
		"UpdatePreviewLightAngles",
		"RefreshPreviewWorldLoadout",
		"CreatePreviewEntity",
		"PREVIEW_DRESS_DELAY_MS",
		"CreatePreviewCloneFromDressedSource",
		"FinalizeDressedPreviewEntity",
		"HasUsablePreviewBounds",
		"ResolveExactPreviewInsertTarget",
		"m_PreviewEntity.SetOrigin(vector.Up)",
		"building fallback mannequin",
		"SetPreviewEntityQualityRecursive",
		"BuildStageToast",
		"UpdatePreviewCamera",
		"UpdatePreviewCameraImmediate",
		"DebugLog(string.Format(`"preview camera",
		"m_bDebugLoggingEnabled",
		'fields[0] == "DEBUG"',
		"AnimatePreviewCamera",
		"ApplyPreviewCameraImmediate",
		"m_vCameraTargetDirection",
		"m_fCameraTargetDistance",
		"m_aCurrentCameraMatrix",
		"m_vPreviewedEntityCenterWorld",
		"UpdatePreviewedEntityMetrics",
		"targetCameraPositionByDistance",
		"GetPreviewCharacterBounds",
		"GetWorldBounds",
		"vector.Distance",
		"boundsHeight",
		"BuildPreviewCameraDirection",
		"RotatePreviewCameraOffset",
		"Math.Clamp",
		"DeletePreviewWorld",
		"HST_LoadoutPreview",
		"BuildPreviewStatusLabel",
		"BuildDraftHeaderSummary",
		"BuildSwapHeaderText",
		"BuildSwapActionLabel",
		"BuildNodeActionLabel",
		"AddLoadoutNodeRow",
		"AddLoadoutStorageContainerRow",
		"AddLoadoutStorageItemRow",
		"AddLoadoutCandidateTile",
		"LOADOUT_TEMPLATE_SLOT_ROW_LAYOUT",
		"TEMPLATE_SAVE_WIDGET_ID_BASE",
		"TEMPLATE_LOAD_WIDGET_ID_BASE",
		"ConfigureTemplateSlotButton",
		"BuildTemplateSlotTitle",
		"BuildTemplateSlotSubtitle",
		"BuildTemplateSlotMeta",
		"IsTemplateSlotEmpty",
		"AddNodePreviewToRow",
		"AddCandidatePreviewToRow",
		"ShowRowPreviewChrome",
		'SetRowText(row, "EmptyText"',
		"BuildStorageCapacityLabel",
		"BuildCountBadgeLabel",
		"BuildNodeCountBadge",
		"BuildCandidateCountLabel",
		"SetRowWidgetVisible",
		"SetRowChildLayer",
		'SetRowChildLayer(row, "PreviewLine", 5)',
		"CountStorageCandidatesForTab",
		"LOADOUT_LAYOUT_FALLBACK_RAIL_WIDTH = 444",
		"LOADOUT_LAYOUT_FALLBACK_RAIL_HEIGHT = 856",
		"LOADOUT_LAYOUT_FALLBACK_MAIN_WIDTH = 704",
		"LOADOUT_LAYOUT_FALLBACK_MAIN_HEIGHT = 856",
		"m_Layout.m_iRailWidth = LOADOUT_LAYOUT_FALLBACK_RAIL_WIDTH",
		"m_Layout.m_iRailHeight = LOADOUT_LAYOUT_FALLBACK_RAIL_HEIGHT",
		"m_Layout.m_iMainWidth = LOADOUT_LAYOUT_FALLBACK_MAIN_WIDTH",
		"m_Layout.m_iMainHeight = LOADOUT_LAYOUT_FALLBACK_MAIN_HEIGHT",
		"GetRegionLayoutSize(workspace, railRoot",
		"GetRegionLayoutSize(workspace, panelRoot",
		"LOADOUT_STORAGE_CATEGORY_TAB_LAYOUT",
		"LOADOUT_TAB_BUTTON_LAYOUT",
		"AddLoadoutTabButton(workspace, items",
		'Widget items = target.FindAnyWidget("TopTabItems")',
		"workspace.CreateWidgets(LOADOUT_STORAGE_CATEGORY_TAB_LAYOUT, root)",
		'SetLoadoutImageTexture(tab, "Icon"',
		"ShortenText(BuildStorageTargetLabel(), 96)",
		'SetLoadoutText(panelRoot, "StorageCandidateEmpty"',
		'Widget items = root.FindAnyWidget("StorageCandidateItems")',
		"BuildStorageCapacityLabel(nodeIndex)",
		"ResolveLoadoutRegion(root, `"StorageBrowser`")",
		"ResolveLoadoutRegion(root, `"LeftRail`")",
		"SetRowTextOrHide(tile, `"Name`", ShortenText(name, 96)",
		"SetRowTextOrHide(tile, `"Count`", countText",
		"SetRowTextOrHide(row, `"Name`", ShortenText(name, 72)",
		"SetRowTextOrHide(row, `"Count`", countText",
		"SetRowTextOrHide(row, `"Primary`", ShortenText(primary, 48)",
		"SetRowTextOrHide(row, `"Secondary`", ShortenText(secondary, 72)",
		"cell.SetZOrder(20)",
		"slotImage.SetZOrder(21)",
		"slotPreview.SetZOrder(22)",
		"imageWidget.SetImage(0)",
		"previewWidget.SetZOrder(22)",
		"0xEE05080A",
		"0x664B5960",
		"PANEL_BACK_COLOR",
		"0xF2151C20",
		"CreateItemPreviewCell(WorkspaceWidget workspace, Widget parent, int userId)",
		"CreateNodePreviewCell(WorkspaceWidget workspace, Widget root, int nodeIndex, int userId, int color)",
		"CreateCandidatePreviewCell(WorkspaceWidget workspace, Widget root, int candidateIndex, int userId, int color)",
		"CreateNodePreviewCell(workspace, anchor, nodeIndex, userId, 0xFFE6E6E6)",
		"CreateCandidatePreviewCell(workspace, anchor, candidateIndex, userId, 0xFFE6E6E6)",
		"AddStorageContainerOverlayText",
		"AddStorageItemOverlayText",
		"slotPreview.SetVisible(false)",
		"slotPreview.SetOpacity(0.0)",
		"SetPreviewCellNeutralFallback",
		"SetPreviewCellFromPrefab(Widget cell, string prefab, ResourceName fallbackIcon, int color, bool showFallbackIcon = true)",
		"SetPreviewCellFromEntity(Widget cell, IEntity entity, ResourceName fallbackIcon, int color, bool showFallbackIcon = true)",
		"imageWidget.SetOpacity(0.0)",
		"SetPreviewCellFromPrefab(cell, prefab, fallbackIcon, color, true)",
		"SetPreviewCellFallbackIcon(cell, fallbackIcon, color)",
		"RenderSelectedNodeHeader",
		"ReturnFromAttachmentCandidateToWeapon",
		"m_sSelectedStorageContainerNodeId",
		"m_sSelectedStoredItemNodeId",
		"ResolveSelectedStorageContainerNodeId",
		"SelectStorageContainerNode",
		"SelectStoredItemNode",
		"EnsureCandidatePayloadForStorageContainer",
		"FindStorageBrowserCategoryIndex",
		"protected int GetStorageBrowserCategoryCount()",
		"return 6;",
		'weapon_group',
		"IsStorageBrowserItemCategory",
		"ConfigurePreviewDimmer",
		"m_PreviewWidget.SetClearColor(true, GetPreviewWorldTintColor())",
		'HST_LoadoutDimmer',
		'PositionPreviewEntityAtStage(m_PreviewEntity, "0 1.15 0")',
		'PositionPreviewEntityAtStage(m_PreviewEntity, "0 0.95 0")',
		"POST_ACTION_REFRESH_DELAY_MS",
		"QueuePostActionRefresh",
		"RequestPostActionRefresh",
		"ResolvePreviewStorageTargets",
		"FindUsablePreviewDepositStorages",
		"FindPreviewCargoDepositStorages",
		"FindPreviewCargoDepositStoragesRecursive",
		"IsPreviewStructuralAttachmentStorage",
		"IsPreviewCargoDepositStorage",
		"GatherPreviewStorageContentEntitiesFromStorages",
		"EnsureSelectedSlotForCategory",
		"m_iEditorWidth",
		"HST_UIWorkspaceMetrics.GetRawWorkspaceSize(workspace, m_iRawWorkspaceWidth, m_iRawWorkspaceHeight)",
		"HST_UIWorkspaceMetrics.GetLayoutSize(workspace, m_iEditorLayoutWidth, m_iEditorLayoutHeight)",
		"m_iEditorWidth = m_iEditorLayoutWidth",
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
		"m_SlotScroll",
		"m_StorageCandidateScroll",
		"m_StorageContainerScroll",
		"m_StorageContentScroll",
		"FindRowText",
		"SetRowText",
		"BindRowClick",
		"SaveLoadoutScrollOffsets",
		"ResetLoadoutScroll",
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
		"TEMPLATE_PAGE_NEXT_WIDGET_ID"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutEditorComponentEntry)) {
		throw "Fullscreen loadout editor component is missing: $requiredLoadoutEditorComponentEntry"
	}
}
$previewWorldSettingsMatch = [regex]::Match($loadoutEditorComponentText, "protected void ApplyPreviewWorldSettings[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void ApplyPreviewSkyPreset[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void SetPreviewSkyMaterialColor[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool ApplyPreviewWorldMaterialToSkySphereRecursive")
if (!$previewWorldSettingsMatch.Success) {
	throw "Loadout editor render-background settings must include preview world and sky material update helpers"
}
foreach ($requiredPreviewWorldSettingsEntry in @(
		"ApplyPreviewSkyPreset()",
		"ApplyPreviewWorldMaterialToSkySphereRecursive(m_PreviewStageEntity, GetPreviewWorldMaterial())",
		"GenericWorldEntity worldEntity = GenericWorldEntity.Cast(m_PreviewStageEntity)",
		"Material skyMaterial = worldEntity.GetSkyMaterial()",
		"int preset = m_VisualSettings.m_iWorldPreset",
		'SetPreviewSkyMaterialColor(skyMaterial, "ColorBottom"',
		'SetPreviewSkyMaterialColor(skyMaterial, "ColorZenith"',
		'SetPreviewSkyMaterialColor(skyMaterial, "ColorUp"',
		'skyMaterial.SetParam("IntensityLV"',
		"material.SetParam(paramName, rgba)"
	)) {
	if ($previewWorldSettingsMatch.Value -notmatch [regex]::Escape($requiredPreviewWorldSettingsEntry)) {
		throw "Loadout editor Render Background Color must update the actual preview sky/background material: $requiredPreviewWorldSettingsEntry"
	}
}
$previewSkySphereMaterialMatch = [regex]::Match($loadoutEditorComponentText, "protected bool ApplyPreviewWorldMaterialToSkySphereRecursive[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool IsPreviewSkySphereEntity")
if (!$previewSkySphereMaterialMatch.Success -or $previewSkySphereMaterialMatch.Value -notmatch [regex]::Escape("SCR_Global.SetMaterial(entity, material, true)")) {
	throw "Loadout editor Render Background Color must also update the spawned preview sky-sphere material"
}
foreach ($forbiddenLoadoutScaledFallbackMetric in @(
		"m_Layout.m_iRailWidth = ScalePx(444)",
		"m_Layout.m_iRailHeight = ScalePx(856)",
		"m_Layout.m_iMainWidth = ScalePx(704)",
		"m_Layout.m_iMainHeight = ScalePx(856)"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutScaledFallbackMetric)) {
		throw "Loadout editor fallback panel dimensions must be layout constants, not viewport-scaled geometry: $forbiddenLoadoutScaledFallbackMetric"
	}
}
foreach ($forbiddenLoadoutScreenLayoutMetric in @(
		"m_Layout.m_iTabHeight",
		"m_Layout.m_iTabWidth",
		"m_Layout.m_iTabGap",
		"m_Layout.m_iTabsHeight",
		"m_Layout.m_iTabsWidth",
		"m_Layout.m_iHeaderHeight",
		"m_Layout.m_iCategoryHeight",
		"m_Layout.m_iListHeight",
		"m_Layout.m_iSlotRowHeight",
		"m_Layout.m_iCountBadgeWidth",
		"Math.Round(w * 0.245)",
		"Math.Round(w * 0.30)",
		"Math.Round(w * 0.34)",
		"Math.Round(w * 0.36)",
		"Math.Round(h * 0.70)",
		"Math.Round(h * 0.64)",
		"desiredContentHeight"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutScreenLayoutMetric)) {
		throw "Loadout editor must not keep screen-derived panel metrics now owned by layouts: $forbiddenLoadoutScreenLayoutMetric"
	}
}
foreach ($requiredLoadoutStorageServiceEntry in @(
		"IsStorageBrowserCandidateCategory",
		'category == "weapon"',
		'category == "launcher"',
		'category == "sidearm"',
		"return IsStorageBrowserCandidateCategory(category)",
		"ResolveArsenalCountForPrefab(state, item.m_sPrefab, availableCount, infiniteAvailable)",
		"candidateCount > 0",
		'logReason = "ready"',
		"ClearSpawnedCargoStorageContents(temp)"
	)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredLoadoutStorageServiceEntry)) {
		throw "Loadout editor service is missing storage browser candidate category support: $requiredLoadoutStorageServiceEntry"
	}
}
$loadoutTemplatePayloadServiceMatch = [regex]::Match($loadoutEditorText, "for \(int loadoutSlotIndex = 0; loadoutSlotIndex < PERSONAL_LOADOUT_SLOT_COUNT; loadoutSlotIndex\+\+\)[\s\S]*?payload = payload \+ string.Format\(`"\\nTEMPLATE\|%1\|%2\|%3`"")
if (!$loadoutTemplatePayloadServiceMatch.Success -or $loadoutTemplatePayloadServiceMatch.Value -notmatch [regex]::Escape("BuildFixedLoadoutId(loadoutSlotIndex)")) {
	throw "Loadout editor service must emit fixed saved loadout slots in deterministic slot order"
}
$loadoutSaveSpecificSlotMatch = [regex]::Match($loadoutEditorText, "string SaveCurrentDraft[\s\S]*?int slotIndex = ResolveSaveSlotIndex\(state, identityId, targetLoadoutId\);")
if (!$loadoutSaveSpecificSlotMatch.Success -or $loadoutSaveSpecificSlotMatch.Value -notmatch [regex]::Escape("ResolveFixedLoadoutIndex(loadoutName)") -or $loadoutSaveSpecificSlotMatch.Value -notmatch [regex]::Escape("targetLoadoutId = loadoutName")) {
	throw "Loadout editor service must allow Save buttons to target a specific fixed saved loadout slot"
}
$parentLoadoutSlotCandidateMatch = [regex]::Match($loadoutEditorText, "protected bool IsCandidateCompatibleWithLoadoutSlot[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool IsParentLoadoutSlotCandidate[\s\S]*?\r?\n\t}")
if (!$parentLoadoutSlotCandidateMatch.Success) {
	throw "Loadout editor service is missing parent loadout-slot wearable candidate filtering"
}
$parentCandidateGateIndex = $parentLoadoutSlotCandidateMatch.Value.IndexOf("IsLoadoutClothingCategory(node.m_sCategory) && !IsParentLoadoutSlotCandidate(node.m_sCategory, temp)")
$parentCandidateReplaceIndex = $parentLoadoutSlotCandidateMatch.Value.IndexOf("storage.CanReplaceItem(temp, slot.GetID())")
if ($parentCandidateGateIndex -lt 0 -or $parentCandidateReplaceIndex -lt 0 -or $parentCandidateGateIndex -gt $parentCandidateReplaceIndex) {
	throw "Loadout editor parent clothing slot candidates must be category-filtered before native storage replacement checks"
}
foreach ($requiredParentLoadoutSlotCandidateEntry in @(
		"ResolveCategoryFromPrefab(prefab, display)",
		'if (slotCategory == "webbing")',
		'return prefabCategory == "webbing"',
		'if (slotCategory == "vest")',
		'return prefabCategory == "vest"',
		"return prefabCategory == slotCategory"
	)) {
	if ($parentLoadoutSlotCandidateMatch.Value -notmatch [regex]::Escape($requiredParentLoadoutSlotCandidateEntry)) {
		throw "Loadout editor parent slot candidate filter must use prefab/display wearable categories: $requiredParentLoadoutSlotCandidateEntry"
	}
}
$storageCategoryTabsMatch = [regex]::Match($loadoutEditorComponentText, "protected void RenderStorageCategoryTabs[\s\S]*?\r?\n\t}")
if (!$storageCategoryTabsMatch.Success) {
	throw "Loadout editor storage category tab renderer is missing"
}
foreach ($forbiddenStorageCategoryTabsGeometry in @(
		"CreateRectWidget",
		"CreateTextWidget",
		"CreateIconWidget",
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"tabLeft",
		"tabWidth",
		"iconLeft",
		"iconTop"
	)) {
	if ($storageCategoryTabsMatch.Value -match [regex]::Escape($forbiddenStorageCategoryTabsGeometry)) {
		throw "Storage category tabs must use the storage category tab layout: $forbiddenStorageCategoryTabsGeometry"
	}
}
$storageBrowserCandidateCategoryMatch = [regex]::Match($loadoutEditorText, "protected bool IsStorageBrowserCandidateCategory[\s\S]*?\r?\n\t}")
if ($storageBrowserCandidateCategoryMatch.Success -and $storageBrowserCandidateCategoryMatch.Value -match [regex]::Escape('category == "attachment"')) {
	throw "Storage browser candidate categories must not include attachment; attachments belong in the attachment editor mode"
}
if ($storageBrowserCandidateCategoryMatch.Success -and $storageBrowserCandidateCategoryMatch.Value -match [regex]::Escape('category == "backpack"')) {
	throw "Storage browser candidate categories must not include wearable storage categories; those belong in loadout slots, not add-items search"
}
$liveStorageScanMatch = [regex]::Match($loadoutEditorText, "protected void ScanEquippedStorageContainers[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void AddStorageContentNodes")
if (!$liveStorageScanMatch.Success) {
	throw "Loadout editor live storage scanner is missing"
}
foreach ($requiredLiveStorageScanEntry in @(
		"FindStorageInsertTargetStorages(containerEntity, insertStorages)",
		"AddUniqueStorages(insertStorages, contentStorages)",
		"CalculateStorageVolume(insertStorages, usedVolume, totalVolume, freeVolume)",
		"node.m_iUsedCapacity = CountStorageDisplayItems(contentStorages)",
		"node.m_iTotalCapacity = CountStorageAvailableFitOptions(state, playerEntity, insertStorages)",
		"AddStorageContentNodes(session, contentStorages, containerNodeId, slotIndex)"
	)) {
	if ($liveStorageScanMatch.Value -notmatch [regex]::Escape($requiredLiveStorageScanEntry)) {
		throw "Loadout editor live storage must use insert-capable child cargo storages for capacity and content rows: $requiredLiveStorageScanEntry"
	}
}
$liveStorageCargoMatch = [regex]::Match($loadoutEditorText, "protected bool IsCargoDepositStorage[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected int FindCargoDepositStorages[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void FindCargoDepositStoragesRecursive[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool IsStructuralAttachmentStorage[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void FindCargoDepositStoragesInAttachedEntities[\s\S]*?\r?\n\t}")
if (!$liveStorageCargoMatch.Success) {
	throw "Loadout editor cargo storage traversal is missing"
}
foreach ($requiredLiveStorageCargoEntry in @(
		"if (IsStructuralAttachmentStorage(storage))",
		"return false",
		"if (IsCargoDepositStorage(storage) && outStorages.Find(storage) < 0)",
		"if (IsStructuralAttachmentStorage(storage))",
		"FindCargoDepositStoragesInAttachedEntities(storage, outStorages, visited, depth + 1)",
		"GatherStorageContentEntities(storage, attached, attachedVisited)",
		"FindCargoDepositStoragesRecursive(child, outStorages, visited, depth)"
	)) {
	if ($liveStorageCargoMatch.Value -notmatch [regex]::Escape($requiredLiveStorageCargoEntry)) {
		throw "Loadout editor storage traversal must skip structural slots but recurse into their child cargo: $requiredLiveStorageCargoEntry"
	}
}
$liveStorageDisplayMatch = [regex]::Match($loadoutEditorText, "protected int CountStorageDisplayItems[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool ShouldDisplayStorageContentEntity[\s\S]*?\r?\n\t}")
if (!$liveStorageDisplayMatch.Success -or $liveStorageDisplayMatch.Value -notmatch [regex]::Escape("ShouldDisplayStorageContentEntity(item, prefab, category, display)") -or $liveStorageDisplayMatch.Value -notmatch [regex]::Escape("return !HST_ArsenalItemFilter.ShouldBlockArsenalEntity(item, prefab, category, display)")) {
	throw "Loadout editor storage contents must count/display inserted items while hiding structural container shells"
}
$liveStorageVolumeMatch = [regex]::Match($loadoutEditorText, "protected float CalculateStorageOccupiedVolume[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected float CalculateStorageMaxVolume[\s\S]*?\r?\n\t}")
if (!$liveStorageVolumeMatch.Success) {
	throw "Loadout editor storage volume helpers are missing"
}
foreach ($requiredLiveStorageVolumeEntry in @(
		"ClothNodeStorageComponent.Cast(storage)",
		"storage.GetOwnedStorages(ownedStorages, 1, false)",
		"SCR_UniversalInventoryStorageComponent.Cast(ownedStorage)",
		"ownedStorage.GetOccupiedSpace()",
		"ownedStorage.GetMaxVolumeCapacity()"
	)) {
	if ($liveStorageVolumeMatch.Value -notmatch [regex]::Escape($requiredLiveStorageVolumeEntry)) {
		throw "Loadout editor storage capacity must sum owned universal storages for cloth-node containers: $requiredLiveStorageVolumeEntry"
	}
}
$storageVolumeFillMatch = [regex]::Match($loadoutEditorComponentText, "protected void SetStorageVolumeFill[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void RenderPreviewStage")
if (!$storageVolumeFillMatch.Success) {
	throw "Loadout editor storage volume fill updater is missing"
}
$loadoutPreviewRenderKeyMatch = [regex]::Match($loadoutEditorComponentText, "protected string BuildPreviewRenderKey\(\)[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected bool GetPreviewCharacterBounds")
if (!$loadoutPreviewRenderKeyMatch.Success) {
	throw "Loadout editor preview render key builder is missing"
}
foreach ($requiredPreviewRenderKeyEntry in @(
		'if (m_sEditorMode != "storage")',
		'if (i < m_aSlotIds.Count() && m_aSlotIds[i].IndexOf(NODE_STORAGE_ITEM_PREFIX) == 0)',
		'continue;',
		'if (nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "storage_item")'
	)) {
	if ($loadoutPreviewRenderKeyMatch.Value -notmatch [regex]::Escape($requiredPreviewRenderKeyEntry)) {
		throw "Loadout editor preview render key must ignore storage selection/content mutations: $requiredPreviewRenderKeyEntry"
	}
}
foreach ($requiredStorageVolumeProgressEntry in @(
		'ProgressBarWidget fill = ProgressBarWidget.Cast(FindRowWidget(row, "VolumeFill"))',
		"float ratio = Math.Clamp(GetNodeVolumeRatio(nodeIndex), 0.0, 1.0)",
		"fill.SetMin(0.0)",
		"fill.SetMax(1.0)",
		"fill.SetCurrent(ratio)",
		"fill.SetDrawBackground(false)",
		"fill.SetColorInt(ResolveStorageVolumeColor(nodeIndex))"
	)) {
	if ($storageVolumeFillMatch.Value -notmatch [regex]::Escape($requiredStorageVolumeProgressEntry)) {
		throw "Loadout storage volume fill must set native progress bar value instead of geometry: $requiredStorageVolumeProgressEntry"
	}
}
foreach ($forbiddenStorageVolumeGeometry in @(
		"FrameSlot.SetPos",
		"FrameSlot.SetSize",
		"barLeft",
		"barTop",
		"barWidth",
		"fillWidth",
		"m_Layout.m_iRailWidth"
	)) {
	if ($storageVolumeFillMatch.Value -match [regex]::Escape($forbiddenStorageVolumeGeometry)) {
		throw "Loadout storage volume fill must not calculate frame geometry: $forbiddenStorageVolumeGeometry"
	}
}
foreach ($requiredCampaignDebugPhysicalLoadoutBlockEntry in @(
		"blocked: player inventory has no cargo or carrier slot for physical reflection check",
		"blocked: physical apply probe blocked because player inventory has no cargo or carrier slot",
		"CampaignDebugLoadoutPhysicalApplyStatus",
		"CampaignDebugLoadoutDraftApplyStatus",
		"CampaignDebugLoadoutPhysicalRestoreStatus",
		"CampaignDebugLoadoutDraftRestoreStatus"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugPhysicalLoadoutBlockEntry)) {
		throw "Campaign debug physical loadout capacity prerequisites must be explicit BLOCKED evidence: $requiredCampaignDebugPhysicalLoadoutBlockEntry"
	}
}
foreach ($physicalLoadoutStatusFunction in @(
		"CampaignDebugLoadoutPhysicalApplyStatus",
		"CampaignDebugLoadoutDraftApplyStatus",
		"CampaignDebugLoadoutPhysicalRestoreStatus",
		"CampaignDebugLoadoutDraftRestoreStatus"
	)) {
	$functionMatch = [regex]::Match($scriptText, "protected string $physicalLoadoutStatusFunction[\s\S]*?\r?\n\t}")
	if (!$functionMatch.Success) {
		throw "Campaign debug physical loadout status helper missing: $physicalLoadoutStatusFunction"
	}
	if ($functionMatch.Value -match [regex]::Escape('if (!vehicleLoadoutContext.m_bPhysicalInventoryCapacityAvailable)') -and $functionMatch.Value -match [regex]::Escape('return "WARN";')) {
		throw "Campaign debug physical loadout missing-capacity path must be BLOCKED, not WARN: $physicalLoadoutStatusFunction"
	}
}
$loadoutPreviewChromeMethods = @(
	@{ Name = "AddCandidatePreviewToListRow"; Pattern = "protected void AddCandidatePreviewToListRow[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void AddCandidateListOverlayText" },
	@{ Name = "AddNodePreviewToRow"; Pattern = "protected void AddNodePreviewToRow[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void AddCandidatePreviewToRow" },
	@{ Name = "AddCandidatePreviewToRow"; Pattern = "protected void AddCandidatePreviewToRow[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void ShowRowPreviewChrome" }
)
foreach ($loadoutPreviewChromeMethod in $loadoutPreviewChromeMethods) {
	$loadoutPreviewChromeMatch = [regex]::Match($loadoutEditorComponentText, $loadoutPreviewChromeMethod.Pattern)
	if (!$loadoutPreviewChromeMatch.Success) {
		throw "Loadout editor preview chrome method is missing: $($loadoutPreviewChromeMethod.Name)"
	}
	foreach ($forbiddenLoadoutPreviewChromeGeometry in @(
			"CreateRectWidget",
			"CreateTextWidget",
			"CreateWrappedTextWidget",
			"FrameSlot.SetPos",
			"FrameSlot.SetSize",
			"previewBack",
			"previewLine"
		)) {
		if ($loadoutPreviewChromeMatch.Value -match [regex]::Escape($forbiddenLoadoutPreviewChromeGeometry)) {
			throw "Loadout editor row preview chrome must be layout-owned in $($loadoutPreviewChromeMethod.Name): $forbiddenLoadoutPreviewChromeGeometry"
		}
	}
}
foreach ($requiredLoadoutPreviewChromeScriptEntry in @(
		"protected void ShowRowPreviewChrome",
		'SetRowImageColor(row, "PreviewBack", 0xEE05080A, 1.0)',
		'SetRowImageColor(row, "PreviewLine", 0x664B5960, 1.0)',
		'SetRowWidgetVisible(row, "PreviewLine", false)'
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutPreviewChromeScriptEntry)) {
		throw "Loadout editor row preview chrome must use named layout widgets: $requiredLoadoutPreviewChromeScriptEntry"
	}
}
foreach ($forbiddenLoadoutPathBRegression in @(
		"CreateScrollList",
		"protected Widget CreateScrollContainer",
		"protected void CreateButton",
		"protected void CreateCountBadge",
		"protected void CreateStorageVolumeBar",
		"protected void RenderCategoryChips",
		"protected Widget CreateRectWidget",
		"protected bool SetupCanvasRect",
		"protected ref array<float> BuildRectVertices",
		"protected TextWidget CreateTextWidget",
		"protected TextWidget CreateWrappedTextWidget",
		"protected bool CreateIconWidget",
		"protected string ResolveNodeIcon(",
		"protected string ResolveCandidateIcon(",
		"protected Widget ResolveRowOverlayRoot",
		"HST_LoadoutEditorDrawCommandSet",
		"TEXT_BASE_LAYOUT",
		"m_aCanvasCommandSets",
		"RenderNodeRowAt",
		"RenderStorageNodeRow",
		"RenderStorageCandidateTile",
		"CalculateStorageCandidateGrid",
		"BuildDisplayWithCount",
		"CreateCountBadge(workspace, row, countText",
		"CreateCountBadge(workspace, tile, countText",
		"m_iSlotPage",
		"SLOTS_PER_PAGE",
		"SLOT_PAGE_PREV_WIDGET_ID",
		"SLOT_PAGE_NEXT_WIDGET_ID",
		'FindAnyWidget("Content")',
		"missing Content"
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenLoadoutPathBRegression)) {
		throw "Loadout editor must not keep old manual scroll/pager pattern: $forbiddenLoadoutPathBRegression"
	}
}
if ($loadoutEditorComponentText -match "(?<![A-Za-z0-9_])SCROLL_LIST_LAYOUT(?![A-Za-z0-9_])") {
	throw "Loadout editor must not keep old manual scroll/pager pattern: SCROLL_LIST_LAYOUT"
}
if ($loadoutEditorComponentText -match "FrameSlot\.Set(Pos|Size)\((scroll|items|content|row|tile)\b") {
	throw "Loadout editor Path B code must not position scroll/items/content/row/tile widgets with FrameSlot"
}
if ($loadoutEditorComponentText -match "InventoryPreviewWorld10") {
	throw "Loadout editor preview must not rely on the old visible/cropped InventoryPreviewWorld10 stage"
}
if ($loadoutEditorComponentText -match [regex]::Escape("angles[0] = m_fPreviewYawDegrees") -or $loadoutEditorComponentText -match [regex]::Escape("angles[1] = m_fPreviewYawDegrees")) {
	throw "Loadout editor preview rotation must orbit the camera instead of rotating the preview mannequin"
}
foreach ($forbiddenSpotlightToggleEntry in @(
		"SPOTLIGHT_WIDGET_ID",
		"m_bSpotlightEnabled",
		"BuildSpotlightLabel",
		'"Spotlight"'
	)) {
	if ($loadoutEditorComponentText -match [regex]::Escape($forbiddenSpotlightToggleEntry)) {
		throw "Loadout editor must not expose a user-facing spotlight toggle: $forbiddenSpotlightToggleEntry"
	}
}
if ($loadoutEditorComponentText -match [regex]::Escape("FrameSlot.SetPos(m_PreviewWidget") -or $loadoutEditorComponentText -match [regex]::Escape("FrameSlot.SetSize(m_PreviewWidget")) {
	throw "Loadout editor must not set position/size on the full-anchor render-target layout widget"
}
if ($loadoutEditorComponentText -match [regex]::Escape("VCFlags.NOFILTER & VCFlags.NOLIGHT")) {
	throw "Loadout editor preview must not collapse visual component flags with a bitwise AND"
}
$cargoStorageFilterMatch = [regex]::Match($loadoutEditorText, "protected bool IsCargoDepositStorage[\s\S]*?\r?\n\t}")
if ($cargoStorageFilterMatch.Success -and $cargoStorageFilterMatch.Value -match "PURPOSE_EQUIPMENT_ATTACHMENT") {
	throw "Loadout editor must not treat equipment-attachment webbing components as inventory cargo"
}
$previewCargoStorageFilterMatch = [regex]::Match($loadoutEditorComponentText, "protected bool IsPreviewCargoDepositStorage[\s\S]*?\r?\n\t}")
if ($previewCargoStorageFilterMatch.Success -and $previewCargoStorageFilterMatch.Value -match "PURPOSE_EQUIPMENT_ATTACHMENT") {
	throw "Loadout preview must not treat equipment-attachment webbing components as inventory cargo"
}
if ($loadoutEditorComponentText -match "CreateTextWidget\([^\r\n]*m_sLastResult") {
	throw "Loadout editor must not render raw server result/debug text across the preview stage"
}
foreach ($requiredLoadoutDedupEntry in @(
		"protected bool IsNestedStorageDraftSlot",
		"IsNestedStorageDraftSlot(slot)",
		'if (slot.m_sSlotKind == "storage_item")',
		"return !slot.m_sParentSlotId.IsEmpty();"
	)) {
	if ($loadoutEditorText -notmatch [regex]::Escape($requiredLoadoutDedupEntry)) {
		throw "Loadout editor service must not re-emit nested storage draft slots as synthetic inventory rows: $requiredLoadoutDedupEntry"
	}
}
foreach ($requiredLoadoutDynamicClearEntry in @(
		"for (int i = m_aWidgets.Count() - 1; i >= 0; i--)",
		"widget.RemoveFromHierarchy();",
		"ClearLoadoutContainerChildren(root, `"TopTabItems`")",
		"m_aWidgets.Clear();"
	)) {
	if ($loadoutEditorComponentText -notmatch [regex]::Escape($requiredLoadoutDynamicClearEntry)) {
		throw "Loadout editor must remove tracked dynamic widgets before rebuilding mode/tab rows: $requiredLoadoutDynamicClearEntry"
	}
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
		'AddMenuAction(actions, TAB_ARSENAL, "Arsenal report"',
		'AddMenuAction(actions, TAB_ARSENAL, "Vehicle cargo report"',
		'AddMenuAction(actions, TAB_ARSENAL, "Loot nearby to arsenal"',
		'AddMenuAction(actions, TAB_ARSENAL, "Load loot to vehicle"',
		'AddMenuAction(actions, TAB_ARSENAL, "Unload vehicle loot to arsenal"',
		'AddMenuAction(actions, TAB_ARSENAL, "Withdraw best item"',
		'AddMenuAction(actions, TAB_ARSENAL, "Loadout editor status"',
		'AddMenuAction(actions, TAB_ARSENAL, "Apply saved loadout"'
	)) {
	if ($commandUiText -notmatch [regex]::Escape($requiredArsenalLootMenuEntry)) {
		throw "Arsenal/Loot I-menu action must remain visible: $requiredArsenalLootMenuEntry"
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
function New-ForbiddenExternalToken([int[]]$codes) {
	return -join ($codes | ForEach-Object { [char]$_ })
}
foreach ($forbiddenBaseGameOnlyEntry in @(
		(New-ForbiddenExternalToken @(82, 72, 83)),
		("#" + (New-ForbiddenExternalToken @(82, 72, 83))),
		(New-ForbiddenExternalToken @(65, 70, 82, 70)),
		(New-ForbiddenExternalToken @(77, 65, 82, 83, 79, 67)),
		(New-ForbiddenExternalToken @(70, 79, 82, 69, 67, 79, 78)),
		(New-ForbiddenExternalToken @(86, 75, 80, 79)),
		(New-ForbiddenExternalToken @(86, 86, 82, 71))
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
$arsenalItemFilterText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ArsenalItemFilter.c"
foreach ($requiredWearableLootFilterEntry in @(
		"static string ResolveWearableCategory",
		"IsKnownWearableContainer(prefab, displayName) || IsLoadoutClothingCategory(category)",
		"if (IsLoadoutClothingCategory(category))",
		"return !ResolveWearableCategory(prefab, displayName).IsEmpty()",
		"value.Contains(`"pouch`")",
		"value.Contains(`"suspenders`")",
		"HasEntrenchingToolToken(value) && HasStructuralContainerQualifier(value)"
	)) {
	if ($arsenalItemFilterText -notmatch [regex]::Escape($requiredWearableLootFilterEntry)) {
		throw "Arsenal structural-container filter must exempt real worn gear while still blocking child containers: $requiredWearableLootFilterEntry"
	}
}
foreach ($requiredWearableLootClassifierEntry in @(
		"string wearableCategory = HST_ArsenalItemFilter.ResolveWearableCategory(prefab, displayName)",
		"!wearableCategory.IsEmpty()",
		"HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(prefab, wearableCategory)",
		"HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(displayName, wearableCategory)",
		"return wearableCategory;"
	)) {
	if ($lootServiceText -notmatch [regex]::Escape($requiredWearableLootClassifierEntry)) {
		throw "Loot service must classify worn gear before structural cargo-storage filtering: $requiredWearableLootClassifierEntry"
	}
}
if ($lootServiceText -notmatch 'RekeyLegacyVehiclePartCargo[\s\S]*?m_sVehicleRuntimeId = vehicleId') {
	throw "Vehicle unload must rekey legacy part-bound cargo to the selected root vehicle"
}
if ($lootServiceText -notmatch 'CaptureNearbyVehicleToGarage[\s\S]*?StoreVehicle\(state, vehicle\)[\s\S]*?DeleteEntityAndChildren\(selectedVehicle\)[\s\S]*?IsVehicleRootStillPresent[\s\S]*?RemoveVehicle\(state, vehicle\.m_sVehicleId\)') {
	throw "Garage capture must store before delete and roll back the garage record if root deletion verification fails"
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
if ($scriptText -match '\bstring\s+reference\b') {
	throw "Enforce keyword 'reference' must not be used as a string variable or parameter name"
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

$coordinatorForPreflightText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
foreach ($requiredFactionCatalogPreflightEntry in @(
		"preflight.faction_templates.catalog_identity",
		"preflight.faction_templates.catalog_mismatches",
		"CountCampaignDebugFactionCatalogMismatches",
		"CampaignDebugCharacterPrefabCatalogFactionMatch",
		"CampaignDebugGroupPrefabCatalogFactionMatch",
		"BuildCampaignDebugFactionCatalogIdentityActual",
		"FIA uses INDFOR/FIA, US uses BLUFOR/US, and USSR uses OPFOR/USSR infantry/group catalog paths",
		"one or more faction templates include wrong-faction infantry/group prefab catalog entries",
		'prefab.Contains("/BLUFOR/US_Army/") || prefab.Contains("Character_US_")',
		'prefab.Contains("/OPFOR/USSR_Army/") || prefab.Contains("Character_USSR_")',
		'prefab.Contains("/INDFOR/") || prefab.Contains("Group_FIA_")',
		'prefab.Contains("/BLUFOR/") || prefab.Contains("Group_US_")',
		'prefab.Contains("/OPFOR/") || prefab.Contains("Group_USSR_")'
	)) {
	if ($coordinatorForPreflightText -notmatch [regex]::Escape($requiredFactionCatalogPreflightEntry)) {
		throw "Campaign-debug preflight must prove faction catalog identity before runtime spawn: $requiredFactionCatalogPreflightEntry"
	}
}
Write-Host "Campaign-debug faction catalog preflight proof OK"

$physicalWarServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PhysicalWarService.c"
$convoyOutcomeServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ConvoyOutcomeService.c"
foreach ($requiredActiveVehicleDetachEntry in @(
		"PLAYER_USED_ACTIVE_VEHICLE_DETACH_DISTANCE_METERS",
		"m_aVehicleSpawnBlockedZoneIds",
		"TryDetachPlayerUsedActiveVehicleFromZoneCleanup",
		"ShouldDetachActiveVehicleFromZoneCleanup",
		"RegisterDetachedActiveVehicle",
		"IsAnyLivingPlayerInVehicle",
		"ResolveEntityVehicle",
		"ResolveActiveVehicleRuntimeId",
		"IsActiveVehicleSpawnBlocked",
		"RecordActiveVehicleSpawnBlocked",
		"ClearActiveVehicleSpawnBlocked",
		"NormalizeStaticActiveGroupRoute",
		"DeleteRuntimeGroupEntity(activeGroup.m_sGroupId, false)",
		"detached_active_vehicle",
		"vehicle spawn blocked",
		"guard_distributed",
		"m_bDetached = true",
		"m_bDeleted = false"
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredActiveVehicleDetachEntry)) {
		throw "Physical war active-zone cleanup is missing stolen/occupied vehicle detach guard: $requiredActiveVehicleDetachEntry"
	}
}
foreach ($forbiddenActiveGroupRouteEntry in @(
		"patrol_distributed",
		"ReportPatrolWaypointUnavailable",
		"group.m_sRouteId = zone.m_sPatrolRouteId"
	)) {
	if ($scriptText -match [regex]::Escape($forbiddenActiveGroupRouteEntry)) {
		throw "Static active defenders must not retain dynamic patrol route/waypoint contract: $forbiddenActiveGroupRouteEntry"
	}
}
Write-Host "Physical active vehicle detach guard OK"

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
		"TryResolveConvoySpawnPlanFromGeneratedRouteSegments",
		"ResolveConvoyRandomStartDistanceMeters",
		"TryBuildConvoyVehicleStartSlots",
		"ResolveConvoyEndPosition",
		"CONVOY_START_PROBE_ATTEMPTS = 72",
		"CONVOY_ROUTE_SEGMENT_FALLBACK_STEPS = 4",
		"CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS = 300.0",
		"CONVOY_START_ROAD_SEARCH_RADIUS_METERS = 250.0",
		"CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS = 40.0",
		"MIN_CONVOY_START_DISTANCE_METERS = 2000.0",
		"MAX_CONVOY_START_DISTANCE_METERS = 5000.0",
		"IsConvoyDistanceInsideBand",
		"planned distance",
		"required band 2000-5000m",
		"band valid",
		"No road-resolved convoy destination",
		"No road-resolved convoy spawn plan found in required 2000-5000m band",
		"generated route segment fallback checked",
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
if ($missionRuntimeServiceText -notmatch [regex]::Escape("convoyWithoutVehicleAssets")) {
	throw "Convoy runtime must preserve asset-plan failures as unspawned convoy state instead of generic fallback"
}
$commandUIServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
if ($commandUIServiceText -notmatch [regex]::Escape('AddMenuAction(actions, TAB_MISSIONS, "Convoy Runtime Report", "inspect_convoy_runtime"')) {
	throw "Missions tab must expose Convoy Runtime Report"
}
Write-Host "Phase 2 convoy runtime report contract OK"

$campaignSaveDataText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$generatedContentServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_GeneratedContentService.c"
$coordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
foreach ($requiredObservationClassifierEntry in @(
		"IsCampaignDebugObservationReportHealthy",
		"read-only report generated without access/service/dispatch errors",
		"observation report was unavailable or missing required access/service/dispatch coverage"
	)) {
	if ($coordinatorText -notmatch [regex]::Escape($requiredObservationClassifierEntry)) {
		throw "Missing campaign-debug read-only observation classifier entry: $requiredObservationClassifierEntry"
	}
}
$observationCaseMatch = [regex]::Match($coordinatorText, "protected HST_CampaignDebugCaseResult BuildCampaignDebugObservationCase[\s\S]*?protected HST_CampaignDebugCaseResult BuildCampaignDebugActionCase")
if (!$observationCaseMatch.Success) {
	throw "Could not locate campaign-debug observation/action case boundary"
}
if ($observationCaseMatch.Value -match "IsCampaignDebugResultSuccessful\(result\)") {
	throw "Campaign-debug read-only observation reports must not use the mutating action result classifier"
}
$actionCaseMatch = [regex]::Match($coordinatorText, "protected HST_CampaignDebugCaseResult BuildCampaignDebugActionCase[\s\S]*?protected string SaveCampaignDebugRunArtifacts")
if (!$actionCaseMatch.Success -or $actionCaseMatch.Value -notmatch "IsCampaignDebugResultSuccessful\(result\)") {
	throw "Campaign-debug action reports must keep the mutating action result classifier"
}
if ($coordinatorText -match [regex]::Escape("protected void EnsureCampaignDebugArtifactRecorded(string artifactPath)")) {
	throw "Campaign-debug artifact helper must avoid the artifactPath parameter spelling; Workbench reports a broken expression at that method declaration"
}
foreach ($requiredConvoyMissionSweepEntry in @(
		"requiredConvoyVehicleAssets = 3",
		"mission.m_iRequiredVehicleCount > requiredConvoyVehicleAssets",
		"convoyVehicleAssets >= requiredConvoyVehicleAssets",
		"convoy runtime has too few convoy vehicle assets"
	)) {
	if ($coordinatorText -notmatch [regex]::Escape($requiredConvoyMissionSweepEntry)) {
		throw "Missing convoy mission-sweep required asset-count assertion: $requiredConvoyMissionSweepEntry"
	}
}
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
		"convoy readiness vehicle assets planned",
		"m_iActiveVehicleAssetCount",
		"m_iResolvedVehicleAssetCount",
		"convoy.assets.active_vehicle_count",
		"active unresolved vehicle assets >= 3 before movement proof",
		"spawned vehicles",
		"convoy readiness crew groups",
		"alive crew count",
		"driver availability",
		"route assignment",
		"waypoint assignment",
		"No planned convoy vehicle assets exist; convoy cannot move.",
		"No active convoy vehicle assets remain",
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
		"RefreshSeatedCrewState",
		"GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true)",
		"server-authoritative compartment move-in completed",
		"MoveInVehicle(vehicleEntity, compartmentType, true, slot)",
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
		"IsMissionConvoyTravelPhase(mission) && state.m_iElapsedSeconds % CONVOY_PROGRESS_SYNC_SECONDS == 0",
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

if ($campaignSchemaVersion -lt 17) {
	throw "Phase 7 convoy waypoint-chain movement requires campaign schema 17 or newer"
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
		"TryRegisterVehicleWithGroup",
		"AddUsableVehicle",
		"vehicle usage registered for group movement",
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
		"CONVOY_RUNTIME_WAYPOINT_MIN_COUNT = 3",
		"CONVOY_RUNTIME_WAYPOINT_MAX_COUNT = 5",
		"AppendSparseConvoyRouteWaypoints",
		"AppendResolvedConvoyWaypoint",
		"desiredWaypointCount = Math.Max(CONVOY_RUNTIME_WAYPOINT_MIN_COUNT, Math.Min(CONVOY_RUNTIME_WAYPOINT_MAX_COUNT, routeWaypoints.Count() + 2))",
		"if (result.Count() >= CONVOY_RUNTIME_WAYPOINT_MAX_COUNT)",
		"segmentCount = Math.Min(segmentCount, CONVOY_RUNTIME_WAYPOINT_MAX_COUNT)",
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
		"convoy.vehicle_usage.",
		"AI vehicle usage registered for group movement",
		"RefreshMissionConvoyCrewCount",
		"return prefab.Contains(""SentryTeam"");"
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase7RuntimeEntry)) {
		throw "Missing Phase 7 convoy runtime waypoint-chain entry: $requiredPhase7RuntimeEntry"
	}
}
foreach ($forbiddenPhase7DenseWaypointEntry in @(
		"AppendConvoyRoadSegmentWaypoints(waypoints, previousPosition"
	)) {
	if ($physicalWarServiceText -match [regex]::Escape($forbiddenPhase7DenseWaypointEntry)) {
		throw "Phase 7 convoy route assignment must keep sparse runtime waypoints: $forbiddenPhase7DenseWaypointEntry"
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

if ($campaignSchemaVersion -lt 17) {
	throw "Phase 8 convoy progress requires campaign schema 17 or newer while progress samples remain transient diagnostics"
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
foreach ($requiredPhase8DebugWindowEntry in @(
		"ResolveCampaignDebugConvoyMovementWaitSeconds",
		"HST_PhysicalWarService.CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS + HST_PhysicalWarService.CONVOY_PROGRESS_SYNC_SECONDS * 3 + 1",
		"movement wait %5s"
	)) {
	if ($coordinatorText -notmatch [regex]::Escape($requiredPhase8DebugWindowEntry)) {
		throw "Missing Phase 8 convoy debug movement-window entry: $requiredPhase8DebugWindowEntry"
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
		"No road-resolved convoy spawn plan found in required 2000-5000m band",
		"vector startSlot = convoyStartSlots[i]",
		"TryResolveConvoyVehicleStartSlot",
		"convoy vehicle slot is outside the required 2000-5000m destination band",
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

if ($campaignSchemaVersion -lt 17) {
	throw "Phase 9 convoy contact requires campaign schema 17 or newer because contact uses existing mission runtime phase state"
}
foreach ($requiredPhase9RuntimeEntry in @(
		"CONVOY_CONTACT_RADIUS_METERS = 120.0",
		"UpdateMissionConvoyContact",
		"TryResolveMissionConvoyContactReason",
		"SetMissionConvoyContact",
		"BuildMissionConvoyContactReport",
		"Convoy contact: living player within",
		"Convoy contact pending: crew count decreased before explicit contact",
		"asset.m_bDelivered",
		"asset.m_bDestroyed",
		"IsVehicleMobile(vehicleEntity, mobileReason)",
		"MarkMissionConvoyVehicleDestroyed",
		"mission.m_sRuntimePhase = MISSION_CONVOY_CONTACT",
		"ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_CONTACT)",
		"convoy contact | active",
		"radius %2m",
		"ShouldSpawnMissionConvoyRuntime",
		"IsRestoredMissionConvoyRuntimeRebindPending",
		"state.m_iLastRestoreSecond + CONVOY_CREW_POPULATION_GRACE_SECONDS",
		"state.m_bRestoredFromPersistence",
		"GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) == null",
		"GetRuntimeVehicleEntity(activeGroup.m_sGroupId) == null",
		"ShouldSpawnMissionConvoyRuntime(state, activeGroup)",
		"IsMissionConvoyTravelPhase",
		"Convoy contact recovery",
		"MoveUnseatedLivingCrewNearVehicle(crewEntity, vehicleEntity, vehicleEntity.GetOrigin())",
		"mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT && state.m_iElapsedSeconds % CONVOY_PROGRESS_SYNC_SECONDS == 0",
		"activeGroup.m_iAssignedWaypointCount = 0",
		"IsMissionConvoyGroupAssetTerminal",
		"asset.m_bDestroyed || asset.m_bDelivered",
		"activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED",
		"aliveCount <= 0 && activeGroup.m_iSpawnedAgentCount <= 0 && (!missionConvoyGroup || activeGroup.m_iLastSeenAliveCount <= 0)",
		"HasMissionConvoyCrewEverBeenObservedAlive",
		"HasMissionConvoyExplicitEliminationContext",
		"TryRepairMissionConvoyCrewPopulation",
		"Convoy crew unobserved before explicit contact",
		"convoy remains pending instead of eliminated",
		"if (!HasMissionConvoyCrewEverBeenObservedAlive(activeGroup))",
		"activeGroup.m_bEverHadLivingCrew = true;",
		"activeGroup.m_iMaxObservedCrewAlive = Math.Max(activeGroup.m_iMaxObservedCrewAlive, 1);",
		"EXPIRED_CONVOY_PLAYER_RENDER_BUBBLE_METERS = 1800.0",
		"ShouldKeepExpiredEngagedConvoyRuntime",
		"expired_combat_preserved",
		"Expired convoy combat preserved until no living player remains inside render bubble."
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredPhase9RuntimeEntry)) {
		throw "Missing Phase 9 convoy contact runtime entry: $requiredPhase9RuntimeEntry"
	}
}
foreach ($requiredConvoyOutcomeLiveHistoryEntry in @(
		"ShouldApplyConvoyCrewEliminatedOutcome(state, mission)",
		"HasConvoyEliminatedCrewEvidence",
		"HasConvoyCrewLiveHistory",
		'MISSION_CONVOY_GROUP_PREFIX = "mission_convoy_"',
		"activeGroup.m_bEverHadLivingCrew",
		"activeGroup.m_iMaxObservedCrewAlive > 0",
		"convoyGroups > 0 && eliminatedGroups == convoyGroups"
	)) {
	if ($convoyOutcomeServiceText -notmatch [regex]::Escape($requiredConvoyOutcomeLiveHistoryEntry)) {
		throw "Convoy outcome crew-elimination rewards must require observed live crew history: $requiredConvoyOutcomeLiveHistoryEntry"
	}
}
if ($convoyOutcomeServiceText -match "ShouldApplyConvoyCrewEliminatedOutcome\s*\(\s*HST_ActiveMissionState\s+mission\s*\)") {
	throw "Convoy outcome crew-elimination guard must receive campaign state so it can prove mission_convoy_ active group live history"
}
foreach ($requiredConvoyPostCompletionLiveHistoryEntry in @(
		"IsPostCompletionConvoyInteractionAllowed(state, mission, asset, commandId)",
		"IsPreservedConvoyAssetAfterCrewElimination(state, mission, runtimeAsset)",
		"IsPreservedConvoyAssetAfterCrewElimination(state, mission, asset)",
		"IsConvoyCrewEliminationCompletionEvent(mission.m_sLastRuntimeEventKey) && HasConvoyEliminatedCrewEvidence(state, mission)",
		"HasConvoyCrewLiveHistory"
	)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredConvoyPostCompletionLiveHistoryEntry)) {
		throw "Mission runtime post-completion convoy preservation must require state-backed live-crew history: $requiredConvoyPostCompletionLiveHistoryEntry"
	}
}
foreach ($requiredConvoyUiLiveHistoryEntry in @(
		"IsPostCompletionConvoyOutcomeMission(state, mission)",
		"HasConvoyEliminatedCrewEvidence",
		"HasConvoyCrewLiveHistory"
	)) {
	if ($commandUIServiceText -notmatch [regex]::Escape($requiredConvoyUiLiveHistoryEntry)) {
		throw "Command UI post-completion convoy actions must require state-backed live-crew history: $requiredConvoyUiLiveHistoryEntry"
	}
}
$convoyLiveHistoryGuards = @(
	@{
		Name = "physical-war convoy completion"
		Text = $physicalWarServiceText
		Pattern = "protected bool HasMissionConvoyCrewEverBeenObservedAlive[\s\S]*?return false;\s*\}"
	},
	@{
		Name = "convoy outcome rewards"
		Text = $convoyOutcomeServiceText
		Pattern = "protected bool HasConvoyCrewLiveHistory[\s\S]*?return false;\s*\}"
	},
	@{
		Name = "mission runtime post-completion"
		Text = $missionRuntimeServiceText
		Pattern = "protected bool HasConvoyCrewLiveHistory[\s\S]*?return false;\s*\}"
	},
	@{
		Name = "command UI post-completion"
		Text = $commandUIServiceText
		Pattern = "protected bool HasConvoyCrewLiveHistory[\s\S]*?return false;\s*\}"
	}
)
foreach ($guard in $convoyLiveHistoryGuards) {
	$match = [regex]::Match($guard.Text, $guard.Pattern)
	if (!$match.Success) {
		throw "Missing convoy live-history guard for $($guard.Name)"
	}
	if ($match.Value -match "m_iLastSeenAliveCount\s*>\s*0" -or $match.Value -match "m_iSurvivorInfantryCount\s*>\s*0") {
		throw "Convoy live-history guard must use explicit observation fields only for $($guard.Name)"
	}
}
if ($missionRuntimeServiceText -match "protected bool HasConvoyCrewEliminatedForPostCompletion\s*\(\s*HST_ActiveMissionState\s+mission\s*\)" -or $commandUIServiceText -match "protected bool HasConvoyCrewEliminatedForPostCompletion\s*\(\s*HST_ActiveMissionState\s+mission\s*\)") {
	throw "Post-completion convoy helpers must not use mission-only event-token checks"
}
foreach ($requiredActiveGroupPopulationRuntimeEntry in @(
		"ACTIVE_GROUP_AGENT_POPULATION_DIRECT_FALLBACK_ATTEMPT = 4",
		"ACTIVE_GROUP_AGENT_POPULATION_SLOT_PRIMARY_ATTEMPT = 4",
		"ACTIVE_GROUP_SPAWN_MODE_DIRECT_INFANTRY_FALLBACK",
		"ACTIVE_GROUP_SPAWN_MODE_GROUP_SLOT_PRIMARY",
		"ACTIVE_GROUP_AI_WORLD_MIN_LIMIT = 512",
		"ACTIVE_GROUP_RUNTIME_STATUS_AIWORLD_BUDGET_DEFERRED",
		"ACTIVE_GROUP_SPAWN_MODE_AIWORLD_BUDGET_DEFERRED",
		'DIRECT_INFANTRY_GROUP_PREFAB = "{6985327711303910}Prefabs/Groups/HST/HST_RuntimeEmptyGroup.et"',
		"SpawnControlledNativeActiveGroupPrefab",
		"controlled group prefab spawn",
		"if (!group.GetSpawnImmediately())",
		"EnsureActiveGroupAIWorldBudget",
		"MarkActiveGroupAIWorldBudgetDeferred",
		"aiWorld.SetAILimit(requiredLimit)",
		"active group AIWorld limit raised",
		"active group AIWorld native spawn deferred",
		"spawn_deferred_aiworld_budget",
		"group.SpawnUnits();",
		"TryPopulatePendingActiveGroupFromNativeSlots",
		"SpawnNativeSlotMembersIntoRuntimeGroup",
		"stock group slot member spawned",
		"active group populated %1 with %2 stock %3 slot members",
		"BuildNativeGroupPopulationDebug",
		"BuildAIWorldBudgetDebug",
		"CountNativeGroupMemberSlots",
		"slotPrimary",
		'TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, requestedStatus, state, "retry", true)',
		"AIGroup stock member-slot population or direct faction infantry fallback attempted but still has zero agents",
		"BuildFinalActiveGroupPopulationFailureReason",
		"zero agents after grace | reason %3",
		'StabilizeRuntimeAIGroupRoot(entity, activeGroup, "pending population registration")',
		"active group stabilized native AIGroup root",
		"active group primary stock root faction verified",
		'"native immediate populated", true',
		"activation pending native population",
		"CountPendingActiveZonePopulationInfantry",
		"CountPendingActiveZonePopulationGroups",
		"pending infantry 0 | folded failures may have returned to abstract garrison",
		"Native SCR_AIGroup.SpawnUnits retry skipped",
		"active group direct infantry fallback skipped",
		"Direct faction infantry fallback replacing missing runtime group entity.",
		"active group direct infantry fallback replacing missing runtime group entity",
		"AttachFactionInfantryMemberToRuntimeGroup",
		"ResolveRuntimeMemberAIAgent",
		"group.AddAgentFromControlledEntity(member)",
		"active group member attach failed",
		"direct infantry spawned",
		"array<IEntity> checkedMemberEntities",
		"checkedMemberEntities.Find(entity) >= 0",
		"return CountAliveRuntimeInfantryGroupAgents(groupId);",
		"friendlyAliveObserved && friendlyFactionMismatches == 0",
		"enemyAliveObserved && enemyFactionMismatches == 0",
		"CollectLivingNativeAIGroupEntities",
		"array<IEntity> livingInfantry",
		"livingInfantry.Find(entity) < 0",
		"int agentCount = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId)",
		"CountCampaignDebugRuntimeFactionMismatches",
		"campaign debug faction audit",
		"IsGroupPrefabCatalogFactionMatch",
		"rejected wrong-faction group prefab",
		"prefab %3 | group root faction",
		"group root faction",
		"ResolveActiveGroupRuntimeRootFactionKey",
		"CountRuntimeGroupControlledEntities",
		"BuildActiveGroupRuntimeVisualEvidence",
		"BuildRuntimeEntityVisualEvidence",
		"visual %9",
		"TryFlushPendingNativeGroupSpawnImmediately",
		"SpawnAllImmediately",
		"nativeImmediate %4",
		"CampaignDebugResolveActiveGroupRouteAssignment",
		"CampaignDebugResolvePendingPopulationActiveGroups",
		"ResolvePendingActiveGroupRequestedStatus",
		"campaign debug route assignment",
		"Assigned infantry route waypoint chain %1 via campaign debug route resolver",
		"ResolveActiveGroupPrimarySpawnMode",
		"IsSupportRequestActiveGroup",
		"CountCampaignDebugDirectFallbackActiveGroups",
		"direct fallback groups %2 | terminal fallback groups %3"
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredActiveGroupPopulationRuntimeEntry)) {
		throw "Active AIGroup population must prove controlled native group spawning, route-proof recovery, and tagged direct fallback detection: $requiredActiveGroupPopulationRuntimeEntry"
	}
}
$campaignDebugPopulationResolverMatch = [regex]::Match($physicalWarServiceText, "bool CampaignDebugResolvePendingActiveGroupPopulation[\s\S]*?bool CampaignDebugResolveActiveGroupRouteAssignment")
if (!$campaignDebugPopulationResolverMatch.Success) {
	throw "Could not locate campaign-debug active-group population resolver"
}
if ($campaignDebugPopulationResolverMatch.Value -match "TryPopulatePendingActiveGroupFromFactionInfantry") {
	throw "Campaign-debug pre-route population resolver must not create direct faction-infantry fallback while certifying primary group population"
}
if ($campaignDebugPopulationResolverMatch.Value -notmatch [regex]::Escape("direct faction-infantry fallback is not certification proof")) {
	throw "Campaign-debug pre-route population resolver must explicitly report pending primary population instead of direct fallback certification"
}
if ($campaignDebugPopulationResolverMatch.Value -notmatch [regex]::Escape("TryFlushPendingNativeGroupSpawnImmediately")) {
	throw "Campaign-debug pre-route population resolver must force-drain the native delayed queue before judging route proof"
}
$campaignDebugRouteResolverMatch = [regex]::Match($physicalWarServiceText, "bool CampaignDebugResolveActiveGroupRouteAssignment[\s\S]*?protected bool TryKickPendingNativeGroupSpawn")
if (!$campaignDebugRouteResolverMatch.Success) {
	throw "Could not locate campaign-debug route assignment resolver"
}
foreach ($requiredRouteResolverEntry in @(
		"CampaignDebugResolvePendingActiveGroupPopulation",
		'TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, requestedStatus, state, "campaign debug route assignment", true)',
		"UpdateRoutedActiveGroupsNow(state, preset, true)",
		"AssignActiveGroupInfantryRouteWaypoints",
		"infantry_waypoints",
	"infantry_sweep"
	)) {
	if ($campaignDebugRouteResolverMatch.Value -notmatch [regex]::Escape($requiredRouteResolverEntry)) {
		throw "Campaign-debug route resolver must force population recovery and prove real move/sweep waypoint assignment: $requiredRouteResolverEntry"
	}
}
$campaignDebugCleanupPopulationDrainMatch = [regex]::Match($physicalWarServiceText, "int CampaignDebugResolvePendingPopulationActiveGroups[\s\S]*?protected string BuildActiveGroupPendingPopulationActual")
if (!$campaignDebugCleanupPopulationDrainMatch.Success) {
	throw "Could not locate campaign-debug cleanup population drain helper"
}
foreach ($requiredCleanupPopulationDrainEntry in @(
		"CampaignDebugResolvePendingActiveGroupPopulation",
		"ResolvePendingActiveGroupRequestedStatus",
		"attempted %1 | resolved %2 | unresolved %3 | deferred %4"
	)) {
	if ($campaignDebugCleanupPopulationDrainMatch.Value -notmatch [regex]::Escape($requiredCleanupPopulationDrainEntry)) {
		throw "Campaign-debug cleanup population drain must force primary pending population before cleanup audits: $requiredCleanupPopulationDrainEntry"
	}
}
if ($campaignDebugCleanupPopulationDrainMatch.Value -match "TryPopulatePendingActiveGroupFromFactionInfantry") {
	throw "Campaign-debug cleanup population drain must not direct-fallback groups while certifying primary population"
}
$controlledStockGroupSpawnMatch = [regex]::Match($physicalWarServiceText, "protected GenericEntity SpawnControlledNativeActiveGroupPrefab[\s\S]*?protected void StabilizeRuntimeAIGroupRoot")
if (!$controlledStockGroupSpawnMatch.Success) {
	throw "Could not locate controlled active-group stock spawn helper"
}
if ($controlledStockGroupSpawnMatch.Value -match [regex]::Escape("SCR_AIGroup.IgnoreSpawning(true)") -or $controlledStockGroupSpawnMatch.Value -match [regex]::Escape("controlled group prefab pre-spawn")) {
	throw "Controlled stock active-group spawning must follow the vanilla SCR_AIGroup spawn path and must not suppress native spawning or force pre-spawn faction rebroadcast"
}
if ($physicalWarServiceText -match [regex]::Escape('ApplyRuntimeGroupFaction(entity, activeGroup, "group prefab spawn")')) {
	throw "Controlled stock active-group spawning must not repair or broadcast faction on an empty/pending group root before native member proof exists"
}
if ($scriptText -notmatch [regex]::Escape('statusOrHistory.Contains("spawn_deferred_aiworld_budget")')) {
	throw "Campaign debug async runtime classifier must treat AIWorld budget deferrals as pending/blocking proof, not generic failures"
}
if ($physicalWarServiceText -match [regex]::Escape("skipped terminal %3")) {
	throw "Direct fallback certification must count terminal fallback rows as degraded primary-method evidence"
}
foreach ($requiredPrimaryGroupCertificationEntry in @(
		"post_cleanup.runtime_group_primary_spawn",
		"cleanup.runtime_group_primary_spawn",
		"runtime_direct_fallback_groups"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPrimaryGroupCertificationEntry)) {
		throw "Campaign debug cleanup must fail active direct-fallback groups instead of certifying them: $requiredPrimaryGroupCertificationEntry"
	}
}
if ($physicalWarServiceText -match [regex]::Escape('TryRepairEmptyRuntimeGroupPopulation(state, activeGroup, "campaign debug faction audit")')) {
	throw "Campaign debug faction audits must not repair empty runtime group shells while measuring primary spawn proof"
}
if ($physicalWarServiceText -match 'DIRECT_INFANTRY_GROUP_PREFAB = "\{000CD338713F2B5A\}Prefabs/AI/Groups/Group_Base\.et"') {
	throw "Active-group direct infantry fallback must use the HST-owned non-deleting runtime group prefab instead of raw Group_Base"
}
foreach ($requiredRuntimeFactionAuditEntry in @(
		"post_cleanup.runtime_factions",
		"cleanup.runtime_factions",
		"runtime_faction_mismatches",
		"pending live-count",
		"skipped pending population",
		"skipped terminal empty",
		"zero live controlled members for infantry group"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredRuntimeFactionAuditEntry)) {
		throw "Campaign debug cleanup must audit runtime group/vehicle faction mismatches: $requiredRuntimeFactionAuditEntry"
	}
}
foreach ($requiredCleanupPopulationDrainAssertionEntry in @(
		"post_cleanup.runtime_population_drain_resolved",
		"cleanup.runtime_population_drain_resolved",
		"population drain | "
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCleanupPopulationDrainAssertionEntry)) {
		throw "Campaign debug cleanup must drain primary pending population before final audits: $requiredCleanupPopulationDrainAssertionEntry"
	}
}
if ($physicalWarServiceText -match [regex]::Escape("ApplyEntityFaction(vehicleEntity")) {
	throw "Physical-war vehicle spawns must remain unclaimed; do not stamp vehicle entities with faction ownership"
}
if ($physicalWarServiceText -match [regex]::Escape("ApplyEntityFaction(entity, activeGroup.m_sFactionKey);")) {
	throw "Physical-war vehicle-only group spawns must remain unclaimed; do not stamp vehicle entities with active-group faction ownership"
}
if ($physicalWarServiceText -match [regex]::Escape("runtime vehicle faction applied")) {
	throw "Runtime faction repair must clear vehicle claims instead of applying a faction to vehicle entities"
}
foreach ($requiredUnclaimedVehicleEntry in @(
		"static bool ClearVehicleFactionAffiliation",
		"static bool ClearVehicleFactionAffiliationRecursive",
		"static bool ClearVehicleFactionAffiliationRecursiveCount",
		"static string ResolveVehicleFactionKey",
		"static int CountVehicleFactionClaimsRecursive"
	)) {
	if ($vehicleRootPolicyText -notmatch [regex]::Escape($requiredUnclaimedVehicleEntry)) {
		throw "Vehicle root policy must expose shared unclaimed-vehicle helpers: $requiredUnclaimedVehicleEntry"
	}
}
foreach ($requiredUnclaimedRuntimeVehicleAuditEntry in @(
		"HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(vehicleEntity)",
		"HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursiveCount(vehicleEntity",
		"CountRuntimeVehicleClaimMismatch",
		"runtime vehicle faction cleared",
		"vehicle claimed faction",
		"vehicles are unclaimed"
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredUnclaimedRuntimeVehicleAuditEntry)) {
		throw "Physical-war runtime proof must keep vehicles unclaimed while factioning crews: $requiredUnclaimedRuntimeVehicleAuditEntry"
	}
}
foreach ($requiredMissionCleanupGroupRemovalEntry in @(
		"CleanupCampaignDebugMissionOwnedGroups",
		"mission.cleanup.group_records_removed",
		"mission.cleanup.group_runtime_removed",
		"CleanupRuntimeGroupEntityForDebug(group.m_sGroupId)",
		"MissionValueHasCampaignDebugPrefix(instanceId, CAMPAIGN_DEBUG_PREFIX_ROOT)"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredMissionCleanupGroupRemovalEntry)) {
		throw "Campaign debug mission cleanup must remove debug-owned active groups before asserting no live groups: $requiredMissionCleanupGroupRemovalEntry"
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
if ($civilianCharacterPoolEntries.Count -lt 2) {
	throw "Civilian character runtime must ship the randomized GUID-qualified default CIV character resource in config and runtime defaults"
}
if ($configResourceText -notmatch "Character_CIV_Randomized.et" -or $defaultCatalog -notmatch "Character_CIV_Randomized.et") {
	throw "Civilian character runtime must use the randomized default CIV character resource"
}
if ($configResourceText -match "Character_CIV_CottonShirt|Character_CIV_DenimJacket|Character_CIV_Turtleneck" -or $defaultCatalog -match "Character_CIV_CottonShirt|Character_CIV_DenimJacket|Character_CIV_Turtleneck") {
	throw "Civilian character runtime must not ship explicit clothing-variant CIV character resources"
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
$missionDestroyTargetComponentText = Get-Content -Raw "Scripts/Game/HST/Components/HST_MissionDestroyTargetComponent.c"
foreach ($requiredDestroyTargetEntry in @(
		"DebugApplyRocketScore",
		'"debug:rpg_test_hit"',
		'"DEBUG: Apply demolition hit"',
		"m_bDebugExplosiveWitnesses",
		"witness candidate",
		"witness scan summary",
		"m_iLastWitnessQueryCount",
		"m_iLastWitnessPotentialCount",
		"damage callback",
		"no SCR_DamageManagerComponent on demolition target/proxy",
		"damage callback bridge still needs Workbench hit-zone invoker wiring",
		"loweredResult.Contains(""demolished"")",
		"loweredResult.Contains(""already destroyed"")",
		"ShouldDebugExplosiveWitnessText",
		"text.Contains(""pg7"")",
		"text.Contains(""maaws"")",
		"text.Contains(""m136"")",
		"text.Contains(""5.56"")",
		"text.Contains(""7.62"")",
		"text.Contains(""buckshot"")",
		"looksLikeProjectileOrAmmo",
		"IsProjectileOrAmmoWitnessText",
		"IsPotentialExplosiveWitnessText",
		"IsGenericWarheadWitnessText",
		"ResolveExplosiveWitnessSourceCooldownSeconds",
		"BuildRoundedWitnessPositionKey",
		"witness:generic-warhead:",
		"HST_MissionDestroyTargetProxyComponent",
		"RelayDamage",
		"demolition debug rocket score applied"
	)) {
	if ($missionDestroyTargetComponentText -notmatch [regex]::Escape($requiredDestroyTargetEntry)) {
		throw "Destroy radio tower demolition debug/classifier contract is missing: $requiredDestroyTargetEntry"
	}
}
if ((Get-Content -Raw "Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et") -notmatch [regex]::Escape("m_bDebugExplosiveWitnesses 1")) {
	throw "Destroy radio target prefab must enable explosive witness debug logging while demolition detection is being verified"
}
if ($missionDestroyTargetComponentText -match [regex]::Escape("m_fLocalExplosiveDamage >= m_fRequiredExplosiveDamage")) {
	throw "Destroy target completion must trust the server demolition result, not local explosive tally"
}
$weaponWitnessMatch = [regex]::Match($missionDestroyTargetComponentText, "protected bool IsWeaponOrVehicleWitnessText[\s\S]*?\r?\n\t}")
if (!$weaponWitnessMatch.Success -or $weaponWitnessMatch.Value -notmatch "looksLikeProjectileOrAmmo") {
	throw "Destroy target witness filtering must allow rocket projectile/ammo identifiers before launcher/weapon rejection"
}
if ($weaponWitnessMatch.Success -and $weaponWitnessMatch.Value -notmatch "if \(looksLikeProjectileOrAmmo\)\s*\r?\n\s*return false;") {
	throw "Destroy target witness filtering must let projectile/ammo identifiers override vehicle/weapon path terms"
}
$witnessCandidateMatch = [regex]::Match($missionDestroyTargetComponentText, "protected bool AddExplosiveWitnessCandidate[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void TickExplosiveWitnessDebugSummary")
if (!$witnessCandidateMatch.Success -or $witnessCandidateMatch.Value -notmatch "IsPotentialExplosiveWitnessText" -or $witnessCandidateMatch.Value -notmatch "m_iLastWitnessQueryCount\+\+" -or $witnessCandidateMatch.Value -notmatch "m_iLastWitnessPotentialCount\+\+") {
	throw "Destroy target witness scan must spend its candidate budget only on explosive-looking entities and track query health"
}
$projectileWitnessMatch = [regex]::Match($missionDestroyTargetComponentText, "protected bool IsProjectileOrAmmoWitnessText[\s\S]*?\r?\n\t}")
if (!$projectileWitnessMatch.Success -or $projectileWitnessMatch.Value -notmatch "rpg" -or $projectileWitnessMatch.Value -notmatch "pg7") {
	throw "Destroy target projectile/ammo witness detection must include RPG/PG7 identifiers"
}
$rocketWitnessMatch = [regex]::Match($missionDestroyTargetComponentText, "protected bool IsRocketWitnessDamageText[\s\S]*?\r?\n\t}")
if (!$rocketWitnessMatch.Success -or $rocketWitnessMatch.Value -match [regex]::Escape('text.Contains("warhead")')) {
	throw "Destroy target witness scoring must not treat generic Warhead_Base evidence as a rocket-strength hit"
}
$genericWarheadCheckIndex = $missionDestroyTargetComponentText.IndexOf("IsGenericWarheadWitnessText(text)")
$rocketWitnessCheckIndex = $missionDestroyTargetComponentText.IndexOf("IsRocketWitnessDamageText(text)")
if ($genericWarheadCheckIndex -lt 0 -or $rocketWitnessCheckIndex -lt 0 -or $genericWarheadCheckIndex -gt $rocketWitnessCheckIndex) {
	throw "Destroy target witness scoring must reject generic warhead evidence before rocket witness scoring"
}
if ($missionDestroyTargetComponentText -notmatch [regex]::Escape("if (IsGenericWarheadWitnessText(text))`r`n`t`t`treturn m_fSmallExplosiveDamageScore") -and $missionDestroyTargetComponentText -notmatch [regex]::Escape("if (IsGenericWarheadWitnessText(text))`n`t`t`treturn m_fSmallExplosiveDamageScore")) {
	throw "Destroy target witness scoring must count generic Warhead_Base evidence only as small explosive fallback"
}
if ($missionDestroyTargetComponentText -notmatch "sourceKey\.Contains\(""generic-warhead""\)[\s\S]*?12\.0") {
	throw "Destroy target generic warhead fallback must use a longer cooldown to avoid lingering witness spam"
}
if ($missionDestroyTargetComponentText -match 'witness:generic-warhead:[^"\r\n]*@') {
	throw "Destroy target generic warhead source keys must not include rounded position; moving warheads must not bypass cooldown"
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
foreach ($requiredCivilianVehicleCatalogEntry in @(
		"CIVILIAN_VEHICLE_ENTITY_CATALOG",
		"AppendRuntimeCivilianVehicleCatalogPrefabs",
		"SCR_EntityCatalogManagerComponent.GetInstance",
		"GetAllFactionEntityCatalogs",
		"entry.GetPrefab()",
		"configured/internal civilian vehicle fallback pool"
	)) {
	if ($civilianRuntimeServiceText -notmatch [regex]::Escape($requiredCivilianVehicleCatalogEntry)) {
		throw "Civilian vehicle runtime must keep CIV entity-catalog discovery entry: $requiredCivilianVehicleCatalogEntry"
	}
}
if ($civilianPoolDefaultText -match "m_aCivilianVehiclePrefabs.Insert") {
	throw "Default civilian vehicle pool must use the CIV entity catalog instead of named prefab fallbacks"
}
$civilianVehicleConfigBlock = [regex]::Match($balanceConfigText, 'm_aCivilianVehiclePrefabs\s*\{([\s\S]*?)\n\s*\}')
if (-not $civilianVehicleConfigBlock.Success) {
	throw "Balance config must still expose an overrideable civilian vehicle prefab pool block"
}
if ($civilianVehicleConfigBlock.Groups[1].Value -match "Prefabs/Vehicles/") {
	throw "Default balance config civilian vehicle pool must remain empty so CIV entity catalogs drive vehicle selection"
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

$commandUiForCoverageText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
$coordinatorForCoverageText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$missingCoordinatorCommandMethods = @()
$coordinatorCommandMethods = @([regex]::Matches($commandUiForCoverageText, "coordinator\.([A-Za-z_][A-Za-z0-9_]*)\(") |
	ForEach-Object { $_.Groups[1].Value } |
	Sort-Object -Unique)
foreach ($method in $coordinatorCommandMethods) {
	if ($coordinatorForCoverageText -notmatch ("\b" + [regex]::Escape($method) + "\s*\(")) {
		$missingCoordinatorCommandMethods += $method
	}
}
if ($missingCoordinatorCommandMethods.Count -gt 0) {
	throw "Command UI dispatch references missing coordinator method(s):`n$($missingCoordinatorCommandMethods -join "`n")"
}
foreach ($requiredCommandCoverageEntry in @(
		"IsVisibleCommandDispatchHandled",
		"missing dispatch",
		"IsNonMutatingPhaseCommand"
	)) {
	if ($commandUiForCoverageText -notmatch [regex]::Escape($requiredCommandCoverageEntry)) {
		throw "Command coverage/mutation audit is missing entry: $requiredCommandCoverageEntry"
	}
}
if ($commandUiForCoverageText -match 'commandId\.Contains\("admin_phase"\)') {
	throw "Admin phase commands must be classified explicitly, not with a broad admin_phase substring exemption"
}
Write-Host "Command UI dispatch coverage OK: $($coordinatorCommandMethods.Count) coordinator method(s)"

$visibleMenuCommandIds = New-Object 'System.Collections.Generic.HashSet[string]'
foreach ($line in ($commandUiForCoverageText -split "`r?`n")) {
	if ($line -notmatch "AddMenuAction\(") {
		continue
	}

	$quotedStrings = @([regex]::Matches($line, '"([^"]*)"') | ForEach-Object { $_.Groups[1].Value })
	if ($quotedStrings.Count -lt 2) {
		continue
	}

	$commandIdCandidate = $quotedStrings[1]
	if ($commandIdCandidate -match '^[A-Za-z0-9_]+$') {
		[void]$visibleMenuCommandIds.Add($commandIdCandidate)
	}
}
$missingVisibleMenuDispatch = @()
foreach ($visibleMenuCommandId in ($visibleMenuCommandIds | Sort-Object)) {
	if ($commandUiForCoverageText -notmatch ('commandId == "' + [regex]::Escape($visibleMenuCommandId) + '"')) {
		$missingVisibleMenuDispatch += $visibleMenuCommandId
	}
}
if ($missingVisibleMenuDispatch.Count -gt 0) {
	throw "Visible menu command(s) missing ExecuteVisibleCommand dispatch:`n$($missingVisibleMenuDispatch -join "`n")"
}
Write-Host "Visible menu command dispatch OK: $($visibleMenuCommandIds.Count) literal command(s)"

$phase15SourceResolver = [regex]::Match($coordinatorForCoverageText, "(?s)protected string ResolveFirstLoadablePhase15SourceVehiclePrefab\(.*?return """";\r?\n\t\}")
if (!$phase15SourceResolver.Success) {
	throw "Could not locate Phase 15 source vehicle resolver for smoke coverage audit"
}
if (@([regex]::Matches($phase15SourceResolver.Value, "candidates\.Insert")).Count -lt 3) {
	throw "Phase 15 source vehicle smoke resolver must keep real candidate vehicle prefabs"
}
$phase15SourceSmokeMethod = [regex]::Match($coordinatorForCoverageText, "(?s)string RequestAdminPhase15SeedSourceVehicle\(.*?string RequestAdminPhase15Report")
if (!$phase15SourceSmokeMethod.Success) {
	throw "Could not locate Phase 15 source vehicle smoke command for coverage audit"
}
foreach ($requiredPhase15SourceEntry in @(
		"ApplyPhase15SmokeSourceCapability",
		"seeded explicit ammo-source metadata",
		"PHASE15_SMOKE_VEHICLE_PREFAB"
	)) {
	if ($coordinatorForCoverageText -notmatch [regex]::Escape($requiredPhase15SourceEntry)) {
		throw "Phase 15 source vehicle smoke coverage is missing entry: $requiredPhase15SourceEntry"
	}
}
foreach ($requiredPhase15SourceMethodEntry in @(
		"m_sDisplayName = ""Phase 15 Ammo Source Vehicle""",
		"m_sSourceVehicleKind = ""ammo""",
		"m_bAmmoSource = true"
	)) {
	if ($phase15SourceSmokeMethod.Value + "`n" + $coordinatorForCoverageText -notmatch [regex]::Escape($requiredPhase15SourceMethodEntry)) {
		throw "Phase 15 source vehicle smoke metadata is missing entry: $requiredPhase15SourceMethodEntry"
	}
}
Write-Host "Phase 15 source vehicle smoke coverage OK"

$enemyCommanderForAuditText = Get-Content -Raw "Scripts/Game/HST/Services/HST_EnemyCommanderService.c"
$normalPetrosQueue = [regex]::Match($enemyCommanderForAuditText, "(?s)HST_EnemyOrderState QueuePetrosAttack\(.*?\r?\n\t\}\r?\n\r?\n\tint DebugResolveDueOrdersNow")
if (!$normalPetrosQueue.Success) {
	throw "Could not locate normal QueuePetrosAttack method for resource-authority audit"
}
if ($normalPetrosQueue.Value -match "AddResources") {
	throw "Normal QueuePetrosAttack must not force-fund enemy resources; use QueueDebugPetrosAttack for smoke helpers"
}
foreach ($requiredPetrosQueueEntry in @(
		"QueueDebugPetrosAttack",
		"QueuePetrosAttack",
		"m_EnemyCommander.QueueDebugPetrosAttack"
	)) {
	if (($enemyCommanderForAuditText + "`n" + $coordinatorForCoverageText) -notmatch [regex]::Escape($requiredPetrosQueueEntry)) {
		throw "Petros attack queue split is missing entry: $requiredPetrosQueueEntry"
	}
}
Write-Host "Petros attack queue resource authority OK"

$supportRequestForPetrosTargetText = Get-Content -Raw "Scripts/Game/HST/Services/HST_SupportRequestService.c"
foreach ($requiredPetrosTargetPreservationEntry in @(
		"petros_attack_support",
		"IsPetrosAttackSupportGroup",
		"IsSupportRequestActiveGroup",
		"if (IsSupportRequestActiveGroup(activeGroup))",
		"ResolvePetrosAttackTargetPosition(state)",
		"phase22.attack.group_target_base_position"
	)) {
	if (($physicalWarServiceText + "`n" + $supportRequestForPetrosTargetText + "`n" + $enemyCommanderForAuditText + "`n" + $coordinatorForCoverageText) -notmatch [regex]::Escape($requiredPetrosTargetPreservationEntry)) {
		throw "Phase 22 Petros attack support must preserve HQ/Petros target instead of static-zone normalization: $requiredPetrosTargetPreservationEntry"
	}
}
Write-Host "Petros attack support target preservation OK"

foreach ($requiredPhase18BackgroundWarIsolationEntry in @(
		"AbortCampaignDebugBackgroundWarPetrosOrders",
		"background_war.unexpected_petros_orders",
		"background_war.commander_tick.no_petros_attack",
		"background-war debug probe does not own Defend Petros"
	)) {
	if ($coordinatorForCoverageText -notmatch [regex]::Escape($requiredPhase18BackgroundWarIsolationEntry)) {
		throw "Phase 18 background-war Petros isolation is missing entry: $requiredPhase18BackgroundWarIsolationEntry"
	}
}
$phase18BackgroundWarContext = [regex]::Match($coordinatorForCoverageText, "(?s)protected bool IsCampaignDebugExpectedBackgroundWarOrderType\(.*?\r?\n\t\}")
if (!$phase18BackgroundWarContext.Success -or $phase18BackgroundWarContext.Value -match "HST_ENEMY_ORDER_PETROS_ATTACK") {
	throw "Phase 18 background-war context must not accept Petros attack orders; Phase 22 owns Defend Petros"
}
Write-Host "Phase 18 background-war Petros isolation OK"

foreach ($requiredPhase24EscalationPhysicalizationEntry in @(
		"ActivateCampaignDebugEscalationOrderTargetZones",
		"IsCampaignDebugPhysicalizableEscalationOrder",
		"m_iPhysicalizationTargetZonesActivated",
		"phase24.escalation.support_physicalization",
		"physicalization_target_zones_activated",
		"active targets +%3"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPhase24EscalationPhysicalizationEntry)) {
		throw "Phase 24 escalation probe must activate target zones before support physicalization: $requiredPhase24EscalationPhysicalizationEntry"
	}
}
Write-Host "Phase 24 escalation physicalization setup OK"

Write-Host "h-istasi foundation validation passed"
