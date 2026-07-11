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

function Get-ScriptMethodBlock {
	param(
		[string] $Text,
		[string] $SignatureToken
	)

	$start = $Text.IndexOf($SignatureToken)
	if ($start -lt 0) {
		return ""
	}

	$braceStart = $Text.IndexOf("{", $start)
	if ($braceStart -lt 0) {
		return ""
	}

	$depth = 0
	$inString = $false
	$escaped = $false
	for ($i = $braceStart; $i -lt $Text.Length; $i++) {
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
			$depth++
		}
		elseif ($char -eq "}") {
			$depth--
			if ($depth -eq 0) {
				return $Text.Substring($start, $i - $start + 1)
			}
		}
	}

	return ""
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

$civilianRuntimeEmptyGroupPath = "Prefabs/Groups/CIV/HST_CivilianRuntimeEmptyGroup.et"
if (!(Test-Path $civilianRuntimeEmptyGroupPath)) {
	throw "Missing CIV runtime empty AIGroup prefab"
}
if ((Get-Content -Raw "$civilianRuntimeEmptyGroupPath.meta") -notmatch '\{6985327711303920\}Prefabs/Groups/CIV/HST_CivilianRuntimeEmptyGroup\.et') {
	throw "CIV runtime empty AIGroup prefab metadata must expose the GUID-qualified resource name"
}
$civilianRuntimeEmptyGroupText = Get-Content -Raw $civilianRuntimeEmptyGroupPath
foreach ($requiredCivilianRuntimeEmptyGroupEntry in @(
		'SCR_AIGroup HST_CivilianRuntimeEmptyGroup : "{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et"',
		"m_bSpawnImmediately 0",
		"m_bDeleteWhenEmpty 0",
		'm_faction "CIV"'
	)) {
	if ($civilianRuntimeEmptyGroupText -notmatch [regex]::Escape($requiredCivilianRuntimeEmptyGroupEntry)) {
		throw "CIV runtime empty AIGroup prefab must inherit the stock behavior/replication base, remain empty and non-deleting, and be CIV-tagged: $requiredCivilianRuntimeEmptyGroupEntry"
	}
}

if ($civilianRuntimeEmptyGroupText -notmatch 'm_aUnitPrefabSlots\s*\{\s*\}') {
	throw "CIV runtime empty AIGroup prefab must explicitly start with no unit slots"
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
if ($civilianCharacterPoolEntriesEarly.Count -lt 40) {
	throw "Civilian character pools must expose the concrete GUID-qualified stock CIV appearance variants in config and runtime defaults"
}
if ($balanceConfigTextEarly -match "Character_CIV_Randomized.et" -or $defaultCatalogEarly -match "Character_CIV_Randomized.et") {
	throw "Direct civilian runtime pools must use concrete appearance variants because stock editor-randomized entries do not randomize direct runtime spawns"
}
$civilianServiceTextEarly = Get-Content -Raw "Scripts/Game/HST/Services/HST_CivilianService.c"
if ($civilianServiceTextEarly -notmatch "MIN_CIVILIAN_CHARACTER_PREFABS = 1") {
	throw "Civilian character prefab minimum must retain the configured-pool availability guard"
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
		"Z_MAP_TARGET_PROMPT",
		"Z_MAP_TARGET_CURSOR",
		"Z_MAP_ACTION_DIALOG",
		"Z_NATIVE_MAP_CURSOR",
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
		"Widget m_Parent",
		"int m_iZOrder = HST_UIConstants.Z_ACTION_DIALOG",
		"workspace.CreateWidgets(ACTION_DIALOG_LAYOUT, parent)",
		"root.SetZOrder(data.m_iZOrder)",
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

$currentStateDocPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md"
)
foreach ($currentStateDocPath in $currentStateDocPaths) {
	$currentStateDocText = Get-Content -Raw $currentStateDocPath
	if ($currentStateDocText -notmatch "(?i)schema[- ]$campaignSchemaVersion\b") {
		throw "$currentStateDocPath must identify current campaign schema $campaignSchemaVersion"
	}
	if ($currentStateDocText -match '(?im)[A-Za-z]:\\|/home/|/Users/|file://') {
		throw "$currentStateDocPath must not contain local filesystem paths"
	}
}
$commandMenuMapTargetText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CommandMenuComponent.c"
foreach ($requiredMapTargetLayerEntry in @(
		"ResolveCommandMapOverlayParent",
		"m_MapTargetEntity.GetMapMenuRoot()",
		"data.m_iZOrder = HST_UIConstants.Z_MAP_ACTION_DIALOG",
		"HST_UIConstants.Z_MAP_TARGET_PROMPT",
		"HST_UIConstants.Z_MAP_TARGET_CURSOR"
	)) {
	if ($commandMenuMapTargetText -notmatch [regex]::Escape($requiredMapTargetLayerEntry)) {
		throw "Map-target overlays must stay on the map-local layer below the native pointer: $requiredMapTargetLayerEntry"
	}
}
$featureChecklistText = Get-Content -Raw "docs/FEATURE_CHECKLIST.md"
$phasePlanText = Get-Content -Raw "docs/PHASE_PLAN.md"
$parityText = Get-Content -Raw "docs/PARITY.md"
$currentDocumentationText = $featureChecklistText + "`n" + $phasePlanText + "`n" + $parityText + "`n" + (Get-Content -Raw "README.md")
foreach ($requiredCurrentDocumentationEntry in @(
	'## Current Delivery Priorities',
	'## Highest-Impact Next Tasks',
	'### Blueprint Milestone Snapshot',
	'## Current Verification Boundary'
)) {
	if ($currentDocumentationText -notmatch [regex]::Escape($requiredCurrentDocumentationEntry)) {
		throw "Current documentation is missing authority/status anchor: $requiredCurrentDocumentationEntry"
	}
}
foreach ($implementedBroadAlphaPhase in 15..22) {
	if ($phasePlanText -match "(?s)## Phase $implementedBroadAlphaPhase\b.{0,500}?Status:\s*Planned") {
		throw "Phase $implementedBroadAlphaPhase is implemented broad-alpha work and must not be documented as Planned"
	}
}
foreach ($staleCurrentClaim in @(
	'Paid support is still on its previous production path',
	'Support pricing still depends on planned rather than committed force counts'
)) {
	if ($featureChecklistText -match [regex]::Escape($staleCurrentClaim) -or $phasePlanText -match [regex]::Escape($staleCurrentClaim)) {
		throw "Current documentation retains superseded claim: $staleCurrentClaim"
	}
}
Write-Host "Current documentation schema, authority, and roadmap truth OK"

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
		throw "Mission definitions are missing expanded campaign metadata contract: $requiredMissionExpansionEntry"
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
		throw "Mission runtime is missing full campaign state-machine entry: $requiredMissionStateMachineEntry"
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
Write-Host "Full campaign mission expansion contracts OK"

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

$simonsWoodBlocks = @($zoneBlocks | Where-Object { $_ -match 'm_sZoneId\s+"town_simons_wood"' })
if ($simonsWoodBlocks.Count -ne 1) {
	throw "Everon taxonomy must contain exactly one stable Simon's Wood zone"
}
foreach ($requiredSimonsWoodEntry in @(
		'm_eType HST_ZONE_RESOURCE',
		'm_sResourceKind "food"',
		'm_iGarrisonSlots 6',
		'm_sSpawnProfileId "spawn_resource_guards"',
		'm_sMarkerStyle "resource"'
	)) {
	if ($simonsWoodBlocks[0] -notmatch [regex]::Escape($requiredSimonsWoodEntry)) {
		throw "Simon's Wood must remain a minor farm/resource locality: $requiredSimonsWoodEntry"
	}
}
foreach ($trueTownId in @("town_figari", "town_morton")) {
	$trueTownBlocks = @($zoneBlocks | Where-Object { $_ -match ('m_sZoneId\s+"' + [regex]::Escape($trueTownId) + '"') })
	if ($trueTownBlocks.Count -ne 1 -or $trueTownBlocks[0] -notmatch 'm_eType HST_ZONE_TOWN' -or $trueTownBlocks[0] -notmatch 'm_sSourceLayoutId "TC_') {
		throw "Everon taxonomy must retain $trueTownId as one stock town-center locality"
	}
}
if ($defaultCatalog -notmatch 'NewZoneState\("town_morton"' -or $defaultCatalog -notmatch 'NewZoneState\("town_simons_wood"[^\r\n]+HST_EZoneType\.HST_ZONE_RESOURCE') {
	throw "Runtime and config location registries must agree on Morton and Simon's Wood taxonomy"
}
Write-Host "Everon true-town/minor-locality taxonomy OK"

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
		"support_virtual",
		"support_arrived_virtual",
		"exact_virtual_outbound",
		"exact_virtual_on_station",
		"virtual en route",
		"virtual on station",
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
		'return "OBJECTIVE_MARKER2";',
		'if (zone.m_sMarkerStyle == "enemy_base" || zone.m_sMarkerStyle == "stronghold")',
		'return "FORTIFICATION";',
		'return "RADIO_SIGNAL";',
		'if (zone.m_sMarkerStyle == "mission_site")',
		'return "POINT_SPECIAL";',
		'return "JOIN3";',
		'return "DOT";',
		'return "MARK_QUESTION";',
		'return "MARK_EXCLAMATION";',
		'return "DESTROY2";',
		'return "HELP";',
		'return "OBJECTIVE_MARKER";',
		'return "TARGET_REFERENCE_POINT";',
		'support.search_marker_icon',
		'phase23.marker.radio_icon',
		'phase23.marker.radar_icon',
		'CountCampaignDebugZoneIconMismatchesForMarkerStyle',
		'CountCampaignDebugRadarZoneIconMismatches',
		'phase23.marker.location_qrf_icon_deconflict',
		'CountCampaignDebugStaticLocationQRFIconCollisions'
	)) {
	if ($runtimeMarkerPipelineText -notmatch [regex]::Escape($requiredMarkerIconDeconflictEntry)) {
		throw "Static location marker/QRF icon deconflict contract is missing entry: $requiredMarkerIconDeconflictEntry"
	}
}

if ($configZones.Count -ne 81 -or $runtimeZones.Count -ne 81) {
	throw "Everon campaign catalog must contain 81 zones in config/runtime, found config=$($configZones.Count) runtime=$($runtimeZones.Count)"
}

$campaignBaseBlocks = @($zoneBlocks | Where-Object { $_ -match 'm_sSourceLayerName "Bases\.layer"' })
$campaignDepotBlocks = @($zoneBlocks | Where-Object { $_ -match 'm_sSourceLayerName "SupplyDepots\.layer"' })
$campaignCallsigns = @($zoneBlocks | ForEach-Object {
		if ($_ -match 'm_sMarkerCallsign "([^"]+)"') {
			$Matches[1]
		}
	} | Where-Object { ![string]::IsNullOrWhiteSpace($_) })
if ($campaignBaseBlocks.Count -ne 68) {
	throw "Expected 68 Everon Bases.layer nodes, found $($campaignBaseBlocks.Count)"
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
		"HST_CANONICAL_MARKER_CONFIG",
		"ResolveValidPlacedEntry",
		"entryConfigs.Insert(playerEntry)",
		"HST_EnsureRadioSignalIconEntry",
		"HST_EnsurePlacedIconEntry",
		"HST_ResolveValidPlacedIconEntry",
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
		"AMBUSH",
		"TARGET_REFERENCE_POINT",
		"FLAG2",
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

$editorRoleGuardPath = "Scripts/Game/HST/Patches/HST_EditorRoleChangeReentryGuard.c"
if (!(Test-Path $editorRoleGuardPath)) {
	throw "Missing editor player-role reentry guard: $editorRoleGuardPath"
}
$editorRoleGuardText = Get-Content -Raw $editorRoleGuardPath
foreach ($requiredEditorRoleGuardEntry in @(
		"[BaseContainerProps(configRoot: true)]",
		"modded class SCR_EditorManagerCore",
		"override protected void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)",
		"m_bHSTApplyDeferredPlayerRoleChange",
		"GetGame().GetCallqueue().Call(HST_ApplyDeferredPlayerRoleChange, playerId, roleFlags)",
		"super.OnPlayerRoleChange(playerId, roleFlags)"
	)) {
	if ($editorRoleGuardText -notmatch [regex]::Escape($requiredEditorRoleGuardEntry)) {
		throw "Editor player-role reentry guard is missing: $requiredEditorRoleGuardEntry"
	}
}
foreach ($requiredRadioMarkerResourceEntry in @(
		'RADIO_SIGNAL_IMAGESET = "{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset"',
		'RADIO_SIGNAL_GLOW_IMAGESET = "{00FE3DBDFD15227B}UI/Textures/Icons/icons_wrapperUI-glow.imageset"',
		'RADIO_SIGNAL_QUAD = "radio-signal"',
		"record.m_sIconImageset = RADIO_SIGNAL_IMAGESET",
		"manager.HST_EnsurePlacedIconEntry",
		"manager.HST_ResolveValidPlacedIconEntry"
	)) {
	if (($campaignMarkerDirectorText + "`n" + $nativeMarkerReconcilerText) -notmatch [regex]::Escape($requiredRadioMarkerResourceEntry)) {
		throw "Radio map markers must resolve a validated runtime icon resource rather than a hard-coded placed-icon index: $requiredRadioMarkerResourceEntry"
	}
}
if (($campaignMarkerDirectorText + "`n" + $markerServiceText) -match "RADIO_SIGNAL_NATIVE_ICON_INDEX") {
	throw "Radio map markers must not depend on a hard-coded placed-icon array index"
}
if ($editorRoleGuardText -match "GivePlayerRole" -or $editorRoleGuardText -match "ClearPlayerRole") {
	throw "Editor player-role reentry guard must defer stock role synchronization rather than own player roles"
}
if ($editorRoleGuardText -match "modded class SCR_EditorManagerEntity" -or $editorRoleGuardText -match "override protected void UpdateLimited") {
	throw "Editor player-role reentry guard must not defer every mode update or editor-manager teardown"
}
Write-Host "Editor player-role reentry guard OK"

$gameMasterBudgetPatchText = Get-Content -Raw "Scripts/Game/HST/Services/HST_GameMasterBudgetService.c"
$mapMarkerConfigPatchText = Get-Content -Raw "Scripts/Game/HST/Map/HST_MapMarkerManagerConfigPatch.c"
foreach ($configBackedModdedClass in @(
		@{
			Text = $editorRoleGuardText
			Declaration = '[BaseContainerProps(configRoot: true)]' + "`n" + 'modded class SCR_EditorManagerCore'
		},
		@{
			Text = $gameMasterBudgetPatchText
			Declaration = '[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(EEditableEntityBudget, "m_BudgetType")]' + "`n" + 'modded class SCR_EditableEntityCoreBudgetSetting'
		},
		@{
			Text = $mapMarkerConfigPatchText
			Declaration = '[BaseContainerProps(), SCR_MapMarkerIconEntryTitle()]' + "`n" + 'modded class SCR_MarkerIconEntry'
		},
		@{
			Text = $mapMarkerConfigPatchText
			Declaration = '[BaseContainerProps(), SCR_MapMarkerTitle()]' + "`n" + 'modded class SCR_MapMarkerEntryPlaced'
		}
	)) {
	$normalizedConfigPatchText = $configBackedModdedClass.Text -replace "`r`n", "`n"
	if ($normalizedConfigPatchText -notmatch [regex]::Escape($configBackedModdedClass.Declaration)) {
		throw "Config-backed modded class lost its stock BaseContainer metadata: $($configBackedModdedClass.Declaration)"
	}
}
Write-Host "Config-backed modded class metadata preservation OK"

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
		"SCR_EScenarioFrameworkMarkerCustom.FLAG2",
		"SCR_EScenarioFrameworkMarkerCustom.TARGET_REFERENCE_POINT",
		"SCR_EScenarioFrameworkMarkerCustom.AMBUSH",
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
$codeOnly = [regex]::Replace($scriptText, '"(?:\\.|[^"\\])*"', "")
$definedSymbols = @($definedSymbols + @([regex]::Matches($codeOnly, "(?m)^\s*(?:static\s+const\s+)?(?:void|bool|int|float|string|ResourceName|vector|typename)\s+(HST_[A-Za-z0-9_]+)\s*(?:\(|=|;)") |
		ForEach-Object { $_.Groups[1].Value }) |
	Sort-Object -Unique)
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
		throw "Missing campaign framework service: $requiredService"
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
		"support.physical_eta_no_false_arrival",
		"support.recall.campaign_clock_no_false_exit",
		"CampaignDebugStatus(recallStillPhysicallyRouting)",
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
if ($scriptText -match [regex]::Escape('group.m_iSpawnedAtSecond = m_State.m_iElapsedSeconds - 10000;')) {
	throw "Campaign debug must not backdate a spawned support group to synthesize recall exit"
}
Write-Host "Physical support runtime debug proof OK"

foreach ($requiredCampaignDebugFalseNegativeRepair in @(
	'CampaignDebugReportBool(observedSupportRequest.m_sDeploymentSummary, "playerClear")',
	'CampaignDebugReportBool(observedSupportRequest.m_sDeploymentSummary, "activeGroupClear")',
	'm_sMarkerIconAfterRequest',
	'markerVisible && markerIcon == "OBJECTIVE_MARKER"',
	'resource %3/12+',
	'resourceCount >= 12',
	'CampaignDebugStatusSeverity',
	'resolvedReason = assertion.m_sFailureReason',
	'CaptureCampaignDebugSupportRuntimeSnapshot',
	'm_bRuntimeGroupSnapshotCapturedBeforeFold',
	'm_bRuntimeMemberCountsVisibleBeforeFold',
	'm_bResponseRunBeforeFold',
	'GetFormationDisplacement()',
	'formationTight'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCampaignDebugFalseNegativeRepair)) {
		throw "Missing Full Campaign Debug false-negative repair: $requiredCampaignDebugFalseNegativeRepair"
	}
}
if ($coordinatorMarkerText -match [regex]::Escape('Contains("playerClear true")') -or $coordinatorMarkerText -match [regex]::Escape('Contains("activeGroupClear true")')) {
	throw "Physical support clearance proof must normalize Enfusion bool text"
}
if ($coordinatorMarkerText -match 'resourceCount\s*>=\s*13') {
	throw "Curated resource-site proof must use the intentional 12-site registry minimum"
}
$incomeProbeBlock = [regex]::Match($coordinatorMarkerText, 'protected HST_CampaignDebugIncomeProbeContext BuildCampaignDebugIncomeProbeContext\(\)[\s\S]*?\r?\n\t}')
if (-not $incomeProbeBlock.Success) {
	throw "Full Campaign Debug income probe boundary is missing"
}
$incomeReportIndex = $incomeProbeBlock.Value.IndexOf('m_sEconomyReport = RequestMemberInspectEconomy')
$incomeTickIndex = $incomeProbeBlock.Value.IndexOf('m_sCommandResult = RequestCommanderApplyIncomeNowReport')
if ($incomeReportIndex -lt 0 -or $incomeTickIndex -lt 0 -or $incomeReportIndex -gt $incomeTickIndex) {
	throw "Full Campaign Debug economy report and expected delta must sample the same pre-tick state"
}
$supportProbeBlock = [regex]::Match($coordinatorMarkerText, 'protected bool ProbeCampaignDebugSupportRequestRuntime\([^\r\n]+\)[\s\S]*?\r?\n\t}\r?\n\r?\n\tprotected void CaptureCampaignDebugSupportRuntimeSnapshot')
if (-not $supportProbeBlock.Success) {
	throw "Full Campaign Debug support runtime probe boundary is missing"
}
$supportSnapshotIndex = $supportProbeBlock.Value.LastIndexOf('CaptureCampaignDebugSupportRuntimeSnapshot(probeContext, group)')
$supportFoldIndex = $supportProbeBlock.Value.IndexOf('group.m_sRuntimeStatus = "folded"')
if ($supportSnapshotIndex -lt 0 -or $supportFoldIndex -lt 0 -or $supportSnapshotIndex -gt $supportFoldIndex) {
	throw "Full Campaign Debug support movement and formation evidence must be captured before group fold"
}
Write-Host "Full campaign debug false-negative guards OK"

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
Write-Host "Campaign framework service spine OK"

foreach ($requiredStrategicEventZoneCaptureEntry in @(
		"BeginZoneCaptureEvent",
		"CompleteStrategicEvent",
		"DiscardStrategicEvent",
		'"zone_captured"',
		"eventSourceId",
		"_phase17_zone_capture",
		"phase17.capture.strategic_event",
		"phase17.capture.strategic_event_save_roundtrip"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredStrategicEventZoneCaptureEntry)) {
		throw "Strategic event zone-capture proof is missing: $requiredStrategicEventZoneCaptureEntry"
	}
}
Write-Host "Strategic event zone-capture proof OK"

foreach ($requiredStrategicEventMissionExpiryEntry in @(
		"ApplyMissionExpiryEvent",
		"GetLastExpiredMissionIds",
		"ApplyPendingMissionExpiryEvents",
		"ApplyMissionExpiryEventForMission",
		"ApplyMissionExpiryConsequences",
		'"mission_expired"',
		"mission_expiry.penalty.contract.runtime",
		"mission_expiry.strategic_event",
		"mission_expiry.save_roundtrip",
		"defense expiry resolves as success"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredStrategicEventMissionExpiryEntry)) {
		throw "Strategic event mission-expiry proof is missing: $requiredStrategicEventMissionExpiryEntry"
	}
}
Write-Host "Strategic event mission-expiry proof OK"

foreach ($requiredStrategicEventConvoyOutcomeEntry in @(
		"BeginConvoyOutcomeEvent",
		"BeginConvoyStrategicEvent",
		"CompleteConvoyStrategicEvent",
		"ResolveConvoyOutcomeTargetZoneId",
		'"convoy_cargo_delivered"',
		'"convoy_vehicle_captured"',
		'"convoy_arrived"',
		'"convoy_crew_eliminated"',
		'"convoy_expired"',
		"BuildCampaignDebugConvoyOutcomeStrategicEventCase",
		"convoy_outcome.strategic_event.contract.runtime",
		"convoy_outcome.strategic_event",
		"convoy_outcome.save_roundtrip"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredStrategicEventConvoyOutcomeEntry)) {
		throw "Strategic event convoy-outcome proof is missing: $requiredStrategicEventConvoyOutcomeEntry"
	}
}
Write-Host "Strategic event convoy-outcome proof OK"

foreach ($requiredStrategicEventSupportNearHQEntry in @(
		"BeginSupportNearHQEvent",
		"ApplySupportNearHQStrategicEvent",
		"IsSupportRequestNearHQ",
		'"support_near_hq"',
		"SUPPORT_NEAR_HQ_KNOWLEDGE_GAIN",
		"BuildCampaignDebugSupportNearHQStrategicEventCase",
		"support_near_hq.strategic_event.contract.runtime",
		"support_near_hq.strategic_event",
		"support_near_hq.save_roundtrip"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredStrategicEventSupportNearHQEntry)) {
		throw "Strategic event support-near-HQ proof is missing: $requiredStrategicEventSupportNearHQEntry"
	}
}
Write-Host "Strategic event support-near-HQ proof OK"

foreach ($requiredStrategicEventVehicleReportEntry in @(
		"BeginVehicleReportEvent",
		"SetStrategicService",
		"RegisterVehicleHeat",
		'"vehicle_reported"',
		"m_iVehicleHeatDelta",
		"m_bVehicleReportedBefore",
		"m_bVehicleReportedAfter",
		"m_iVehicleReportedUntilDelta",
		"vehicle_heat.strategic_event",
		"vehicle_heat.passenger_strategic_event",
		"vehicle_heat.strategic_report_surface"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredStrategicEventVehicleReportEntry)) {
		throw "Strategic event vehicle-report proof is missing: $requiredStrategicEventVehicleReportEntry"
	}
}
Write-Host "Strategic event vehicle-report proof OK"

foreach ($requiredEconomyIncomeSourceEntry in @(
		"BuildIncomeSourceBreakdown",
		"income sources | towns money",
		"resources money",
		"factories money",
		"seaports money",
		"airfields money",
		"banks money",
		"m_sEconomyReport",
		"RequestMemberInspectEconomy",
		"economy.income.report_breakdown",
		"ApplyTownPopulationIncomeMultiplier",
		"ResolveTownPopulationIncomePercent",
		"DebugResolveTownPopulationIncomePercent",
		"economy.income.population_scaling.contract.runtime",
		"economy.income.population.money_scaling",
		"economy.income.population.hr_gate",
		"economy.income.population.report",
		"town population %4 pct",
		"population %1 pct",
		"expectedReportMoney",
		"expectedReportHR"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredEconomyIncomeSourceEntry)) {
		throw "Economy income source-report proof is missing: $requiredEconomyIncomeSourceEntry"
	}
}
Write-Host "Economy income source-report proof OK"

foreach ($requiredTrainingWarCapEntry in @(
		"ResolveTrainingCap",
		"m_iTrainingCap",
		"training capped by war level",
		"training %1/%2",
		"recruitment.training.war_level_cap.contract.runtime",
		"training.war_cap.scaling",
		"training.war_cap.low_block",
		"training.war_cap.high_advance",
		"training.war_cap.report",
		"phase16.training.war_level_cap",
		"RecordCampaignDebugCase(BuildCampaignDebugTrainingWarLevelCapCase())"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredTrainingWarCapEntry)) {
		throw "Training war-level cap proof is missing: $requiredTrainingWarCapEntry"
	}
}
Write-Host "Training war-level cap proof OK"

foreach ($requiredTrainingQualityEntry in @(
		"ResolveTrainingQualityBonusPercentForLevel",
		"ResolveTrainingEffectiveInfantryStrengthForLevel",
		"m_iEffectiveManpower",
		"m_iFriendlyInfantryStrengthNearby",
		"ApplyTrainingQualitySummaryToActiveGroup",
		"BuildTrainingQualityLabel",
		"quality +%3 pct",
		"effective manpower",
		"training quality +%1 pct",
		"recruitment.training_quality.contract.runtime",
		"training.quality.scaling",
		"training.quality.capture_strength",
		"training.quality.force_composition",
		"training.quality.report",
		"RecordCampaignDebugCase(BuildCampaignDebugTrainingQualityCase())"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredTrainingQualityEntry)) {
		throw "Training quality proof is missing: $requiredTrainingQualityEntry"
	}
}
Write-Host "Training quality proof OK"

foreach ($requiredUndercoverSecurityScanEntry in @(
		"UNDERCOVER_ROADBLOCK_SCAN_COOLDOWN_SECONDS",
		"UNDERCOVER_POLICE_SCAN_COOLDOWN_SECONDS",
		"CalculateRoadblockScanChance",
		"CalculatePoliceScanChance",
		"ResolveUndercoverSecurityWarLevel",
		"ResolveUndercoverSecurityAggression",
		"BuildUndercoverSecurityScanRoll",
		"BuildUndercoverSecurityScanReason",
		"DebugCalculateRoadblockScanChance",
		"DebugCalculatePoliceScanChance",
		"undercover_security_scan_scaling.contract.runtime",
		"undercover_security_scan.roadblock_scaling",
		"undercover_security_scan.police_scaling",
		"undercover_security_scan.blocked_vehicle_pressure",
		'undercover.m_sLastReason.Contains("chance")',
		'undercover.m_sLastReason.Contains("war")',
		'undercover.m_sLastReason.Contains("aggression")'
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredUndercoverSecurityScanEntry)) {
		throw "Undercover security scan scaling contract missing: $requiredUndercoverSecurityScanEntry"
	}
}
Write-Host "Undercover security scan scaling proof OK"

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
		"BuildRuntimeSummary()",
		'lines.Insert("build " + HST_BuildInfo.BuildRuntimeSummary())'
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
		"lootSkipUnlockedItems",
		"removeLootedItems",
		"vehicleLootEnabled",
		"vehicleLootRadiusMeters",
		"vehicleLootSkipUnlockedItems",
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
		"civilianDrivingVehicleCountPerTown",
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
if ($scriptText -notmatch "SCHEMA_VERSION = 22") {
	throw "Runtime settings schema must be 22 for the true-town civilian traffic default migration"
}
if ($scriptText -notmatch 'BUILD_SHA\s*=\s*"[0-9a-f]{40}"') {
	throw "HST_BuildInfo.BUILD_SHA must be a full lowercase Git revision"
}
if ($scriptText -match "RUNTIME_AUTHORITY_BUILD" -or $scriptText -match "COMMAND_MENU_BUILD") {
	throw "Runtime provenance must use HST_BuildInfo instead of component-local build constants"
}
foreach ($requiredAuthorityFoundationEntry in @(
		"HST_StableIdService",
		"m_iNextAuthoritySequence",
		"HST_CampaignCommandEnvelope",
		"HST_CampaignCommandResult",
		"HST_COMMAND_ALREADY_APPLIED",
		"HST_CommandReceiptState",
		"m_aCommandReceipts",
		"HST_ResourceTransactionState",
		"m_aResourceTransactions",
		"HST_CampaignEventState",
		"m_aCampaignEvents",
		"CopyCommandReceipt",
		"CopyResourceTransaction",
		"CopyCampaignEvent",
		"ReserveCost",
		"CommitReserved",
		"ReconcileOpenReservations",
		"RpcAsk_RequestAction(string selectedTabId, string commandId, string argument, string requestId, int clientPlayerId)",
		"ExecuteVisibleCommandDetailed(this, playerId, commandId, argument, requestId",
		"CompleteExplicit(m_State, envelope, explicitCommandStatus",
		"TrainTroopsDetailed(m_State, m_Economy, moneyCost, m_ResourceLedger, requestId",
		"BuildCampaignDebugAuthorityFoundationCase",
		"authority.command.duplicate",
		"authority.command.explicit_status",
		"authority.ledger.single_charge",
		"authority.persistence.roundtrip"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredAuthorityFoundationEntry)) {
		throw "Campaign authority foundation contract missing: $requiredAuthorityFoundationEntry"
	}
}
Write-Host "Campaign authority foundation contract OK"
foreach ($requiredForceAuthorityEntry in @(
		"SCHEMA_VERSION = 58",
		"HST_ForceManifestState",
		"HST_ForceQuoteState",
		"HST_ForceSpawnResultState",
		"m_aForceManifests",
		"m_aForceQuotes",
		"m_aForceSpawnResults",
		"m_aAcceptedManifestIds",
		"CopyForceManifest",
		"CopyForceQuote",
		"CopyForceSpawnResult",
		"force_catalog_1",
		"TryReadGroupSlots",
		"ValidateMemberCatalog",
		'member slot %1 resource is invalid',
		"IssueGarrisonQuote",
		"ConfirmGarrisonQuote",
		"MAX_OPEN_GARRISON_QUOTES",
		"ReconcileInterruptedGarrisonConfirmations",
		"m_bStateChanged",
		"AddManifestForcesExact",
		"exact garrison recruitment",
		"confirm_garrison_quote",
		"cancel_garrison_quote",
		"RequestCommanderQuoteGarrisonAtMapTargetReport",
		"RequestCommanderConfirmGarrisonQuoteReport",
		"force_authority.quantities",
		"force_authority.duplicate_confirmation",
		"force_authority.capacity_all_or_nothing",
		"force_authority.stale_context",
		"force_authority.reservation_rollback",
		"force_authority.restore_reconciliation",
		"force_authority.persistence",
		"force_authority.catalog"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredForceAuthorityEntry)) {
		throw "Exact force authority contract missing: $requiredForceAuthorityEntry"
	}
}
if ($scriptText -notmatch 'array<int> quantities\s*=\s*\{1, 4, 7, 12\}') {
	throw "Exact force authority proof must cover quantities 1, 4, 7, and 12"
}
if ($scriptText -match '(?m)^\s*(?:bool|string)\s+RequestCommanderRecruitGarrison(?:AtMapTarget)?(?:Report)?\s*\(') {
	throw "Legacy caller-priced garrison recruitment wrappers must not remain public authority surfaces"
}
Write-Host "Exact force manifest and garrison quote contract OK"
foreach ($requiredPaidQRFExactEntry in @(
		'QUOTE_KIND_PLAYER_SUPPORT_QRF = "player_support_qrf"',
		'SUPPORT_QRF_POLICY_ID = "support_qrf_exact_infantry_1"',
		'SUPPORT_QRF_MONEY_COST = 250',
		'SUPPORT_QRF_ETA_SECONDS = 120',
		'SUPPORT_QRF_COOLDOWN_SECONDS = 600',
		'SUPPORT_QRF_CAPABILITY_ID = "qrf"',
		'SUPPORT_QRF_ASSET_PROFILE_ID = "fia_qrf_reserve_alpha"',
		'IssuePlayerSupportQuote',
		'ConfirmPlayerSupportQuote',
		'CancelPlayerSupportQuote',
		'ReconcileInterruptedPlayerSupportConfirmations',
		'FindIssuedPlayerSupportQuote',
		'ValidateFrozenPlayerSupportQuote',
		'RegisterAcceptedExactPlayerSupport',
		'EnqueueAcceptedExactPlayerSupportProjection',
		'TickExactPlayerSupportSettlements',
		'ReconcileSuccessfulExactPlayerSupportRuntimeAfterRestore',
		'IsFullyRefundedExactPlayerSupportRequest',
		'exact player QRF requires an accepted server quote',
		'RequestCampaignDebugLegacyPlayerSupportDetailed',
		'confirm_support_quote',
		'cancel_support_quote',
		'Request exact QRF quote at map location',
		'force_authority.paid_qrf_issue_confirm',
		'force_authority.paid_qrf_queue_replay',
		'force_authority.paid_qrf_failure_refund',
		'force_authority.paid_qrf_cancel_refund',
		'force_authority.paid_qrf_recall_refund',
		'force_authority.paid_qrf_terminal_refund',
		'force_authority.paid_qrf_legacy_migration',
		'migration_schema46_player_qrf_ledger_imported'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPaidQRFExactEntry)) {
		throw "Exact paid-QRF authority contract missing: $requiredPaidQRFExactEntry"
	}
}
$paidQRFEnqueueBlock = [regex]::Match($scriptText, 'HST_ForceSpawnQueueEnqueueResult EnqueueAcceptedExactPlayerSupportProjection[\s\S]*?\r?\n\t}')
if (!$paidQRFEnqueueBlock.Success -or $paidQRFEnqueueBlock.Value -match '\.Compose\(' -or $paidQRFEnqueueBlock.Value -match 'EstimatePlayerSupportHRCost') {
	throw "Exact paid-QRF queue admission must consume the frozen manifest without composition or prefab-name manpower estimation"
}
$paidQRFRouteInitializeIndex = $paidQRFEnqueueBlock.Value.IndexOf('InitializeExactPlayerQRFRoute(')
$paidQRFOutboundCommitIndex = $paidQRFEnqueueBlock.Value.IndexOf('LinkOutboundVirtual(')
if ($paidQRFRouteInitializeIndex -lt 0 -or $paidQRFOutboundCommitIndex -lt 0 -or $paidQRFRouteInitializeIndex -gt $paidQRFOutboundCommitIndex) {
	throw "Exact paid-QRF admission must initialize its strategic route while staging before LinkOutboundVirtual commits OUTBOUND service"
}
foreach ($requiredPaidQRFCommitSettlementProofEntry in @(
	'route ready in staging',
	'outbound commit',
	'staging full refund',
	'outbound survivor settlement',
	'on-station survivor settlement',
	'materialization_failed_virtual_survivors_refunded',
	'outboundMoney.m_iRefundedAmount == 0',
	'outboundHR.m_iRefundedAmount == outboundSurvivors',
	'preCommitMoney.m_iRefundedAmount == preCommit.m_Request.m_iMoneyCost',
	'preCommitHR.m_iRefundedAmount == preCommit.m_Request.m_iHRCost',
	'bool outboundReplayChanged = fixture.m_Support.TickExactPlayerSupportSettlements',
	'bool replayChanged = onStation.m_Support.TickExactPlayerSupportSettlements'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredPaidQRFCommitSettlementProofEntry)) {
		throw "Exact paid-QRF commit-boundary settlement proof is missing: $requiredPaidQRFCommitSettlementProofEntry"
	}
}
$paidQRFIssueBlock = [regex]::Match($scriptText, 'HST_ForceQuoteResult IssuePlayerSupportQuote[\s\S]*?\r?\n\t}')
if (!$paidQRFIssueBlock.Success -or $paidQRFIssueBlock.Value -match 'SpendFactionMoney' -or $paidQRFIssueBlock.Value -match 'SpendHR') {
	throw "Exact paid-QRF issue must freeze a quote without directly debiting resources"
}
if ($scriptText -notmatch 'm_SupportRequests\.Tick\(m_State, m_Preset, m_Garrisons, m_PhysicalWar, m_Strategic, m_HQ, m_Economy, m_ForceSpawnAdapter\)') {
	throw "Production support tick must receive the exact spawn adapter for retirement settlement"
}
Write-Host "Exact paid-QRF quote, ledger, manifest, queue, and settlement contract OK"
foreach ($requiredForceRuntimeEntry in @(
		'HST_FORCE_SLOT_RETIRED',
		'm_bEverAlive',
		'm_bCasualtyConfirmed',
		'm_iSuccessfulHandoffCount',
		'm_iReprojectionCount',
		'm_iDurableLivingInfantryCount',
		'm_bEverPopulated',
		'm_bSpawnCompleted',
		'ConfirmRegisteredMemberCasualty',
		'RequeueSuccessfulProjectionAfterRestore',
		'CountDurableLivingMemberSlots',
		'exact_terminal_settlement_queue_unavailable',
		'exact_recall_settlement_queue_unavailable',
		'DetachConfirmedDeadForceSpawnMember',
		'FinalizeEliminatedForceSpawnProjection',
		'SettleEliminatedExactPlayerSupport',
		'HST_ForceRuntimeAuthorityProofService',
		'force_runtime.casualty_idempotency',
		'force_runtime.persistence',
		'force_runtime.survivor_reprojection',
		'force_runtime.terminal_roster',
		'force_runtime.schema46_migration',
		'migration_schema47_force_runtime_lifecycle'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredForceRuntimeEntry)) {
		throw "Exact force-runtime lifecycle contract missing: $requiredForceRuntimeEntry"
	}
}
if ($scriptText -match 'successful exact QRF projection could not be restored; temporary fail-closed refund') {
	throw "Successful exact paid QRF restore must reproject durable survivors instead of applying the temporary full-refund fallback"
}
$forceRuntimeSettlementBlock = [regex]::Match($scriptText, 'protected bool SettleExactPlayerSupportDeploymentTerminal[\s\S]*?\r?\n\t}')
$forceRuntimeSettlementValid = $forceRuntimeSettlementBlock.Success
$forceRuntimeSettlementValid = $forceRuntimeSettlementValid -and $forceRuntimeSettlementBlock.Value -match 'm_iSuccessfulHandoffCount <= 0\)\s*\{'
$forceRuntimeSettlementValid = $forceRuntimeSettlementValid -and $forceRuntimeSettlementBlock.Value -match 'FindOperation\(request\.m_sOperationId\)'
$forceRuntimeSettlementValid = $forceRuntimeSettlementValid -and $forceRuntimeSettlementBlock.Value -match '!HasExactSupportStrategicServiceCommitted\(operation\)\)\s*return SettleExactPlayerSupportFullRefund'
$forceRuntimeSettlementValid = $forceRuntimeSettlementValid -and $forceRuntimeSettlementBlock.Value -match 'operation\.m_iLastVirtualFriendlyCount'
$forceRuntimeSettlementValid = $forceRuntimeSettlementValid -and $forceRuntimeSettlementBlock.Value -match 'SettleExactPlayerSupportHRRefund\(state, request, virtualSurvivors'
$forceRuntimeSettlementValid = $forceRuntimeSettlementValid -and $forceRuntimeSettlementBlock.Value -match '!m_ExactForceSpawnQueue\)\s*return SetExactSupportRuntimeStatus\(request, "exact_terminal_settlement_queue_unavailable"\)'
if (!$forceRuntimeSettlementValid) {
	throw "Exact force terminal settlement must fully refund only before strategic service commitment, use the virtual survivor count after commitment, and wait when post-handoff queue authority is unavailable"
}
$strategicCommitClassifierBlock = [regex]::Match($scriptText, 'protected bool HasExactSupportStrategicServiceCommitted[\s\S]*?\r?\n\t}')
if (!$strategicCommitClassifierBlock.Success -or
	$strategicCommitClassifierBlock.Value -notmatch 'm_iProjectionContractVersion <= 0' -or
	$strategicCommitClassifierBlock.Value -notmatch 'HST_OPERATION_DUTY_UNKNOWN' -or
	$strategicCommitClassifierBlock.Value -notmatch 'HST_OPERATION_DUTY_STAGING') {
	throw "Exact force strategic-service commitment must exclude unknown and staging operations from post-commit settlement"
}
$forceRuntimeCleanupBlock = [regex]::Match($scriptText, 'bool FinalizeEliminatedForceSpawnProjection[\s\S]*?\r?\n\t}')
if (!$forceRuntimeCleanupBlock.Success -or $forceRuntimeCleanupBlock.Value -notmatch 'DetachForceSpawnMember' -or $forceRuntimeCleanupBlock.Value -match 'DeleteEntityAndChildren\(member\)') {
	throw "Exact force terminal cleanup must detach confirmed-dead members and preserve their entities before deleting the group root"
}
if ($scriptText -notmatch 'ReconcileHandedOffMemberLifecycle\(state, queue, physicalWar, nowSecond, result\);\s*ReconcileZeroLivingProjectionCleanup\(state, queue, physicalWar, nowSecond, result\);\s*ReconcileDeletedStagedHandles') {
	throw "Exact force lifecycle reconciliation must sample authoritative life state and retry zero-living cleanup before pruning deleted transient handles"
}
Write-Host "Schema-47 exact force-runtime casualty, cleanup, and survivor-reprojection contract OK"
foreach ($requiredForceArchiveEntry in @(
		'HST_ForceSettlementTransactionTombstoneState',
		'HST_ForceSettlementTombstoneState',
		'm_aForceSettlementTombstones',
		'CopyForceSettlementTombstone',
		'FindForceSettlementTombstoneByCommandRequest',
		'FindForceSettlementTombstoneByTransaction',
		'HST_ForceSettlementArchiveService',
		'MIN_ACCEPTED_RECORD_RETENTION_SECONDS = 600',
		'MIN_TOMBSTONE_RETENTION_SECONDS = 86400',
		'MAX_TOMBSTONE_ROWS = 256',
		'MAX_TOTAL_PLANNING_AUTHORITY_ROWS = 320',
		'ArchiveSettledRecords',
		'CanAdmitPlanningRecord',
		'BuildReplayQuote',
		'BuildReplayManifest',
		'HasProjectionBacklink',
		'archived transaction id conflict',
		'CompactForceSpawnQueueTerminalHistory',
		'HST_ForceSettlementArchiveProofService',
		'force_archive.garrison_replay',
		'force_archive.support_replay',
		'force_archive.backlink_protection',
		'force_archive.retention_capacity',
		'force_archive.persistence',
		'force_archive.schema47_migration',
		'migration_schema48_force_settlement_archive'
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredForceArchiveEntry)) {
		throw "Schema-48 force settlement archive contract missing: $requiredForceArchiveEntry"
	}
}
$forceArchiveBlock = [regex]::Match($scriptText, 'HST_ForceSettlementArchiveResult ArchiveSettledRecords[\s\S]*?\r?\n\t}')
$forceArchiveValid = $forceArchiveBlock.Success
$forceArchiveValid = $forceArchiveValid -and $forceArchiveBlock.Value -match 'm_eStatus != HST_EForceQuoteStatus\.HST_FORCE_QUOTE_ACCEPTED'
$forceArchiveValid = $forceArchiveValid -and $forceArchiveBlock.Value -match 'ValidateArchiveCandidate'
$forceArchiveValid = $forceArchiveValid -and $forceArchiveBlock.Value -match 'm_aForceSettlementTombstones\.Insert\(tombstone\)[\s\S]*RemoveArchivedTransactions[\s\S]*RemoveArchivedManifest[\s\S]*m_aForceQuotes\.Remove'
if (!$forceArchiveValid) {
	throw "Schema-48 settlement archive must prevalidate accepted aggregates and insert replay evidence before atomically removing full transaction, manifest, and quote rows"
}
$resourceReserveBlock = [regex]::Match($scriptText, 'HST_ResourceTransactionResult ReserveCost[\s\S]*?\r?\n\t}')
$resourceArchiveReplayValid = $resourceReserveBlock.Success
$resourceArchiveReplayValid = $resourceArchiveReplayValid -and $resourceReserveBlock.Value.IndexOf('FindForceSettlementTombstoneByTransaction') -ge 0
$resourceArchiveReplayValid = $resourceArchiveReplayValid -and $resourceReserveBlock.Value.IndexOf('FindForceSettlementTombstoneByTransaction') -lt $resourceReserveBlock.Value.IndexOf('SpendResource')
if (!$resourceArchiveReplayValid) {
	throw "Resource reservation must consult archived transaction tombstones before any resource debit"
}
if ($scriptText -notmatch 'CompactTerminalRows\(\s*m_State\.m_aForceSpawnResults,\s*pins,\s*nowSecond\)') {
	throw "Production force-spawn runtime must invoke pin-aware terminal compaction so retained rows cannot deadlock admission or settlement archival"
}
Write-Host "Schema-48 bounded force settlement archive and replay contract OK"
$operationTypesPath = "Scripts/Game/HST/HST_Types.c"
$operationServicePath = "Scripts/Game/HST/Services/HST_OperationService.c"
$operationProofPath = "Scripts/Game/HST/Services/HST_OperationRecordProofService.c"
$operationProjectionProofPath = "Scripts/Game/HST/Services/HST_OperationProjectionProofService.c"
$strategicMovementPath = "Scripts/Game/HST/Services/HST_StrategicMovementService.c"
$materializationPath = "Scripts/Game/HST/Services/HST_MaterializationService.c"
$virtualCombatPath = "Scripts/Game/HST/Services/HST_VirtualCombatService.c"
$operationSupportPath = "Scripts/Game/HST/Services/HST_SupportRequestService.c"
$operationArchivePath = "Scripts/Game/HST/Services/HST_ForceSettlementArchiveService.c"
$operationPlanningPath = "Scripts/Game/HST/Services/HST_ForcePlanningService.c"
foreach ($requiredOperationFile in @(
		$operationTypesPath,
		$operationServicePath,
		$operationProofPath,
		$operationProjectionProofPath,
		$strategicMovementPath,
		$materializationPath,
		$virtualCombatPath,
		$operationSupportPath,
		$operationArchivePath,
		$operationPlanningPath
	)) {
	if (!(Test-Path $requiredOperationFile)) {
		throw "Missing schema-49 exact-QRF operation authority file: $requiredOperationFile"
	}
}
$operationTypesText = Get-Content -Raw $operationTypesPath
$operationServiceText = Get-Content -Raw $operationServicePath
$operationProofText = Get-Content -Raw $operationProofPath
$operationProjectionProofText = Get-Content -Raw $operationProjectionProofPath
$strategicMovementText = Get-Content -Raw $strategicMovementPath
$materializationText = Get-Content -Raw $materializationPath
$virtualCombatText = Get-Content -Raw $virtualCombatPath
$operationProjectionQueueText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSpawnQueueService.c"
$operationSupportText = Get-Content -Raw $operationSupportPath
$operationArchiveText = Get-Content -Raw $operationArchivePath
$operationPlanningText = Get-Content -Raw $operationPlanningPath
$forceAuthorityDataText = Get-Content -Raw "Scripts/Game/HST/Data/HST_ForceAuthority.c"
$forceSaveDataText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"

foreach ($requiredOperationEnumEntry in @(
		'enum HST_EOperationType',
		'HST_OPERATION_TYPE_UNKNOWN',
		'HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF',
		'enum HST_EOperationDutyState',
		'HST_OPERATION_DUTY_UNKNOWN',
		'HST_OPERATION_DUTY_STAGING',
		'HST_OPERATION_DUTY_OUTBOUND',
		'HST_OPERATION_DUTY_ON_STATION',
		'HST_OPERATION_DUTY_RECALL_REQUESTED',
		'HST_OPERATION_DUTY_EXITING',
		'HST_OPERATION_DUTY_SETTLED',
		'enum HST_EOperationEngagementMode',
		'HST_OPERATION_ENGAGEMENT_UNKNOWN',
		'HST_OPERATION_ENGAGEMENT_CLEAR',
		'HST_OPERATION_ENGAGEMENT_CONTACT',
		'HST_OPERATION_ENGAGEMENT_ENGAGED',
		'HST_OPERATION_ENGAGEMENT_DISENGAGING',
		'enum HST_EOperationMaterializationState',
		'HST_OPERATION_MATERIALIZATION_UNKNOWN',
		'HST_OPERATION_MATERIALIZATION_VIRTUAL',
		'HST_OPERATION_MATERIALIZATION_MATERIALIZING',
		'HST_OPERATION_MATERIALIZATION_PHYSICAL',
		'HST_OPERATION_MATERIALIZATION_RETIRED',
		'enum HST_EOperationPositionAuthority',
		'HST_OPERATION_POSITION_UNKNOWN',
		'HST_OPERATION_POSITION_STRATEGIC',
		'HST_OPERATION_POSITION_LIVE',
		'enum HST_EOperationSettlementState',
		'HST_OPERATION_SETTLEMENT_UNKNOWN',
		'HST_OPERATION_SETTLEMENT_OPEN',
		'HST_OPERATION_SETTLEMENT_SETTLED',
		'enum HST_EOperationTerminalResult',
		'HST_OPERATION_TERMINAL_UNKNOWN',
		'HST_OPERATION_TERMINAL_NONE',
		'HST_OPERATION_TERMINAL_RECALLED',
		'HST_OPERATION_TERMINAL_DESTROYED',
		'HST_OPERATION_TERMINAL_CANCELLED',
		'HST_OPERATION_TERMINAL_SPAWN_FAILED',
		'HST_OPERATION_TERMINAL_INVALIDATED'
	)) {
	if ($operationTypesText -notmatch [regex]::Escape($requiredOperationEnumEntry)) {
		throw "Schema-49 operation enum contract missing: $requiredOperationEnumEntry"
	}
}
foreach ($operationEnumUnknownFirstPattern in @(
		'enum HST_EOperationType\s*\{\s*HST_OPERATION_TYPE_UNKNOWN\s*,',
		'enum HST_EOperationDutyState\s*\{\s*HST_OPERATION_DUTY_UNKNOWN\s*,',
		'enum HST_EOperationEngagementMode\s*\{\s*HST_OPERATION_ENGAGEMENT_UNKNOWN\s*,',
		'enum HST_EOperationMaterializationState\s*\{\s*HST_OPERATION_MATERIALIZATION_UNKNOWN\s*,',
		'enum HST_EOperationPositionAuthority\s*\{\s*HST_OPERATION_POSITION_UNKNOWN\s*,',
		'enum HST_EOperationSettlementState\s*\{\s*HST_OPERATION_SETTLEMENT_UNKNOWN\s*,',
		'enum HST_EOperationTerminalResult\s*\{\s*HST_OPERATION_TERMINAL_UNKNOWN\s*,'
	)) {
	if ($operationTypesText -notmatch $operationEnumUnknownFirstPattern) {
		throw "Schema-49 operation enum must reserve ordinal zero for UNKNOWN: $operationEnumUnknownFirstPattern"
	}
}

$operationRecordBlock = [regex]::Match($campaignStateText, 'class HST_OperationRecordState\r?\n\{[\s\S]*?\r?\n\}')
if (!$operationRecordBlock.Success) {
	throw "Schema-49 OperationRecord state class is missing"
}
foreach ($requiredOperationStateEntry in @(
		'm_sOperationId',
		'm_eType',
		'm_iContractVersion',
		'm_sOwnerFactionKey',
		'm_sActorIdentityId',
		'm_sIssueRequestId',
		'm_sConfirmationRequestId',
		'm_sSupportRequestId',
		'm_sQuoteId',
		'm_sManifestId',
		'm_sSpawnResultId',
		'm_sForceId',
		'm_sProjectionId',
		'm_sGroupId',
		'm_sOriginZoneId',
		'm_vOriginPosition',
		'm_sAssignmentKind',
		'm_sAssignmentZoneId',
		'm_vAssignmentPosition',
		'm_sTacticalTargetZoneId',
		'm_vTacticalTargetPosition',
		'm_vStrategicPosition',
		'm_sCurrentRouteId',
		'm_sRecallPolicyId',
		'm_sSettlementPolicyId',
		'm_eDutyState',
		'm_eResumeDutyState',
		'm_eEngagementMode',
		'm_eMaterializationState',
		'm_ePositionAuthority',
		'm_eSettlementState',
		'm_eTerminalResult',
		'm_sSettlementId',
		'm_sTerminalReason',
		'm_iDeterministicSeed',
		'm_iCreatedAtSecond',
		'm_iDutyStateEnteredAtSecond',
		'm_iEngagementStateEnteredAtSecond',
		'm_iMaterializationStateEnteredAtSecond',
		'm_iLastContactAtSecond',
		'm_iLastProgressAtSecond',
		'm_iSettledAtSecond',
		'm_iRevision'
	)) {
	if ($operationRecordBlock.Value -notmatch [regex]::Escape($requiredOperationStateEntry)) {
		throw "Schema-49 OperationRecord state missing canonical field: $requiredOperationStateEntry"
	}
}
foreach ($requiredOperationStateRootEntry in @(
		'SCHEMA_VERSION = 58',
		'ref array<ref HST_OperationRecordState> m_aOperations = {};',
		'HST_OperationRecordState FindOperation(string operationId)',
		'int m_iOperationContractVersion;'
	)) {
	if ($campaignStateText -notmatch [regex]::Escape($requiredOperationStateRootEntry)) {
		throw "Schema-49 operation state root missing: $requiredOperationStateRootEntry"
	}
}

foreach ($requiredOperationServiceEntry in @(
		'class HST_OperationTransitionResult',
		'class HST_OperationService',
		'EXACT_PLAYER_QRF_CONTRACT_VERSION = 1',
		'EXACT_PLAYER_QRF_ASSIGNMENT_KIND = "support_on_station"',
		'EXACT_PLAYER_QRF_RECALL_POLICY = "exit_then_refund_living_hr"',
		'EXACT_PLAYER_QRF_SETTLEMENT_POLICY = "exact_paid_qrf_ledger"',
		'RegisterExactPlayerQRF',
		'RemoveUncommittedExactPlayerQRF',
		'MarkOutboundMaterializing',
		'MarkPhysical',
		'MarkOnStation',
		'MarkRestoreMaterializing',
		'CanBeginRecall',
		'BeginRecall',
		'MarkRecallExiting',
		'RecordEngagement',
		'IsLegalEngagementTransition',
		'CanSettleExactPlayerQRF',
		'SettleExactPlayerQRF',
		'ValidateExactPlayerQRF',
		'RemoveArchivedOperation',
		'operation.m_iSettledAtSecond < 0'
	)) {
	if ($operationServiceText -notmatch [regex]::Escape($requiredOperationServiceEntry)) {
		throw "Schema-49 operation service contract missing: $requiredOperationServiceEntry"
	}
}
if ($operationServiceText -notmatch 'HST_OPERATION_ENGAGEMENT_CLEAR[\s\S]*HST_OPERATION_ENGAGEMENT_CONTACT[\s\S]*HST_OPERATION_ENGAGEMENT_ENGAGED[\s\S]*HST_OPERATION_ENGAGEMENT_DISENGAGING[\s\S]*HST_OPERATION_ENGAGEMENT_CLEAR') {
	throw "Operation engagement authority must enforce the clear/contact/engaged/disengaging/clear transition sequence"
}
if ($operationServiceText -notmatch 'operation\.m_sOriginZoneId != quote\.m_sSourceZoneId[\s\S]*operation\.m_sAssignmentZoneId != quote\.m_sTargetZoneId' -or
	$operationServiceText -notmatch 'operation\.m_sRecallPolicyId != EXACT_PLAYER_QRF_RECALL_POLICY[\s\S]*operation\.m_sSettlementPolicyId != EXACT_PLAYER_QRF_SETTLEMENT_POLICY') {
	throw "Operation validation must protect immutable origin, assignment, and policy authority"
}

$paidQRFIssueOperationBlock = [regex]::Match($operationPlanningText, 'HST_ForceQuoteResult IssuePlayerSupportQuote[\s\S]*?\r?\n\t\}')
if (!$paidQRFIssueOperationBlock.Success -or $paidQRFIssueOperationBlock.Value -match 'RegisterExactPlayerQRF|m_aOperations') {
	throw "Exact paid-QRF issue must allocate quote authority without creating an OperationRecord"
}
$operationRegistrationBlock = [regex]::Match($operationSupportText, 'bool RegisterAcceptedExactPlayerSupport[\s\S]*?\r?\n\t\}')
if (!$operationRegistrationBlock.Success) {
	throw "Exact paid-QRF accepted-support registration block is missing"
}
$operationContractIndex = $operationRegistrationBlock.Value.IndexOf('m_iOperationContractVersion = HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION')
$operationRegisterIndex = $operationRegistrationBlock.Value.LastIndexOf('m_Operations.RegisterExactPlayerQRF')
$operationRequestInsertIndex = $operationRegistrationBlock.Value.IndexOf('m_aSupportRequests.Insert(request)')
if ($operationContractIndex -lt 0 -or $operationRegisterIndex -le $operationContractIndex -or $operationRequestInsertIndex -le $operationRegisterIndex) {
	throw "Accepted exact paid-QRF confirmation must version and register one OperationRecord before exposing the support request"
}
if ($operationRegistrationBlock.Value -match 'm_aQRFs') {
	throw "Exact paid player-QRF registration must not reuse legacy QRF state authority"
}
foreach ($requiredOperationReplayValidationEntry in @(
		'protected bool PlayerSupportRequestMatchesQuote(HST_CampaignState state',
		'HST_OperationService.RequiresOperation(request)',
		'm_Operations.ValidateExactPlayerQRF(state, operation, request, quote, manifest)'
	)) {
	if ($operationPlanningText -notmatch [regex]::Escape($requiredOperationReplayValidationEntry)) {
		throw "Accepted exact paid-QRF replay must validate versioned OperationRecord authority: $requiredOperationReplayValidationEntry"
	}
}
foreach ($requiredOperationLifecycleHook in @(
		'm_Operations.LinkOutboundVirtual(state, request, activeGroup, result.m_Batch)',
		'm_Operations.MarkMaterializingFromVirtual',
		'm_Operations.MarkPhysical(state, request, group, batch)',
		'm_Operations.MarkOnStation(state, request, group)',
		'm_Operations.BeginDematerialization',
		'm_Operations.CompleteDematerialization',
		'm_Operations.CanBeginRecall(state, request)',
		'm_Operations.MarkRecallExiting(state, request, group, exitPosition)',
		'm_Operations.CanSettleExactPlayerQRF',
		'm_Operations.SettleExactPlayerQRF'
	)) {
	if ($operationSupportText -notmatch [regex]::Escape($requiredOperationLifecycleHook)) {
		throw "Exact paid-QRF production lifecycle is missing OperationRecord hook: $requiredOperationLifecycleHook"
	}
}
foreach ($operationSettlementMethodPattern in @(
		'protected bool SettleExactPlayerSupportFullRefund[\s\S]*?\r?\n\t\}',
		'protected bool SettleExactPlayerSupportHRRefund[\s\S]*?\r?\n\t\}'
	)) {
	$operationSettlementBlock = [regex]::Match($operationSupportText, $operationSettlementMethodPattern)
	if (!$operationSettlementBlock.Success) {
		throw "Exact paid-QRF typed settlement block is missing: $operationSettlementMethodPattern"
	}
	$operationPreflightIndex = $operationSettlementBlock.Value.IndexOf('m_Operations.CanSettleExactPlayerQRF')
	$operationLedgerIndex = $operationSettlementBlock.Value.IndexOf('SettleExactSupportTransaction')
	$operationApplyIndex = $operationSettlementBlock.Value.IndexOf('m_Operations.SettleExactPlayerQRF')
	if ($operationPreflightIndex -lt 0 -or $operationLedgerIndex -le $operationPreflightIndex -or $operationApplyIndex -le $operationLedgerIndex) {
		throw "Exact paid-QRF settlement must preflight operation authority before ledger mutation and apply the typed terminal result afterward"
	}
}

foreach ($requiredOperationPersistenceEntry in @(
		'ref array<ref HST_OperationRecordState> m_aOperations = {};',
		'm_aOperations.Insert(CopyOperation(operation));',
		'state.m_aOperations.Insert(CopyOperation(operation));',
		'protected HST_OperationRecordState CopyOperation',
		'NormalizeOperationAuthority(restoredSchemaVersion);',
		'NormalizeRestoredOperationProjectionState();',
		'migration_schema49_operation_record',
		'migration_schema49_operation_record_conflict',
		'BuildSchema49MigratedOperation',
		'CountSchema49OperationIdentityMatches',
		'CountSchema49TombstoneIdentityMatches'
	)) {
	if ($forceSaveDataText -notmatch [regex]::Escape($requiredOperationPersistenceEntry)) {
		throw "Schema-49 OperationRecord persistence or migration contract missing: $requiredOperationPersistenceEntry"
	}
}
$operationCopyBlock = [regex]::Match($forceSaveDataText, 'protected HST_OperationRecordState CopyOperation[\s\S]*?\r?\n\t\}')
if (!$operationCopyBlock.Success) {
	throw "Schema-49 OperationRecord deep-copy block is missing"
}
foreach ($requiredOperationCopyEntry in @(
		'm_sOperationId',
		'm_sOriginZoneId',
		'm_vOriginPosition',
		'm_sAssignmentKind',
		'm_sAssignmentZoneId',
		'm_vAssignmentPosition',
		'm_sTacticalTargetZoneId',
		'm_vTacticalTargetPosition',
		'm_vStrategicPosition',
		'm_eDutyState',
		'm_eResumeDutyState',
		'm_eEngagementMode',
		'm_eMaterializationState',
		'm_ePositionAuthority',
		'm_eSettlementState',
		'm_eTerminalResult',
		'm_sSettlementId',
		'm_iRevision'
	)) {
	if ($operationCopyBlock.Value -notmatch [regex]::Escape("target.$requiredOperationCopyEntry = source.$requiredOperationCopyEntry;")) {
		throw "Schema-49 OperationRecord deep copy missing: $requiredOperationCopyEntry"
	}
}
$operationMigrationBlock = [regex]::Match($forceSaveDataText, 'protected void NormalizeOperationAuthority[\s\S]*?\r?\n\t\}\r?\n\r?\n\tprotected void NormalizeRestoredOperationProjectionState')
if (!$operationMigrationBlock.Success -or $operationMigrationBlock.Value -notmatch 'restoredSchemaVersion >= 49' -or
	$operationMigrationBlock.Value -notmatch 'request\.m_iOperationContractVersion = 0' -or
	$operationMigrationBlock.Value -notmatch 'IsSchema49OperationMigrationCandidate' -or
	$operationMigrationBlock.Value -notmatch 'm_aOperations\.Insert\(operation\)') {
	throw "Schema-49 migration must conservatively opt coherent pre-schema-49 exact paid QRFs into one OperationRecord"
}
$operationRestoreProjectionBlock = [regex]::Match($forceSaveDataText, 'protected void NormalizeRestoredOperationProjectionState[\s\S]*?\r?\n\t\}')
if (!$operationRestoreProjectionBlock.Success -or
	$operationRestoreProjectionBlock.Value -notmatch 'HST_OPERATION_MATERIALIZATION_PHYSICAL' -or
	$operationRestoreProjectionBlock.Value -notmatch 'HST_OPERATION_MATERIALIZATION_DEMATERIALIZING' -or
	$operationRestoreProjectionBlock.Value -notmatch 'HST_OPERATION_MATERIALIZATION_MATERIALIZING' -or
	$operationRestoreProjectionBlock.Value -notmatch 'HST_OPERATION_POSITION_STRATEGIC' -or
	$operationRestoreProjectionBlock.Value -notmatch 'm_iRevision\+\+') {
	throw "Current-schema restore must return process-local physical OperationRecords to strategic materialization authority"
}
$operationRestoreRequestProjectionBlock = [regex]::Match($forceSaveDataText, 'protected void NormalizeRestoredStrategicProjectionBatch[\s\S]*?\r?\n\t}')
foreach ($requiredRestoreRequestProjectionEntry in @(
	'request.m_bPhysicalized = false;',
	'request.m_bAbstractResolved = true;',
	'request.m_sRuntimeStatus = "exact_virtual_on_station";',
	'request.m_bAbstractResolved = false;',
	'request.m_sRuntimeStatus = "exact_restore_survivor_virtual";'
)) {
	if (!$operationRestoreRequestProjectionBlock.Success -or $operationRestoreRequestProjectionBlock.Value -notmatch [regex]::Escape($requiredRestoreRequestProjectionEntry)) {
		throw "Schema-50 restore must normalize linked request projection authority: $requiredRestoreRequestProjectionEntry"
	}
}
foreach ($requiredRestoreRequestProofEntry in @(
	'runtime.m_Base.m_Request.m_bPhysicalized = true;',
	'!restoredRequest.m_bPhysicalized',
	'restoredRequest.m_sRuntimeStatus == "exact_virtual_on_station"',
	'request virtualized %3 status %4'
)) {
	if ($operationProofText -notmatch [regex]::Escape($requiredRestoreRequestProofEntry)) {
		throw "Operation restore proof must assert linked request virtualization: $requiredRestoreRequestProofEntry"
	}
}

foreach ($requiredOperationArchiveEntry in @(
		'm_sOperationSettlementId',
		'm_iOperationContractVersion',
		'm_iOperationRevision',
		'm_eOperationTerminalResult'
	)) {
	if ($forceAuthorityDataText -notmatch [regex]::Escape($requiredOperationArchiveEntry) -or
		$forceSaveDataText -notmatch [regex]::Escape("target.$requiredOperationArchiveEntry = source.$requiredOperationArchiveEntry;")) {
		throw "Schema-49 operation settlement tombstone contract missing: $requiredOperationArchiveEntry"
	}
}
foreach ($requiredOperationArchiveServiceEntry in @(
		'HST_OperationRecordState operation',
		'HST_OperationService.RequiresOperation(supportRequest)',
		'm_Operations.ValidateExactPlayerQRF',
		'HST_OPERATION_SETTLEMENT_SETTLED',
		'HST_OperationService.BuildSettlementId',
		'm_Operations.RemoveArchivedOperation(state, operation)',
		'tombstone.m_sOperationSettlementId = operation.m_sSettlementId;',
		'tombstone.m_iOperationContractVersion = operation.m_iContractVersion;',
		'tombstone.m_iOperationRevision = operation.m_iRevision;',
		'tombstone.m_eOperationTerminalResult = operation.m_eTerminalResult;'
	)) {
	if ($operationArchiveText -notmatch [regex]::Escape($requiredOperationArchiveServiceEntry)) {
		throw "Schema-49 operation archive contract missing: $requiredOperationArchiveServiceEntry"
	}
}
$operationArchiveMethodBlock = [regex]::Match($operationArchiveText, 'HST_ForceSettlementArchiveResult ArchiveSettledRecords[\s\S]*?\r?\n\t\}')
if (!$operationArchiveMethodBlock.Success -or
	$operationArchiveMethodBlock.Value.IndexOf('m_Operations.RemoveArchivedOperation') -lt 0 -or
	$operationArchiveMethodBlock.Value.IndexOf('m_Operations.RemoveArchivedOperation') -gt $operationArchiveMethodBlock.Value.IndexOf('m_aForceSettlementTombstones.Insert(tombstone)')) {
	throw "Schema-49 archive must retire the full settled OperationRecord into its validated tombstone"
}

foreach ($requiredOperationProofEntry in @(
		'class HST_OperationRecordProofReport',
		'class HST_OperationRecordProofService',
		'HST_OperationRecordProofReport Run()',
		'm_bIssueConfirmExact',
		'm_bMaterializationExact',
		'm_bEngagementExact',
		'm_bRecallSettlementExact',
		'm_bRestoreProjectionExact',
		'm_bSchema48MigrationExact',
		'm_bArchiveExact',
		'm_bLegacyQRFIsolationExact',
		'AppendCampaignDebugOperationRecordAssertions',
		'operation_record.issue_confirm',
		'operation_record.materialization',
		'operation_record.engagement',
		'operation_record.recall_settlement',
		'operation_record.restore_projection',
		'operation_record.schema48_migration',
		'operation_record.archive',
		'operation_record.legacy_qrf_isolation'
	)) {
	if (($operationProofText + "`n" + $scriptText) -notmatch [regex]::Escape($requiredOperationProofEntry)) {
		throw "Schema-49 OperationRecord proof integration missing: $requiredOperationProofEntry"
	}
}
Write-Host "Schema-49 exact paid-QRF OperationRecord authority, persistence, migration, archive, and proof contract OK"
foreach ($requiredOperationProjectionEntry in @(
		'class HST_StrategicMovementService',
		'EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND = 2.5',
		'Math.Ceil(distance / EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND)',
		'bool routeInitialized',
		'MAX_CATCHUP_SECONDS_PER_TICK = 30',
		'ResolveExactPlayerQRFETASeconds',
		'AdvanceExactPlayerQRF',
		'class HST_MaterializationService',
		'MIN_HYSTERESIS_METERS = 350.0',
		'HST_OPERATION_PROJECTION_MATERIALIZE',
		'HST_OPERATION_PROJECTION_DEMATERIALIZE',
		'class HST_VirtualCombatService',
		'COMBAT_STEP_SECONDS = 30',
		'MAX_COMBAT_STEPS_PER_TICK = 4',
		'ConfirmStrategicMemberCasualty',
		'HoldPendingProjectionForStrategicSimulation',
		'ReleaseStrategicProjectionForMaterialization',
		'RequeueSuccessfulProjectionForStrategicHold',
		'CompleteStrategicProjectionElimination',
		'm_bStrategicProjectionHeld',
		'm_iProjectionContractVersion',
		'm_fRouteProgressMeters',
		'm_iVirtualCombatLastStepSecond',
		'physical interval excluded from virtual combat catch-up',
		'NormalizeAbstractEngagementForPhysicalHandoff',
		'abstract engagement handed off clear to physical projection',
		'foldCombatClockExact',
		'physicalEngagementClear',
		'wasSuccessfulPhysical',
		'restoredBatch.m_iReprojectionCount == sourceReprojectionCount + 1',
		'PositionsMatch(restoredOperation.m_vRouteStartPosition, savedGroupPosition)',
		'NormalizeOperationProjectionAuthority(restoredSchemaVersion);',
		'NormalizeRestoredStrategicProjectionBatch',
		'migration_schema50_operation_projection',
		'class HST_OperationProjectionProofService',
		'operation_projection.movement',
		'operation_projection.hysteresis',
		'operation_projection.roster_transfer',
		'operation_projection.combat_restore',
		'virtual en route',
		'virtual on station'
	)) {
	$operationProjectionCorpus = $strategicMovementText + "`n" + $materializationText + "`n" + $virtualCombatText + "`n" + $operationProjectionProofText + "`n" + $operationServiceText + "`n" + $operationSupportText + "`n" + $operationProjectionQueueText + "`n" + $forceAuthorityDataText + "`n" + $forceSaveDataText + "`n" + $campaignStateText + "`n" + $markerServiceText + "`n" + $scriptText
	if ($operationProjectionCorpus -notmatch [regex]::Escape($requiredOperationProjectionEntry)) {
		throw "Schema-50 exact infantry-QRF strategic projection contract missing: $requiredOperationProjectionEntry"
	}
}
Write-Host "Schema-50 exact infantry-QRF strategic movement, hysteresis, roster transfer, virtual combat, restore, and proof contract OK"
$enemyQRFOperationPath = "Scripts/Game/HST/Services/HST_EnemyQRFOperationService.c"
$enemyQRFProofPath = "Scripts/Game/HST/Services/HST_EnemyQRFOperationProofService.c"
$enemyCommanderPath = "Scripts/Game/HST/Services/HST_EnemyCommanderService.c"
$forcePlanningPath = "Scripts/Game/HST/Services/HST_ForcePlanningService.c"
$physicalWarPath = "Scripts/Game/HST/Services/HST_PhysicalWarService.c"
foreach ($schema51Path in @($enemyQRFOperationPath, $enemyQRFProofPath, $enemyCommanderPath, $forcePlanningPath, $physicalWarPath)) {
	if (!(Test-Path $schema51Path)) {
		throw "Schema-51 exact enemy defensive-QRF source is missing: $schema51Path"
	}
}
$enemyQRFOperationText = Get-Content -Raw $enemyQRFOperationPath
$enemyQRFProofText = Get-Content -Raw $enemyQRFProofPath
$enemyCommanderText = Get-Content -Raw $enemyCommanderPath
$forcePlanningText = Get-Content -Raw $forcePlanningPath
$physicalWarText = Get-Content -Raw $physicalWarPath
$schema51StateCorpus = $operationTypesText + "`n" + $campaignStateText + "`n" + $forceSaveDataText
foreach ($requiredSchema51StateEntry in @(
		'HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF',
		'HST_OPERATION_TERMINAL_COMPLETED',
		'int m_iOperationContractVersion;',
		'string m_sSourceZoneId;',
		'string m_sManifestHash;',
		'bool m_bStrategicServiceCommitted;',
		'string m_sResourceSettlementId;',
		'string m_sResourceSettlementKind;',
		'int m_iSettlementAcceptedMemberCount;',
		'int m_iSettlementSurvivorMemberCount;',
		'bool m_bResourceSettlementApplied;',
		'string m_sEnemyOrderId;',
		'int m_iArrivalConfirmationCount;',
		'int m_iLastArrivalConfirmationSecond;',
		'NormalizeSchema51EnemyDefensiveQRFAuthority',
		'ValidateSchema51EnemyDefensiveQRFRestore',
		'FindSchema51EnemyOrder',
		'FindSchema51ForceSpawnResult',
		'FindSchema51ActiveGroup',
		'migration_schema51_enemy_defensive_qrf_authority',
		'legacy_enemy_orders_preserved_contract_zero'
	)) {
	if ($schema51StateCorpus -notmatch [regex]::Escape($requiredSchema51StateEntry)) {
		throw "Schema-51 enemy defensive-QRF state/persistence contract missing: $requiredSchema51StateEntry"
	}
}
foreach ($requiredSchema51CopyEntry in @(
		'target.m_sEnemyOrderId = source.m_sEnemyOrderId;',
		'target.m_sSourceZoneId = source.m_sSourceZoneId;',
		'target.m_sManifestHash = source.m_sManifestHash;',
		'target.m_bStrategicServiceCommitted = source.m_bStrategicServiceCommitted;',
		'target.m_sResourceSettlementId = source.m_sResourceSettlementId;',
		'target.m_sResourceSettlementKind = source.m_sResourceSettlementKind;',
		'target.m_iSettlementAcceptedMemberCount = source.m_iSettlementAcceptedMemberCount;',
		'target.m_iSettlementSurvivorMemberCount = source.m_iSettlementSurvivorMemberCount;',
		'target.m_bResourceSettlementApplied = source.m_bResourceSettlementApplied;',
		'target.m_iArrivalConfirmationCount = source.m_iArrivalConfirmationCount;',
		'target.m_iLastArrivalConfirmationSecond = source.m_iLastArrivalConfirmationSecond;'
	)) {
	if ($forceSaveDataText -notmatch [regex]::Escape($requiredSchema51CopyEntry)) {
		throw "Schema-51 enemy defensive-QRF deep copy missing: $requiredSchema51CopyEntry"
	}
}
foreach ($requiredSchema51PlanningEntry in @(
		'class HST_EnemyDefensiveQRFManifestResult',
		'PlanExactEnemyDefensiveQRF',
		'HST_FactionRelationService.IsEnemyFaction',
		'source and defended target must remain faction-owned',
		'EXACT_ENEMY_DEFENSIVE_QRF_FORCE_KIND',
		'EXACT_ENEMY_DEFENSIVE_QRF_POLICY_ID',
		'manifest.m_aGroups.Insert(groupElement);',
		'manifest.m_aMembers.Insert(member);',
		'manifest.m_bFrozen = true;'
	)) {
	if ($forcePlanningText -notmatch [regex]::Escape($requiredSchema51PlanningEntry)) {
		throw "Schema-51 enemy defensive-QRF planning contract missing: $requiredSchema51PlanningEntry"
	}
}
foreach ($requiredSchema51OperationEntry in @(
		'EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION = 1',
		'EXACT_ENEMY_DEFENSIVE_QRF_FORCE_KIND',
		'EXACT_ENEMY_DEFENSIVE_QRF_RECALL_POLICY',
		'EXACT_ENEMY_DEFENSIVE_QRF_SETTLEMENT_POLICY',
		'RegisterExactEnemyDefensiveQRF',
		'LinkExactEnemyDefensiveQRFOutboundVirtual',
		'MarkExactEnemyDefensiveQRFMaterializingFromVirtual',
		'MarkExactEnemyDefensiveQRFPhysical',
		'ConfirmExactEnemyDefensiveQRFArrivalSample',
		'MarkExactEnemyDefensiveQRFOnStation',
		'BeginExactEnemyDefensiveQRFReturnToOrigin',
		'CanPrepareExactEnemyDefensiveQRFSettlement',
		'RecordExactEnemyDefensiveQRFResourceSettlement',
		'SettleExactEnemyDefensiveQRF',
		'ValidateExactEnemyDefensiveQRF',
		'CountEnemyOrderId',
		'CountForceManifestId'
	)) {
	if ($operationServiceText -notmatch [regex]::Escape($requiredSchema51OperationEntry)) {
		throw "Schema-51 enemy defensive-QRF operation contract missing: $requiredSchema51OperationEntry"
	}
}
foreach ($requiredSchema51RuntimeEntry in @(
		'CanAdmitPreparedOrder',
		'ResolveCommittedAdmissionReplay',
		'AdmitPreparedOrder',
		'InitializeExactInfantryQRFRoute',
		'RestartExactEnemyQRFInfantryRoute',
		'HST_OPERATION_TERMINAL_ROUTE_FAILED',
		'RetireAndSettlePhysicalFailure',
		'ApplyDefensiveArrivalOutcome',
		'ReconcileAfterRestore',
		'ReconcileSettledRuntimeCleanup',
		'ResolveRestoreSettlementSurvivors',
		'ValidateAppliedResourceSettlement',
		'HasPartialResourceSettlementAuthority',
		'FindAmbiguousAuthorityRows',
		'CountForceSpawnResultsByAnyAuthorityIdentity',
		'CountActiveGroupsByAnyAuthorityIdentity',
		'AbortAmbiguousAuthority'
	)) {
	if ($enemyQRFOperationText -notmatch [regex]::Escape($requiredSchema51RuntimeEntry)) {
		throw "Schema-51 enemy defensive-QRF runtime contract missing: $requiredSchema51RuntimeEntry"
	}
}
$schema51QueueOrderBlock = [regex]::Match($enemyCommanderText, 'protected bool QueueOrder[\s\S]*?\r?\n\t\}\r?\n\r?\n\tprotected bool TickActiveOrderRuntime')
if (!$schema51QueueOrderBlock.Success) {
	throw "Schema-51 enemy defensive-QRF production admission block is missing"
}
$schema51PlanIndex = $schema51QueueOrderBlock.Value.IndexOf('PlanExactEnemyDefensiveQRF')
$schema51PreflightIndex = $schema51QueueOrderBlock.Value.IndexOf('CanAdmitPreparedOrder')
$schema51DebitIndex = $schema51QueueOrderBlock.Value.IndexOf('TrySpendDefense')
$schema51InsertIndex = $schema51QueueOrderBlock.Value.IndexOf('m_aEnemyOrders.Insert(order)')
$schema51AdmitIndex = $schema51QueueOrderBlock.Value.IndexOf('m_ExactEnemyQRF.AdmitPreparedOrder')
if ($schema51PlanIndex -lt 0 -or $schema51PreflightIndex -le $schema51PlanIndex -or
	$schema51DebitIndex -le $schema51PreflightIndex -or $schema51InsertIndex -le $schema51DebitIndex -or
	$schema51AdmitIndex -le $schema51InsertIndex) {
	throw "Schema-51 enemy defensive-QRF production flow must plan and preflight before debit, then expose and admit one queued order"
}
foreach ($requiredSchema51CommanderEntry in @(
		'SetExactEnemyQRFAuthorityServices',
		'QueueDebugLegacyOrder',
		'forceDebugLegacyOperation',
		'ResolveRuntimeOwner(order)',
		'm_ExactEnemyQRF.TickOrder',
		'HasActiveLegacyEnemyQRFSupport',
		'HST_ZONE_HIDEOUT',
		'HST_ZONE_MISSION_SITE'
	)) {
	if ($enemyCommanderText -notmatch [regex]::Escape($requiredSchema51CommanderEntry)) {
		throw "Schema-51 enemy defensive-QRF commander isolation missing: $requiredSchema51CommanderEntry"
	}
}
foreach ($requiredSchema51PhysicalEntry in @(
		'RestartExactEnemyQRFInfantryRoute',
		'IsLiveConfirmedOperationRouteGroup',
		'HasOpenEnemyCommanderQRF',
		'HST_ENEMY_ORDER_QUEUED',
		'HST_ENEMY_ORDER_ACTIVE'
	)) {
	if (($physicalWarText + "`n" + $enemyQRFOperationText) -notmatch [regex]::Escape($requiredSchema51PhysicalEntry)) {
		throw "Schema-51 enemy defensive-QRF physical/legacy isolation missing: $requiredSchema51PhysicalEntry"
	}
}
foreach ($requiredSchema51MarkerEntry in @(
		'AddExactEnemyQRFOperationMarkers',
		'ShouldShowExactEnemyDefensiveQRFMarker',
		'ResolveExactEnemyDefensiveQRFMarkerLiving',
		'else if (!authoritative && group)',
		'hst_exact_enemy_qrf_',
		'enemy_response_exact',
		'CountOpenExactEnemyDefensiveQRFs'
	)) {
	if ($markerServiceText -notmatch [regex]::Escape($requiredSchema51MarkerEntry)) {
		throw "Schema-51 enemy defensive-QRF marker contract missing: $requiredSchema51MarkerEntry"
	}
}
foreach ($requiredSchema51ProofEntry in @(
		'class HST_EnemyQRFOperationProofService',
		'HST_EnemyQRFOperationProofReport Run()',
		'AppendCampaignDebugEnemyQRFOperationAssertions',
		'enemy_qrf.admission',
		'enemy_qrf.legacy_isolation',
		'enemy_qrf.projection',
		'enemy_qrf.settlement',
		'enemy_qrf.persistence',
		'enemy_qrf.rejection',
		'ProvePartialReceiptRestoreQuarantine',
		'ProveMissingGroupBacklinkRestore',
		'ProveMissingCanonicalReplayRejection',
		'ProveShadowReplayRejection'
	)) {
	if (($enemyQRFProofText + "`n" + $coordinatorText) -notmatch [regex]::Escape($requiredSchema51ProofEntry)) {
		throw "Schema-51 enemy defensive-QRF proof integration missing: $requiredSchema51ProofEntry"
	}
}
$restoreSettlementStart = $enemyQRFOperationText.IndexOf('protected bool SettleInvalidatedRestoreAuthority')
$restoreSettlementEnd = $enemyQRFOperationText.IndexOf('protected bool ForceSettleInvalidatedRestore')
if ($restoreSettlementStart -lt 0 -or $restoreSettlementEnd -le $restoreSettlementStart) {
	throw "Schema-51 restore settlement authority block is missing"
}
$restoreSettlementBlock = $enemyQRFOperationText.Substring($restoreSettlementStart, $restoreSettlementEnd - $restoreSettlementStart)
if ($restoreSettlementBlock -match 'm_aOperations\.Remove') {
	throw "Schema-51 restore must preserve ambiguous operation evidence instead of deleting rows to manufacture uniqueness"
}
$schema51OperationValidationStart = $operationServiceText.IndexOf('string ValidateExactEnemyDefensiveQRF(')
$schema51OperationValidationEnd = $operationServiceText.IndexOf('bool RemoveArchivedOperation', $schema51OperationValidationStart)
$schema51SaveValidationStart = $forceSaveDataText.IndexOf('protected string ValidateSchema51EnemyDefensiveQRFRestore')
$schema51SaveValidationEnd = $forceSaveDataText.IndexOf('protected HST_OperationRecordState FindSchema51Operation', $schema51SaveValidationStart)
if ($schema51OperationValidationStart -lt 0 -or $schema51OperationValidationEnd -le $schema51OperationValidationStart -or
	$schema51SaveValidationStart -lt 0 -or $schema51SaveValidationEnd -le $schema51SaveValidationStart) {
	throw "Schema-51 partial-settlement validation blocks are missing"
}
$schema51OperationValidationBlock = $operationServiceText.Substring($schema51OperationValidationStart, $schema51OperationValidationEnd - $schema51OperationValidationStart)
$schema51SaveValidationBlock = $forceSaveDataText.Substring($schema51SaveValidationStart, $schema51SaveValidationEnd - $schema51SaveValidationStart)
foreach ($partialSettlementBlock in @($schema51OperationValidationBlock, $schema51SaveValidationBlock)) {
	if ($partialSettlementBlock -notmatch [regex]::Escape('m_iRefundedAttackResources != 0') -or
		$partialSettlementBlock -notmatch [regex]::Escape('m_iRefundedSupportResources != 0')) {
		throw "Schema-51 unsettled validation must quarantine refund-only partial receipts"
	}
}
$schema51RuntimeContextStart = $enemyQRFOperationText.IndexOf('protected string ResolveRuntimeContext')
$schema51RuntimeContextEnd = $enemyQRFOperationText.IndexOf('protected HST_ActiveGroupState BuildActiveGroup', $schema51RuntimeContextStart)
if ($schema51RuntimeContextStart -lt 0 -or $schema51RuntimeContextEnd -le $schema51RuntimeContextStart) {
	throw "Schema-51 runtime-context authority block is missing"
}
$schema51RuntimeContextBlock = $enemyQRFOperationText.Substring($schema51RuntimeContextStart, $schema51RuntimeContextEnd - $schema51RuntimeContextStart)
if ($schema51RuntimeContextBlock.IndexOf('FindAmbiguousAuthorityRows') -lt 0 -or
	$schema51RuntimeContextBlock.IndexOf('FindAmbiguousAuthorityRows') -gt $schema51RuntimeContextBlock.IndexOf('state.FindOperation')) {
	throw "Schema-51 runtime replay must reject cross-backlink ambiguity before canonical lookup"
}
$schema51AmbiguityStart = $enemyQRFOperationText.IndexOf('protected string FindAmbiguousAuthorityRows')
$schema51AmbiguityEnd = $enemyQRFOperationText.IndexOf('protected int CountEnemyOrdersByAnyAuthorityIdentity', $schema51AmbiguityStart)
if ($schema51AmbiguityStart -lt 0 -or $schema51AmbiguityEnd -le $schema51AmbiguityStart) {
	throw "Schema-51 cross-backlink ambiguity block is missing"
}
$schema51AmbiguityBlock = $enemyQRFOperationText.Substring($schema51AmbiguityStart, $schema51AmbiguityEnd - $schema51AmbiguityStart)
foreach ($requiredAmbiguityGuard in @(
		'(operation && operationCount != 1)',
		'(!operation && operationCount > 0)',
		'(batch && batchCount != 1)',
		'(!batch && batchCount > 0)',
		'(group && groupCount != 1)',
		'(!group && groupCount > 0)'
	)) {
	if ($schema51AmbiguityBlock -notmatch [regex]::Escape($requiredAmbiguityGuard)) {
		throw "Schema-51 missing-canonical or shadow authority guard is missing: $requiredAmbiguityGuard"
	}
}
$schema51SourceLinkStart = $forceSaveDataText.IndexOf('protected void NormalizeActiveGroupSourceLinks')
$schema51SourceLinkEnd = $forceSaveDataText.IndexOf('protected HST_ActiveGroupState FindActiveGroupForMigration', $schema51SourceLinkStart)
if ($schema51SourceLinkStart -lt 0 -or $schema51SourceLinkEnd -le $schema51SourceLinkStart) {
	throw "Schema-51 active-group source-link normalization block is missing"
}
$schema51SourceLinkBlock = $forceSaveDataText.Substring($schema51SourceLinkStart, $schema51SourceLinkEnd - $schema51SourceLinkStart)
if ($forceSaveDataText -notmatch [regex]::Escape('NormalizeActiveGroupSourceLinks(restoredSchemaVersion);') -or
	$schema51SourceLinkBlock.IndexOf('restoredSchemaVersion < 51') -lt 0 -or
	$schema51SourceLinkBlock.IndexOf('restoredSchemaVersion < 51') -gt $schema51SourceLinkBlock.IndexOf('group.m_sEnemyOrderId = order.m_sOrderId;')) {
	throw "Schema-51 current-schema restore must not synthesize the required enemy-order group backlink"
}
Write-Host "Schema-51 exact enemy defensive-QRF admission, projection, settlement, persistence, marker, and proof contract OK"
$missionConvoyOperationPath = "Scripts/Game/HST/Services/HST_MissionConvoyOperationService.c"
$missionConvoyProofPath = "Scripts/Game/HST/Services/HST_MissionConvoyOperationProofService.c"
$missionConvoySaveValidationPath = "Scripts/Game/HST/Services/HST_MissionConvoySaveValidationService.c"
$missionConvoyP1PolicyPath = "Scripts/Game/HST/Services/HST_MissionConvoyP1Policy.c"
foreach ($schema52Path in @($missionConvoyOperationPath, $missionConvoyProofPath, $missionConvoySaveValidationPath, $missionConvoyP1PolicyPath, $physicalWarPath)) {
	if (!(Test-Path $schema52Path)) {
		throw "Schema-52 exact mission-convoy source is missing: $schema52Path"
	}
}
$missionConvoyOperationText = Get-Content -Raw $missionConvoyOperationPath
$missionConvoyProofText = Get-Content -Raw $missionConvoyProofPath
$missionConvoySaveValidationText = Get-Content -Raw $missionConvoySaveValidationPath
$missionConvoyP1PolicyText = Get-Content -Raw $missionConvoyP1PolicyPath
$physicalWarText = Get-Content -Raw $physicalWarPath
$schema52SaveValidationCorpus = $forceSaveDataText + "`n" + $missionConvoySaveValidationText
$schema52StateCorpus = $operationTypesText + "`n" + $campaignStateText + "`n" + $schema52SaveValidationCorpus
foreach ($requiredSchema52StateEntry in @(
		'SCHEMA_VERSION = 58',
		'HST_OPERATION_TYPE_MISSION_CONVOY',
		'HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED',
		'class HST_ConvoyElementState',
		'int m_iLastNormalizedRestoreSequence = -1;',
		'string m_sAssignedVehicleSlotId;',
		'string m_sConvoyElementId;',
		'ref array<ref HST_ConvoyElementState> m_aConvoyElements = {};',
		'HST_ConvoyElementState FindConvoyElement(string elementId)',
		'class HST_MissionConvoySaveValidationService',
		'NormalizeSchema52MissionConvoyAuthority(restoredSchemaVersion);',
		'ValidateSchema52MissionConvoyRestore',
		'NormalizeSchema52MissionConvoyDerivedGroupSurvivors',
		'QuarantineSchema52MissionConvoy',
		'CountSchema52MissionConvoyMissionsByAnyIdentity',
		'CountSchema52MissionConvoyOperationsByAnyIdentity',
		'CountSchema52MissionConvoyManifestsByAnyIdentity',
		'CountSchema52MissionConvoyBatchesByAnyIdentity',
		'migration_schema52_mission_convoy_authority',
		'legacy_mission_convoys_preserved_contract_zero',
		'normalization_schema52_mission_convoy_authority_conflict'
	)) {
	if ($schema52StateCorpus -notmatch [regex]::Escape($requiredSchema52StateEntry)) {
		throw "Schema-52 mission-convoy state, migration, or restore contract missing: $requiredSchema52StateEntry"
	}
}
$schema52SaveValidationDelegatePattern = 'HST_MissionConvoySaveValidationService\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*new\s+HST_MissionConvoySaveValidationService\s*\(\s*\)\s*;\s*\1\.Normalize\s*\(\s*this\s*,\s*restoredSchemaVersion\s*\)\s*;'
$schema52SaveValidationDelegateMatch = [regex]::Match($forceSaveDataText, $schema52SaveValidationDelegatePattern)
if (!$schema52SaveValidationDelegateMatch.Success) {
	throw "Schema-52 campaign-save migration must delegate exact mission-convoy normalization to HST_MissionConvoySaveValidationService"
}
foreach ($requiredSchema52CopyEntry in @(
		'target.m_iLastNormalizedRestoreSequence = source.m_iLastNormalizedRestoreSequence;',
		'target.m_sOperationId = source.m_sOperationId;',
		'target.m_sManifestId = source.m_sManifestId;',
		'target.m_sSpawnResultId = source.m_sSpawnResultId;',
		'target.m_iOperationContractVersion = source.m_iOperationContractVersion;',
		'target.m_sAssignedVehicleSlotId = source.m_sAssignedVehicleSlotId;',
		'target.m_sConvoyElementId = source.m_sConvoyElementId;',
		'protected HST_ConvoyElementState CopyConvoyElement',
		'target.m_iSurvivingCrewCount = source.m_iSurvivingCrewCount;',
		'target.m_fVehicleDamageFraction = source.m_fVehicleDamageFraction;',
		'target.m_fFuelFraction = source.m_fFuelFraction;',
		'target.m_fAmmoFraction = source.m_fAmmoFraction;',
		'target.m_eDisposition = source.m_eDisposition;',
		'target.m_bPhysicalized = source.m_bPhysicalized;',
		'target.m_bMobile = source.m_bMobile;'
	)) {
	if ($forceSaveDataText -notmatch [regex]::Escape($requiredSchema52CopyEntry)) {
		throw "Schema-52 mission-convoy deep copy missing exact authority: $requiredSchema52CopyEntry"
	}
}
$schema52MigrationStart = $missionConvoySaveValidationText.IndexOf('protected void NormalizeSchema52MissionConvoyAuthority')
$schema52MigrationEnd = $missionConvoySaveValidationText.IndexOf('static bool IsSchema52MissionConvoyMissionClaimant', $schema52MigrationStart)
if ($schema52MigrationStart -lt 0 -or $schema52MigrationEnd -le $schema52MigrationStart) {
	throw "Schema-52 mission-convoy migration and current-schema validation block is missing"
}
$schema52MigrationBlock = $missionConvoySaveValidationText.Substring($schema52MigrationStart, $schema52MigrationEnd - $schema52MigrationStart)
foreach ($requiredSchema52MigrationEntry in @(
		'restoredSchemaVersion < 52',
		'legacyMission.m_iOperationContractVersion = 0;',
		'legacyMission.m_sOperationId = "";',
		'legacyMission.m_sManifestId = "";',
		'legacyMission.m_sSpawnResultId = "";',
		'ValidateSchema52MissionConvoyRestore',
		'NormalizeSchema52MissionConvoyDerivedGroupSurvivors',
		'QuarantineSchema52MissionConvoy'
	)) {
	if ($schema52MigrationBlock -notmatch [regex]::Escape($requiredSchema52MigrationEntry)) {
		throw "Schema-52 migration must preserve pre-exact convoys at contract zero and validate current exact authority: $requiredSchema52MigrationEntry"
	}
}
$schema52RestoreValidateIndex = $schema52MigrationBlock.IndexOf('ValidateSchema52MissionConvoyRestore')
$schema52DerivedNormalizeIndex = $schema52MigrationBlock.IndexOf('NormalizeSchema52MissionConvoyDerivedGroupSurvivors', $schema52RestoreValidateIndex)
$schema52RestoreQuarantineIndex = $schema52MigrationBlock.IndexOf('QuarantineSchema52MissionConvoy', $schema52RestoreValidateIndex)
if ($schema52RestoreValidateIndex -lt 0 -or $schema52DerivedNormalizeIndex -le $schema52RestoreValidateIndex -or
	$schema52RestoreQuarantineIndex -le $schema52DerivedNormalizeIndex) {
	throw "Schema-52 restore must validate exact authority before normalizing derivative group survivors and quarantine only invalid claimants"
}
$schema52ValidateElementsStart = $missionConvoySaveValidationText.IndexOf('protected string ValidateSchema52MissionConvoyElements')
$schema52ValidateElementsEnd = $missionConvoySaveValidationText.IndexOf('protected string ValidateSchema52MissionConvoyCargo', $schema52ValidateElementsStart)
if ($schema52ValidateElementsStart -lt 0 -or $schema52ValidateElementsEnd -le $schema52ValidateElementsStart) {
	throw "Schema-52 exact mission-convoy element restore validator is missing"
}
$schema52ValidateElementsBlock = $missionConvoySaveValidationText.Substring($schema52ValidateElementsStart, $schema52ValidateElementsEnd - $schema52ValidateElementsStart)
if ($schema52ValidateElementsBlock -notmatch [regex]::Escape('CountSchema52LivingMemberSlotsForGroup') -or
	$schema52ValidateElementsBlock -match 'group\.m_i(?:InfantryCount|LastSeenAliveCount|SurvivorInfantryCount|DurableLivingInfantryCount)\s*!=\s*element\.m_iSurvivingCrewCount') {
	throw "Schema-52 restore must validate casualties from exact member slots plus convoy elements, then normalize stale derivative group counters"
}
$schema52ManifestRestoreBlock = Get-ScriptMethodBlock $missionConvoySaveValidationText 'protected string ValidateSchema52MissionConvoyManifest('
if ([string]::IsNullOrEmpty($schema52ManifestRestoreBlock)) {
	throw "Schema-52 exact mission-convoy frozen manifest restore validator is missing"
}
foreach ($requiredSchema52ManifestRestoreEntry in @(
		'BuildMemberSlotId(mission, ordinal, seatIndex)',
		'memberSlot.m_iOrdinal == expectedMemberOrdinal',
		'memberSlot.m_iSeatIndex == seatIndex',
		'seatIndex == 0 && memberSlot.m_sSeatRole != "driver"',
		'seatIndex > 0 && memberSlot.m_sSeatRole != "passenger"'
	)) {
	if ($schema52ManifestRestoreBlock -notmatch [regex]::Escape($requiredSchema52ManifestRestoreEntry)) {
		throw "Schema-52 restore must validate deterministic member-slot, ordinal, and seat identity after verifying the hash: $requiredSchema52ManifestRestoreEntry"
	}
}
$schema52SettlementRestoreBlock = Get-ScriptMethodBlock $missionConvoySaveValidationText 'protected string ValidateSchema52MissionConvoySettlement('
$schema52ArrivalEvidenceBlock = Get-ScriptMethodBlock $missionConvoySaveValidationText 'protected bool HasSchema52SettledArrivalEvidence('
if ([string]::IsNullOrEmpty($schema52SettlementRestoreBlock) -or [string]::IsNullOrEmpty($schema52ArrivalEvidenceBlock)) {
	throw "Schema-52 settled-arrival restore evidence validator is missing"
}
foreach ($requiredSchema52ArrivalRestoreEntry in @(
		'SCHEMA52_CONVOY_FAILURE_PHASE',
		'SCHEMA52_CONVOY_FAILURE_EVENT',
		'Convoy reached its destination:',
		'operation.m_vRouteEndPosition',
		'HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED'
	)) {
	if ($schema52ArrivalEvidenceBlock -notmatch [regex]::Escape($requiredSchema52ArrivalRestoreEntry)) {
		throw "Schema-52 completed-arrival restore must require durable phase/event/reason/route/element evidence: $requiredSchema52ArrivalRestoreEntry"
	}
}
if ($schema52SettlementRestoreBlock -notmatch [regex]::Escape('HasSchema52SettledArrivalEvidence(mission, operation)')) {
	throw "Schema-52 completed settlement restore must invoke the durable arrival-evidence validator"
}
$schema52ProjectionRestoreBlock = Get-ScriptMethodBlock $missionConvoySaveValidationText 'protected string ValidateSchema52MissionConvoyProjection('
$schema52BatchRestoreBlock = Get-ScriptMethodBlock $missionConvoySaveValidationText 'protected string ValidateSchema52MissionConvoyBatch('
if ([string]::IsNullOrEmpty($schema52ProjectionRestoreBlock) -or [string]::IsNullOrEmpty($schema52BatchRestoreBlock)) {
	throw "Schema-52 exact mission-convoy projection or roster restore validator is missing"
}
foreach ($requiredSchema52OpenStateEntry in @(
		'HST_MissionConvoyP1Policy.IsOpenDutyPair',
		'HST_MissionConvoyP1Policy.IsProjectionPair',
		'HST_OPERATION_DUTY_STAGING',
		'HST_OPERATION_DUTY_OUTBOUND',
		'HST_OPERATION_DUTY_ON_STATION',
		'HST_OPERATION_MATERIALIZATION_RETIRING',
		'HST_OPERATION_MATERIALIZATION_RETIRED'
	)) {
	if (($missionConvoyP1PolicyText + "`n" + $schema52ProjectionRestoreBlock + "`n" + $schema52SettlementRestoreBlock) -notmatch [regex]::Escape($requiredSchema52OpenStateEntry)) {
		throw "Schema-52 OPEN convoy restore must enumerate legal duty/resume and projection cross-products: $requiredSchema52OpenStateEntry"
	}
}
foreach ($requiredSchema52RootRosterEntry in @(
		'non-member root contains casualty authority',
		'root is retired or unregistered',
		'slot.m_sSlotKind == "group"',
		'slot.m_sSlotKind == "vehicle" || slot.m_sSlotKind == "asset"',
		'!slot.m_bAliveVerified || !slot.m_bEverAlive'
	)) {
	if ($schema52BatchRestoreBlock -notmatch [regex]::Escape($requiredSchema52RootRosterEntry)) {
		throw "Schema-52 OPEN convoy restore must reserve casualty tombstones for member slots and validate live roots: $requiredSchema52RootRosterEntry"
	}
}
$schema52GenericRestoreBlock = Get-ScriptMethodBlock $forceSaveDataText 'protected void NormalizeRestoredOperationProjectionState('
if ([string]::IsNullOrEmpty($schema52GenericRestoreBlock)) {
	throw "Schema-52 generic projection restore block is missing"
}
$schema52MissionConvoySkipIndex = $schema52GenericRestoreBlock.IndexOf('operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY')
$schema52GenericArrivalResetIndex = $schema52GenericRestoreBlock.IndexOf('operation.m_iArrivalConfirmationCount = 0;')
if ($schema52MissionConvoySkipIndex -lt 0 -or $schema52GenericArrivalResetIndex -lt 0 -or $schema52MissionConvoySkipIndex -gt $schema52GenericArrivalResetIndex) {
	throw "Schema-52 exact mission convoys must bypass the single-root generic projection restore normalizer"
}

foreach ($requiredSchema52OperationEntry in @(
		'class HST_MissionConvoyOperationService',
		'EXACT_CONTRACT_VERSION = 1',
		'QUARANTINED_CONTRACT_VERSION = -52',
		'EXACT_VEHICLE_COUNT = 3',
		'EXACT_MATERIALIZE_IN_RADIUS_METERS',
		'EXACT_MATERIALIZE_OUT_RADIUS_METERS',
		'EXACT_ARRIVAL_RADIUS_METERS',
		'PrepareNewMissionContract',
		'AdmitNewMission',
		'HST_MissionConvoyP1Policy.ValidateAdmissionCargo(state, mission)',
		'TickBeforePhysical',
		'TickAfterPhysical',
		'TickAfterOutcomes',
		'ReconcileAfterRestore',
		'ReconcileSettledRuntimeCleanup',
		'NormalizeRestoredProjection',
		'ResolveAuthority',
		'QuarantineAmbiguousAuthority',
		'CountOperationClaimants',
		'CountManifestClaimants',
		'CountBatchClaimants',
		'CountElementClaimants',
		'CountElementIdentityClaimants',
		'CountAssetIdentityClaimants',
		'CountGroupIdentityClaimants',
		'CountCargoIdentityClaimants',
		'authority quarantined without mutating ambiguous operation, manifest, batch, group, asset, or element rows',
		'IsArrivalPositionConfirmed',
		'EnterRecoveryHold',
		'HasPendingRecoveryOutcome'
	)) {
	if ($missionConvoyOperationText -notmatch [regex]::Escape($requiredSchema52OperationEntry)) {
		throw "Schema-52 exact mission-convoy operation authority missing: $requiredSchema52OperationEntry"
	}
}
$schema52AdmissionCargoBlock = Get-ScriptMethodBlock $missionConvoyP1PolicyText 'static string ValidateAdmissionCargo('
$schema52CargoContractBlock = Get-ScriptMethodBlock $missionConvoyP1PolicyText 'static string ValidateCargoContract('
$schema52ExpectedCargoContractBlock = Get-ScriptMethodBlock $missionConvoyP1PolicyText 'protected static int ResolveExpectedCargoContract('
$schema52CargoPrefabContractBlock = Get-ScriptMethodBlock $missionConvoyP1PolicyText 'protected static string ValidateCargoPrefabContract('
if ([string]::IsNullOrEmpty($schema52AdmissionCargoBlock) -or
	[string]::IsNullOrEmpty($schema52CargoContractBlock) -or
	[string]::IsNullOrEmpty($schema52ExpectedCargoContractBlock) -or
	[string]::IsNullOrEmpty($schema52CargoPrefabContractBlock)) {
	throw "Schema-52 exact mission-convoy cargo admission validator is missing"
}
foreach ($requiredSchema52AdmissionDelegateEntry in @(
		'asset.m_sRole == HST_MissionConvoyOperationService.VEHICLE_ROLE',
		'return ValidateCargoContract(mission, cargo, cargoRowCount);'
	)) {
	if ($schema52AdmissionCargoBlock -notmatch [regex]::Escape($requiredSchema52AdmissionDelegateEntry)) {
		throw "Schema-52 exact mission-convoy admission must enumerate cargo rows and delegate to the complete cargo contract: $requiredSchema52AdmissionDelegateEntry"
	}
}
$schema52CargoPolicyCorpus = $schema52CargoContractBlock + "`n" + $schema52ExpectedCargoContractBlock + "`n" + $schema52CargoPrefabContractBlock
foreach ($requiredSchema52CargoContractEntry in @(
		'mission.m_sRuntimeType != HST_MissionConvoyOperationService.EXACT_RUNTIME_TYPE',
		'cargoRowCount > 1',
		'ResolveExpectedCargoContract(mission.m_sMissionId, expectedRole, expectedKind)',
		'cargo.m_sRole != expectedRole || cargo.m_sKind != expectedKind',
		'missionId == "convoy_money" || missionId == "convoy_supplies"',
		'missionId == "convoy_prisoners"',
		'missionId == "convoy_ammo" || missionId == "convoy_armored"',
		'missionId == "convoy_reinforcements"',
		'expectedRole = HST_MissionConvoyOperationService.PAYLOAD_ROLE',
		'expectedKind = HST_MissionConvoyOperationService.CARGO_KIND',
		'expectedRole = HST_MissionConvoyOperationService.CAPTIVE_ROLE',
		'expectedKind = HST_MissionConvoyOperationService.CAPTIVE_KIND',
		'Resource.Load(cargoResourceName)',
		'SCR_BaseContainerTools.FindEntitySource(cargoResource)',
		'cargoSource.GetClassName().ToType()',
		'SCR_BaseContainerTools.FindComponentSource(cargoSource, HST_MissionAssetComponent)',
		'cargoType.IsInherited(SCR_ChimeraCharacter)',
		'SCR_BaseContainerTools.FindComponentSource(cargoSource, SCR_CompartmentAccessComponent)',
		'expectedRole == HST_MissionConvoyOperationService.CAPTIVE_ROLE',
		'exact mission convoy payload prefab must be a non-character mission-asset entity'
	)) {
	if ($schema52CargoPolicyCorpus -notmatch [regex]::Escape($requiredSchema52CargoContractEntry)) {
		throw "Schema-52 exact mission-convoy admission/restore cargo contract is incomplete: $requiredSchema52CargoContractEntry"
	}
}
$schema52CaptiveBoardingBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool TryMoveCaptiveIntoVehicle('
if ([string]::IsNullOrEmpty($schema52CaptiveBoardingBlock) -or
	$schema52CaptiveBoardingBlock -notmatch [regex]::Escape('captiveEntity.FindComponent(SCR_CompartmentAccessComponent)')) {
	throw "Schema-52 captive cargo policy must match the runtime boarding component contract"
}
$schema52RestoreCargoContractBlock = Get-ScriptMethodBlock $missionConvoySaveValidationText 'protected string ValidateSchema52MissionConvoyCargo('
if ([string]::IsNullOrEmpty($schema52RestoreCargoContractBlock)) {
	throw "Schema-52 exact mission-convoy restore cargo validator is missing"
}
$schema52RestoreCargoPolicyCorpus = $schema52RestoreCargoContractBlock + "`n" + $schema52ManifestRestoreBlock
foreach ($requiredSchema52RestoreCargoContractEntry in @(
		'candidate.m_sRole == HST_MissionConvoyOperationService.VEHICLE_ROLE',
		'HST_MissionConvoyP1Policy.ValidateCargoContract(',
		'cargoAsset != contractCargo',
		'cargoAsset.m_sKind != assetSlot.m_sKind',
		'assetSlot.m_sKind == HST_MissionConvoyOperationService.CARGO_KIND',
		'assetSlot.m_sKind == HST_MissionConvoyOperationService.CAPTIVE_KIND'
	)) {
	if ($schema52RestoreCargoPolicyCorpus -notmatch [regex]::Escape($requiredSchema52RestoreCargoContractEntry)) {
		throw "Schema-52 restore must reapply the complete mission cargo contract before accepting durable backlinks: $requiredSchema52RestoreCargoContractEntry"
	}
}
$schema52OperationalGroupBlock = Get-ScriptMethodBlock $campaignStateText 'bool IsOperationalActiveGroup('
$schema52CombatGroupBlock = Get-ScriptMethodBlock $campaignStateText 'bool IsCombatPresentActiveGroup('
if ([string]::IsNullOrEmpty($schema52OperationalGroupBlock) -or [string]::IsNullOrEmpty($schema52CombatGroupBlock)) {
	throw "Schema-52 exact-convoy operational/archive group policy is missing"
}
foreach ($requiredSchema52OperationalGroupEntry in @(
		'missionClaimants != 1',
		'operationClaimants != 1',
		'groupClaimants != 1',
		'elementClaimants != 1',
		'HST_OPERATION_SETTLEMENT_OPEN',
		'HST_OPERATION_TERMINAL_NONE',
		'mission.m_iOperationContractVersion == 1',
		'operation.m_iContractVersion == 1'
	)) {
	if ($schema52OperationalGroupBlock -notmatch [regex]::Escape($requiredSchema52OperationalGroupEntry)) {
		throw "Schema-52 operational group policy must reject settled, quarantined, legacy, and ambiguous exact roots: $requiredSchema52OperationalGroupEntry"
	}
}
foreach ($requiredSchema52CombatGroupEntry in @(
		'IsOperationalActiveGroup(group)',
		'element.m_iSurvivingCrewCount > 0',
		'HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED',
		'HST_CONVOY_ELEMENT_DISPOSITION_RETIRED'
	)) {
	if ($schema52CombatGroupBlock -notmatch [regex]::Escape($requiredSchema52CombatGroupEntry)) {
		throw "Schema-52 combat-presence policy must exclude settled/quarantined and crewless recovery archives: $requiredSchema52CombatGroupEntry"
	}
}
$schema52OperationalConsumerContracts = @{
	"Scripts/Game/HST/Services/HST_ZoneCaptureService.c" = 'state.IsCombatPresentActiveGroup(activeGroup)'
	"Scripts/Game/HST/Services/HST_CivilianService.c" = 'state.IsCombatPresentActiveGroup(group)'
	"Scripts/Game/HST/Services/HST_HQService.c" = 'state.IsCombatPresentActiveGroup(group)'
	"Scripts/Game/HST/Services/HST_SpawnPlacementService.c" = 'state.IsOperationalActiveGroup(activeGroup)'
	"Scripts/Game/HST/Services/HST_MissionRuntimeService.c" = 'state.IsCombatPresentActiveGroup(group)'
	"Scripts/Game/HST/Services/HST_CommandUIService.c" = 'state.IsOperationalActiveGroup(activeGroup)'
	"Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c" = 'm_State.IsOperationalActiveGroup(activeGroup)'
}
foreach ($schema52OperationalConsumerPath in $schema52OperationalConsumerContracts.Keys) {
	$schema52OperationalConsumerText = Get-Content -Raw $schema52OperationalConsumerPath
	$schema52OperationalConsumerToken = $schema52OperationalConsumerContracts[$schema52OperationalConsumerPath]
	if ($schema52OperationalConsumerText -notmatch [regex]::Escape($schema52OperationalConsumerToken)) {
		throw "Schema-52 generic consumer must use the exact operational/archive policy: $schema52OperationalConsumerPath -> $schema52OperationalConsumerToken"
	}
}
$schema52RestoreStart = $missionConvoyOperationText.IndexOf('protected bool NormalizeRestoredProjection')
$schema52RestoreEnd = $missionConvoyOperationText.IndexOf('protected bool AdvanceVirtualRoute', $schema52RestoreStart)
if ($schema52RestoreStart -lt 0 -or $schema52RestoreEnd -le $schema52RestoreStart) {
	throw "Schema-52 exact mission-convoy restore projection block is missing"
}
$schema52RestoreBlock = $missionConvoyOperationText.Substring($schema52RestoreStart, $schema52RestoreEnd - $schema52RestoreStart)
foreach ($requiredSchema52RestoreEntry in @(
		'm_iPersistenceRestoreSequence',
		'm_iLastNormalizedRestoreSequence',
		'NormalizeExactMissionConvoyRuntimeForRestore',
		'HST_OPERATION_MATERIALIZATION_VIRTUAL',
		'HST_OPERATION_POSITION_STRATEGIC',
		'SetElementsPhysicalized(state, mission, false)',
		'ProjectStrategicState'
	)) {
	if ($schema52RestoreBlock -notmatch [regex]::Escape($requiredSchema52RestoreEntry)) {
		throw "Schema-52 restore must normalize exact convoy runtime once while preserving strategic authority: $requiredSchema52RestoreEntry"
	}
}
$schema52PhysicalRestoreStart = $physicalWarText.IndexOf('bool NormalizeExactMissionConvoyRuntimeForRestore')
$schema52PhysicalRestoreEnd = $physicalWarText.IndexOf('bool FoldExactMissionConvoyRuntime', $schema52PhysicalRestoreStart)
if ($schema52PhysicalRestoreStart -lt 0 -or $schema52PhysicalRestoreEnd -le $schema52PhysicalRestoreStart) {
	throw "Schema-52 exact mission-convoy physical restore normalizer is missing"
}
$schema52PhysicalRestoreBlock = $physicalWarText.Substring($schema52PhysicalRestoreStart, $schema52PhysicalRestoreEnd - $schema52PhysicalRestoreStart)
foreach ($requiredSchema52PhysicalRestoreEntry in @(
		'IsExactMissionConvoyRecoveryHold',
		'element.m_iSurvivingCrewCount',
		'activeGroup.m_iInfantryCount',
		'activeGroup.m_iSurvivorInfantryCount',
		'activeGroup.m_iDurableLivingInfantryCount',
		'asset.m_bSpawned = false;',
		'element.m_bPhysicalized = false;'
	)) {
	if ($schema52PhysicalRestoreBlock -notmatch [regex]::Escape($requiredSchema52PhysicalRestoreEntry)) {
		throw "Schema-52 physical restore must rebind groups from durable element survivors without resurrecting process-local handles: $requiredSchema52PhysicalRestoreEntry"
	}
}
$schema52PendingArrivalRestoreBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool IsValidatedExactMissionConvoyPendingArrivalRestore('
if ([string]::IsNullOrEmpty($schema52PendingArrivalRestoreBlock)) {
	throw "Schema-52 pending-arrival restore evidence classifier is missing"
}
foreach ($requiredSchema52PendingArrivalRestoreEntry in @(
		'MISSION_CONVOY_FAILED',
		'CONVOY_FAIL_EVENT_KEY',
		'Convoy reached its destination:',
		'HST_OPERATION_SETTLEMENT_OPEN',
		'operation.m_fRouteTotalDistanceMeters <= 0.0',
		'IsZeroVector(operation.m_vRouteEndPosition)',
		'HST_MissionConvoyOperationService.EXACT_ARRIVAL_RADIUS_METERS',
		'HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED'
	)) {
	if ($schema52PendingArrivalRestoreBlock -notmatch [regex]::Escape($requiredSchema52PendingArrivalRestoreEntry)) {
		throw "Schema-52 pending-arrival restore must require exact durable arrival evidence: $requiredSchema52PendingArrivalRestoreEntry"
	}
}
foreach ($requiredSchema52RawRestoreCleanupEntry in @(
		'EXACT_MISSION_CONVOY_VEHICLE_COUNT',
		'RemoveExactMissionConvoyOutboundProjectionTransaction',
		'ClearPendingActiveGroupPopulation',
		'DeleteRuntimeGroupEntity',
		'CountExactMissionConvoyMemberMappings(mission.m_sInstanceId) != 0',
		'GetRuntimeCrewGroupEntity(verifyGroup.m_sGroupId)',
		'GetRuntimeVehicleEntity(verifyGroup.m_sGroupId)'
	)) {
	if ($schema52PhysicalRestoreBlock -notmatch [regex]::Escape($requiredSchema52RawRestoreCleanupEntry)) {
		throw "Schema-52 physical restore must clear and verify every process-local exact root before durable normalization: $requiredSchema52RawRestoreCleanupEntry"
	}
}
$schema52RestoreDeleteIndex = $schema52PhysicalRestoreBlock.IndexOf('DeleteRuntimeGroupEntity(cleanupGroup.m_sGroupId)')
$schema52RestoreRawVerifyIndex = $schema52PhysicalRestoreBlock.IndexOf('GetRuntimeCrewGroupEntity(verifyGroup.m_sGroupId)', $schema52RestoreDeleteIndex)
$schema52RestoreDurableRewriteIndex = $schema52PhysicalRestoreBlock.IndexOf('activeGroup.m_iOriginalInfantryCount', $schema52RestoreRawVerifyIndex)
if ($schema52RestoreDeleteIndex -lt 0 -or $schema52RestoreRawVerifyIndex -le $schema52RestoreDeleteIndex -or
	$schema52RestoreDurableRewriteIndex -le $schema52RestoreRawVerifyIndex) {
	throw "Schema-52 restore must prove raw runtime absence before rewriting durable process flags"
}

$schema52DocumentationPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/MIGRATIONS.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md",
	"docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md",
	"docs/HST_ENFUSION_ENFORCE_NOTES.md"
)
foreach ($schema52DocumentationPath in $schema52DocumentationPaths) {
	$schema52DocumentationText = Get-Content -Raw $schema52DocumentationPath
	if ($schema52DocumentationText -notmatch '(?is)(?:schema[- ]52.{0,500}(?:mission[- ]convoy|convoy[- ]mission)|(?:mission[- ]convoy|convoy[- ]mission).{0,500}schema[- ]52)') {
		throw "$schema52DocumentationPath must describe the current schema-52 mission-convoy boundary"
	}
}
if ($migrationsText -notmatch '(?is)##\s+Schema\s+52\b[\s\S]*?(?:pre-schema-52|historical\s+convoy)[\s\S]*?contract\s+version\s+`?0`?' -or
	$migrationsText -notmatch '(?is)##\s+Schema\s+52\b[\s\S]*?(?:does not|never|no)\s+(?:invent|infer|create)[\s\S]{0,240}(?:operation|authority|manifest|element)') {
	throw "Schema-52 migration notes must state that historical convoys remain contract version 0 without invented exact authority"
}

$schema52ManifestBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected HST_ForceManifestState BuildManifest('
if ([string]::IsNullOrEmpty($schema52ManifestBlock)) {
	throw "Schema-52 exact mission-convoy manifest builder is missing"
}
foreach ($requiredSchema52ManifestEntry in @(
		'manifest.m_bFrozen = true;',
		'manifest.m_iRequestedVehicleCount = EXACT_VEHICLE_COUNT;',
		'manifest.m_iAcceptedVehicleCount = EXACT_VEHICLE_COUNT;',
		'catalogGroup.m_aMemberSlots',
		'member.m_sCatalogSlotId = catalogMember.m_sSlotId;',
		'member.m_sPrefab = catalogMember.m_sPrefab;',
		'member.m_sRole = catalogMember.m_sRole;',
		'member.m_sAssignedVehicleSlotId = vehicleSlot.m_sSlotId;',
		'manifest.m_aMembers.Insert(member);',
		'assetSlot.m_sAssignedVehicleSlotId = BuildVehicleSlotId(mission, 0);',
		'manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);'
	)) {
	if ($schema52ManifestBlock -notmatch [regex]::Escape($requiredSchema52ManifestEntry)) {
		throw "Schema-52 exact mission-convoy frozen manifest/member-slot contract missing: $requiredSchema52ManifestEntry"
	}
}
if ($schema52ManifestBlock -match '\.Compose\s*\(' -or $schema52ManifestBlock -match 'SelectMissionConvoyVehiclePrefab\s*\(') {
	throw "Schema-52 exact mission-convoy admission must freeze planned vehicle/member slots without runtime recomposition or vehicle reselection"
}
foreach ($requiredSchema52RouteContractEntry in @(
		'string m_sRouteContractHash;',
		'target.m_sRouteContractHash = source.m_sRouteContractHash;',
		'operation.m_sRouteContractHash = BuildRouteContractHash(route, routePositions);',
		'static string BuildRouteContractHash(HST_GeneratedRouteState route, array<vector> positions)',
		'operation.m_sRouteContractHash != expectedRouteHash'
	)) {
	if (($campaignStateText + "`n" + $schema52SaveValidationCorpus + "`n" + $missionConvoyOperationText) -notmatch [regex]::Escape($requiredSchema52RouteContractEntry)) {
		throw "Schema-52 exact mission-convoy frozen route contract missing: $requiredSchema52RouteContractEntry"
	}
}

$schema52FrozenPrefabResolverBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TryResolveExactMissionConvoyFrozenVehiclePrefab('
$schema52ConvoySpawnBlock = Get-ScriptMethodBlock $physicalWarText 'protected GenericEntity SpawnMissionConvoyVehicle('
if ([string]::IsNullOrEmpty($schema52FrozenPrefabResolverBlock) -or [string]::IsNullOrEmpty($schema52ConvoySpawnBlock)) {
	throw "Schema-52 frozen vehicle-prefab resolver or spawn boundary is missing"
}
foreach ($requiredSchema52FrozenPrefabEntry in @(
		'manifest.m_bFrozen',
		'vehicle.m_sSlotId != element.m_sVehicleSlotId',
		'matches != 1',
		'frozenVehicle.m_sPrefab != element.m_sVehiclePrefab',
		'frozenVehicle.m_sPrefab != asset.m_sPrefab',
		'prefab = frozenVehicle.m_sPrefab;'
	)) {
	if ($schema52FrozenPrefabResolverBlock -notmatch [regex]::Escape($requiredSchema52FrozenPrefabEntry)) {
		throw "Schema-52 exact mission-convoy frozen vehicle-prefab authority missing: $requiredSchema52FrozenPrefabEntry"
	}
}
if ($schema52ConvoySpawnBlock -notmatch '(?s)if\s*\(exactContract\).*?TryResolveExactMissionConvoyFrozenVehiclePrefab.*?frozenPrefab\s*!=\s*vehiclePrefab.*?vehiclePrefab\s*=\s*frozenPrefab;.*?else\s*\{.*?SelectMissionConvoyVehiclePrefab') {
	throw "Schema-52 exact mission convoys must spawn the frozen manifest prefab; runtime vehicle selection is legacy-only"
}
if ($schema52ConvoySpawnBlock -notmatch '(?s)if\s*\(!exactContract\)\s*asset\.m_sPrefab\s*=\s*vehiclePrefab;') {
	throw "Schema-52 exact mission-convoy physicalization must not rewrite the frozen vehicle prefab"
}

$schema52RemainingRouteBlock = Get-ScriptMethodBlock $physicalWarText 'protected ref array<vector> BuildRemainingMissionConvoyRouteWaypoints('
$schema52GroupWaypointsBlock = Get-ScriptMethodBlock $physicalWarText 'protected ref array<vector> BuildMissionConvoyGroupWaypointPositions('
if ([string]::IsNullOrEmpty($schema52RemainingRouteBlock) -or [string]::IsNullOrEmpty($schema52GroupWaypointsBlock)) {
	throw "Schema-52 forward/current-cursor convoy waypoint builder is missing"
}
foreach ($requiredSchema52ForwardRouteEntry in @(
		'ClosestPointOnSegment2D(routeWaypoints[index - 1], routeWaypoints[index], currentPosition)',
		'int firstForwardIndex = 1;',
		'for (int forwardIndex = firstForwardIndex; forwardIndex < routeWaypoints.Count(); forwardIndex++)',
		'remaining.Insert(routeWaypoints[forwardIndex]);'
	)) {
	if ($schema52RemainingRouteBlock -notmatch [regex]::Escape($requiredSchema52ForwardRouteEntry)) {
		throw "Schema-52 convoy route must resume from the current route cursor: $requiredSchema52ForwardRouteEntry"
	}
}
foreach ($requiredSchema52CurrentCursorEntry in @(
		'currentPosition = vehicleEntity.GetOrigin();',
		'BuildRemainingMissionConvoyRouteWaypoints(currentPosition, routeWaypoints)',
		'AppendConvoyLeadInWaypoints(result, currentPosition, remainingRouteWaypoints[0]'
	)) {
	if ($schema52GroupWaypointsBlock -notmatch [regex]::Escape($requiredSchema52CurrentCursorEntry)) {
		throw "Schema-52 physical route assignment must begin at the current live cursor: $requiredSchema52CurrentCursorEntry"
	}
}
if ($schema52GroupWaypointsBlock -match 'AppendConvoyLeadInWaypoints\s*\(\s*result\s*,\s*activeGroup\.m_vSourcePosition') {
	throw "Schema-52 physical route assignment must not send a folded/restored convoy back to its original source"
}

$schema52RecoveryMaterializationBlock = Get-ScriptMethodBlock $physicalWarText 'bool MaterializeExactMissionConvoyRecoveryVehicles('
$schema52RecoveryEligibilityBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool IsExactMissionConvoyRecoveryVehicleEligible('
$schema52CrewEliminationBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool MarkAllCrewsEliminated('
if ([string]::IsNullOrEmpty($schema52RecoveryMaterializationBlock) -or [string]::IsNullOrEmpty($schema52RecoveryEligibilityBlock) -or [string]::IsNullOrEmpty($schema52CrewEliminationBlock)) {
	throw "Schema-52 crewless recovery/no-resurrection boundary is missing"
}
foreach ($requiredSchema52RecoveryEntry in @(
		'IsExactMissionConvoyRecoveryHold(mission)',
		'TryResolveExactMissionConvoyFrozenVehiclePrefab',
		'GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId)',
		'activeGroup.m_iSpawnedAgentCount = 0;',
		'activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;',
		'element.m_bMobile = false;',
		'materialized %1 exact crewless abandoned vehicles'
	)) {
	if ($schema52RecoveryMaterializationBlock -notmatch [regex]::Escape($requiredSchema52RecoveryEntry)) {
		throw "Schema-52 exact mission-convoy crewless recovery contract missing: $requiredSchema52RecoveryEntry"
	}
}
if ($schema52RecoveryMaterializationBlock -match 'TrySpawnActiveGroup\s*\(' -or
	$schema52RecoveryMaterializationBlock -match '\.SpawnUnits\s*\(' -or
	$schema52RecoveryMaterializationBlock -match 'm_iSurvivingCrewCount\s*=') {
	throw "Schema-52 recovery materialization must not create crew or rewrite durable survivor authority"
}
foreach ($requiredSchema52AbandonedEntry in @(
		'HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED',
		'element.m_iSurvivingCrewCount > 0',
		'element.m_bMobile',
		'element.m_fVehicleDamageFraction >= 0.0',
		'element.m_fVehicleDamageFraction < 1.0'
	)) {
	if ($schema52RecoveryEligibilityBlock -notmatch [regex]::Escape($requiredSchema52AbandonedEntry)) {
		throw "Schema-52 recovery must expose only intact, immobile, zero-crew ABANDONED elements: $requiredSchema52AbandonedEntry"
	}
}
foreach ($requiredSchema52NoResurrectionEntry in @(
		'element.m_iSurvivingCrewCount = 0;',
		'HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED',
		'group.m_iInfantryCount = 0;',
		'group.m_iLastSeenAliveCount = 0;',
		'group.m_iSurvivorInfantryCount = 0;',
		'group.m_iDurableLivingInfantryCount = 0;'
	)) {
	if ($schema52CrewEliminationBlock -notmatch [regex]::Escape($requiredSchema52NoResurrectionEntry)) {
		throw "Schema-52 crew elimination must persist zero survivors and ABANDONED vehicle roots: $requiredSchema52NoResurrectionEntry"
	}
}
foreach ($requiredSchema52VehicleConditionEntry in @(
		'ApplyExactMissionConvoyVehicleRuntimeState',
		'SampleExactMissionConvoyVehicleRuntimeState',
		'TrySampleMissionConvoyVehicleDamageFraction',
		'TrySampleMissionConvoyVehicleFuelFraction',
		'TrySampleMissionConvoyVehicleAmmoFraction',
		'ApplyMissionConvoyVehicleAmmoFraction'
	)) {
	if ($physicalWarText -notmatch [regex]::Escape($requiredSchema52VehicleConditionEntry)) {
		throw "Schema-52 exact convoy must preserve vehicle damage/fuel/ammo state across projection: $requiredSchema52VehicleConditionEntry"
	}
}

$schema52RestoreCallIndex = $schema52SaveValidationDelegateMatch.Index
$schema52ActiveGroupNormalizeCallIndex = $forceSaveDataText.IndexOf('NormalizeActiveGroupSourceLinks(restoredSchemaVersion);')
$schema52ForceNormalizeCallIndex = $forceSaveDataText.IndexOf('NormalizeForceAuthority(restoredSchemaVersion);')
$schema52GenericProjectionCallIndex = $forceSaveDataText.IndexOf('NormalizeRestoredOperationProjectionState();')
if ($schema52RestoreCallIndex -lt 0 -or $schema52ActiveGroupNormalizeCallIndex -le $schema52RestoreCallIndex -or
	$schema52ForceNormalizeCallIndex -le $schema52RestoreCallIndex -or $schema52GenericProjectionCallIndex -le $schema52RestoreCallIndex) {
	throw "Schema-52 exact mission-convoy validation/quarantine must run before generic restore normalization"
}
foreach ($requiredSchema52RestoreTypeGate in @(
		'HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyMissionClaimant(this, mission)',
		'HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyRouteClaimant(this, route)',
		'HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyAssetClaimant(this, asset)',
		'HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyGroupClaimant(this, group)',
		'HST_MissionConvoySaveValidationService.HasSchema52MissionConvoyMissionClaimant(this)',
		'HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyManifestClaimant(this, manifest)',
		'HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyBatchClaimant(this, spawnResult)',
		'operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY'
	)) {
	if ($forceSaveDataText -notmatch [regex]::Escape($requiredSchema52RestoreTypeGate)) {
		throw "Schema-52 generic restore normalization is missing an exact mission-convoy type/claimant gate: $requiredSchema52RestoreTypeGate"
	}
}
$schema52RuntimeQuarantineBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool QuarantineAmbiguousAuthority('
$schema52RestoreQuarantineBlock = Get-ScriptMethodBlock $missionConvoySaveValidationText 'protected void QuarantineSchema52MissionConvoy('
if ([string]::IsNullOrEmpty($schema52RuntimeQuarantineBlock) -or [string]::IsNullOrEmpty($schema52RestoreQuarantineBlock)) {
	throw "Schema-52 mission-only authority quarantine boundary is missing"
}
foreach ($schema52QuarantineBlock in @($schema52RuntimeQuarantineBlock, $schema52RestoreQuarantineBlock)) {
	if ($schema52QuarantineBlock -notmatch 'mission\.m_iOperationContractVersion\s*=\s*(?:QUARANTINED_CONTRACT_VERSION|HST_MissionConvoyOperationService\.QUARANTINED_CONTRACT_VERSION)') {
		throw "Schema-52 authority corruption must quarantine the mission contract"
	}
	if ($schema52QuarantineBlock -match '\b(?:state|operation|manifest|batch|group|asset|element|route)\.(?:m_|Find|Settle|Remove|Insert)') {
		throw "Schema-52 authority quarantine must not mutate ambiguous foreign authority rows"
	}
}

$schema52VirtualArrivalStart = $missionConvoyOperationText.IndexOf('protected bool AdvanceVirtualRoute')
$schema52VirtualArrivalEnd = $missionConvoyOperationText.IndexOf('protected bool ProjectStrategicState', $schema52VirtualArrivalStart)
$schema52PhysicalArrivalStart = $missionConvoyOperationText.IndexOf('protected bool TryConfirmPhysicalArrival')
$schema52PhysicalArrivalEnd = $missionConvoyOperationText.IndexOf('protected bool MarkConvoyArrived', $schema52PhysicalArrivalStart)
$schema52ArrivalPositionStart = $missionConvoyOperationText.IndexOf('protected bool IsArrivalPositionConfirmed')
$schema52ArrivalPositionEnd = $missionConvoyOperationText.IndexOf('protected bool SettleOperation', $schema52ArrivalPositionStart)
if ($schema52VirtualArrivalStart -lt 0 -or $schema52VirtualArrivalEnd -le $schema52VirtualArrivalStart -or
	$schema52PhysicalArrivalStart -lt 0 -or $schema52PhysicalArrivalEnd -le $schema52PhysicalArrivalStart -or
	$schema52ArrivalPositionStart -lt 0 -or $schema52ArrivalPositionEnd -le $schema52ArrivalPositionStart) {
	throw "Schema-52 exact mission-convoy endpoint confirmation blocks are missing"
}
$schema52VirtualArrivalBlock = $missionConvoyOperationText.Substring($schema52VirtualArrivalStart, $schema52VirtualArrivalEnd - $schema52VirtualArrivalStart)
$schema52PhysicalArrivalBlock = $missionConvoyOperationText.Substring($schema52PhysicalArrivalStart, $schema52PhysicalArrivalEnd - $schema52PhysicalArrivalStart)
$schema52ArrivalPositionBlock = $missionConvoyOperationText.Substring($schema52ArrivalPositionStart, $schema52ArrivalPositionEnd - $schema52ArrivalPositionStart)
if ($schema52VirtualArrivalBlock -notmatch [regex]::Escape('IsArrivalPositionConfirmed(state, mission, operation)') -or
	$schema52PhysicalArrivalBlock -notmatch [regex]::Escape('IsArrivalPositionConfirmed(state, mission, operation)') -or
	$schema52ArrivalPositionBlock -notmatch [regex]::Escape('operation.m_vRouteEndPosition') -or
	$schema52ArrivalPositionBlock -notmatch [regex]::Escape('DistanceSq2D(carrierPosition, operation.m_vRouteEndPosition)')) {
	throw "Schema-52 exact mission-convoy arrival must require an authoritative carrier position inside the actual route endpoint radius"
}

$schema52SettleOperationBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool SettleOperation('
if ([string]::IsNullOrEmpty($schema52SettleOperationBlock) -or
	$schema52SettleOperationBlock -notmatch '(?s)if\s*\(hasPhysicalRuntime\)\s*\{\s*operation\.m_eMaterializationState\s*=\s*HST_EOperationMaterializationState\.HST_OPERATION_MATERIALIZATION_RETIRING\s*;\s*operation\.m_ePositionAuthority\s*=\s*HST_EOperationPositionAuthority\.HST_OPERATION_POSITION_LIVE\s*;') {
	throw "Schema-52 exact mission-convoy settlement must pair retiring runtime with live position authority"
}

foreach ($requiredSchema52RecoveryEntry in @(
		'MaterializeExactMissionConvoyRecoveryVehicles',
		'IsExactMissionConvoyRecoveryProjectionReady',
		'ValidateExactMissionConvoyRecoveryCarrierAvailability',
		'SyncExactMissionConvoyRecoveryCarrierAssets',
		'IsExactMissionConvoyRecoveryHold',
		'FoldExactMissionConvoyRuntime',
		'HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED',
		'RECOVERY_VEHICLE',
		'STRANDED_VIRTUAL'
	)) {
	if (($physicalWarText + "`n" + $missionConvoyOperationText) -notmatch [regex]::Escape($requiredSchema52RecoveryEntry)) {
		throw "Schema-52 crewless convoy recovery projection missing: $requiredSchema52RecoveryEntry"
	}
}
$schema52ElementTerminalStart = $physicalWarText.IndexOf('protected bool IsExactMissionConvoyElementTerminal')
$schema52ElementTerminalEnd = $physicalWarText.IndexOf('protected void RemoveRestoredMissionConvoyRuntimeRebuildAttempt', $schema52ElementTerminalStart)
if ($schema52ElementTerminalStart -lt 0 -or $schema52ElementTerminalEnd -le $schema52ElementTerminalStart) {
	throw "Schema-52 exact convoy element terminal classifier is missing"
}
$schema52ElementTerminalBlock = $physicalWarText.Substring($schema52ElementTerminalStart, $schema52ElementTerminalEnd - $schema52ElementTerminalStart)
if ($schema52ElementTerminalBlock -notmatch [regex]::Escape('HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE') -or
	$schema52ElementTerminalBlock -notmatch [regex]::Escape('HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED')) {
	throw "Schema-52 ABANDONED convoy elements must remain recoverable rather than terminal"
}
$schema52RecoveryMaterializeStart = $physicalWarText.IndexOf('bool MaterializeExactMissionConvoyRecoveryVehicles')
$schema52RecoveryMaterializeEnd = $physicalWarText.IndexOf('bool IsExactMissionConvoyRecoveryProjectionReady', $schema52RecoveryMaterializeStart)
if ($schema52RecoveryMaterializeStart -lt 0 -or $schema52RecoveryMaterializeEnd -le $schema52RecoveryMaterializeStart) {
	throw "Schema-52 crewless recovery materialization block is missing"
}
$schema52RecoveryMaterializeBlock = $physicalWarText.Substring($schema52RecoveryMaterializeStart, $schema52RecoveryMaterializeEnd - $schema52RecoveryMaterializeStart)
foreach ($requiredSchema52RecoveryMaterializeEntry in @(
		'TryResolveExactMissionConvoyFrozenVehiclePrefab',
		'SpawnMissionConvoyVehicle',
		'RollbackExactMissionConvoyRecoveryVehicles',
		'm_aRuntimeVehicleGroupIds.Insert',
		'm_aRuntimeVehicleEntities.Insert',
		'activeGroup.m_iSpawnedAgentCount = 0;',
		'element.m_bPhysicalized = true;',
		'SyncExactMissionConvoyRecoveryCarrierAssets'
	)) {
	if ($schema52RecoveryMaterializeBlock -notmatch [regex]::Escape($requiredSchema52RecoveryMaterializeEntry)) {
		throw "Schema-52 crewless recovery must rematerialize the frozen vehicle roots atomically without inventing crew: $requiredSchema52RecoveryMaterializeEntry"
	}
}
$schema52FoldStart = $physicalWarText.IndexOf('bool FoldExactMissionConvoyRuntime')
$schema52FoldEnd = $physicalWarText.IndexOf('bool ReconcileInactiveMissionConvoyRuntime', $schema52FoldStart)
if ($schema52FoldStart -lt 0 -or $schema52FoldEnd -le $schema52FoldStart) {
	throw "Schema-52 exact mission-convoy fold block is missing"
}
$schema52FoldBlock = $physicalWarText.Substring($schema52FoldStart, $schema52FoldEnd - $schema52FoldStart)
foreach ($requiredSchema52FoldEntry in @(
		'IsExactMissionConvoyRecoveryHold',
		'HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED',
		'activeGroup.m_iSurvivorVehicleCount = 1;',
		'element.m_bPhysicalized = false;',
		'element.m_bMobile = false;',
		'SyncExactMissionConvoyRecoveryCarrierAssets'
	)) {
	if ($schema52FoldBlock -notmatch [regex]::Escape($requiredSchema52FoldEntry)) {
		throw "Schema-52 exact convoy fold must preserve crewless vehicles and assigned cargo as recoverable virtual authority: $requiredSchema52FoldEntry"
	}
}
if ($schema52FoldBlock -match 'element\.m_eDisposition\s*=\s*HST_EConvoyElementDisposition\.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED') {
	throw "Schema-52 exact convoy fold must not retire an unresolved crewless vehicle or its recovery authority"
}
$schema52AssetPositionStart = $physicalWarText.IndexOf('protected void UpdateMissionConvoyAssetPosition')
$schema52AssetPositionEnd = $physicalWarText.IndexOf('protected bool ApplyMissionConvoyObjectiveProgress', $schema52AssetPositionStart)
if ($schema52AssetPositionStart -lt 0 -or $schema52AssetPositionEnd -le $schema52AssetPositionStart) {
	throw "Schema-52 convoy cargo-position synchronization block is missing"
}
$schema52AssetPositionBlock = $physicalWarText.Substring($schema52AssetPositionStart, $schema52AssetPositionEnd - $schema52AssetPositionStart)
if ($schema52AssetPositionBlock -notmatch [regex]::Escape('assignedVehicleSlotId = asset.m_sManifestSlotId') -or
	$schema52AssetPositionBlock -notmatch [regex]::Escape('asset.m_sAssignedVehicleSlotId != assignedVehicleSlotId')) {
	throw "Schema-52 exact convoy cargo must follow only its frozen assigned vehicle slot"
}

foreach ($requiredSchema52ContactClearEntry in @(
		'EXACT_CONVOY_CONTACT_CLEAR_SECONDS = 30',
		'CONVOY_CONTACT_CLEAR_EVENT_KEY = "convoy_contact_cleared"',
		'TryClearExactMissionConvoyContact',
		'operation.m_iLastContactAtSecond',
		'SetMissionConvoyMoving',
		'AssignMissionConvoyWaypoints'
	)) {
	if ($physicalWarText -notmatch [regex]::Escape($requiredSchema52ContactClearEntry)) {
		throw "Schema-52 exact convoy contact-clear contract missing: $requiredSchema52ContactClearEntry"
	}
}
$schema52ContactUpdateStart = $physicalWarText.IndexOf('protected bool UpdateMissionConvoyContact')
$schema52ContactUpdateEnd = $physicalWarText.IndexOf('protected bool TryResolveMissionConvoyContactReason', $schema52ContactUpdateStart)
if ($schema52ContactUpdateStart -lt 0 -or $schema52ContactUpdateEnd -le $schema52ContactUpdateStart) {
	throw "Schema-52 exact convoy contact update/clear block is missing"
}
$schema52ContactUpdateBlock = $physicalWarText.Substring($schema52ContactUpdateStart, $schema52ContactUpdateEnd - $schema52ContactUpdateStart)
foreach ($requiredSchema52ContactClearGuard in @(
		'TryResolveMissionConvoyContactReasonForUpdate',
		'TryClearExactMissionConvoyContact',
		'operation.m_iLastContactAtSecond + EXACT_CONVOY_CONTACT_CLEAR_SECONDS',
		'IsAnyPlayerInVehicle',
		'IsConvoyCrewPopulationPending',
		'IsConvoyCrewControlPending',
		'CONVOY_CONTACT_CLEAR_EVENT_KEY'
	)) {
	if ($schema52ContactUpdateBlock -notmatch [regex]::Escape($requiredSchema52ContactClearGuard)) {
		throw "Schema-52 exact convoy contact may clear only after a quiet grace period with no player or lifecycle ownership conflict: $requiredSchema52ContactClearGuard"
	}
}
$schema52SetContactStart = $physicalWarText.IndexOf('protected bool SetMissionConvoyContact')
$schema52SetContactEnd = $physicalWarText.IndexOf('protected void MarkMissionConvoyVehicleDestroyed', $schema52SetContactStart)
if ($schema52SetContactStart -lt 0 -or $schema52SetContactEnd -le $schema52SetContactStart) {
	throw "Schema-52 exact convoy contact evidence timestamp block is missing"
}
$schema52SetContactBlock = $physicalWarText.Substring($schema52SetContactStart, $schema52SetContactEnd - $schema52SetContactStart)
if ($schema52SetContactBlock -notmatch [regex]::Escape('operation.m_iLastContactAtSecond = state.m_iElapsedSeconds;')) {
	throw "Schema-52 each new exact convoy contact sample must refresh the durable quiet-period clock"
}

$schema52AdmissionStart = $coordinatorText.IndexOf('m_MissionConvoyOperations.PrepareNewMissionContract(mission)')
$schema52RuntimeInitializeIndex = $coordinatorText.IndexOf('m_MissionRuntime.InitializeMissionRuntime', $schema52AdmissionStart)
$schema52AdmitIndex = $coordinatorText.IndexOf('m_MissionConvoyOperations.AdmitNewMission', $schema52AdmissionStart)
if ($schema52AdmissionStart -lt 0 -or $schema52RuntimeInitializeIndex -le $schema52AdmissionStart -or $schema52AdmitIndex -le $schema52RuntimeInitializeIndex) {
	throw "Schema-52 convoy creation must declare the exact contract before runtime assets are planned and admit authority only after those assets exist"
}
foreach ($requiredSchema52CoordinatorEntry in @(
		'protected ref HST_MissionConvoyOperationService m_MissionConvoyOperations;',
		'm_MissionConvoyOperations = new HST_MissionConvoyOperationService();',
		'm_MissionConvoyOperations.SetRuntimeServices(m_PhysicalWar, m_MissionRuntime);',
		'm_MissionConvoyOperations.ReconcileAfterRestore(m_State);',
		'm_MissionConvoyOperations.TickBeforePhysical',
		'm_MissionConvoyOperations.TickAfterPhysical',
		'm_MissionConvoyOperations.TickAfterOutcomes',
		'm_MissionConvoyOperations.ReconcileSettledRuntimeCleanup',
		'AppendCampaignDebugMissionConvoyOperationAssertions'
	)) {
	if ($coordinatorText -notmatch [regex]::Escape($requiredSchema52CoordinatorEntry)) {
		throw "Schema-52 exact mission-convoy coordinator integration missing: $requiredSchema52CoordinatorEntry"
	}
}
$schema52BeforePhysicalIndex = $coordinatorText.IndexOf('m_MissionConvoyOperations.TickBeforePhysical')
$schema52PhysicalTickIndex = $coordinatorText.IndexOf('m_PhysicalWar.UpdateMissionConvoys', $schema52BeforePhysicalIndex)
$schema52AfterPhysicalIndex = $coordinatorText.IndexOf('m_MissionConvoyOperations.TickAfterPhysical', $schema52BeforePhysicalIndex)
$schema52OutcomeIndex = $coordinatorText.IndexOf('ApplyConvoyOutcomesNow', $schema52BeforePhysicalIndex)
$schema52AfterOutcomeIndex = $coordinatorText.IndexOf('m_MissionConvoyOperations.TickAfterOutcomes', $schema52BeforePhysicalIndex)
if ($schema52BeforePhysicalIndex -lt 0 -or $schema52PhysicalTickIndex -le $schema52BeforePhysicalIndex -or
	$schema52AfterPhysicalIndex -le $schema52PhysicalTickIndex -or $schema52OutcomeIndex -le $schema52AfterPhysicalIndex -or
	$schema52AfterOutcomeIndex -le $schema52OutcomeIndex) {
	throw "Schema-52 exact mission-convoy coordinator tick order must bracket physical runtime and consume outcomes afterward"
}

$schema52AggregateMarkerBlock = Get-ScriptMethodBlock $mapMarkerServiceText 'protected void AddMissionConvoyMarkers('
$schema52MarkerLabelBlock = Get-ScriptMethodBlock $mapMarkerServiceText 'protected string BuildConvoyCurrentMarkerLabel('
$schema52IndividualMarkerBlock = Get-ScriptMethodBlock $mapMarkerServiceText 'protected bool ShouldShowIndividualConvoyVehicleMarkers('
if ([string]::IsNullOrEmpty($schema52AggregateMarkerBlock) -or [string]::IsNullOrEmpty($schema52MarkerLabelBlock) -or [string]::IsNullOrEmpty($schema52IndividualMarkerBlock)) {
	throw "Schema-52 aggregate mission-convoy marker boundary is missing"
}
foreach ($requiredSchema52AggregateMarkerEntry in @(
		'BuildConvoyCurrentMarkerLabel(state, mission, title)',
		'"hst_mission_convoy_current_" + mission.m_sInstanceId',
		'BuildConvoyDestinationMarkerLabel(mission, title, destinationName)',
		'element.m_iSurvivingCrewCount',
		'vehicle && !vehicle.m_bDestroyed && !vehicle.m_bDelivered',
		'%2 vehicles | %3 crew'
	)) {
	if (($schema52AggregateMarkerBlock + "`n" + $schema52MarkerLabelBlock) -notmatch [regex]::Escape($requiredSchema52AggregateMarkerEntry)) {
		throw "Schema-52 exact mission-convoy aggregate marker/count contract missing: $requiredSchema52AggregateMarkerEntry"
	}
}
if ($schema52IndividualMarkerBlock -notmatch '(?s)m_iOperationContractVersion\s*==\s*HST_MissionConvoyOperationService\.EXACT_CONTRACT_VERSION\s*\)\s*return false;') {
	throw "Schema-52 exact mission convoy must publish one aggregate marker instead of three per-vehicle markers"
}

$schema52CargoAccessBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool TryResolveConvoyAssetAccessPosition('
if ([string]::IsNullOrEmpty($schema52CargoAccessBlock)) {
	throw "Schema-52 convoy cargo access resolver is missing"
}
$schema52ExactCargoAccessStart = $schema52CargoAccessBlock.IndexOf('mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION')
$schema52LegacyCargoAccessStart = $schema52CargoAccessBlock.IndexOf('bool found;', $schema52ExactCargoAccessStart)
if ($schema52ExactCargoAccessStart -lt 0 -or $schema52LegacyCargoAccessStart -le $schema52ExactCargoAccessStart) {
	throw "Schema-52 assigned-carrier cargo access branch is missing"
}
$schema52ExactCargoAccessBlock = $schema52CargoAccessBlock.Substring($schema52ExactCargoAccessStart, $schema52LegacyCargoAccessStart - $schema52ExactCargoAccessStart)
foreach ($requiredSchema52AssignedCarrierEntry in @(
		'asset.m_sAssignedVehicleSlotId.IsEmpty()',
		'assignedVehicle.m_sManifestSlotId != asset.m_sAssignedVehicleSlotId',
		'accessPosition = assignedVehicle.m_vCurrentPosition;',
		'accessPosition = assignedVehicle.m_vLastKnownPosition;',
		'accessPosition = assignedVehicle.m_vSourcePosition;'
	)) {
	if ($schema52ExactCargoAccessBlock -notmatch [regex]::Escape($requiredSchema52AssignedCarrierEntry)) {
		throw "Schema-52 exact convoy cargo access must resolve only its assigned vehicle slot: $requiredSchema52AssignedCarrierEntry"
	}
}
if ($schema52ExactCargoAccessBlock -match 'DistanceSq2D\s*\(\s*playerPosition' -or $schema52ExactCargoAccessBlock -match 'bestDistance') {
	throw "Schema-52 exact convoy cargo access must not migrate to the nearest surviving vehicle"
}

$schema52CargoProjectionBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'bool TickExactMissionConvoyCargoProjections('
if ([string]::IsNullOrEmpty($schema52CargoProjectionBlock)) {
	throw "Schema-52 assigned-carrier cargo/captive runtime projection tick is missing"
}
$schema52CargoProjectionCorpus = $schema52CargoProjectionBlock
foreach ($schema52CargoHelperToken in @(
		'protected bool IsExactMissionConvoyCargoProjectionPlayerBoundOrResolved(',
		'protected bool ProjectExactMissionConvoyCargoAtCarrier(',
		'protected bool ProjectExactMissionConvoyCargoAtDurablePosition(',
		'protected IEntity EnsureExactMissionConvoyCargoProjectionEntity(',
		'protected bool DematerializeExactMissionConvoyCargoProjection('
	)) {
	$schema52CargoHelperBlock = Get-ScriptMethodBlock $missionRuntimeServiceText $schema52CargoHelperToken
	if ([string]::IsNullOrEmpty($schema52CargoHelperBlock)) {
		throw "Schema-52 assigned-carrier cargo/captive helper is missing: $schema52CargoHelperToken"
	}
	$schema52CargoProjectionCorpus = $schema52CargoProjectionCorpus + "`n" + $schema52CargoHelperBlock
}
foreach ($requiredSchema52CargoProjectionEntry in @(
		'HST_MissionConvoyOperationService.IsExactMission(mission)',
		'asset.m_sAssignedVehicleSlotId',
		'physicalWar.GetExactMissionConvoyVehicleRuntimeEntity',
		'RegisterAssetRuntimeEntityState',
		'TryMoveCaptiveIntoVehicle',
		'DeleteRuntimeEntity',
		'asset.m_bPickedUp',
		'asset.m_bDelivered'
	)) {
	if ($schema52CargoProjectionCorpus -notmatch [regex]::Escape($requiredSchema52CargoProjectionEntry)) {
		throw "Schema-52 assigned-carrier cargo/captive runtime projection missing: $requiredSchema52CargoProjectionEntry"
	}
}
$schema52CargoReadinessBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'bool IsExactMissionConvoyCargoProjectionReady('
$schema52CargoCarrierProjectionBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool ProjectExactMissionConvoyCargoAtCarrier('
$schema52CargoEnsureBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected IEntity EnsureExactMissionConvoyCargoProjectionEntity('
$schema52CargoPublishBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool SetExactMissionConvoyCargoProjectionPublished('
$schema52OperationCargoReadyBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool IsCargoProjectionReady('
if ([string]::IsNullOrEmpty($schema52CargoReadinessBlock) -or [string]::IsNullOrEmpty($schema52CargoCarrierProjectionBlock) -or
	[string]::IsNullOrEmpty($schema52CargoEnsureBlock) -or [string]::IsNullOrEmpty($schema52CargoPublishBlock) -or
	[string]::IsNullOrEmpty($schema52OperationCargoReadyBlock)) {
	throw "Schema-52 frozen cargo process-readiness or publication helpers are missing"
}
foreach ($requiredSchema52CargoReadinessEntry in @(
		'cargoEntity.GetPrefabData().GetPrefabName() != cargo.m_sPrefab',
		'runtimeEntity.m_sPrefab != cargo.m_sPrefab',
		'cargo.m_sCarriedByVehicleId != carrierAsset.m_sAssetId',
		'ResolveEntityVehicle(cargoEntity) == carrierEntity',
		'cargoEntity.GetParent() == carrierEntity'
	)) {
	if ($schema52CargoReadinessBlock -notmatch [regex]::Escape($requiredSchema52CargoReadinessEntry)) {
		throw "Schema-52 cargo readiness must verify the frozen prefab and real carrier relationship: $requiredSchema52CargoReadinessEntry"
	}
}
foreach ($requiredSchema52CargoCarrierEntry in @(
		'TryMoveCaptiveIntoVehicle(entity, carrierEntity',
		'ResolveEntityVehicle(entity) != carrierEntity',
		'carrierEntity.AddChild(entity',
		'entity.GetParent() != carrierEntity'
	)) {
	if ($schema52CargoCarrierProjectionBlock -notmatch [regex]::Escape($requiredSchema52CargoCarrierEntry)) {
		throw "Schema-52 cargo projection must establish a real captive compartment or payload parent: $requiredSchema52CargoCarrierEntry"
	}
}
if ($schema52CargoEnsureBlock -match 'SpawnPrefab\s*\(\s*PROP_CAPTIVES' -or
	$schema52CargoEnsureBlock -notmatch [regex]::Escape('entity.GetPrefabData().GetPrefabName() != asset.m_sPrefab')) {
	throw "Schema-52 exact cargo projection must spawn and verify only the frozen manifest prefab without a generic captive fallback"
}
foreach ($requiredSchema52CargoPublishEntry in @(
		'EntityFlags.ACTIVE',
		'EntityFlags.VISIBLE',
		'EntityFlags.TRACEABLE'
	)) {
	if ($schema52CargoPublishBlock -notmatch [regex]::Escape($requiredSchema52CargoPublishEntry)) {
		throw "Schema-52 staged cargo publication must control recursive active/visible/traceable state: $requiredSchema52CargoPublishEntry"
	}
}
if ($schema52OperationCargoReadyBlock -notmatch [regex]::Escape('m_MissionRuntime.IsExactMissionConvoyCargoProjectionReady(state, mission, m_PhysicalWar)') -or
	$coordinatorText -notmatch [regex]::Escape('m_MissionConvoyOperations.SetRuntimeServices(m_PhysicalWar, m_MissionRuntime)') -or
	$schema52CargoProjectionBlock -notmatch [regex]::Escape('!physicalWar.IsExactMissionConvoyOutboundProjectionTransactionOpen(mission)')) {
	throw "Schema-52 operation readiness and cargo publication must share verified process state and wait for outbound transaction commit"
}
$schema52CargoProjectionTickIndex = $coordinatorText.IndexOf('m_MissionRuntime.TickExactMissionConvoyCargoProjections', $schema52AfterPhysicalIndex)
if ($schema52CargoProjectionTickIndex -le $schema52AfterPhysicalIndex -or $schema52CargoProjectionTickIndex -ge $schema52OutcomeIndex) {
	throw "Schema-52 cargo/captive projection must synchronize after physical convoy authority and before mission outcomes"
}

$schema52RouteReissueMatch = [regex]::Match($physicalWarText, 'CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS\s*=\s*(\d+)')
$schema52HardStuckMatch = [regex]::Match($physicalWarText, 'CONVOY_HARD_STUCK_THRESHOLD_SECONDS\s*=\s*(\d+)')
$schema52TerminalStuckMatch = [regex]::Match($physicalWarText, 'CONVOY_TERMINAL_STUCK_THRESHOLD_SECONDS\s*=\s*(\d+)')
if (!$schema52RouteReissueMatch.Success -or !$schema52HardStuckMatch.Success -or !$schema52TerminalStuckMatch.Success) {
	throw "Schema-52 mission-convoy route watchdog thresholds are missing"
}
$schema52RouteReissueSeconds = [int] $schema52RouteReissueMatch.Groups[1].Value
$schema52HardStuckSeconds = [int] $schema52HardStuckMatch.Groups[1].Value
$schema52TerminalStuckSeconds = [int] $schema52TerminalStuckMatch.Groups[1].Value
if ($schema52RouteReissueSeconds -le 0 -or $schema52HardStuckSeconds -le $schema52RouteReissueSeconds -or $schema52TerminalStuckSeconds -le $schema52HardStuckSeconds) {
	throw "Schema-52 mission-convoy route watchdog must escalate from reissue to hard-stuck to bounded terminal failure"
}
$schema52ProgressWatchdogBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool UpdateConvoyVehicleProgressStatus('
if ([string]::IsNullOrEmpty($schema52ProgressWatchdogBlock)) {
	throw "Schema-52 mission-convoy route watchdog method is missing"
}
$schema52ReissueIndex = $schema52ProgressWatchdogBlock.IndexOf('TryReissueMissionConvoyRouteForProgress')
$schema52SnapIndex = $schema52ProgressWatchdogBlock.IndexOf('TrySnapMissionConvoyVehicleToRoute')
$schema52TerminalIndex = $schema52ProgressWatchdogBlock.IndexOf('CONVOY_TERMINAL_STUCK_THRESHOLD_SECONDS')
$schema52WatchdogFailureIndex = $schema52ProgressWatchdogBlock.IndexOf('Convoy route watchdog exhausted recovery')
if ($schema52ReissueIndex -lt 0 -or $schema52SnapIndex -le $schema52ReissueIndex -or
	$schema52TerminalIndex -le $schema52SnapIndex -or $schema52WatchdogFailureIndex -le $schema52TerminalIndex) {
	throw "Schema-52 mission-convoy route watchdog must attempt bounded route recovery before terminal failure"
}
foreach ($requiredSchema52CampaignCleanupEntry in @(
		'SettleOpenOperationsForCampaignStop',
		'ReconcileSettledRuntimeCleanup',
		'ReconcileInactiveMissionConvoyRuntime',
		'RetireExactMissionConvoyRuntime',
		'HST_OPERATION_TERMINAL_CANCELLED',
		'campaign outcome is terminal',
		'campaign is in setup'
	)) {
	if (($missionConvoyOperationText + "`n" + $physicalWarText + "`n" + $coordinatorText) -notmatch [regex]::Escape($requiredSchema52CampaignCleanupEntry)) {
		throw "Schema-52 exact mission-convoy terminal/campaign cleanup contract missing: $requiredSchema52CampaignCleanupEntry"
	}
}

$schema52SurvivorProjectionReadyBlock = Get-ScriptMethodBlock $physicalWarText 'bool IsExactMissionConvoySurvivorProjectionReady('
$schema52TerminalCrewClassifierBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool IsExactMissionConvoyTerminalSurvivingCrew('
$schema52CrewProjectionAuthorityBlock = Get-ScriptMethodBlock $physicalWarText 'protected string ValidateExactMissionConvoyCrewProjectionAuthority('
$schema52FrozenCrewMatchBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots('
if ([string]::IsNullOrEmpty($schema52SurvivorProjectionReadyBlock) -or
	[string]::IsNullOrEmpty($schema52TerminalCrewClassifierBlock) -or
	[string]::IsNullOrEmpty($schema52CrewProjectionAuthorityBlock) -or
	[string]::IsNullOrEmpty($schema52FrozenCrewMatchBlock)) {
	throw "Schema-52 exact convoy survivor projection authority helpers are missing"
}
foreach ($requiredSchema52SurvivorProjectionEntry in @(
		'EXACT_MISSION_CONVOY_VEHICLE_COUNT',
		'ValidateExactMissionConvoyCrewProjectionAuthority',
		'CountAliveRuntimeCrewAgents',
		'ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots',
		'HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE',
		'IsExactMissionConvoyTerminalSurvivingCrew',
		'HasRuntimeVehicleRegistration',
		'observedTerminalTransition'
	)) {
	if ($schema52SurvivorProjectionReadyBlock -notmatch [regex]::Escape($requiredSchema52SurvivorProjectionEntry)) {
		throw "Schema-52 survivor projection readiness must validate every living exact root and its frozen crew authority: $requiredSchema52SurvivorProjectionEntry"
	}
}
foreach ($requiredSchema52TerminalCrewClassifierEntry in @(
		'IsMissionConvoyVehicleAssetResolved',
		'element.m_iSurvivingCrewCount <= 0',
		'element.m_bMobile',
		'HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED',
		'HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED'
	)) {
	if ($schema52TerminalCrewClassifierBlock -notmatch [regex]::Escape($requiredSchema52TerminalCrewClassifierEntry)) {
		throw "Schema-52 terminal exact convoy crew eligibility must be limited to immobile destroyed/captured vehicles with living crew: $requiredSchema52TerminalCrewClassifierEntry"
	}
}
foreach ($requiredSchema52CrewAuthorityEntry in @(
		'manifest.m_bFrozen',
		'groupClaimants != 1',
		'groupSlot.m_iExpectedMemberCount != element.m_iOriginalCrewCount',
		'ResolveExactMissionConvoyManifestMemberForSeat',
		'resultClaimants != 1',
		'memberResult.m_bCasualtyConfirmed',
		'livingMemberCount != element.m_iSurvivingCrewCount'
	)) {
	if (($schema52CrewProjectionAuthorityBlock + "`n" + $schema52FrozenCrewMatchBlock) -notmatch [regex]::Escape($requiredSchema52CrewAuthorityEntry)) {
		throw "Schema-52 crew-only reprojection must derive its exact living roster from the frozen member slots: $requiredSchema52CrewAuthorityEntry"
	}
}
$schema52TrySpawnActiveGroupBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TrySpawnActiveGroup('
$schema52FrozenCrewSpawnBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TrySpawnExactMissionConvoyFrozenCrewGroup('
$schema52RegisterMappedMemberBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool RegisterExactMissionConvoyMemberEntity('
$schema52MappedCrewMatchBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots('
$schema52ExplicitMemberDeathBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool HasExplicitExactMissionConvoyMemberDeathEvidence('
if ([string]::IsNullOrEmpty($schema52TrySpawnActiveGroupBlock) -or [string]::IsNullOrEmpty($schema52FrozenCrewSpawnBlock) -or
	[string]::IsNullOrEmpty($schema52RegisterMappedMemberBlock) -or [string]::IsNullOrEmpty($schema52MappedCrewMatchBlock) -or
	[string]::IsNullOrEmpty($schema52ExplicitMemberDeathBlock)) {
	throw "Schema-52 frozen exact-member spawn or identity-bijection helper is missing"
}
foreach ($requiredSchema52MemberIdentityEntry in @(
		'm_aExactMissionConvoyMemberMissionIds',
		'm_aExactMissionConvoyMemberGroupIds',
		'm_aExactMissionConvoyMemberSlotIds',
		'm_aExactMissionConvoyMemberEntities',
		'TryGetExactMissionConvoyMappedMemberEntity',
		'ClearExactMissionConvoyMemberMappingsForGroup',
		'ClearExactMissionConvoyMemberMappingsForMission',
		'RemoveExactMissionConvoyMemberMapping'
	)) {
	if ($physicalWarText -notmatch [regex]::Escape($requiredSchema52MemberIdentityEntry)) {
		throw "Schema-52 exact convoy member-slot/entity identity surface is missing: $requiredSchema52MemberIdentityEntry"
	}
}
$schema52ExactSpawnDispatchIndex = $schema52TrySpawnActiveGroupBlock.IndexOf('return TrySpawnExactMissionConvoyFrozenCrewGroup(state, exactMission, activeGroup);')
$schema52GenericSpawnDispatchIndex = $schema52TrySpawnActiveGroupBlock.IndexOf('ShouldDeferActiveGroupRuntimePhysicalization', $schema52ExactSpawnDispatchIndex)
if ($schema52ExactSpawnDispatchIndex -lt 0 -or $schema52GenericSpawnDispatchIndex -le $schema52ExactSpawnDispatchIndex) {
	throw "Schema-52 exact convoy crew must dispatch synchronously from frozen slots before any generic group-spawn path"
}
foreach ($requiredSchema52FrozenCrewSpawnEntry in @(
		'SCR_AIGroup.IgnoreSpawning(true)',
		'ResolveExactMissionConvoyManifestMemberForSeat',
		'SpawnFallbackInfantryCharacter(member.m_sPrefab',
		'ResolveEntityPrefabName(memberEntity) != member.m_sPrefab',
		'AttachFactionInfantryMemberToRuntimeGroup',
		'RegisterExactMissionConvoyMemberEntity',
		'CountExactMissionConvoyMemberMappings',
		'CountAliveRuntimeCrewAgents'
	)) {
	if ($schema52FrozenCrewSpawnBlock -notmatch [regex]::Escape($requiredSchema52FrozenCrewSpawnEntry)) {
		throw "Schema-52 exact convoy crew projection must synchronously project each frozen living slot: $requiredSchema52FrozenCrewSpawnEntry"
	}
}
foreach ($requiredSchema52MemberBijectionEntry in @(
		'ValidateForceSpawnGroupMember',
		'bool sameSlot',
		'bool sameEntity',
		'return sameSlot && sameEntity;',
		'm_aExactMissionConvoyMemberSlotIds.Insert(member.m_sSlotId)',
		'm_aExactMissionConvoyMemberEntities.Insert(entity)'
	)) {
	if ($schema52RegisterMappedMemberBlock -notmatch [regex]::Escape($requiredSchema52MemberBijectionEntry)) {
		throw "Schema-52 exact convoy member registration must preserve a validated slot/entity bijection: $requiredSchema52MemberBijectionEntry"
	}
}
foreach ($requiredSchema52MappedCrewReadinessEntry in @(
		'TryGetExactMissionConvoyMappedMemberEntity',
		'IsRuntimeEntityRegisteredExactlyOnceForGroup',
		'ValidateForceSpawnGroupMember',
		'mappedEntities.Contains(mappedEntity)',
		'CountExactMissionConvoyMemberMappings'
	)) {
	if ($schema52MappedCrewMatchBlock -notmatch [regex]::Escape($requiredSchema52MappedCrewReadinessEntry)) {
		throw "Schema-52 exact convoy readiness must prove every frozen living slot against one mapped native/editable member: $requiredSchema52MappedCrewReadinessEntry"
	}
}
if ($schema52ExplicitMemberDeathBlock -notmatch [regex]::Escape('ECharacterLifeState.DEAD') -or
	$schema52ExplicitMemberDeathBlock -notmatch [regex]::Escape('EDamageState.DESTROYED') -or
	$schema52ExplicitMemberDeathBlock -notmatch [regex]::Escape('if (!entity || entity.IsDeleted())') -or
	$schema52ExplicitMemberDeathBlock -notmatch 'entity\.IsDeleted\(\)\)\s*\r?\n\s*return false;') {
	throw "Schema-52 casualties must require an extant mapped entity with explicit DEAD/DESTROYED evidence"
}

$schema52SpawnConvoyGroupBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TrySpawnMissionConvoyGroup('
$schema52SpawnTerminalCrewBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TrySpawnExactMissionConvoyTerminalSurvivingCrew('
$schema52ShouldSpawnConvoyBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool ShouldSpawnMissionConvoyRuntime('
if ([string]::IsNullOrEmpty($schema52SpawnConvoyGroupBlock) -or
	[string]::IsNullOrEmpty($schema52SpawnTerminalCrewBlock) -or
	[string]::IsNullOrEmpty($schema52ShouldSpawnConvoyBlock)) {
	throw "Schema-52 exact convoy terminal surviving-crew spawn boundary is missing"
}
$schema52TerminalEligibilityIndex = $schema52SpawnConvoyGroupBlock.IndexOf('IsExactMissionConvoyTerminalSurvivingCrew')
$schema52TerminalCrewRouteIndex = $schema52SpawnConvoyGroupBlock.IndexOf('TrySpawnExactMissionConvoyTerminalSurvivingCrew', $schema52TerminalEligibilityIndex)
$schema52ResolvedVehicleRejectIndex = $schema52SpawnConvoyGroupBlock.IndexOf('IsMissionConvoyVehicleAssetResolved(asset)', $schema52TerminalCrewRouteIndex)
if ($schema52TerminalEligibilityIndex -lt 0 -or $schema52TerminalCrewRouteIndex -le $schema52TerminalEligibilityIndex -or
	$schema52ResolvedVehicleRejectIndex -le $schema52TerminalCrewRouteIndex) {
	throw "Schema-52 destroyed/captured convoy survivors must route to crew-only reprojection before the resolved-vehicle rejection"
}
foreach ($requiredSchema52TerminalCrewSpawnEntry in @(
		'ValidateExactMissionConvoyCrewProjectionAuthority',
		'GetRuntimeVehicleEntity',
		'HasRuntimeVehicleRegistration',
		'ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots',
		'activeGroup.m_iInfantryCount = element.m_iSurvivingCrewCount;',
		'activeGroup.m_iSurvivorVehicleCount = 0;',
		'asset.m_bSpawned = false;',
		'element.m_bPhysicalized = false;',
		'element.m_bMobile = false;',
		'runtimeEntity.m_bSpawned = false;',
		'TrySpawnActiveGroup',
		'TERMINAL_VEHICLE_RESURRECTION_REJECTED',
		'TERMINAL_CREW_ROSTER_REJECTED',
		'DISMOUNTED_SURVIVORS'
	)) {
	if ($schema52SpawnTerminalCrewBlock -notmatch [regex]::Escape($requiredSchema52TerminalCrewSpawnEntry)) {
		throw "Schema-52 terminal surviving-crew materialization must remain an exact crew-only projection: $requiredSchema52TerminalCrewSpawnEntry"
	}
}
foreach ($forbiddenSchema52TerminalCrewVehicleSpawnEntry in @(
		'SpawnMissionConvoyVehicle',
		'm_aRuntimeVehicleGroupIds.Insert',
		'm_aRuntimeVehicleEntities.Insert'
	)) {
	if ($schema52SpawnTerminalCrewBlock -match [regex]::Escape($forbiddenSchema52TerminalCrewVehicleSpawnEntry)) {
		throw "Schema-52 terminal surviving-crew materialization must not resurrect or register a vehicle: $forbiddenSchema52TerminalCrewVehicleSpawnEntry"
	}
}
$schema52TerminalCrewNativeSpawnIndex = $schema52SpawnTerminalCrewBlock.IndexOf('TrySpawnActiveGroup')
$schema52TerminalCrewVehiclePreflightIndex = $schema52SpawnTerminalCrewBlock.IndexOf('GetRuntimeVehicleEntity')
$schema52TerminalCrewRegistrationPreflightIndex = $schema52SpawnTerminalCrewBlock.IndexOf('HasRuntimeVehicleRegistration')
$schema52TerminalCrewVehiclePostflightIndex = $schema52SpawnTerminalCrewBlock.IndexOf('GetRuntimeVehicleEntity', $schema52TerminalCrewNativeSpawnIndex)
$schema52TerminalCrewRegistrationPostflightIndex = $schema52SpawnTerminalCrewBlock.IndexOf('HasRuntimeVehicleRegistration', $schema52TerminalCrewNativeSpawnIndex)
if ($schema52TerminalCrewNativeSpawnIndex -lt 0 -or
	$schema52TerminalCrewVehiclePreflightIndex -lt 0 -or $schema52TerminalCrewVehiclePreflightIndex -ge $schema52TerminalCrewNativeSpawnIndex -or
	$schema52TerminalCrewRegistrationPreflightIndex -lt 0 -or $schema52TerminalCrewRegistrationPreflightIndex -ge $schema52TerminalCrewNativeSpawnIndex -or
	$schema52TerminalCrewVehiclePostflightIndex -le $schema52TerminalCrewNativeSpawnIndex -or
	$schema52TerminalCrewRegistrationPostflightIndex -le $schema52TerminalCrewNativeSpawnIndex) {
	throw "Schema-52 terminal crew-only reprojection must reject vehicle handles and registrations both before and after native crew spawn"
}
foreach ($requiredSchema52ShouldSpawnTerminalEntry in @(
		'IsExactMissionConvoyTerminalSurvivingCrew',
		'GetRuntimeVehicleEntity',
		'HasRuntimeVehicleRegistration',
		'GetRuntimeCrewGroupEntity',
		'WasRestoredMissionConvoyRuntimeRebuildAttempted'
	)) {
	if ($schema52ShouldSpawnConvoyBlock -notmatch [regex]::Escape($requiredSchema52ShouldSpawnTerminalEntry)) {
		throw "Schema-52 convoy runtime admission must permit terminal living crew only without a vehicle runtime: $requiredSchema52ShouldSpawnTerminalEntry"
	}
}

$schema52PhysicalProjectionReadyBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool IsPhysicalProjectionReady('
if ([string]::IsNullOrEmpty($schema52PhysicalProjectionReadyBlock) -or
	$schema52PhysicalProjectionReadyBlock -notmatch [regex]::Escape('IsExactMissionConvoySurvivorProjectionReady') -or
	$schema52PhysicalProjectionReadyBlock -notmatch [regex]::Escape('IsCargoProjectionReady')) {
	throw "Schema-52 outbound materialization readiness must combine exact survivor-root and cargo-root readiness"
}
$schema52OutboundBeginBlock = Get-ScriptMethodBlock $physicalWarText 'protected HST_ExactMissionConvoyOutboundProjectionTransaction BeginExactMissionConvoyOutboundProjectionTransaction('
$schema52OutboundRollbackBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool RollbackExactMissionConvoyOutboundProjectionTransaction('
$schema52OutboundCompleteBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool CompleteExactMissionConvoyOutboundProjectionTransaction('
$schema52OutboundPublishBlock = Get-ScriptMethodBlock $physicalWarText 'protected void SetExactMissionConvoyProjectionEntityPublished('
$schema52OutboundReconcileBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool ReconcileExactMissionConvoyOutboundProjectionTransactions('
$schema52OutboundParticipantCommitBlock = Get-ScriptMethodBlock $physicalWarText 'bool CommitExactMissionConvoyOutboundProjectionTransaction('
$schema52OutboundVisibilityBlock = Get-ScriptMethodBlock $physicalWarText 'protected void SetExactMissionConvoyOutboundProjectionTransactionVisible('
$schema52CargoParticipantPublicationBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'bool SetExactMissionConvoyCargoProjectionPublication('
if ($physicalWarText -notmatch [regex]::Escape('class HST_ExactMissionConvoyOutboundProjectionTransaction') -or
	[string]::IsNullOrEmpty($schema52OutboundBeginBlock) -or [string]::IsNullOrEmpty($schema52OutboundRollbackBlock) -or
	[string]::IsNullOrEmpty($schema52OutboundCompleteBlock) -or [string]::IsNullOrEmpty($schema52OutboundPublishBlock) -or
	[string]::IsNullOrEmpty($schema52OutboundReconcileBlock) -or [string]::IsNullOrEmpty($schema52OutboundParticipantCommitBlock) -or
	[string]::IsNullOrEmpty($schema52OutboundVisibilityBlock) -or [string]::IsNullOrEmpty($schema52CargoParticipantPublicationBlock)) {
	throw "Schema-52 all-root outbound publication transaction is missing"
}
foreach ($requiredSchema52OutboundBeginEntry in @(
		'HST_OPERATION_MATERIALIZATION_MATERIALIZING',
		'CountExactMissionConvoyMemberMappings(mission.m_sInstanceId) != 0',
		'EXACT_MISSION_CONVOY_VEHICLE_COUNT',
		'FindPendingActiveGroupPopulationIndex',
		'm_aExactMissionConvoyOutboundProjectionTransactions.Insert(transaction)'
	)) {
	if ($schema52OutboundBeginBlock -notmatch [regex]::Escape($requiredSchema52OutboundBeginEntry)) {
		throw "Schema-52 outbound transaction must preflight and snapshot every exact root before staging: $requiredSchema52OutboundBeginEntry"
	}
}
foreach ($requiredSchema52OutboundPublicationEntry in @(
		'EntityFlags.ACTIVE',
		'EntityFlags.VISIBLE',
		'EntityFlags.TRACEABLE',
		'entity.SetFlags(publicationFlags, true)',
		'entity.ClearFlags(publicationFlags, true)'
	)) {
	if ($schema52OutboundPublishBlock -notmatch [regex]::Escape($requiredSchema52OutboundPublicationEntry)) {
		throw "Schema-52 staged exact roots must recursively control active/visible/traceable publication: $requiredSchema52OutboundPublicationEntry"
	}
}
foreach ($requiredSchema52OutboundTerminalRollbackEntry in @(
		'HST_OPERATION_MATERIALIZATION_VIRTUAL',
		'HST_OPERATION_POSITION_STRATEGIC',
		'SetMissionConvoyFailure',
		'RemoveExactMissionConvoyOutboundProjectionTransaction(transaction)'
	)) {
	if ($schema52OutboundRollbackBlock -notmatch [regex]::Escape($requiredSchema52OutboundTerminalRollbackEntry)) {
		throw "Schema-52 terminal outbound failure must return to durable virtual/strategic authority and close the transaction: $requiredSchema52OutboundTerminalRollbackEntry"
	}
}
$schema52OutboundPhysicalIndex = $schema52OutboundParticipantCommitBlock.IndexOf('HST_OPERATION_MATERIALIZATION_PHYSICAL')
$schema52OutboundReadyIndex = $schema52OutboundParticipantCommitBlock.IndexOf('IsExactMissionConvoySurvivorProjectionReady', $schema52OutboundPhysicalIndex)
$schema52OutboundCargoReadyIndex = $schema52OutboundParticipantCommitBlock.IndexOf('IsExactMissionConvoyCargoProjectionReady', $schema52OutboundReadyIndex)
$schema52OutboundCargoPublishIndex = $schema52OutboundParticipantCommitBlock.IndexOf('SetExactMissionConvoyCargoProjectionPublication', $schema52OutboundCargoReadyIndex)
$schema52OutboundCommitIndex = $schema52OutboundParticipantCommitBlock.IndexOf('CompleteExactMissionConvoyOutboundProjectionTransaction', $schema52OutboundCargoPublishIndex)
if ($schema52OutboundPhysicalIndex -lt 0 -or $schema52OutboundReadyIndex -le $schema52OutboundPhysicalIndex -or
	$schema52OutboundCargoReadyIndex -le $schema52OutboundReadyIndex -or $schema52OutboundCargoPublishIndex -le $schema52OutboundCargoReadyIndex -or
	$schema52OutboundCommitIndex -le $schema52OutboundCargoPublishIndex -or
	$schema52OutboundCompleteBlock -notmatch [regex]::Escape('IsExactMissionConvoyOutboundProjectionTransactionPublished(transaction, true)') -or
	$schema52OutboundCompleteBlock -notmatch [regex]::Escape('SetExactMissionConvoyOutboundProjectionTransactionVisible(transaction, true)')) {
	throw "Schema-52 outbound exact roots and cargo may publish only through one revalidated PHYSICAL participant commit"
}
foreach ($requiredSchema52MappedMemberPublicationEntry in @(
		'm_aExactMissionConvoyMemberMissionIds[memberIndex] != transaction.m_sMissionInstanceId',
		'SetExactMissionConvoyProjectionEntityPublished(m_aExactMissionConvoyMemberEntities[memberIndex], visible)'
	)) {
	if ($schema52OutboundVisibilityBlock -notmatch [regex]::Escape($requiredSchema52MappedMemberPublicationEntry)) {
		throw "Schema-52 outbound publication must include every individually staged exact member: $requiredSchema52MappedMemberPublicationEntry"
	}
}
if ($schema52OutboundReconcileBlock -match [regex]::Escape('CompleteExactMissionConvoyOutboundProjectionTransaction') -or
	$missionConvoyOperationText -notmatch [regex]::Escape('CommitExactMissionConvoyOutboundProjectionTransaction(state, mission, m_MissionRuntime, publicationFailure)')) {
	throw "Schema-52 PhysicalWar may not close outbound publication without the operation-layer cargo participant seam"
}
$schema52NearestConvoyPlayerBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected float ResolveNearestLivingPlayerDistanceForConvoy('
$schema52AbandonedBubbleRootBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'static bool IsRecoverableAbandonedVehicleRoot('
$schema52BeginMaterializationBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool TryBeginMaterialization('
$schema52TickAfterPhysicalBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'bool TickAfterPhysical('
if ([string]::IsNullOrEmpty($schema52NearestConvoyPlayerBlock) -or [string]::IsNullOrEmpty($schema52AbandonedBubbleRootBlock) -or
	[string]::IsNullOrEmpty($schema52BeginMaterializationBlock) -or
	[string]::IsNullOrEmpty($schema52TickAfterPhysicalBlock)) {
	throw "Schema-52 mixed-root convoy materialization bubble methods are missing"
}
foreach ($requiredSchema52MixedBubbleEntry in @(
		'operation.m_vStrategicPosition',
		'EXACT_VEHICLE_COUNT',
		'element.m_iSurvivingCrewCount > 0',
		'element.m_vCurrentPosition',
		'IsRecoverableAbandonedVehicleRoot(vehicle, element)',
		'cargo.m_bPickedUp',
		'cargo.m_bDelivered',
		'cargo.m_bDestroyed',
		'cargo.m_bOutcomeApplied',
		'cargo.m_vCurrentPosition'
	)) {
	if ($schema52NearestConvoyPlayerBlock -notmatch [regex]::Escape($requiredSchema52MixedBubbleEntry)) {
		throw "Schema-52 player-bubble ownership must measure every separated living/recovery convoy root: $requiredSchema52MixedBubbleEntry"
	}
}
foreach ($requiredSchema52AbandonedBubbleRootEntry in @(
		'HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED',
		'element.m_iSurvivingCrewCount <= 0',
		'!element.m_bMobile',
		'element.m_fVehicleDamageFraction >= 0.0',
		'element.m_fVehicleDamageFraction < 1.0'
	)) {
	if ($schema52AbandonedBubbleRootBlock -notmatch [regex]::Escape($requiredSchema52AbandonedBubbleRootEntry)) {
		throw "Schema-52 separated abandoned-vehicle bubble classifier is incomplete: $requiredSchema52AbandonedBubbleRootEntry"
	}
}
if ($schema52NearestConvoyPlayerBlock -match '\brecoveryHold\b' -or $schema52NearestConvoyPlayerBlock -match 'if\s*\(\s*IsRecoveryHold') {
	throw "Schema-52 player-bubble ownership must not gate separated abandoned vehicles or unresolved cargo on global recovery hold"
}
$schema52BeginBubbleIndex = $schema52BeginMaterializationBlock.IndexOf('ResolveNearestLivingPlayerDistanceForConvoy')
$schema52BeginRadiusIndex = $schema52BeginMaterializationBlock.IndexOf('EXACT_MATERIALIZE_IN_RADIUS_METERS', $schema52BeginBubbleIndex)
$schema52FoldBubbleIndex = $schema52TickAfterPhysicalBlock.IndexOf('ResolveNearestLivingPlayerDistanceForConvoy')
$schema52FoldRadiusIndex = $schema52TickAfterPhysicalBlock.IndexOf('EXACT_MATERIALIZE_OUT_RADIUS_METERS', $schema52FoldBubbleIndex)
if ($schema52BeginBubbleIndex -lt 0 -or $schema52BeginRadiusIndex -le $schema52BeginBubbleIndex -or
	$schema52FoldBubbleIndex -lt 0 -or $schema52FoldRadiusIndex -le $schema52FoldBubbleIndex) {
	throw "Schema-52 materialize/fold decisions must use the nearest separated convoy root before applying bubble thresholds"
}

foreach ($requiredSchema52AtomicFoldEntry in @(
		'terminalAssets',
		'terminalElements',
		'terminalGroups',
		'preserveTerminalVehicles',
		'crewlessAssets',
		'crewlessElements',
		'crewlessGroups',
		'sampledCrewlessPositions',
		'sampledCrewlessSurvivors',
		'sampledTerminalPositions',
		'sampledTerminalSurvivors',
		'sampledPositions',
		'sampledSurvivors',
		'allRosterElements',
		'allRosterSurvivors',
		'BuildExactMissionConvoyRosterMutationPlan',
		'ApplyExactMissionConvoyRosterMutationPlan'
	)) {
	if ($schema52FoldBlock -notmatch [regex]::Escape($requiredSchema52AtomicFoldEntry)) {
		throw "Schema-52 exact convoy fold must sample active, terminal, and crewless roots in one atomic preflight: $requiredSchema52AtomicFoldEntry"
	}
}
$schema52RosterPlanBlock = Get-ScriptMethodBlock $physicalWarText 'protected string BuildExactMissionConvoyRosterMutationPlan('
$schema52RosterApplyBlock = Get-ScriptMethodBlock $physicalWarText 'protected void ApplyExactMissionConvoyRosterMutationPlan('
if ([string]::IsNullOrEmpty($schema52RosterPlanBlock) -or [string]::IsNullOrEmpty($schema52RosterApplyBlock)) {
	throw "Schema-52 exact convoy mapped-member roster plan/apply helpers are missing"
}
foreach ($requiredSchema52RosterPlanEntry in @(
		'elements.Count() != sampledSurvivors.Count()',
		'IsExactMissionConvoyOutboundProjectionTransactionOpen(mission)',
		'manifest.m_bFrozen',
		'ResolveExactMissionConvoyManifestMemberForSeat',
		'TryGetExactMissionConvoyMappedMemberEntity',
		'ValidateForceSpawnGroupMember',
		'HasExplicitExactMissionConvoyMemberDeathEvidence',
		'result.m_aNewCasualtySlots.Insert(memberResult)',
		'memberResult.m_bCasualtyConfirmed',
		'mappingCount != currentLivingSlots',
		'desiredSurvivors != observedLivingSlots'
	)) {
	if ($schema52RosterPlanBlock -notmatch [regex]::Escape($requiredSchema52RosterPlanEntry)) {
		throw "Schema-52 fold preflight must map each frozen living slot to explicit entity/death evidence: $requiredSchema52RosterPlanEntry"
	}
}
foreach ($requiredSchema52RosterApplyEntry in @(
		'casualtySlot.m_bCasualtyConfirmed = true;',
		'casualtySlot.m_bAliveVerified = false;',
		'HST_FORCE_SLOT_RETIRED',
		'casualtySlot.m_iCasualtyAtSecond',
		'RemoveExactMissionConvoyMemberMapping',
		'exact mission convoy explicitly mapped physical casualty'
	)) {
	if ($schema52RosterApplyBlock -notmatch [regex]::Escape($requiredSchema52RosterApplyEntry)) {
		throw "Schema-52 roster commit must retire only the explicitly mapped dead slot: $requiredSchema52RosterApplyEntry"
	}
}
$schema52RosterPlanIndex = $schema52FoldBlock.IndexOf('BuildExactMissionConvoyRosterMutationPlan(state, mission, allRosterElements, allRosterSurvivors, rosterPlan)')
$schema52RosterApplyIndex = $schema52FoldBlock.IndexOf('ApplyExactMissionConvoyRosterMutationPlan(rosterPlan)', $schema52RosterPlanIndex)
$schema52FirstFoldDurableWriteMatch = [regex]::Match(
	$schema52FoldBlock,
	'(?:crewlessGroup|crewlessAsset|crewlessElement|activeGroup|asset|element|terminalGroup|terminalAsset|terminalElement)\.m_(?:vPosition|vSourcePosition|vCurrentPosition|iInfantryCount|iSurvivingCrewCount)\s*='
)
$schema52FirstFoldDurableWriteIndex = $schema52FirstFoldDurableWriteMatch.Index
if (!$schema52FirstFoldDurableWriteMatch.Success) {
	$schema52FirstFoldDurableWriteIndex = -1
}
$schema52FirstFoldDeleteIndex = $schema52FoldBlock.IndexOf('DeleteRuntimeGroupEntity')
if ($schema52RosterPlanIndex -lt 0 -or $schema52RosterApplyIndex -le $schema52RosterPlanIndex -or
	$schema52FirstFoldDurableWriteIndex -le $schema52RosterApplyIndex -or $schema52FirstFoldDeleteIndex -le $schema52RosterApplyIndex) {
	throw "Schema-52 exact convoy fold must build one all-root member plan before applying it, durable writes, or runtime deletion"
}
$schema52FoldPostCommitBlock = $schema52FoldBlock.Substring($schema52RosterApplyIndex)
if ($schema52FoldBlock -match 'UpdateRuntimeGroupSurvivors\s*\(\s*state\s*\)' -or
	$schema52FoldPostCommitBlock -match 'return\s+false\s*;') {
	throw "Schema-52 exact convoy fold must not invoke global survivor mutation or fail after its durable commit begins"
}
$schema52RetireBlock = Get-ScriptMethodBlock $physicalWarText 'bool RetireExactMissionConvoyRuntime('
if ([string]::IsNullOrEmpty($schema52RetireBlock)) {
	throw "Schema-52 exact convoy runtime retirement method is missing"
}
$schema52RetirePlanIndex = $schema52RetireBlock.IndexOf('BuildExactMissionConvoyRosterMutationPlan')
$schema52RetireApplyIndex = $schema52RetireBlock.IndexOf('ApplyExactMissionConvoyRosterMutationPlan', $schema52RetirePlanIndex)
if ($schema52RetirePlanIndex -lt 0 -or $schema52RetireApplyIndex -le $schema52RetirePlanIndex) {
	throw "Schema-52 exact convoy retirement must preflight one mapped-member mutation plan before commit"
}
$schema52RetirePostCommitBlock = $schema52RetireBlock.Substring($schema52RetireApplyIndex)
if ($schema52RetireBlock -match 'UpdateRuntimeGroupSurvivors\s*\(\s*state\s*\)' -or
	$schema52RetirePostCommitBlock -match 'return\s+false\s*;') {
	throw "Schema-52 exact convoy retirement must not invoke aggregate survivor mutation or fail after commit begins"
}

$schema52FoldPayloadRoleIndex = $schema52FoldBlock.IndexOf('missionAsset.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE')
$schema52FoldCaptiveRoleIndex = $schema52FoldBlock.IndexOf('missionAsset.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE', $schema52FoldPayloadRoleIndex)
$schema52FoldCargoSafetyIndex = $schema52FoldBlock.IndexOf('HasUnsafeExactMissionConvoyCargoCarrierClaim', $schema52FoldCaptiveRoleIndex)
if ($schema52FoldPayloadRoleIndex -lt 0 -or $schema52FoldCaptiveRoleIndex -le $schema52FoldPayloadRoleIndex -or
	$schema52FoldCargoSafetyIndex -le $schema52FoldCaptiveRoleIndex) {
	throw "Schema-52 fold interaction guards must select only payload/captive roots before validating carrier ownership"
}
foreach ($requiredSchema52CapturedVehicleFoldEntry in @(
		'HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED',
		'IsLivingEntity(terminalVehicleEntity)',
		'preserveTerminalVehicles.Insert',
		'HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive',
		'DeleteRuntimeGroupEntity(terminalGroup.m_sGroupId, !preserveTerminalVehicle)'
	)) {
	if ($schema52FoldBlock -notmatch [regex]::Escape($requiredSchema52CapturedVehicleFoldEntry)) {
		throw "Schema-52 captured vehicle fold must preserve and neutralize a living captured root without treating it as carried cargo: $requiredSchema52CapturedVehicleFoldEntry"
	}
}

$schema52ContactClearBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TryClearExactMissionConvoyContact('
$schema52ContactReasonBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TryResolveMissionConvoyContactReasonInternal('
if ([string]::IsNullOrEmpty($schema52ContactClearBlock) -or [string]::IsNullOrEmpty($schema52ContactReasonBlock)) {
	throw "Schema-52 role-aware exact convoy contact methods are missing"
}
$schema52ContactPayloadRoleIndex = $schema52ContactClearBlock.IndexOf('missionAsset.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE')
$schema52ContactCaptiveRoleIndex = $schema52ContactClearBlock.IndexOf('missionAsset.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE', $schema52ContactPayloadRoleIndex)
$schema52ContactCargoSafetyIndex = $schema52ContactClearBlock.IndexOf('HasUnsafeExactMissionConvoyCargoCarrierClaim', $schema52ContactCaptiveRoleIndex)
if ($schema52ContactPayloadRoleIndex -lt 0 -or $schema52ContactCaptiveRoleIndex -le $schema52ContactPayloadRoleIndex -or
	$schema52ContactCargoSafetyIndex -le $schema52ContactCaptiveRoleIndex) {
	throw "Schema-52 contact clear must select only payload/captive roots before evaluating carrier safety"
}
foreach ($requiredSchema52TerminalContactEntry in @(
		'IsExactMissionConvoyTerminalSurvivingCrew',
		'HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE',
		'asset.m_bDelivered && !terminalSurvivingCrew',
		'asset.m_bDestroyed && !terminalSurvivingCrew',
		'ResolveActiveGroupLiveRuntimePosition(activeGroup, true)',
		'from dismounted crew',
		'if (terminalSurvivingCrew)',
		'GetRuntimeVehicleEntity'
	)) {
	if ($schema52ContactReasonBlock -notmatch [regex]::Escape($requiredSchema52TerminalContactEntry)) {
		throw "Schema-52 contact detection must keep destroyed/captured living crew as a dismounted root and ignore their resolved carrier: $requiredSchema52TerminalContactEntry"
	}
}
$schema52TerminalContactSkipIndex = $schema52ContactReasonBlock.IndexOf('if (terminalSurvivingCrew)')
$schema52TerminalContactVehicleIndex = $schema52ContactReasonBlock.IndexOf('GetRuntimeVehicleEntity', $schema52TerminalContactSkipIndex)
if ($schema52TerminalContactSkipIndex -lt 0 -or $schema52TerminalContactVehicleIndex -le $schema52TerminalContactSkipIndex) {
	throw "Schema-52 terminal surviving-crew contact must finish crew evidence handling before vehicle mobility checks"
}

$schema52RecoveryCarrierAvailabilityBlock = Get-ScriptMethodBlock $physicalWarText 'protected string ValidateExactMissionConvoyRecoveryCarrierAvailability('
$schema52UnresolvedRecoveryCargoBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool IsExactMissionConvoyUnresolvedRecoveryCargo('
$schema52RecoveryCarrierElementBlock = Get-ScriptMethodBlock $physicalWarText 'protected HST_ConvoyElementState ResolveExactMissionConvoyRecoveryCarrierElement('
$schema52TerminalRecoveryCarrierBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool IsExactMissionConvoyTerminalRecoveryCarrier('
$schema52FreezeTerminalCargoBlock = Get-ScriptMethodBlock $physicalWarText 'protected void FreezeExactMissionConvoyTerminalRecoveryCargo('
$schema52SyncRecoveryCarrierAssetsBlock = Get-ScriptMethodBlock $physicalWarText 'protected void SyncExactMissionConvoyRecoveryCarrierAssets('
if ([string]::IsNullOrEmpty($schema52RecoveryCarrierAvailabilityBlock) -or
	[string]::IsNullOrEmpty($schema52UnresolvedRecoveryCargoBlock) -or
	[string]::IsNullOrEmpty($schema52RecoveryCarrierElementBlock) -or
	[string]::IsNullOrEmpty($schema52TerminalRecoveryCarrierBlock) -or
	[string]::IsNullOrEmpty($schema52FreezeTerminalCargoBlock) -or
	[string]::IsNullOrEmpty($schema52SyncRecoveryCarrierAssetsBlock)) {
	throw "Schema-52 destroyed/captured carrier ground-cargo authority helpers are missing"
}
foreach ($requiredSchema52RecoveryCarrierEntry in @(
		'ResolveExactMissionConvoyRecoveryCarrierElement',
		'IsExactMissionConvoyRecoveryVehicleEligible',
		'IsExactMissionConvoyTerminalRecoveryCarrier',
		'terminal carrier has no durable ground position'
	)) {
	if ($schema52RecoveryCarrierAvailabilityBlock -notmatch [regex]::Escape($requiredSchema52RecoveryCarrierEntry)) {
		throw "Schema-52 recovery materialization must preflight unresolved cargo against its frozen carrier root: $requiredSchema52RecoveryCarrierEntry"
	}
}
foreach ($requiredSchema52RecoveryCarrierIdentityEntry in @(
		'candidate.m_sOperationId != mission.m_sOperationId',
		'candidate.m_sMissionInstanceId != mission.m_sInstanceId',
		'candidate.m_sVehicleSlotId != vehicleSlotId',
		'carrierClaimants != 1'
	)) {
	if ($schema52RecoveryCarrierElementBlock -notmatch [regex]::Escape($requiredSchema52RecoveryCarrierIdentityEntry)) {
		throw "Schema-52 recovery cargo must resolve exactly one frozen operation/mission/vehicle-slot carrier: $requiredSchema52RecoveryCarrierIdentityEntry"
	}
}
foreach ($requiredSchema52TerminalCarrierEntry in @(
		'IsMissionConvoyVehicleAssetResolved',
		'carrierElement.m_iSurvivingCrewCount > 0',
		'carrierElement.m_bMobile',
		'HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED',
		'carrierAsset.m_bDestroyed',
		'HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED',
		'carrierAsset.m_bDelivered',
		'carrierAsset.m_sLastInteraction == "captured"'
	)) {
	if ($schema52TerminalRecoveryCarrierBlock -notmatch [regex]::Escape($requiredSchema52TerminalCarrierEntry)) {
		throw "Schema-52 ground cargo may detach only from a resolved, crewless, immobile destroyed/captured frozen carrier: $requiredSchema52TerminalCarrierEntry"
	}
}
foreach ($requiredSchema52FrozenGroundCargoEntry in @(
		'IsExactMissionConvoyUnresolvedRecoveryCargo',
		'ResolveExactMissionConvoyRecoveryCarrierElement',
		'IsExactMissionConvoyTerminalRecoveryCarrier',
		'cargo.m_vCurrentPosition = position;',
		'cargo.m_vLastKnownPosition = position;',
		'cargo.m_bAttachedToCarrier = false;',
		'cargo.m_sCarriedByVehicleId = "";'
	)) {
	if ($schema52FreezeTerminalCargoBlock -notmatch [regex]::Escape($requiredSchema52FrozenGroundCargoEntry)) {
		throw "Schema-52 terminal frozen-carrier cargo must become detached ground authority at the durable carrier position: $requiredSchema52FrozenGroundCargoEntry"
	}
}
foreach ($requiredSchema52CarrierSyncEntry in @(
		'IsMissionConvoyVehicleAssetResolved(carrierAsset)',
		'HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED',
		'HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED',
		'cargo.m_sAssignedVehicleSlotId != carrier.m_sVehicleSlotId',
		'cargo.m_bAttachedToCarrier = !terminalResolvedCarrier;',
		'cargo.m_sCarriedByVehicleId = "";',
		'cargo.m_sCarriedByVehicleId = carrier.m_sVehicleAssetId;'
	)) {
	if ($schema52SyncRecoveryCarrierAssetsBlock -notmatch [regex]::Escape($requiredSchema52CarrierSyncEntry)) {
		throw "Schema-52 recovery carrier sync must preserve frozen slot ownership while detaching terminal-carrier cargo: $requiredSchema52CarrierSyncEntry"
	}
}
$schema52RecoveryFreezeIndex = $schema52RecoveryMaterializeBlock.IndexOf('FreezeExactMissionConvoyTerminalRecoveryCargo')
$schema52RecoveryPlanningIndex = $schema52RecoveryMaterializeBlock.IndexOf('ref array<ref HST_MissionAssetState> assets')
$schema52RecoveryVehicleSpawnIndex = $schema52RecoveryMaterializeBlock.IndexOf('SpawnMissionConvoyVehicle')
if ($schema52RecoveryFreezeIndex -lt 0 -or $schema52RecoveryPlanningIndex -le $schema52RecoveryFreezeIndex -or
	$schema52RecoveryVehicleSpawnIndex -le $schema52RecoveryPlanningIndex) {
	throw "Schema-52 recovery materialization must freeze terminal-carrier cargo on the ground before planning or spawning vehicle roots"
}

$schema52RuntimeCarrierAuthorityBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool ResolveExactMissionConvoyCargoCarrierAuthority('
$schema52RuntimeTerminalCarrierBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool IsExactMissionConvoyTerminalCarrierGroundRecovery('
$schema52RuntimeGroundCargoBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool ProjectExactMissionConvoyCargoAtDurablePosition('
if ([string]::IsNullOrEmpty($schema52RuntimeCarrierAuthorityBlock) -or
	[string]::IsNullOrEmpty($schema52RuntimeTerminalCarrierBlock) -or
	[string]::IsNullOrEmpty($schema52RuntimeGroundCargoBlock)) {
	throw "Schema-52 standalone terminal-carrier cargo projection helpers are missing"
}
foreach ($requiredSchema52RuntimeCarrierAuthorityEntry in @(
		'cargo.m_sAssignedVehicleSlotId',
		'carrierAssetClaimants != 1',
		'carrierElementClaimants != 1',
		'candidateAsset.m_sManifestSlotId != cargo.m_sAssignedVehicleSlotId',
		'candidateElement.m_sVehicleSlotId != cargo.m_sAssignedVehicleSlotId',
		'cargo.m_sConvoyElementId == carrierElement.m_sElementId'
	)) {
	if ($schema52RuntimeCarrierAuthorityBlock -notmatch [regex]::Escape($requiredSchema52RuntimeCarrierAuthorityEntry)) {
		throw "Schema-52 standalone cargo projection must use one exact frozen carrier asset/element authority: $requiredSchema52RuntimeCarrierAuthorityEntry"
	}
}
foreach ($requiredSchema52RuntimeTerminalCarrierEntry in @(
		'mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE',
		'carrierElement.m_bMobile',
		'HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED',
		'carrierAsset.m_bDestroyed',
		'HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED',
		'carrierAsset.m_sLastInteraction == "captured"'
	)) {
	if ($schema52RuntimeTerminalCarrierBlock -notmatch [regex]::Escape($requiredSchema52RuntimeTerminalCarrierEntry)) {
		throw "Schema-52 standalone ground projection must remain active only for unresolved destroyed/captured carrier recovery: $requiredSchema52RuntimeTerminalCarrierEntry"
	}
}
foreach ($requiredSchema52RuntimeGroundCargoEntry in @(
		'ResolveExactMissionConvoyStandaloneRecoveryPosition',
		'HST_WorldPositionService.ResolveSafeGroundPosition',
		'EnsureExactMissionConvoyCargoProjectionEntity',
		'asset.m_bAttachedToCarrier = false;',
		'asset.m_sCarriedByVehicleId = "";',
		'RegisterAssetRuntimeEntityState'
	)) {
	if ($schema52RuntimeGroundCargoBlock -notmatch [regex]::Escape($requiredSchema52RuntimeGroundCargoEntry)) {
		throw "Schema-52 terminal-carrier cargo must project as a safe standalone ground entity: $requiredSchema52RuntimeGroundCargoEntry"
	}
}
foreach ($requiredSchema52CargoTickGroundEntry in @(
		'ResolveExactMissionConvoyCargoCarrierAuthority',
		'GetExactMissionConvoyVehicleRuntimeEntity',
		'IsExactMissionConvoyTerminalCarrierGroundRecovery',
		'operationProjectionActive',
		'ProjectExactMissionConvoyCargoAtDurablePosition',
		'DematerializeExactMissionConvoyCargoProjection'
	)) {
	if ($schema52CargoProjectionBlock -notmatch [regex]::Escape($requiredSchema52CargoTickGroundEntry)) {
		throw "Schema-52 cargo projection tick must independently switch between attached, standalone salvage, and virtual roots: $requiredSchema52CargoTickGroundEntry"
	}
}

$schema52CargoRestoreStart = $schema52PhysicalRestoreBlock.IndexOf('foreach (HST_MissionAssetState cargo')
if ($schema52CargoRestoreStart -lt 0) {
	throw "Schema-52 physical restore must include an exact cargo/captive runtime reset pass"
}
$schema52CargoRestoreBlock = $schema52PhysicalRestoreBlock.Substring($schema52CargoRestoreStart)
foreach ($requiredSchema52CargoRestoreEntry in @(
		'cargo.m_sAssignedVehicleSlotId.IsEmpty()',
		'MISSION_CONVOY_PAYLOAD_ROLE',
		'MISSION_CONVOY_CAPTIVE_ROLE',
		'cargo.m_bPickedUp',
		'cargo.m_bDelivered',
		'cargo.m_bDestroyed',
		'cargo.m_bOutcomeApplied',
		'cargo.m_bSpawned = false;',
		'cargo.m_bAttachedToCarrier = false;',
		'cargo.m_sCarriedByVehicleId = "";',
		'state.FindMissionRuntimeEntity(cargo.m_sEntityId)',
		'cargoRuntimeEntity.m_bSpawned = false;'
	)) {
	if ($schema52CargoRestoreBlock -notmatch [regex]::Escape($requiredSchema52CargoRestoreEntry)) {
		throw "Schema-52 restore must clear stale spawned/attached/runtime flags for unresolved exact cargo: $requiredSchema52CargoRestoreEntry"
	}
}

$schema52InactiveCleanupBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool CleanupInactiveMissionConvoyRuntime('
$schema52SettledCleanupBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'bool ReconcileSettledRuntimeCleanup('
$schema52DurableClaimantBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool IsCurrentSchemaExactMissionConvoyDurableClaimant('
if ([string]::IsNullOrEmpty($schema52InactiveCleanupBlock) -or [string]::IsNullOrEmpty($schema52SettledCleanupBlock) -or
	[string]::IsNullOrEmpty($schema52DurableClaimantBlock)) {
	throw "Schema-52 settled exact convoy cleanup deferral methods are missing"
}
foreach ($requiredSchema52DurableClaimantEntry in @(
		'deterministicGroupId',
		'deterministicPrefixAnchor',
		'deterministicAuthorityIdAnchor',
		'elementIdentityAnchor',
		'operation_mission_convoy_',
		'manifest_mission_convoy_',
		'HST_OPERATION_TYPE_MISSION_CONVOY',
		'HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION',
		'HST_MissionConvoyOperationService.EXACT_FORCE_KIND',
		'HST_MissionConvoyOperationService.EXACT_POLICY_ID',
		'convoy_element_',
		'MISSION_CONVOY_VEHICLE_ROLE'
	)) {
	if ($schema52DurableClaimantBlock -notmatch [regex]::Escape($requiredSchema52DurableClaimantEntry)) {
		throw "Schema-52 orphan cleanup must recognize current exact durable evidence and fail closed: $requiredSchema52DurableClaimantEntry"
	}
}
$schema52OrphanCleanupIndex = $schema52InactiveCleanupBlock.IndexOf('IsCurrentSchemaExactMissionConvoyDurableClaimant')
$schema52OrphanGenericFilterIndex = $schema52InactiveCleanupBlock.IndexOf('if (!IsMissionConvoyGroup(activeGroup) && !durableExactClaimant)', $schema52OrphanCleanupIndex)
$schema52OrphanContinueIndex = $schema52InactiveCleanupBlock.IndexOf('continue;', $schema52OrphanCleanupIndex)
if ($schema52OrphanCleanupIndex -lt 0 -or $schema52OrphanGenericFilterIndex -le $schema52OrphanCleanupIndex -or
	$schema52OrphanContinueIndex -le $schema52OrphanGenericFilterIndex) {
	throw "Schema-52 orphan exact rows must retain durable authority after process-local cleanup"
}
$schema52OrphanCleanupBlock = $schema52InactiveCleanupBlock.Substring($schema52OrphanCleanupIndex, $schema52OrphanContinueIndex - $schema52OrphanCleanupIndex)
if ($schema52OrphanCleanupBlock -match 'm_aActiveGroups\.Remove|m_aMissionAssets\.Remove|m_aConvoyElements\.Remove') {
	throw "Schema-52 orphan cleanup must never delete ambiguous durable group, asset, or element evidence"
}
foreach ($requiredSchema52InactiveCleanupEntry in @(
		'IsProtectedExactMissionConvoyAuthority',
		'HST_OPERATION_SETTLEMENT_SETTLED',
		'protectedMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE',
		'RetireExactMissionConvoyRuntime',
		'continue;'
	)) {
	if ($schema52InactiveCleanupBlock -notmatch [regex]::Escape($requiredSchema52InactiveCleanupEntry)) {
		throw "Schema-52 generic inactive cleanup must defer active settled mission salvage authority: $requiredSchema52InactiveCleanupEntry"
	}
}
$schema52InactiveSettledIndex = $schema52InactiveCleanupBlock.IndexOf('HST_OPERATION_SETTLEMENT_SETTLED')
$schema52InactiveMissionStatusIndex = $schema52InactiveCleanupBlock.IndexOf('protectedMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE', $schema52InactiveSettledIndex)
$schema52InactiveRetireIndex = $schema52InactiveCleanupBlock.IndexOf('RetireExactMissionConvoyRuntime', $schema52InactiveSettledIndex)
if ($schema52InactiveSettledIndex -lt 0 -or $schema52InactiveMissionStatusIndex -le $schema52InactiveSettledIndex -or
	$schema52InactiveRetireIndex -le $schema52InactiveMissionStatusIndex) {
	throw "Schema-52 physical cleanup may retire settled exact runtime only after the mission leaves ACTIVE status"
}
$schema52SettledActiveGuardIndex = $schema52SettledCleanupBlock.IndexOf('mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE')
$schema52SettledResolveIndex = $schema52SettledCleanupBlock.IndexOf('ResolveAuthority', $schema52SettledActiveGuardIndex)
$schema52SettledRetireStateIndex = $schema52SettledCleanupBlock.IndexOf('HST_OPERATION_MATERIALIZATION_RETIRED', $schema52SettledResolveIndex)
if ($schema52SettledActiveGuardIndex -lt 0 -or $schema52SettledResolveIndex -le $schema52SettledActiveGuardIndex -or
	$schema52SettledRetireStateIndex -le $schema52SettledResolveIndex) {
	throw "Schema-52 settled cleanup reconciliation must skip active missions before resolving or retiring salvage runtime"
}

$schema52MaterializingRecoveryGuardIndex = $schema52TickAfterPhysicalBlock.IndexOf('&& !IsRecoveryHold(mission)')
$schema52MaterializingRuntimeIndex = $schema52TickAfterPhysicalBlock.IndexOf('m_PhysicalWar.HasExactMissionConvoyRuntime(mission)', $schema52MaterializingRecoveryGuardIndex)
$schema52MaterializingSyncIndex = $schema52TickAfterPhysicalBlock.IndexOf('SyncPhysicalProjection(state, mission, operation)', $schema52MaterializingRuntimeIndex)
$schema52MaterializingReadyIndex = $schema52TickAfterPhysicalBlock.IndexOf('IsPhysicalProjectionReady(state, mission)', $schema52MaterializingSyncIndex)
if ($schema52MaterializingRecoveryGuardIndex -lt 0 -or $schema52MaterializingRuntimeIndex -le $schema52MaterializingRecoveryGuardIndex -or
	$schema52MaterializingSyncIndex -le $schema52MaterializingRuntimeIndex -or $schema52MaterializingReadyIndex -le $schema52MaterializingSyncIndex) {
	throw "Schema-52 partial materialization must reconcile published casualty/terminal evidence before readiness evaluation"
}
$schema52SyncPhysicalProjectionBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool SyncPhysicalProjection('
if ([string]::IsNullOrEmpty($schema52SyncPhysicalProjectionBlock)) {
	throw "Schema-52 materializing casualty reconciliation method is missing"
}
foreach ($requiredSchema52MaterializingCasualtyEntry in @(
		'asset.m_bDestroyed',
		'HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED',
		'asset.m_bDelivered',
		'asset.m_sLastInteraction == "captured"',
		'HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED',
		'element.m_iSurvivingCrewCount',
		'SynchronizeElementSurvivorsFromMemberSlots'
	)) {
	if ($schema52SyncPhysicalProjectionBlock -notmatch [regex]::Escape($requiredSchema52MaterializingCasualtyEntry)) {
		throw "Schema-52 physical sync must reconcile partial-spawn casualties and terminal transitions into exact member authority: $requiredSchema52MaterializingCasualtyEntry"
	}
}

$schema52RosterDerivationBlock = Get-ScriptMethodBlock $missionConvoyOperationText 'protected bool SynchronizeElementSurvivorsFromMemberSlots('
if ([string]::IsNullOrEmpty($schema52RosterDerivationBlock)) {
	throw "Schema-52 operation-layer member-slot survivor derivation is missing"
}
foreach ($requiredSchema52RosterDerivationEntry in @(
		'manifest.FindMemberSlot(memberSlotId)',
		'batch.FindSlotResult(memberSlotId)',
		'slot.m_bCasualtyConfirmed',
		'element.m_iSurvivingCrewCount = livingSlots;'
	)) {
	if ($schema52RosterDerivationBlock -notmatch [regex]::Escape($requiredSchema52RosterDerivationEntry)) {
		throw "Schema-52 operation-layer survivor derivation must consume frozen member tombstones: $requiredSchema52RosterDerivationEntry"
	}
}
if ($schema52RosterDerivationBlock -match 'm_bCasualtyConfirmed\s*=\s*true' -or
	$schema52RosterDerivationBlock -match 'm_sRetirementReason\s*=' -or
	$schema52RosterDerivationBlock -match 'group\.m_i(?:LastSeenAliveCount|SurvivorInfantryCount)') {
	throw "Schema-52 operation-layer synchronization must never infer or write casualties from aggregate group counts"
}
$schema52GenericSurvivorUpdateBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool UpdateRuntimeGroupSurvivors('
if ([string]::IsNullOrEmpty($schema52GenericSurvivorUpdateBlock)) {
	throw "Schema-52 generic survivor reconciliation exclusion boundary is missing"
}
$schema52ExactSurvivorBranchStart = $schema52GenericSurvivorUpdateBlock.IndexOf('if (IsExactMissionConvoyContract(convoyMission))')
$schema52ExactSurvivorBranchEnd = $schema52GenericSurvivorUpdateBlock.IndexOf('if (missionConvoyGroup)', $schema52ExactSurvivorBranchStart)
if ($schema52ExactSurvivorBranchStart -lt 0 -or $schema52ExactSurvivorBranchEnd -le $schema52ExactSurvivorBranchStart) {
	throw "Schema-52 generic survivor updater must isolate exact convoys before legacy repair"
}
$schema52ExactSurvivorBranch = $schema52GenericSurvivorUpdateBlock.Substring($schema52ExactSurvivorBranchStart, $schema52ExactSurvivorBranchEnd - $schema52ExactSurvivorBranchStart)
if ($schema52ExactSurvivorBranch -notmatch [regex]::Escape('ReconcileExactMissionConvoyMappedSurvivors') -or
	$schema52ExactSurvivorBranch -notmatch 'continue\s*;' -or
	$schema52ExactSurvivorBranch -match 'TryRepairMissionConvoyCrewPopulation|TryRepairEmptyRuntimeGroupPopulation|ReconcileActiveGroupRuntimeMemberCounts') {
	throw "Schema-52 exact convoys must bypass all generic member repair and aggregate survivor mutation paths"
}

$schema52PersistenceText = Get-Content -Raw 'Scripts/Game/HST/Services/HST_PersistenceService.c'
$schema52PreSaveAuthorityBlock = Get-ScriptMethodBlock $physicalWarText 'bool PrepareExactMissionConvoyAuthorityForPersistence('
$schema52PreSaveReconcileBlock = Get-ScriptMethodBlock $physicalWarText 'protected bool TryReconcileExactMissionConvoyMappedSurvivors('
$schema52PrepareCaptureBlock = Get-ScriptMethodBlock $schema52PersistenceText 'protected bool PrepareStateForCapture('
$schema52RequestCheckpointBlock = Get-ScriptMethodBlock $schema52PersistenceText 'bool RequestCheckpoint('
$schema52PersistenceTickBlock = Get-ScriptMethodBlock $schema52PersistenceText 'void Tick('
if ([string]::IsNullOrEmpty($schema52PreSaveAuthorityBlock) -or [string]::IsNullOrEmpty($schema52PreSaveReconcileBlock) -or
	[string]::IsNullOrEmpty($schema52PrepareCaptureBlock) -or [string]::IsNullOrEmpty($schema52RequestCheckpointBlock) -or
	[string]::IsNullOrEmpty($schema52PersistenceTickBlock)) {
	throw "Schema-52 fail-closed persistence roster boundary is missing"
}
foreach ($requiredSchema52PreSaveAuthorityEntry in @(
		'HasConsistentExactMissionConvoyMemberIdentityArrays',
		'CountExactMissionConvoyMemberMappings',
		'IsExactMissionConvoyOutboundProjectionTransactionOpen',
		'without open PHYSICAL authority',
		'TryReconcileExactMissionConvoyMappedSurvivors'
	)) {
	if ($schema52PreSaveAuthorityBlock -notmatch [regex]::Escape($requiredSchema52PreSaveAuthorityEntry)) {
		throw "Schema-52 pre-save boundary must reconcile or reject every process-local exact roster: $requiredSchema52PreSaveAuthorityEntry"
	}
}
foreach ($requiredSchema52PreSaveReconcileEntry in @(
		'TrySampleExactMissionConvoyMappedSurvivors',
		'BuildExactMissionConvoyRosterMutationPlan',
		'ApplyExactMissionConvoyRosterMutationPlan',
		'out bool changed',
		'out string reason'
	)) {
	if ($schema52PreSaveReconcileBlock -notmatch [regex]::Escape($requiredSchema52PreSaveReconcileEntry)) {
		throw "Schema-52 pre-save roster reconciliation must separate acceptance from mutation: $requiredSchema52PreSaveReconcileEntry"
	}
}
foreach ($requiredSchema52PersistenceBoundaryEntry in @(
		'SetPhysicalWarService',
		'PrepareExactMissionConvoyAuthorityForPersistence',
		'checkpoint deferred: exact convoy roster',
		'PrepareStateForCapture(state, "campaign debug isolation baseline")',
		'PrepareStateForCapture(state, persistenceStatus)',
		'PrepareStateForCapture(state, "campaign debug tracked-state restore")'
	)) {
	if ($schema52PersistenceText -notmatch [regex]::Escape($requiredSchema52PersistenceBoundaryEntry)) {
		throw "Schema-52 production persistence must gate every real capture on exact roster reconciliation: $requiredSchema52PersistenceBoundaryEntry"
	}
}
$schema52CaptureAndTrackBlock = Get-ScriptMethodBlock $schema52PersistenceText 'HST_CampaignSaveData CaptureAndTrackState('
if ([string]::IsNullOrEmpty($schema52CaptureAndTrackBlock) -or
	$schema52CaptureAndTrackBlock.IndexOf('PrepareStateForCapture(state, persistenceStatus)') -lt 0 -or
	$schema52CaptureAndTrackBlock.IndexOf('PrepareStateForCapture(state, persistenceStatus)') -gt $schema52CaptureAndTrackBlock.IndexOf('m_LastCapturedSave.Capture(state)')) {
	throw "Schema-52 tracked capture must reconcile exact rosters before mutating or serializing the save"
}
if ($schema52RequestCheckpointBlock -notmatch [regex]::Escape('state && !CaptureAndTrackState(state, "captured before checkpoint")')) {
	throw "Schema-52 checkpoint requests must stop before flushing stale data when exact-roster capture is deferred"
}
foreach ($requiredSchema52PersistenceRetryEntry in @(
		'bool majorCheckpointSaved = RequestCheckpoint',
		'if (majorCheckpointSaved)',
		'if (RequestCheckpoint("h-istasi autosave", state))',
		'retrySeconds'
	)) {
	if ($schema52PersistenceTickBlock -notmatch [regex]::Escape($requiredSchema52PersistenceRetryEntry)) {
		throw "Schema-52 deferred checkpoints must retain intent and retry on a bounded cadence: $requiredSchema52PersistenceRetryEntry"
	}
}
if ($coordinatorText -notmatch [regex]::Escape('m_Persistence.SetPhysicalWarService(m_PhysicalWar)')) {
	throw "Schema-52 coordinator must inject PhysicalWar into persistence before campaign restore/capture"
}

$schema52QueueText = $operationProjectionQueueText
$schema52QueueAcquireBlock = Get-ScriptMethodBlock $schema52QueueText 'HST_ForceSpawnQueueTickResult AcquireWork('
$schema52QueueSelectBlock = Get-ScriptMethodBlock $schema52QueueText 'protected HST_ForceSpawnResultState SelectNextWorkBatch('
$schema52QueueRestoreBlock = Get-ScriptMethodBlock $schema52QueueText 'HST_ForceSpawnQueueMaintenanceResult ReconcileAfterRestore('
$schema52QueueDuplicateBlock = Get-ScriptMethodBlock $schema52QueueText 'protected bool FailClosedDuplicateNonterminalKeys('
$schema52QueueCollisionBlock = Get-ScriptMethodBlock $schema52QueueText 'protected bool CollidesWithExternallyManagedMissionConvoyBatch('
$schema52QueueExternalOwnerBlock = Get-ScriptMethodBlock $schema52QueueText 'protected bool IsExternallyManagedMissionConvoyBatch('
if ([string]::IsNullOrEmpty($schema52QueueAcquireBlock) -or [string]::IsNullOrEmpty($schema52QueueSelectBlock) -or
	[string]::IsNullOrEmpty($schema52QueueRestoreBlock) -or [string]::IsNullOrEmpty($schema52QueueDuplicateBlock) -or
	[string]::IsNullOrEmpty($schema52QueueCollisionBlock) -or [string]::IsNullOrEmpty($schema52QueueExternalOwnerBlock)) {
	throw "Schema-52 generic spawn-queue collision hold methods are missing"
}
$schema52QueueAcquireExternalIndex = $schema52QueueAcquireBlock.IndexOf('IsExternallyManagedMissionConvoyBatch')
$schema52QueueAcquireCollisionIndex = $schema52QueueAcquireBlock.IndexOf('CollidesWithExternallyManagedMissionConvoyBatch', $schema52QueueAcquireExternalIndex)
$schema52QueueAcquirePrepareIndex = $schema52QueueAcquireBlock.IndexOf('PrepareBatchForTick', $schema52QueueAcquireCollisionIndex)
if ($schema52QueueAcquireExternalIndex -lt 0 -or $schema52QueueAcquireCollisionIndex -le $schema52QueueAcquireExternalIndex -or
	$schema52QueueAcquirePrepareIndex -le $schema52QueueAcquireCollisionIndex) {
	throw "Schema-52 generic queue acquisition must hold external and identity-colliding exact convoy batches before preparation"
}
$schema52QueueSelectExternalIndex = $schema52QueueSelectBlock.IndexOf('IsExternallyManagedMissionConvoyBatch')
$schema52QueueSelectCollisionIndex = $schema52QueueSelectBlock.IndexOf('CollidesWithExternallyManagedMissionConvoyBatch', $schema52QueueSelectExternalIndex)
$schema52QueueSelectEligibilityIndex = $schema52QueueSelectBlock.IndexOf('IsWorkEligible', $schema52QueueSelectCollisionIndex)
if ($schema52QueueSelectExternalIndex -lt 0 -or $schema52QueueSelectCollisionIndex -le $schema52QueueSelectExternalIndex -or
	$schema52QueueSelectEligibilityIndex -le $schema52QueueSelectCollisionIndex) {
	throw "Schema-52 generic queue selection must hold external and identity-colliding exact convoy batches before eligibility"
}
$schema52QueueRestoreExternalIndex = $schema52QueueRestoreBlock.IndexOf('IsExternallyManagedMissionConvoyBatch')
$schema52QueueRestoreCollisionIndex = $schema52QueueRestoreBlock.IndexOf('CollidesWithExternallyManagedMissionConvoyBatch', $schema52QueueRestoreExternalIndex)
$schema52QueueRestoreTerminalIndex = $schema52QueueRestoreBlock.IndexOf('IsTerminalBatch', $schema52QueueRestoreCollisionIndex)
$schema52QueueRestoreReconcileIndex = $schema52QueueRestoreBlock.IndexOf('ReconcileRestoredBatch', $schema52QueueRestoreCollisionIndex)
if ($schema52QueueRestoreExternalIndex -lt 0 -or $schema52QueueRestoreCollisionIndex -le $schema52QueueRestoreExternalIndex -or
	$schema52QueueRestoreTerminalIndex -le $schema52QueueRestoreCollisionIndex -or $schema52QueueRestoreReconcileIndex -le $schema52QueueRestoreCollisionIndex) {
	throw "Schema-52 generic restore reconciliation must hold identity-colliding exact convoy evidence before any terminal or mutable reconciliation"
}
$schema52QueueDuplicateExternalIndex = $schema52QueueDuplicateBlock.IndexOf('IsExternallyManagedMissionConvoyBatch')
$schema52QueueDuplicateCollisionIndex = $schema52QueueDuplicateBlock.IndexOf('CollidesWithExternallyManagedMissionConvoyBatch', $schema52QueueDuplicateExternalIndex)
$schema52QueueDuplicateMutationIndex = $schema52QueueDuplicateBlock.IndexOf('BeginCleanup', $schema52QueueDuplicateCollisionIndex)
if ($schema52QueueDuplicateExternalIndex -lt 0 -or $schema52QueueDuplicateCollisionIndex -le $schema52QueueDuplicateExternalIndex -or
	$schema52QueueDuplicateMutationIndex -le $schema52QueueDuplicateCollisionIndex) {
	throw "Schema-52 duplicate-key fail-closed logic must preserve externally owned and colliding mission-convoy evidence"
}
foreach ($requiredSchema52QueueCollisionEntry in @(
		'IsTerminalBatch(batch)',
		'IsExternallyManagedMissionConvoyBatch(batch, manifests)',
		'IsTerminalBatch(externalBatch)',
		'IsExternallyManagedMissionConvoyBatch(externalBatch, manifests)',
		'batch.m_sResultId == externalBatch.m_sResultId',
		'batch.m_sRequestId == externalBatch.m_sRequestId',
		'batch.m_sProjectionId == externalBatch.m_sProjectionId'
	)) {
	if ($schema52QueueCollisionBlock -notmatch [regex]::Escape($requiredSchema52QueueCollisionEntry)) {
		throw "Schema-52 generic queue collision hold must compare every nonterminal durable identity against externally managed convoy evidence: $requiredSchema52QueueCollisionEntry"
	}
}
foreach ($requiredSchema52QueueOwnerEntry in @(
		'HST_MissionConvoyOperationService.EXACT_FORCE_KIND',
		'HST_MissionConvoyOperationService.EXACT_POLICY_ID',
		'spawn_mission_convoy_',
		'manifest_mission_convoy_',
		'operation_mission_convoy_',
		'force_mission_convoy_',
		'projection_mission_convoy_'
	)) {
	if ($schema52QueueExternalOwnerBlock -notmatch [regex]::Escape($requiredSchema52QueueOwnerEntry)) {
		throw "Schema-52 generic queue must recognize exact mission-convoy ownership even when manifest evidence is missing or conflicting: $requiredSchema52QueueOwnerEntry"
	}
}

foreach ($requiredSchema52ProofEntry in @(
		'class HST_MissionConvoyOperationProofReport',
		'class HST_MissionConvoyOperationProofService',
		'HST_MissionConvoyOperationProofReport Run()',
		'm_bAdmissionExact',
		'm_bAdmissionRollbackExact',
		'm_bProjectionExact',
		'm_bCasualtyRestoreExact',
		'm_bSettlementExact',
		'm_bRestoreExact',
		'm_bCorruptionRejected',
		'm_bMarkerExact',
		'm_bWatchdogExact',
		'ProveAdmissionRollback',
		'ProveRestore',
		'ProveMarkers',
		'ProveWatchdog',
		'staleGroupSnapshotPrepared',
		'cursorOnlyRejected',
		'ProveRecoveryRestore',
		'missingRejected',
		'AppendCampaignDebugMissionConvoyOperationAssertions',
		'mission_convoy.admission',
		'mission_convoy.admission_rollback',
		'mission_convoy.projection',
		'mission_convoy.casualty_restore',
		'mission_convoy.settlement',
		'mission_convoy.restore',
		'mission_convoy.corruption_rejection',
		'mission_convoy.marker',
		'mission_convoy.watchdog',
		'ProveRestoreForeignAuthorityCorruption',
		'ProveRestoreSeatTopologyCorruption',
		'ProveRestoreArrivalReceiptCorruption',
		'restore duplicate seat rejected',
		'forged arrival receipt rejected',
		'restoredCanonical.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN',
		'restoredBatch.m_bStrategicProjectionHeld',
		'ProveMissionlessDurableClaimantPreservation',
		'missionless prefix/element claimant survived restore/cleanup',
		'ReconcileInactiveMissionConvoyRuntime',
		'ProveInvalidCargoAdmission',
		'ProveInvalidCargoSourceAdmission',
		'cargo admission rejected captive-object/payload-character source types',
		'ProveRestoreMissingRequiredCargo',
		'coherent restore without required payload rejected',
		'requires exactly one compatible cargo row',
		'manifest.m_aAssets.Remove(assetSlotIndex)',
		'saveData.m_aMissionAssets.Remove(cargoIndex)',
		'batch.m_iExpectedSlotCount = batch.m_aSlotResults.Count()',
		'ProveRestoreCaptiveSourceTypeCorruption',
		'coherent restore with non-character captive rejected',
		'not a boardable character with compartment access',
		'batchSlot.m_sSpawnedPrefab = PROOF_CARGO_PREFAB',
		'batch.m_sManifestHash = manifest.m_sManifestHash',
		'integrity.BuildManifestHash(restoredManifest) == restoredManifest.m_sManifestHash',
		'ProveRestoreLifecycleGuards',
		'restore rejected casualty group/vehicle/asset'
	)) {
	if (($missionConvoyProofText + "`n" + $coordinatorText) -notmatch [regex]::Escape($requiredSchema52ProofEntry)) {
		throw "Schema-52 exact mission-convoy proof integration missing: $requiredSchema52ProofEntry"
	}
}

foreach ($requiredSchema52MissionRequirement in @(
	'Kill all convoy soldiers before the convoy reaches its destination, then capture a surviving vehicle to establish the ammo point.',
	'Kill all convoy soldiers before the convoy reaches its destination, then capture a surviving vehicle for the garage.',
	'Kill all convoy soldiers before the convoy reaches its destination, then recover and deliver the money payload to HQ.',
	'Kill all convoy soldiers before the convoy reaches its destination, then free and extract the prisoners.',
	'Kill all convoy soldiers before the convoy reaches its destination, then recover and deliver the supplies.'
)) {
	if ($missionConfig -notmatch [regex]::Escape($requiredSchema52MissionRequirement) -or
		$defaultCatalog -notmatch [regex]::Escape($requiredSchema52MissionRequirement)) {
		throw "Schema-52 convoy requirement text must describe its actual required recovery outcome: $requiredSchema52MissionRequirement"
	}
}
if (($missionConfig + "`n" + $defaultCatalog + "`n" + $commandUIServiceText) -match [regex]::Escape('Capturing surviving vehicles is optional.')) {
	throw "Schema-52 convoy player text must not describe required ammo/armor recovery as optional"
}

$schema52ConvoyOutcomeOwnershipText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ConvoyOutcomeService.c"
foreach ($schema52GroupOwnershipText in @($schema52ConvoyOutcomeOwnershipText, $missionRuntimeServiceText, $commandUIServiceText)) {
	$schema52GroupOwnershipBlock = Get-ScriptMethodBlock $schema52GroupOwnershipText 'protected bool IsConvoyGroupOwnedByMission('
	if ([string]::IsNullOrEmpty($schema52GroupOwnershipBlock)) {
		throw "Schema-52 convoy consumer is missing deterministic group-prefix ownership validation"
	}
	foreach ($requiredSchema52GroupOwnershipEntry in @(
		'activeGroup.m_sGroupId.StartsWith(groupPrefix)',
		'activeGroup.m_sMissionInstanceId == mission.m_sInstanceId',
		'activeGroup.m_sOperationId == mission.m_sOperationId',
		'!activeGroup.m_sConvoyElementId.IsEmpty()'
	)) {
		if ($schema52GroupOwnershipBlock -notmatch [regex]::Escape($requiredSchema52GroupOwnershipEntry)) {
			throw "Schema-52 convoy group ownership must use an anchored ID plus reciprocal exact authority: $requiredSchema52GroupOwnershipEntry"
		}
	}
	if ($schema52GroupOwnershipText -match [regex]::Escape('.m_sGroupId.Contains(groupPrefix)')) {
		throw "Schema-52 convoy group consumers must not accept substring ID collisions"
	}
}

$schema52ExactCaptureBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool ApplyExactMissionConvoyVehicleCaptureInteraction('
$schema52CargoHandoffValidateBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool ValidateExactMissionConvoyCarrierCargoForVehicleHandoff('
$schema52CargoHandoffCommitBlock = Get-ScriptMethodBlock $missionRuntimeServiceText 'protected bool CommitExactMissionConvoyCarrierCargoForVehicleHandoff('
if ([string]::IsNullOrEmpty($schema52ExactCaptureBlock) -or [string]::IsNullOrEmpty($schema52CargoHandoffValidateBlock) -or
	[string]::IsNullOrEmpty($schema52CargoHandoffCommitBlock)) {
	throw "Schema-52 exact vehicle capture cargo transaction helpers are missing"
}
if ($schema52CargoHandoffValidateBlock -match 'DeleteRuntimeEntity\s*\(' -or
	$schema52CargoHandoffValidateBlock -match 'cargo\.m_(?:bSpawned|bAttachedToCarrier|sCarriedByVehicleId|vCurrentPosition|vLastKnownPosition)\s*=') {
	throw "Schema-52 exact capture cargo validation must be read-only"
}
foreach ($requiredSchema52CargoHandoffCommitEntry in @(
	'DeleteRuntimeEntity(cargo.m_sEntityId)',
	'cargo.m_bSpawned = false;',
	'cargo.m_bAttachedToCarrier = false;',
	'cargo.m_sCarriedByVehicleId = "";',
	'runtimeCargo.m_bSpawned = false;'
)) {
	if ($schema52CargoHandoffCommitBlock -notmatch [regex]::Escape($requiredSchema52CargoHandoffCommitEntry)) {
		throw "Schema-52 exact capture cargo commit is incomplete: $requiredSchema52CargoHandoffCommitEntry"
	}
}
$schema52CargoValidateIndex = $schema52ExactCaptureBlock.IndexOf('ValidateExactMissionConvoyCarrierCargoForVehicleHandoff')
$schema52GarageReserveIndex = $schema52ExactCaptureBlock.IndexOf('arsenal.StoreVehicle', $schema52CargoValidateIndex)
$schema52PhysicalHandoffIndex = $schema52ExactCaptureBlock.IndexOf('TryHandoffExactMissionConvoyVehicleCapture', $schema52GarageReserveIndex)
$schema52CargoCommitIndex = $schema52ExactCaptureBlock.IndexOf('CommitExactMissionConvoyCarrierCargoForVehicleHandoff', $schema52PhysicalHandoffIndex)
$schema52CargoProjectionIndex = $schema52ExactCaptureBlock.IndexOf('TickExactMissionConvoyCargoProjections', $schema52CargoCommitIndex)
$schema52CarrierDeleteIndex = $schema52ExactCaptureBlock.IndexOf('SCR_EntityHelper.DeleteEntityAndChildren(handedOffVehicle)', $schema52CargoProjectionIndex)
if ($schema52CargoValidateIndex -lt 0 -or $schema52GarageReserveIndex -le $schema52CargoValidateIndex -or
	$schema52PhysicalHandoffIndex -le $schema52GarageReserveIndex -or $schema52CargoCommitIndex -le $schema52PhysicalHandoffIndex -or
	$schema52CargoProjectionIndex -le $schema52CargoCommitIndex -or $schema52CarrierDeleteIndex -le $schema52CargoProjectionIndex) {
	throw "Schema-52 exact capture must validate cargo, reserve the garage, hand off authority, commit/project cargo, then delete the carrier"
}
Write-Host "Schema-52 exact mission-convoy authority, persistence, projection, arrival, recovery, and proof contract OK"
$forceSpawnQueueServicePath = "Scripts/Game/HST/Services/HST_ForceSpawnQueueService.c"
if (!(Test-Path $forceSpawnQueueServicePath)) {
	throw "Missing durable force spawn queue service: $forceSpawnQueueServicePath"
}
$forceSpawnQueueServiceText = Get-Content -Raw $forceSpawnQueueServicePath
$forceAuthorityDataText = Get-Content -Raw "Scripts/Game/HST/Data/HST_ForceAuthority.c"
$forceSaveDataText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$forcePersistenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PersistenceService.c"
foreach ($requiredForceSpawnQueueStateEntry in @(
		"HST_FORCE_SLOT_SPAWNING",
		"HST_FORCE_SLOT_CLEANUP_PENDING",
		"HST_FORCE_SPAWN_IN_PROGRESS",
		"HST_FORCE_SPAWN_CLEANUP_PENDING",
		"m_sRequestId",
		"m_sManifestHash",
		"m_sLastFailureReason",
		"m_iAttemptGeneration",
		"m_iLastAttemptSecond",
		"m_iNextAttemptSecond",
		"m_iUpdatedAtSecond",
		"m_bCancelRequested",
		"m_sSpawnedPrefab",
		"m_bGameMasterVerified",
		"m_bAliveVerified"
	)) {
	if ($forceAuthorityDataText -notmatch [regex]::Escape($requiredForceSpawnQueueStateEntry) -and $scriptText -notmatch [regex]::Escape($requiredForceSpawnQueueStateEntry)) {
		throw "Schema-45 force spawn queue state is missing entry: $requiredForceSpawnQueueStateEntry"
	}
}
foreach ($requiredForceSpawnQueueCopyEntry in @(
		"target.m_sRequestId = source.m_sRequestId;",
		"target.m_sManifestHash = source.m_sManifestHash;",
		"target.m_sLastFailureReason = source.m_sLastFailureReason;",
		"target.m_iAttemptGeneration = source.m_iAttemptGeneration;",
		"target.m_iLastAttemptSecond = source.m_iLastAttemptSecond;",
		"target.m_iNextAttemptSecond = source.m_iNextAttemptSecond;",
		"target.m_iUpdatedAtSecond = source.m_iUpdatedAtSecond;",
		"target.m_bCancelRequested = source.m_bCancelRequested;",
		"target.m_sSpawnedPrefab = source.m_sSpawnedPrefab;",
		"target.m_bGameMasterVerified = source.m_bGameMasterVerified;",
		"target.m_bAliveVerified = source.m_bAliveVerified;"
	)) {
	if ($forceSaveDataText -notmatch [regex]::Escape($requiredForceSpawnQueueCopyEntry)) {
		throw "Schema-45 force spawn queue deep copy is missing entry: $requiredForceSpawnQueueCopyEntry"
	}
}
foreach ($requiredForceSpawnQueueRestoreEntry in @(
		"m_iPersistenceRestoreSequence",
		"m_iForceSpawnQueueReconciledRestoreSequence",
		"m_iPersistenceRestoreSequence = state.m_iPersistenceRestoreSequence;",
		"m_iForceSpawnQueueReconciledRestoreSequence = state.m_iForceSpawnQueueReconciledRestoreSequence;",
		"state.m_iPersistenceRestoreSequence = m_iPersistenceRestoreSequence;",
		"state.m_iForceSpawnQueueReconciledRestoreSequence = m_iForceSpawnQueueReconciledRestoreSequence;",
		"targetState.m_iPersistenceRestoreSequence = Math.Max(0, targetState.m_iPersistenceRestoreSequence) + 1;",
		"ReconcileAfterRestore",
		"ReconcileCampaignAfterRestore",
		"ClearTerminalProcessIds",
		"state.m_iForceSpawnQueueReconciledRestoreSequence = restoreSequence;"
	)) {
	if (($campaignStateText + "`n" + $forceSaveDataText + "`n" + $forcePersistenceText + "`n" + $forceSpawnQueueServiceText) -notmatch [regex]::Escape($requiredForceSpawnQueueRestoreEntry)) {
		throw "Schema-45 force spawn queue restore contract is missing entry: $requiredForceSpawnQueueRestoreEntry"
	}
}
if ($coordinatorText -notmatch 'RestoreOrCreateCampaignState[\s\S]*?ReconcileCampaignAfterRestore\(m_State\)[\s\S]*?ReconcileInterruptedGarrisonConfirmations[\s\S]*?ReconcileOpenReservations') {
	throw "Coordinator must reconcile the durable force spawn queue immediately after campaign restore and before normal authority reconciliation"
}
foreach ($requiredForceSpawnQueueLimit in @(
		"MAX_NONTERMINAL_BATCHES = 64",
		"MAX_TOTAL_SLOT_ROWS = 512",
		"MAX_SLOTS_PER_REQUEST = 64",
		"MAX_TERMINAL_ROWS = 128",
		"MAX_BATCHES_PER_TICK = 2",
		"MAX_SLOTS_PER_TICK = 8",
		"TERMINAL_RETENTION_SECONDS = 600"
	)) {
	if ($forceSpawnQueueServiceText -notmatch [regex]::Escape($requiredForceSpawnQueueLimit)) {
		throw "Durable force spawn queue bound is missing: $requiredForceSpawnQueueLimit"
	}
}
foreach ($requiredForceSpawnQueueApi in @(
		"Enqueue(",
		"AcquireWork(",
		"CompleteSlotSuccess(",
		"FailSlot(",
		"DeferSlot(",
		"RequestCancel(",
		"FailProjectionFinal(",
		"CompleteProjectionHandoff(",
		"CompleteCleanup(",
		"CompactTerminalRows(",
		"BuildReport("
	)) {
	if ($forceSpawnQueueServiceText -notmatch [regex]::Escape($requiredForceSpawnQueueApi)) {
		throw "Durable force spawn queue API is missing: $requiredForceSpawnQueueApi"
	}
}
foreach ($requiredForceSpawnQueueIdentityEntry in @(
		"manifest has no executable group root",
		"optional force manifest groups require a deployed-manifest policy",
		"optional force manifest vehicles require a deployed-manifest policy",
		"optional force manifest members require a deployed-manifest policy",
		"optional force manifest assets require a deployed-manifest policy",
		"caller-supplied durable result id missing",
		"durable request id missing",
		"projection id missing",
		"CountByRequest",
		"CountByProjection",
		"CountByResult",
		"BatchIdentityMatches",
		"spawn queue idempotency key conflicts with existing payload",
		"terminal compaction refused without explicit backlink retention pins"
	)) {
	if ($forceSpawnQueueServiceText -notmatch [regex]::Escape($requiredForceSpawnQueueIdentityEntry)) {
		throw "Durable force spawn queue identity/admission contract is missing: $requiredForceSpawnQueueIdentityEntry"
	}
}
$spawnQueueProofFiles = @(Get-ChildItem -File "Scripts/Game/HST/Services" -Filter "*SpawnQueue*Proof*.c")
if ($spawnQueueProofFiles.Count -eq 0) {
	throw "Schema-45 force spawn queue requires a standalone deterministic proof service"
}
$spawnQueueProofText = ($spawnQueueProofFiles | ForEach-Object { Get-Content -Raw $_.FullName }) -join "`n"
foreach ($requiredSpawnQueueProofEntry in @(
		"BuildCampaignDebugSpawnQueueCase",
		'RecordCampaignDebugCase(BuildCampaignDebugSpawnQueueCase());',
		"early_mechanics.spawn_queue",
		"spawn_queue.admission_required_slots",
		"spawn_queue.duplicate_request",
		"spawn_queue.identity_conflict",
		"spawn_queue.priority_order",
		"spawn_queue.fifo_order",
		"spawn_queue.retry_backoff",
		"spawn_queue.retry_no_duplicate",
		"spawn_queue.stale_generation",
		"spawn_queue.same_wave_progression",
		"spawn_queue.cleanup_dependency_order",
		"spawn_queue.deadline_cleanup",
		"spawn_queue.cancel_idempotency",
		"spawn_queue.capacity_bounds",
		"spawn_queue.terminal_pruning",
		"spawn_queue.interrupted_restore",
		"spawn_queue.terminal_restore",
		"spawn_queue.schema43_migration",
		"spawn_queue.schema45_active_group_identity"
	)) {
	if (($coordinatorText + "`n" + $spawnQueueProofText) -notmatch [regex]::Escape($requiredSpawnQueueProofEntry)) {
		throw "Spawn queue deterministic proof is missing phase-0 entry: $requiredSpawnQueueProofEntry"
	}
}
Write-Host "Schema-45 durable bounded force spawn queue contract OK"
$forceSpawnAdapterServicePath = "Scripts/Game/HST/Services/HST_ForceSpawnAdapterService.c"
$physicalWarServicePath = "Scripts/Game/HST/Services/HST_PhysicalWarService.c"
if (!(Test-Path $forceSpawnAdapterServicePath)) {
	throw "Missing exact engine-facing force spawn adapter: $forceSpawnAdapterServicePath"
}
if (!(Test-Path $physicalWarServicePath)) {
	throw "Missing physical-war force spawn bridge: $physicalWarServicePath"
}
$forceSpawnAdapterServiceText = Get-Content -Raw $forceSpawnAdapterServicePath
$physicalWarServiceText = Get-Content -Raw $physicalWarServicePath
foreach ($requiredSchema45ProjectionEntry in @(
		"string m_sForceId;",
		"string m_sProjectionId;",
		"target.m_sForceId = source.m_sForceId;",
		"target.m_sProjectionId = source.m_sProjectionId;",
		"MigrateActiveGroupProjectionIdentity",
		"migration_schema45_active_group_projection_derived",
		"migration_schema45_active_group_projection_unresolved"
	)) {
	if (($campaignStateText + "`n" + $forceSaveDataText) -notmatch [regex]::Escape($requiredSchema45ProjectionEntry)) {
		throw "Schema-45 active-group projection identity contract is missing: $requiredSchema45ProjectionEntry"
	}
}
if ($forceSpawnQueueServiceText -notmatch 'AppendCleanupWorkItemsForKind\(workItems, batch, SLOT_KIND_ASSET[\s\S]*?SLOT_KIND_MEMBER[\s\S]*?SLOT_KIND_VEHICLE[\s\S]*?SLOT_KIND_GROUP') {
	throw "Force spawn cleanup acquisition must remain dependency ordered: asset, member, vehicle, group"
}
foreach ($requiredForceSpawnAdapterEntry in @(
		"class HST_ForceSpawnAdapterService",
		'ADAPTER_MODE = "exact_spawn_queue"',
		"HST_FORCE_SPAWN_READY_FOR_HANDOFF",
		"UNSUPPORTED_MANIFEST_REASON",
		"queue.AcquireWork(",
		"queue.CompleteSlotSuccess(",
		"queue.CompleteCleanup(",
		"SpawnGroupRoot(",
		"SpawnGroupMember(",
		"FinalizeTouchedBatches(",
		"RetireProjectionRuntime(",
		"PrepareForceSpawnProjectionCleanup(",
		"m_bGameMasterVerified = gameMasterVerified;",
		"SCR_AIGroup.IgnoreSpawning(true);",
		"GetGame().SpawnEntityPrefabEx(resourceName, false",
		"physical adapter supports one exact infantry group without vehicle or asset slots"
		"MAX_RECONCILE_BATCHES_PER_TICK = 4",
		"MAX_HANDLE_RECONCILE_PER_TICK = 8",
		"acknowledged unmaterialized cleanup slot",
		"TryResolveSafeGroundPosition(",
		"active-group faction conflicts with the frozen manifest",
		"FindActiveGroupForBatch(",
		"PruneDeletedProjectionBindings("
	)) {
	if ($forceSpawnAdapterServiceText -notmatch [regex]::Escape($requiredForceSpawnAdapterEntry)) {
		throw "Exact engine-facing force spawn adapter contract is missing: $requiredForceSpawnAdapterEntry"
	}
}
foreach ($requiredForceSpawnBridgeEntry in @(
		"IsForceSpawnQueueManaged(",
		"ShouldHoldForceSpawnProjection(",
		"TryRegisterForceSpawnGroupRoot(",
		"TryRegisterForceSpawnGroupMember(",
		"TryUnregisterForceSpawnGroupMember(",
		"TryUnregisterForceSpawnGroupRoot(",
		"FinalizeForceSpawnProjection(",
		"PrepareForceSpawnProjectionCleanup(",
		"ValidateForceSpawnGroupCardinality(",
		"EnsureForceSpawnNextMemberAIWorldBudget(",
		"AcquireForceSpawnRuntimeOwnership(",
		"IsForceSpawnRuntimeOwnershipHeldForGroup(",
		"force spawn group root cleanup requires all exact members to be removed first",
		"if (IsForceSpawnQueueManaged(activeGroup))"
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredForceSpawnBridgeEntry)) {
		throw "Physical-war exact force spawn bridge/legacy guard is missing: $requiredForceSpawnBridgeEntry"
	}
}
foreach ($requiredForceSpawnCoordinatorEntry in @(
		"m_ForceSpawnAdapter = new HST_ForceSpawnAdapterService();",
		"TickForceSpawnQueueRuntime()",
		"m_ForceSpawnAdapter.Tick(",
		"forceSpawnQueueChanged",
		"m_ForceSpawnAdapter.BuildReport()"
		"TickForceSpawnQueueTerminalCleanup(",
		"HasUnsafeForceSpawnRuntimeForDebugIsolation(",
		"physicalWarMarkerChanged = m_PhysicalWar.ConsumeMarkerRefreshNeeded();"
	)) {
	if ($coordinatorText -notmatch [regex]::Escape($requiredForceSpawnCoordinatorEntry)) {
		throw "Coordinator exact force spawn tick wiring is missing: $requiredForceSpawnCoordinatorEntry"
	}
}
$forceRuntimeElapsedIndex = $coordinatorText.IndexOf("m_State.m_iElapsedSeconds += elapsedSeconds;")
$forceRuntimeQueueIndex = $coordinatorText.IndexOf("bool forceSpawnQueueChanged = TickForceSpawnQueueRuntime();", $forceRuntimeElapsedIndex + 1)
$forceRuntimeGarrisonCleanupIndex = $coordinatorText.IndexOf("bool exactGarrisonPatrolCleanupChanged = m_GarrisonPatrolOperations", $forceRuntimeQueueIndex + 1)
$forceRuntimeGarrisonTickIndex = $coordinatorText.IndexOf("bool exactGarrisonPatrolChanged = m_GarrisonPatrolOperations", $forceRuntimeGarrisonCleanupIndex + 1)
$forceRuntimeMissionIndex = $coordinatorText.IndexOf("bool missionChanged = m_Missions.Tick", $forceRuntimeGarrisonTickIndex + 1)
$forceRuntimeOrderExact = $forceRuntimeElapsedIndex -ge 0
$forceRuntimeOrderExact = $forceRuntimeOrderExact -and $forceRuntimeQueueIndex -gt $forceRuntimeElapsedIndex
$forceRuntimeOrderExact = $forceRuntimeOrderExact -and $forceRuntimeGarrisonCleanupIndex -gt $forceRuntimeQueueIndex
$forceRuntimeOrderExact = $forceRuntimeOrderExact -and $forceRuntimeGarrisonTickIndex -gt $forceRuntimeGarrisonCleanupIndex
$forceRuntimeOrderExact = $forceRuntimeOrderExact -and $forceRuntimeMissionIndex -gt $forceRuntimeGarrisonTickIndex
if (!$forceRuntimeOrderExact) {
	throw "Coordinator must order active runtime as elapsed time -> exact queue -> exact garrison cleanup/tick -> mission work"
}
$forceSpawnAdapterProofPath = "Scripts/Game/HST/Services/HST_ForceSpawnAdapterProofService.c"
if (!(Test-Path $forceSpawnAdapterProofPath)) {
	throw "Missing engine-facing exact spawn-adapter proof service: $forceSpawnAdapterProofPath"
}
$forceSpawnAdapterProofText = Get-Content -Raw $forceSpawnAdapterProofPath
foreach ($requiredForceSpawnAdapterProofEntry in @(
		"class HST_ForceSpawnAdapterProofService",
		"early_mechanics.spawn_queue_physical_adapter",
		"spawn_adapter.root_before_members",
		"spawn_adapter.cancel_cleanup",
		"spawn_adapter.exact_prefabs",
		"spawn_adapter.native_membership",
		"spawn_adapter.game_master_membership",
		"spawn_adapter.durable_success",
		"spawn_adapter.active_group_handoff",
		"spawn_adapter.retirement",
		"spawn_adapter.same_wave_failure_cleanup",
		"spawn_adapter.state_isolation",
		"PruneDeletedProjectionBindings(",
		"UNAVAILABLE_MEMBER_PREFAB"
	)) {
	if (($coordinatorText + "`n" + $forceSpawnAdapterProofText) -notmatch [regex]::Escape($requiredForceSpawnAdapterProofEntry)) {
		throw "Engine-facing exact spawn-adapter proof is missing: $requiredForceSpawnAdapterProofEntry"
	}
}
Write-Host "Schema-45 exact force spawn adapter and PhysicalWar bridge contract OK"
foreach ($requiredDebugIsolationEntry in @(
		"PrepareCampaignDebugIsolation",
		"CaptureIsolatedCampaignDebugState",
		"RestoreTrackedStateAfterCampaignDebug",
		"m_bCampaignDebugIsolationActive",
		"m_CampaignDebugLiveState",
		"m_CampaignDebugStateSnapshot",
		'worldFile.Contains("worlds/hst_dev/hst_dev.ent")',
		'normalizedProfile == "persistence_restart_external" || normalizedProfile == "background_soak" || normalizedProfile == "external_required"',
		"BeginCampaignDebugStateIsolation",
		"BuildCampaignDebugStateIsolationStartCase",
		"preflight.state_isolation",
		"RestoreCampaignDebugStateSnapshot",
		'CaptureIsolatedCampaignDebugState(m_State, "isolated manual checkpoint")',
		'RecordCampaignDebugCase(RestoreCampaignDebugStateSnapshot("run cancellation"))',
		'RecordCampaignDebugCase(RestoreCampaignDebugStateSnapshot("run completion"))',
		"isolation.world_scope",
		"development session restart required"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredDebugIsolationEntry)) {
		throw "Campaign debug state-isolation contract missing: $requiredDebugIsolationEntry"
	}
}
Write-Host "Campaign debug state-isolation contract OK"
foreach ($requiredMarkerReadinessEntry in @(
		"modded class SCR_MapMarkerManagerComponent",
		"if (!marker.GetRootWidget())",
		"marker.OnCreateMarker(true)",
		"SetStaticMarkerDisabled(marker, true)",
		"staticRootsReady",
		"staticRootMissing",
		"staticRootMissingSamples",
		"disabledStaticRootMissing",
		"map_ui.rendered_static_marker_widgets",
		"delayed owner-client proof has a root and widget component for every active static marker",
		"BuildNativeHandleReport"
	)) {
	if ($scriptText -notmatch [regex]::Escape($requiredMarkerReadinessEntry)) {
		throw "Static map-marker readiness contract missing: $requiredMarkerReadinessEntry"
	}
}
if ($scriptText -match "HasRuntimeNativeMarkerWidgets" -or $scriptText -match "CountRuntimeNativeMarkerWidgets" -or $scriptText -match "BuildNativeWidgetReport") {
	throw "Server marker publication must not masquerade as client widget readiness"
}
Write-Host "Static map-marker readiness contract OK"
if ($scriptText -match "m_sDefaultHideoutId" -or $scriptText -match '"defaultHideoutId"') {
	throw "Runtime settings JSON must not expose defaultHideoutId after map-based HQ selection"
}
foreach ($requiredSettingsComment in @(
		'\\"_comment\\":',
		'\\"_comment_schemaVersion\\":',
		'\\"_comment_startingFactionMoney\\":',
		'\\"_comment_adminIdentityIds\\":',
		'\\"_comment_civilianDrivingVehicleCountPerTown\\":',
		'\\"_comment_debugLoggingEnabled\\":',
		'\\"_comment_trackResistanceSupportGroupsOnMap\\":'
	)) {
	if ($scriptText -notmatch $requiredSettingsComment) {
		throw "Runtime settings generated JSON must include explanatory comment field: $requiredSettingsComment"
	}
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
if ($configResourceText -notmatch "m_bLootSkipUnlockedItems 1" -or $configResourceText -notmatch "m_bVehicleLootSkipUnlockedItems 1") {
	throw "Loot defaults must skip items already unlimited in the arsenal"
}
if ($scriptText -notmatch "settings.m_ArsenalLoot.m_bLootSkipUnlockedItems = true" -or $scriptText -notmatch "settings.m_VehicleLoot.m_bSkipUnlockedItems = true") {
	throw "Runtime settings migration must switch old profiles to skip-unlocked loot defaults"
}
if ($configResourceText -notmatch "m_bAllowExplosiveUnlocks 1" -or $configResourceText -notmatch "m_bAllowGuidedLauncherUnlocks 1") {
	throw "Loot defaults must allow explosive and guided launcher unlocks"
}
if ($scriptText -notmatch "settings.m_ArsenalLoot.m_bAllowExplosiveUnlocks = true" -or $scriptText -notmatch "settings.m_ArsenalLoot.m_bAllowGuidedLauncherUnlocks = true") {
	throw "Runtime settings migration must default explosive and guided launcher unlocks to enabled"
}
if ($scriptText -match '"arsenalUnlockThreshold": 15' -or $configResourceText -match "m_iArsenalUnlockThreshold 15") {
	throw "Generated/default balance settings must not keep the old 15-item infinite unlock threshold"
}
foreach ($requiredCivilianDefault in @(
		"m_iCivilianMaxActivePerTown = 12",
		"m_iCivilianVehicleMinPerTown = 1",
		"m_iCivilianVehicleMaxPerTown = 5",
		"m_iCivilianDrivingVehicleCountPerTown = 5",
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
		"m_iCivilianDrivingVehicleCountPerTown 5",
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
		"m_bVehicleLootSkipUnlockedItems",
		"m_bLootSkipUnlockedItems",
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
		"m_bLootSkipUnlockedItems",
		"m_bRemoveLootedItems",
		"m_bAllowExplosiveUnlocks",
		"m_bAllowGuidedLauncherUnlocks",
		"m_iMagazineUnlockMultiplier",
		"m_bVehicleLootEnabled",
		"m_iVehicleLootRadiusMeters",
		"m_bVehicleLootSkipUnlockedItems",
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
		"HQ_ZONE_ACTIVATION_FALLBACK_RADIUS_METERS",
		"IsZoneInsideHQSafeArea",
		"IsZoneInsideHQActivationExclusion",
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
$supportRequestServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_SupportRequestService.c"
$paidSupportAuthorityProofText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PaidSupportAuthorityProofService.c"
$campaignCommandServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CampaignCommandService.c"
$convoyOutcomeServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ConvoyOutcomeService.c"
foreach ($requiredHQZoneActivationEntry in @(
		"!IsZoneInsideHQActivationExclusion(state, zone)",
		"Math.Max(HQ_ZONE_ACTIVATION_FALLBACK_RADIUS_METERS, zone.m_iCaptureRadiusMeters)",
		"HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS"
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredHQZoneActivationEntry)) {
		throw "HQ policy must separate location-footprint activation from operation staging clearance: $requiredHQZoneActivationEntry"
	}
}
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
		"m_bDetached = !preservePersistentFieldRecord",
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

$activeGroupLifecycleProofText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ActiveGroupLifecycleProofService.c"
$activeGroupLifecycleSaveText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$activeGroupLifecycleCorpus = $physicalWarServiceText + "`n" + $mapMarkerServiceText + "`n" + $activeGroupLifecycleSaveText + "`n" + $activeGroupLifecycleProofText + "`n" + $coordinatorForPreflightText
foreach ($requiredActiveGroupLifecycleEntry in @(
	"TryEliminateCrewlessMixedActiveGroup",
	"IsMixedPersonnelVehicleActiveGroup",
	"CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId)",
	"HasObservedActiveGroupPersonnel",
	"ShouldApplyCrewlessMixedPersonnelElimination",
	"IsActiveGroupNativeDelayedPopulationActive(activeGroup)",
	"IsActiveGroupLiveCountGraceActive(state, activeGroup)",
	"ApplyObservedPersonnelElimination",
	"CleanupTerminalActiveGroupRuntime",
	"personnel_eliminated_vehicle_salvage",
	"RegisterDetachedActiveVehicle(state, zone, activeGroup, vehicle, source)",
	"HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(vehicle)",
	"DeleteRuntimeGroupEntity(activeGroup.m_sGroupId, false)",
	"DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId)",
	'activeGroup.m_sRuntimeStatus == "eliminated"',
	'activeGroup.m_sSpawnFallbackMode.Contains("personnel_eliminated_vehicle_salvage")',
	"string prefab = activeGroup.m_sVehiclePrefab",
	"CountAliveRuntimeGroupVehicles",
	"IsSessionOnlyDetachedActiveVehicle",
	"PruneSessionOnlyDetachedActiveVehicles",
	"sessionOnlyDetachedVehicleIds.Find(cargoItem.m_sVehicleRuntimeId)",
	"CountCampaignDebugIsolationRuntimeVehicles",
	"ShouldPreservePersistentDetachedVehicleRecord",
	'runtimeVehicle.m_sRuntimeKind == "field_vehicle"',
	"FailTerminalLinkedQRF",
	"HST_ActiveGroupLifecycleProofService",
	"active_group_lifecycle.qrf_uncrewed_noncombat",
	"active_group_lifecycle.capture_presence",
	"active_group_lifecycle.qrf_marker_retired",
	"active_group_lifecycle.persistence",
	"active_group_lifecycle.controls",
	"terminalGroup",
	'group.m_sRuntimeStatus == "eliminated"',
	"group.m_iSurvivorVehicleCount = 0",
	"group.m_bSpawnedEntity = false"
)) {
	if ($activeGroupLifecycleCorpus -notmatch [regex]::Escape($requiredActiveGroupLifecycleEntry)) {
		throw "Mixed active-group personnel lifecycle authority is missing entry: $requiredActiveGroupLifecycleEntry"
	}
}

$terminalSaveNormalization = [regex]::Match($activeGroupLifecycleSaveText, '(?s)bool terminalGroup = .*?;')
if (!$terminalSaveNormalization.Success) {
	throw "Could not locate terminal active-group save normalization"
}
if ($terminalSaveNormalization.Value -match [regex]::Escape('group.m_sRuntimeStatus == "folded"')) {
	throw "Folded active groups must retain survivor provenance already credited to the abstract garrison"
}

$qrfMarkerPredicate = [regex]::Match($mapMarkerServiceText, '(?s)protected bool ShouldShowQRFMarker\(.*?\r?\n\t\}')
if (!$qrfMarkerPredicate.Success) {
	throw "Could not locate QRF marker visibility predicate for terminal-group ordering audit"
}
$qrfTerminalMarkerIndex = $qrfMarkerPredicate.Value.IndexOf('group.m_sRuntimeStatus == "eliminated"')
$qrfUnresolvedMarkerIndex = $qrfMarkerPredicate.Value.IndexOf('if (!qrf.m_bResolved)')
if ($qrfTerminalMarkerIndex -lt 0 -or $qrfUnresolvedMarkerIndex -lt 0 -or $qrfTerminalMarkerIndex -gt $qrfUnresolvedMarkerIndex) {
	throw "QRF marker visibility must reject a linked terminal group before unresolved-QRF visibility"
}

$activeGroupCombatPredicate = [regex]::Match($physicalWarServiceText, '(?s)protected bool IsActiveGroupCombatEffective\(.*?\r?\n\t\}')
if (!$activeGroupCombatPredicate.Success) {
	throw "Could not locate active-group combat-effectiveness predicate"
}
foreach ($requiredCombatControlEntry in @(
	"if (activeGroup.m_iInfantryCount > 0)",
	"activeGroup.m_iSurvivorInfantryCount > 0",
	"activeGroup.m_iSurvivorVehicleCount > 0"
)) {
	if ($activeGroupCombatPredicate.Value -notmatch [regex]::Escape($requiredCombatControlEntry)) {
		throw "Active-group combat effectiveness is missing mixed/vehicle-only control entry: $requiredCombatControlEntry"
	}
}
if ($activeGroupCombatPredicate.Value -match [regex]::Escape("activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0")) {
	throw "A mixed active group must not remain combat-effective from an intact crewless vehicle alone"
}

$mixedLifecycleUpdateIndex = $physicalWarServiceText.IndexOf('TryEliminateCrewlessMixedActiveGroup(state, activeGroup, "survivor update")')
$mixedLifecycleGenericCountIndex = $physicalWarServiceText.IndexOf('else if (IsMixedPersonnelVehicleActiveGroup(activeGroup))', $mixedLifecycleUpdateIndex)
if ($mixedLifecycleUpdateIndex -lt 0 -or $mixedLifecycleGenericCountIndex -lt 0 -or $mixedLifecycleUpdateIndex -gt $mixedLifecycleGenericCountIndex) {
	throw "Mixed active-group personnel terminal decision must run before aggregate survivor handling"
}
Write-Host "Crewless mixed active-group terminal, salvage, marker, capture, and persistence contract OK"

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
		"RplComponent crewReplication",
		"crewReplication.IsOwner()",
		"MoveInVehicle(vehicleEntity, compartmentType, true, slot)",
		"owner compartment move-in request accepted",
		"Convoy adapter cannot bind crew before seating",
		'" | pre-seat "',
		"waiting for authoritative seat transition to confirm a driver",
		"HasLivingDriver",
		"AreLivingCrewMounted",
		"CountLivingCrew",
		"BuildCrewSeatingReport"
	)) {
	if ($convoyVehicleControlAdapterText -notmatch [regex]::Escape($requiredPhase6SeatingEntry)) {
		throw "Missing Phase 6 convoy seating contract entry: $requiredPhase6SeatingEntry"
	}
}
if ($convoyVehicleControlAdapterText -match [regex]::Escape("waiting for animated AI boarding to seat a driver")) {
	throw "Convoy pending evidence must describe the authoritative seat transition rather than obsolete animated boarding"
}
$tryBindCrewStart = $convoyVehicleControlAdapterText.IndexOf("bool TryBindCrewToVehicle")
$tryAssignRouteStart = $convoyVehicleControlAdapterText.IndexOf("bool TryAssignVehicleRoute")
if ($tryBindCrewStart -lt 0 -or $tryAssignRouteStart -le $tryBindCrewStart) {
	throw "Phase 6 convoy seating validator could not isolate TryBindCrewToVehicle"
}
$tryBindCrewText = $convoyVehicleControlAdapterText.Substring($tryBindCrewStart, $tryAssignRouteStart - $tryBindCrewStart)
$preSeatRegistrationIndex = $tryBindCrewText.IndexOf("TryRegisterVehicleWithGroup")
$seatBuildIndex = $tryBindCrewText.IndexOf("BuildCrewSeatingResult")
if ($preSeatRegistrationIndex -lt 0 -or $seatBuildIndex -lt 0 -or $preSeatRegistrationIndex -gt $seatBuildIndex) {
	throw "Convoy binding must register the usable vehicle before issuing any crew seating request"
}
$tryMoveCrewStart = $convoyVehicleControlAdapterText.IndexOf("protected bool TryMoveCrewIntoSlot")
$isSlotTypeStart = $convoyVehicleControlAdapterText.IndexOf("protected bool IsSlotForCompartmentType")
if ($tryMoveCrewStart -lt 0 -or $isSlotTypeStart -le $tryMoveCrewStart) {
	throw "Phase 6 convoy seating validator could not isolate TryMoveCrewIntoSlot"
}
$tryMoveCrewText = $convoyVehicleControlAdapterText.Substring($tryMoveCrewStart, $isSlotTypeStart - $tryMoveCrewStart)
$authorityLocalSeatIndex = $tryMoveCrewText.IndexOf("access.GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true)")
$ownerRpcSeatIndex = $tryMoveCrewText.IndexOf("access.MoveInVehicle(vehicleEntity, compartmentType, true, slot)")
if ($authorityLocalSeatIndex -lt 0 -or $ownerRpcSeatIndex -lt 0 -or $authorityLocalSeatIndex -gt $ownerRpcSeatIndex) {
	throw "Server-owned convoy AI must attempt authority-local forced seating before the owner-RPC fallback"
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
		"IsVehicleRegisteredWithGroup",
		"AddUsableVehicle",
		"utility.IsUsableVehicle(vehicleUsage)",
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
$vehicleRegistrationProofStart = $convoyVehicleControlAdapterText.IndexOf("bool IsVehicleRegisteredWithGroup")
$countLivingCrewStart = $convoyVehicleControlAdapterText.IndexOf("int CountLivingCrew")
if ($vehicleRegistrationProofStart -lt 0 -or $countLivingCrewStart -le $vehicleRegistrationProofStart) {
	throw "Phase 7 convoy validator could not isolate IsVehicleRegisteredWithGroup"
}
$vehicleRegistrationProofText = $convoyVehicleControlAdapterText.Substring($vehicleRegistrationProofStart, $countLivingCrewStart - $vehicleRegistrationProofStart)
foreach ($requiredVehicleRegistrationProofEntry in @(
		"vehicleUsage.IsVehicleTypeValid()",
		"vehicleUsage.CanBePiloted()",
		"utility.IsUsableVehicle(vehicleUsage)"
	)) {
	if ($vehicleRegistrationProofText -notmatch [regex]::Escape($requiredVehicleRegistrationProofEntry)) {
		throw "Live convoy vehicle-registration proof must recheck pilotability and retained utility state: $requiredVehicleRegistrationProofEntry"
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
		"BuildRemainingMissionConvoyRouteWaypoints",
		"AppendConvoyLeadInWaypoints",
		"AppendConvoyLeadInWaypoints(result, currentPosition, remainingRouteWaypoints[0]",
		"CONVOY_RUNTIME_WAYPOINT_MIN_COUNT = 3",
		"CONVOY_RUNTIME_WAYPOINT_MAX_COUNT = 5",
		"AppendSparseConvoyRouteWaypoints",
		"AppendResolvedConvoyWaypoint",
		"desiredWaypointCount = Math.Max(CONVOY_RUNTIME_WAYPOINT_MIN_COUNT, Math.Min(CONVOY_RUNTIME_WAYPOINT_MAX_COUNT, remainingRouteWaypoints.Count() + 2))",
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
		"IsVehicleRegisteredWithGroup(crewEntity, vehicleEntity, vehicleUsageEvidence)",
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
		"CountDeadTrackedRuntimeGroupMembers",
		"deadTrackedMembers <= 0",
		"DetachDeadRuntimeMembersFromGroupRoot",
		"ReconcileTrackedRuntimeMembersWithAIGroup",
		"DiscoverRuntimeGroupMemberHandles",
		"RegisterRuntimeGroupEntityHandle",
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
		"CampaignDebugResolveActiveGroupEditableSize",
		"active group member handles discovered",
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

$supportRouteUpdateMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected bool UpdateActiveGroupRoutes\(.*?(?=\r?\n\tprotected bool UpdatePhysicalSupportActiveGroupRoute\()')
$supportPhysicalRouteMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected bool UpdatePhysicalSupportActiveGroupRoute\(.*?(?=\r?\n\tprotected HST_ActiveGroupRouteProgressStatus EnsureActiveGroupRouteProgressStatus\()')
$supportRouteResetMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected void ResetActiveGroupRouteProgressForCurrentLeg\(.*?(?=\r?\n\tprotected void CleanupActiveGroupRouteProgressStatuses\()')
$supportWaypointAssignMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected int AssignActiveGroupInfantryRouteWaypoints\(.*?(?=\r?\n\tprotected IEntity SpawnActiveGroupRouteWaypoint\()')
$supportWaypointReadyMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected bool IsActiveGroupInfantryWaypointAssigned\(.*?(?=\r?\n\tprotected int CountRuntimeGroupWaypointEntities\()')
$supportWaypointCountMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected int CountRuntimeGroupWaypointEntities\(.*?(?=\r?\n\tprotected bool UpdateTownPolicePatrols\()')
$supportBuildRouteMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected ref array<vector> BuildActiveGroupRoutePositions\(.*?(?=\r?\n\tprotected ref array<vector> BuildDirectSupportRoutePositions\()')
$supportDirectRouteMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected ref array<vector> BuildDirectSupportRoutePositions\(.*?(?=\r?\n\tprotected void AppendActiveGroupRoutePosition\()')
$supportSpawnStatusMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected string ResolveSpawnedRuntimeStatus\(.*?(?=\r?\n\tprotected string SelectValidGroupPrefabFromList\()')
$supportDeleteWaypointsMatch = [regex]::Match($physicalWarServiceText, '(?s)\tprotected void DeleteRuntimeGroupWaypoints\(.*?(?=\r?\n\tprotected bool IsZoneInsideHQSafeArea\()')
$supportLiveDistanceMatch = [regex]::Match($physicalWarServiceText, '(?s)\tbool IsActiveSupportGroupPhysicallyWithinDistance\(.*?(?=\r?\n\tint CountRuntimeGroupHandlesForMission\()')
$supportRecallRouteMatch = [regex]::Match($physicalWarServiceText, '(?s)\tbool RecallActiveSupportGroup\(.*?(?=\r?\n\tbool TryRefreshActiveSupportGroupLivePosition\()')
$supportGroundTickMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected bool TickPhysicalGroundSupport\(.*?(?=\r?\n\tprotected bool TickAcceptedExactPlayerSupport\()')
$supportExactTickMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected bool TickSucceededExactPlayerSupport\(.*?(?=\r?\n\tprotected bool TickRecalledExactPlayerSupport\()')
$supportExactRecallTickMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected bool TickRecalledExactPlayerSupport\(.*?(?=\r?\n\tprotected bool TickRecalledPhysicalGroundSupport\()')
$supportArrivalConfirmMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected bool ConfirmPhysicalSupportArrival\(.*?(?=\r?\n\tprotected bool ConfirmPhysicalSupportRecallExit\()')
$supportRecallConfirmMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected bool ConfirmPhysicalSupportRecallExit\(.*?(?=\r?\n\tprotected bool ResolveSupport\()')
$supportNormalRecallMatch = [regex]::Match($supportRequestServiceText, '(?s)\tHST_SupportRecallResult RecallSupportRequestDetailed\(.*?(?=\r?\n\tstring RecallSupportRequestReport\()')
$supportExactRecallMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected HST_SupportRecallResult BeginExactPlayerSupportRecall\(.*?(?=\r?\n\tprotected HST_SupportRecallResult BuildSupportRecallResult\()')
foreach ($supportRouteBoundary in @(
		[pscustomobject]@{ Name = "active-group route update"; Match = $supportRouteUpdateMatch },
		[pscustomobject]@{ Name = "physical support route update"; Match = $supportPhysicalRouteMatch },
		[pscustomobject]@{ Name = "physical support route reset"; Match = $supportRouteResetMatch },
		[pscustomobject]@{ Name = "physical support waypoint assignment"; Match = $supportWaypointAssignMatch },
		[pscustomobject]@{ Name = "physical support waypoint readiness"; Match = $supportWaypointReadyMatch },
		[pscustomobject]@{ Name = "physical support waypoint handle count"; Match = $supportWaypointCountMatch },
		[pscustomobject]@{ Name = "active-group route builder"; Match = $supportBuildRouteMatch },
		[pscustomobject]@{ Name = "direct support route builder"; Match = $supportDirectRouteMatch },
		[pscustomobject]@{ Name = "spawned support status resolver"; Match = $supportSpawnStatusMatch },
		[pscustomobject]@{ Name = "runtime waypoint deletion"; Match = $supportDeleteWaypointsMatch },
		[pscustomobject]@{ Name = "live support distance query"; Match = $supportLiveDistanceMatch },
		[pscustomobject]@{ Name = "support recall route"; Match = $supportRecallRouteMatch },
		[pscustomobject]@{ Name = "physical support tick"; Match = $supportGroundTickMatch },
		[pscustomobject]@{ Name = "exact support success tick"; Match = $supportExactTickMatch },
		[pscustomobject]@{ Name = "exact support recall tick"; Match = $supportExactRecallTickMatch },
		[pscustomobject]@{ Name = "physical support arrival confirmation"; Match = $supportArrivalConfirmMatch },
		[pscustomobject]@{ Name = "physical support recall confirmation"; Match = $supportRecallConfirmMatch },
		[pscustomobject]@{ Name = "ordinary support recall command"; Match = $supportNormalRecallMatch },
		[pscustomobject]@{ Name = "exact support recall command"; Match = $supportExactRecallMatch }
	)) {
	if (!$supportRouteBoundary.Match.Success) {
		throw "Could not isolate $($supportRouteBoundary.Name) for the physical support route contract"
	}
}

foreach ($requiredTypedRecallResultEntry in @(
	"class HST_SupportRecallResult",
	"bool m_bAccepted;",
	"bool m_bAlreadyApplied;",
	"bool m_bStateChanged;",
	"bool m_bTerminal;",
	"string m_sDisposition;",
	"string m_sFailureReason;",
	"string m_sRequestId;",
	"string m_sOperationId;",
	"string m_sDisplayMessage;",
	"ref HST_SupportRequestState m_Request;",
	"HST_ECampaignCommandStatus ResolveCommandStatus()",
	"string BuildSummary()"
)) {
	if ($supportRequestServiceText -notmatch [regex]::Escape($requiredTypedRecallResultEntry)) {
		throw "Typed support-recall result contract is missing: $requiredTypedRecallResultEntry"
	}
}

$supportRecallReportAdapterMatch = [regex]::Match($supportRequestServiceText, '(?s)\tstring RecallSupportRequestReport\(.*?(?=\r?\n\tbool RecallSupportRequest\()')
$supportRecallBoolAdapterMatch = [regex]::Match($supportRequestServiceText, '(?s)\tbool RecallSupportRequest\(.*?(?=\r?\n\tprotected HST_SupportRecallResult BeginExactPlayerSupportRecall\()')
$coordinatorRecallDetailedMatch = [regex]::Match($coordinatorText, '(?s)\tHST_SupportRecallResult RequestCommanderRecallSupportDetailed\(.*?(?=\r?\n\tstring BuildCommanderRecallSupportReport\()')
$coordinatorRecallReportMatch = [regex]::Match($coordinatorText, '(?s)\tstring BuildCommanderRecallSupportReport\(.*?(?=\r?\n\tprotected HST_SupportRecallResult BuildRejectedSupportRecallResult\()')
$coordinatorRecallBoolMatch = [regex]::Match($coordinatorText, '(?s)\tbool RequestCommanderRecallSupport\(.*?(?=\r?\n\tbool RequestCommanderAidNearestTown\()')
$exactFullRefundMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected bool SettleExactPlayerSupportFullRefund\(.*?(?=\r?\n\tprotected bool SettleExactPlayerSupportHRRefund\()')
$visibleCommandExplicitPrefixMatch = [regex]::Match($commandUIServiceText, '(?s)\tstring ExecuteVisibleCommandDetailed\(.*?(?=\r?\n\t\tif \(!coordinator \|\| commandId\.IsEmpty\(\)\))')
$visibleRecallDispatchMatch = [regex]::Match($commandUIServiceText, '(?s)\t\tif \(commandId == "support_recall"\)\s*\{.*?\r?\n\t\t\}')
$commandCompleteExplicitMatch = [regex]::Match($campaignCommandServiceText, '(?s)\tHST_CampaignCommandResult CompleteExplicit\(.*?(?=\r?\n\tstring BuildReport\()')
$visibleCommandCoordinatorMatch = [regex]::Match($coordinatorText, '(?s)\tstring RequestVisibleMenuCommand\(.*?(?=\r?\n\tprotected string ResolveCommandAggregateId\()')
$exactSettlementEligibilityMatch = [regex]::Match($supportRequestServiceText, '(?s)\tprotected bool CanSettleExactSupportTransaction\(.*?(?=\r?\n\tprotected bool RetireExactSupportProjectionRuntime\()')
foreach ($typedRecallBoundary in @(
	[pscustomobject]@{ Name = "support recall report adapter"; Match = $supportRecallReportAdapterMatch },
	[pscustomobject]@{ Name = "support recall bool adapter"; Match = $supportRecallBoolAdapterMatch },
	[pscustomobject]@{ Name = "coordinator recall typed entry"; Match = $coordinatorRecallDetailedMatch },
	[pscustomobject]@{ Name = "coordinator recall report adapter"; Match = $coordinatorRecallReportMatch },
	[pscustomobject]@{ Name = "coordinator recall bool adapter"; Match = $coordinatorRecallBoolMatch },
	[pscustomobject]@{ Name = "exact full-refund transaction"; Match = $exactFullRefundMatch },
	[pscustomobject]@{ Name = "visible-command explicit-status prefix"; Match = $visibleCommandExplicitPrefixMatch },
	[pscustomobject]@{ Name = "visible recall explicit dispatch"; Match = $visibleRecallDispatchMatch },
	[pscustomobject]@{ Name = "explicit command completion"; Match = $commandCompleteExplicitMatch },
	[pscustomobject]@{ Name = "visible command coordinator completion"; Match = $visibleCommandCoordinatorMatch },
	[pscustomobject]@{ Name = "exact transaction settlement eligibility"; Match = $exactSettlementEligibilityMatch }
)) {
	if (!$typedRecallBoundary.Match.Success) {
		throw "Could not isolate $($typedRecallBoundary.Name) for the typed support-recall contract"
	}
}

foreach ($typedRecallAuthorityBlock in @(
	[pscustomobject]@{ Name = "support typed entry"; Text = $supportNormalRecallMatch.Value },
	[pscustomobject]@{ Name = "support exact typed helper"; Text = $supportExactRecallMatch.Value },
	[pscustomobject]@{ Name = "support report adapter"; Text = $supportRecallReportAdapterMatch.Value },
	[pscustomobject]@{ Name = "support bool adapter"; Text = $supportRecallBoolAdapterMatch.Value },
	[pscustomobject]@{ Name = "coordinator typed entry"; Text = $coordinatorRecallDetailedMatch.Value },
	[pscustomobject]@{ Name = "coordinator report adapter"; Text = $coordinatorRecallReportMatch.Value },
	[pscustomobject]@{ Name = "coordinator bool adapter"; Text = $coordinatorRecallBoolMatch.Value },
	[pscustomobject]@{ Name = "visible recall dispatch"; Text = $visibleRecallDispatchMatch.Value }
)) {
	if ($typedRecallAuthorityBlock.Text -match [regex]::Escape('Contains("failed")')) {
		throw "$($typedRecallAuthorityBlock.Name) must not infer recall authority from presentation wording"
	}
}

foreach ($requiredTypedRecallAdapterEntry in @(
	"RecallSupportRequestDetailed(state, preset, economy, physicalWar, requestId, playerRequestedOnly)",
	"return result.BuildSummary();",
	"return result && result.m_bAccepted;"
)) {
	if ($supportRecallReportAdapterMatch.Value -notmatch [regex]::Escape($requiredTypedRecallAdapterEntry) -and $supportRecallBoolAdapterMatch.Value -notmatch [regex]::Escape($requiredTypedRecallAdapterEntry)) {
		throw "Support recall compatibility adapters must delegate to the typed outcome: $requiredTypedRecallAdapterEntry"
	}
}
foreach ($requiredCoordinatorRecallEntry in @(
	"m_SupportRequests.RecallSupportRequestDetailed",
	"result.m_bStateChanged",
	"MarkMajorCampaignChange(true);",
	"BuildCommanderRecallSupportReport(RequestCommanderRecallSupportDetailed",
	"return result && result.m_bAccepted;"
)) {
	if ($coordinatorText -notmatch [regex]::Escape($requiredCoordinatorRecallEntry)) {
		throw "Coordinator support recall must checkpoint and classify the typed result explicitly: $requiredCoordinatorRecallEntry"
	}
}
foreach ($requiredVisibleRecallStatusEntry in @(
	'hasExplicitCommandStatus = commandId == "support_recall";',
	"explicitCommandStatus = HST_ECampaignCommandStatus.HST_COMMAND_REJECTED;",
	"explicitCommandStatus = recallResult.ResolveCommandStatus();",
	"explicitAggregateId = recallResult.m_sOperationId;"
)) {
	if ($visibleCommandExplicitPrefixMatch.Value -notmatch [regex]::Escape($requiredVisibleRecallStatusEntry) -and $visibleRecallDispatchMatch.Value -notmatch [regex]::Escape($requiredVisibleRecallStatusEntry)) {
		throw "Visible support recall must publish explicit command status and operation identity: $requiredVisibleRecallStatusEntry"
	}
}
foreach ($requiredCommandCompletionEntry in @(
	"receipt.m_eStatus = completionStatus;",
	"completionStatus != HST_ECampaignCommandStatus.HST_COMMAND_APPLIED"
)) {
	if ($commandCompleteExplicitMatch.Value -notmatch [regex]::Escape($requiredCommandCompletionEntry)) {
		throw "Typed recall command completion contract is missing: $requiredCommandCompletionEntry"
	}
}
foreach ($requiredVisibleCommandCompletionEntry in @(
	"CompleteExplicit(m_State, envelope, explicitCommandStatus, result, aggregateId)",
	"completedResult.m_Receipt && m_Persistence && !m_bCampaignDebugStateIsolationActive",
	"m_Persistence.MarkMajorChange();"
)) {
	if ($visibleCommandCoordinatorMatch.Value -notmatch [regex]::Escape($requiredVisibleCommandCompletionEntry)) {
		throw "Visible command receipts must use explicit status when supplied and schedule durable persistence: $requiredVisibleCommandCompletionEntry"
	}
}
if ($coordinatorText -notmatch [regex]::Escape("authority.command.explicit_status")) {
	throw "Explicit command-status proof assertion is missing"
}

if ($supportExactRecallMatch.Value -match '(?m)^\s*SettleExactPlayerSupport(?:DeploymentTerminal|HRRefund)\(') {
	throw "Exact support recall must inspect every settlement result instead of ignoring a mutation outcome"
}
foreach ($requiredLostGroupSettlementEntry in @(
	"bool lostSettled = SettleExactPlayerSupportHRRefund",
	"if (!lostSettled)",
	'"lost_group_settlement_failed"'
)) {
	if ($supportExactRecallMatch.Value -notmatch [regex]::Escape($requiredLostGroupSettlementEntry)) {
		throw "Exact lost-group recall must fail closed on settlement result: $requiredLostGroupSettlementEntry"
	}
}
$fullRefundMoneyValidationIndex = $exactFullRefundMatch.Value.IndexOf("ValidateExactSupportTransactionIdentity(moneyTransaction")
$fullRefundHRValidationIndex = $exactFullRefundMatch.Value.IndexOf("ValidateExactSupportTransactionIdentity(hrTransaction")
$fullRefundMoneyEligibilityIndex = $exactFullRefundMatch.Value.IndexOf("CanSettleExactSupportTransaction(moneyTransaction")
$fullRefundHREligibilityIndex = $exactFullRefundMatch.Value.IndexOf("CanSettleExactSupportTransaction(hrTransaction")
$fullRefundMutationIndex = $exactFullRefundMatch.Value.IndexOf("SettleExactSupportTransaction(state, request")
if ($fullRefundMoneyValidationIndex -lt 0 -or $fullRefundHRValidationIndex -lt 0 -or $fullRefundMoneyEligibilityIndex -lt 0 -or $fullRefundHREligibilityIndex -lt 0 -or $fullRefundMutationIndex -lt 0 -or
	$fullRefundMoneyValidationIndex -gt $fullRefundMutationIndex -or $fullRefundHRValidationIndex -gt $fullRefundMutationIndex -or $fullRefundMoneyEligibilityIndex -gt $fullRefundMutationIndex -or $fullRefundHREligibilityIndex -gt $fullRefundMutationIndex) {
	throw "Exact full refund must prevalidate money and HR identity/settlement eligibility before mutating either transaction"
}
foreach ($requiredSettlementEligibilityEntry in @(
	"transaction.m_iRefundedAmount < 0",
	"transaction.m_iRefundedAmount > transaction.m_iAmount",
	"transaction.m_sLastSettlementId == settlementId",
	"HST_TRANSACTION_PARTIALLY_REFUNDED",
	"HST_TRANSACTION_CANCELLED"
)) {
	if ($exactSettlementEligibilityMatch.Value -notmatch [regex]::Escape($requiredSettlementEligibilityEntry)) {
		throw "Exact transaction settlement preflight must reject incoherent or deterministic-replay state: $requiredSettlementEligibilityEntry"
	}
}
foreach ($requiredTypedRecallProofEntry in @(
	"m_bRecallTypedTextExact",
	"m_bRecallSettlementFailureExact",
	"m_bRecallLostGroupExact",
	'recallResult.m_bAccepted && recallResult.m_bStateChanged && recallResult.m_bTerminal && recallResult.m_sDisplayMessage.Contains("failed")',
	"commands.CompleteExplicit",
	"HST_COMMAND_ALREADY_APPLIED",
	'hr.m_sOperationId = "corrupt_recall_settlement_operation";',
	"eligibilityHR.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED;",
	"money.m_iRefundedAmount == 0 && hr.m_iRefundedAmount == 0",
	"PrepareLostGroupRecallFixture",
	'validRecall.m_sDisposition == "recalled_group_lost"',
	'invalidRecall.m_sDisposition == "lost_group_settlement_failed"',
	"force_authority.paid_qrf_recall_typed_text",
	"force_authority.paid_qrf_recall_settlement_failure",
	"force_authority.paid_qrf_recall_lost_group"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredTypedRecallProofEntry)) {
		throw "Typed support-recall deterministic proof is missing: $requiredTypedRecallProofEntry"
	}
}
Write-Host "Typed fail-closed support recall and explicit command-result contract OK"

$supportRouteUpdateText = $supportRouteUpdateMatch.Value
foreach ($requiredPhysicalSupportBranchEntry in @(
		"bool physicalConfirmedRoute = activeGroup.m_bSpawnedEntity",
		"&& activeGroup.m_iInfantryCount > 0",
		"&& !IsMissionConvoyGroup(activeGroup)",
		"&& IsLiveConfirmedOperationRouteGroup(state, activeGroup);"
	)) {
	if ($supportRouteUpdateText -notmatch [regex]::Escape($requiredPhysicalSupportBranchEntry)) {
		throw "Spawned physical support must branch away from elapsed-time route simulation: $requiredPhysicalSupportBranchEntry"
	}
}
if ($supportRouteUpdateText -notmatch '(?s)if \(physicalConfirmedRoute\)\s*\{\s*changed = UpdatePhysicalSupportActiveGroupRoute\(state, activeGroup, routePositions\) \|\| changed;\s*continue;\s*\}') {
	throw "Spawned physical support must continue after its live route update instead of entering interpolation"
}
$physicalSupportBranchIndex = $supportRouteUpdateText.IndexOf("bool physicalConfirmedRoute = activeGroup.m_bSpawnedEntity")
foreach ($simulatedRouteWrite in @(
		"float progress = Math.Min",
		"vector position = ResolveActiveGroupRoutePosition",
		"activeGroup.m_vPosition = position;",
		"SetRuntimeGroupEntitiesOrigin(activeGroup.m_sGroupId, position)"
	)) {
	$simulatedRouteWriteIndex = $supportRouteUpdateText.IndexOf($simulatedRouteWrite)
	if ($simulatedRouteWriteIndex -ge 0 -and $simulatedRouteWriteIndex -lt $physicalSupportBranchIndex) {
		throw "Spawned physical support must branch before simulated route/origin write: $simulatedRouteWrite"
	}
}

if ($supportRequestServiceText -match "MarkPhysicalSupportArrived") {
	throw "Support ETA must not mutate active-group arrival through the removed MarkPhysicalSupportArrived shortcut"
}
$exactSupportArrivalText = $supportExactTickMatch.Value
$exactSupportArrivalGateIndex = $exactSupportArrivalText.IndexOf("int arrivalAtSecond")
if ($exactSupportArrivalGateIndex -ge 0) {
	$exactSupportArrivalText = $exactSupportArrivalText.Substring($exactSupportArrivalGateIndex)
}
foreach ($supportArrivalTick in @(
		[pscustomobject]@{ Name = "physical support"; Text = $supportGroundTickMatch.Value },
		[pscustomobject]@{ Name = "exact support"; Text = $exactSupportArrivalText }
)) {
	if ($supportArrivalTick.Text -match '(?m)\b(?:group|activeGroup)\.m_sRuntimeStatus\s*=\s*"support_arrived";') {
		throw "$($supportArrivalTick.Name) ETA must not set group arrival directly"
	}
	$supportArrivalConfirmIndex = $supportArrivalTick.Text.IndexOf("ConfirmPhysicalSupportArrival(")
	$supportRequestArrivedIndex = $supportArrivalTick.Text.IndexOf('request.m_sRuntimeStatus = "physical_arrived";')
	if ($supportArrivalConfirmIndex -lt 0 -or $supportRequestArrivedIndex -lt 0 -or $supportArrivalConfirmIndex -gt $supportRequestArrivedIndex) {
		throw "$($supportArrivalTick.Name) must confirm live physical arrival before settling the request"
	}
}

$supportGroundTickText = $supportGroundTickMatch.Value
$restoredArrivalNormalizationIndex = $supportGroundTickText.IndexOf('linkedGroup.m_sRuntimeStatus == "support_arrived"')
$abstractArrivalGateIndex = $supportGroundTickText.IndexOf('if (!request.m_bAbstractResolved)')
if ($restoredArrivalNormalizationIndex -lt 0 -or $abstractArrivalGateIndex -lt 0 -or $restoredArrivalNormalizationIndex -gt $abstractArrivalGateIndex) {
	throw "Restored unspawned support arrival must normalize before the abstract-resolved gate"
}
foreach ($requiredRestoredArrivalEntry in @(
		"!linkedGroup.m_bSpawnedEntity",
		"HST_WorldPositionService.IsPositionInsidePlayerEventBubble(linkedGroup.m_vPosition)",
		'request.m_bAbstractResolved = false;',
		'simulated_arrived_waiting_physicalization',
		'arrivalProofRecorded',
		'Physical arrival confirmed:',
		'Physical support route completion confirmed'
	)) {
	if ($supportGroundTickText -notmatch [regex]::Escape($requiredRestoredArrivalEntry)) {
		throw "Restored support arrival compatibility/provenance is missing: $requiredRestoredArrivalEntry"
	}
}
foreach ($requiredExactSupportNormalizationEntry in @(
		'group.m_sRuntimeStatus == EXACT_PLAYER_SUPPORT_GROUP_STATUS',
		'operationPhysical.m_Operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION',
		'group.m_sRuntimeStatus = "support_active";',
		'group.m_iAssignedWaypointCount = 0;',
		'request.m_bAbstractResolved = false;'
	)) {
	if ($supportExactTickMatch.Value -notmatch [regex]::Escape($requiredExactSupportNormalizationEntry)) {
		throw "Successful exact support must normalize into live support routing: $requiredExactSupportNormalizationEntry"
	}
}
if ($supportExactTickMatch.Value -match [regex]::Escape('group.m_sRuntimeStatus.StartsWith("exact_")') -or
	$supportSpawnStatusMatch.Value -match [regex]::Escape('requestedStatus.StartsWith("exact_")')) {
	throw "Exact support routing normalization must not erase unresolved exact runtime-integrity statuses"
}
foreach ($requiredFoldedRecallEntry in @(
		'group.m_sRuntimeStatus == "folded" && !group.m_bSpawnedEntity',
		'group.m_sRuntimeStatus != "support_recall_exited" && group.m_sRuntimeStatus != "folded"'
	)) {
	if ($supportRecallConfirmMatch.Value -notmatch [regex]::Escape($requiredFoldedRecallEntry)) {
		throw "Spawned folded recall state must still require current live exit proof: $requiredFoldedRecallEntry"
	}
}
foreach ($requiredExactRestoreRecallEntry in @(
		'exact_restore_waiting_queue_capacity',
		'recalled_restore_waiting_reprojection',
		'CountDurableLivingMemberSlots(batch)'
	)) {
	if ($supportExactRecallMatch.Value -notmatch [regex]::Escape($requiredExactRestoreRecallEntry)) {
		throw "Exact support recall command must settle an unspawned restore-capacity row: $requiredExactRestoreRecallEntry"
	}
	if ($supportExactRecallTickMatch.Value -notmatch [regex]::Escape($requiredExactRestoreRecallEntry)) {
		throw "Exact support recall tick must recover a legacy unspawned restore-recall row: $requiredExactRestoreRecallEntry"
	}
}
foreach ($requiredExactRestoreRecallRuntimeGuard in @(
		'physicalWar.GetForceSpawnGroupRoot(group)',
		'forceSpawnAdapter.CountHandlesForProjection(group.m_sProjectionId)',
		'if (!runtimeProjectionPresent)'
	)) {
	if ($supportExactRecallTickMatch.Value -notmatch [regex]::Escape($requiredExactRestoreRecallRuntimeGuard)) {
		throw "Exact restore-recall settlement must refuse to bypass an existing runtime projection: $requiredExactRestoreRecallRuntimeGuard"
	}
}

$supportSpawnStatusText = $supportSpawnStatusMatch.Value
$supportIdentityIndex = $supportSpawnStatusText.IndexOf("!activeGroup.m_sSupportRequestId.IsEmpty()")
$exactRequestedStatusIndex = $supportSpawnStatusText.IndexOf('requestedStatus == "exact_support_spawn_queued"')
$supportActiveStatusIndex = $supportSpawnStatusText.IndexOf('return "support_active";')
$genericRequestedStatusIndex = $supportSpawnStatusText.IndexOf("return requestedStatus;")
if ($supportIdentityIndex -lt 0 -or $exactRequestedStatusIndex -lt $supportIdentityIndex -or $supportActiveStatusIndex -lt $exactRequestedStatusIndex -or $genericRequestedStatusIndex -lt $supportActiveStatusIndex) {
	throw "The benign exact support deployment status must normalize to support_active before generic requested-status retention"
}

foreach ($requiredSupportRouteConstant in @(
		"ACTIVE_GROUP_ROUTE_ARRIVAL_SAMPLE_COUNT = 2",
		"ACTIVE_GROUP_ROUTE_STALL_REISSUE_SECONDS = 45",
		"ACTIVE_GROUP_ROUTE_REISSUE_COOLDOWN_SECONDS = 30",
		"ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS = 3"
	)) {
	if ($physicalWarServiceText -notmatch [regex]::Escape($requiredSupportRouteConstant)) {
		throw "Physical support route timing/recovery policy is missing: $requiredSupportRouteConstant"
	}
}
$supportPhysicalRouteText = $supportPhysicalRouteMatch.Value
$lastPhysicalRouteEvidenceIndex = -1
foreach ($orderedPhysicalRouteEvidence in @(
		"vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);",
		"float distanceToTargetMeters = Math.Sqrt(DistanceSq2D(livePosition, activeGroup.m_vTargetPosition));",
		"bool newTimedSample = progress.m_iLastSampleSecond < state.m_iElapsedSeconds;",
		"if (newTimedSample)",
		"progress.m_fBestDistanceToTargetMeters = distanceToTargetMeters;",
		"progress.m_fBestDistanceToTargetMeters - distanceToTargetMeters >= ACTIVE_GROUP_ROUTE_PROGRESS_THRESHOLD_METERS",
		"progress.m_iLastProgressSecond = state.m_iElapsedSeconds;",
		"progress.m_iRouteReissueAttemptCount = 0;",
		"progress.m_fBestDistanceToTargetMeters = distanceToTargetMeters;",
		"progress.m_iLastSampleSecond = state.m_iElapsedSeconds;",
		"if (distanceToTargetMeters <= ACTIVE_GROUP_ROUTE_ARRIVAL_RADIUS_METERS)",
		"if (newTimedSample)",
		"progress.m_iArrivalSampleCount++;",
		"if (progress.m_iArrivalSampleCount < ACTIVE_GROUP_ROUTE_ARRIVAL_SAMPLE_COUNT)",
		"activeGroup.m_sRuntimeStatus = arrivedStatus;",
		"progress.m_iArrivalSampleCount = 0;"
	)) {
	$physicalRouteEvidenceIndex = $supportPhysicalRouteText.IndexOf($orderedPhysicalRouteEvidence, $lastPhysicalRouteEvidenceIndex + 1)
	if ($physicalRouteEvidenceIndex -lt 0) {
		throw "Physical support route live-distance/sample ordering is missing: $orderedPhysicalRouteEvidence"
	}
	$lastPhysicalRouteEvidenceIndex = $physicalRouteEvidenceIndex
}
if ($supportPhysicalRouteText -notmatch '(?s)if \(newTimedSample\)\s*progress\.m_iArrivalSampleCount\+\+;') {
	throw "Physical support arrival samples must be separated by distinct elapsed-second observations"
}
if ($supportPhysicalRouteText -notmatch '(?s)else if \(progress\.m_fBestDistanceToTargetMeters < 0\s*\|\| progress\.m_fBestDistanceToTargetMeters - distanceToTargetMeters >= ACTIVE_GROUP_ROUTE_PROGRESS_THRESHOLD_METERS\)\s*\{\s*progress\.m_iLastProgressSecond = state\.m_iElapsedSeconds;\s*progress\.m_iRouteReissueAttemptCount = 0;\s*progress\.m_fBestDistanceToTargetMeters = distanceToTargetMeters;\s*\}') {
	throw "Physical support progress age and retry budget must reset only after best-distance net closure"
}
foreach ($forbiddenSupportProgressShortcut in @(
		"bool routeProgressed",
		"movedMeters >= ACTIVE_GROUP_ROUTE_PROGRESS_THRESHOLD_METERS",
		"progress.m_fLastDistanceToTargetMeters - distanceToTargetMeters >= ACTIVE_GROUP_ROUTE_PROGRESS_THRESHOLD_METERS"
	)) {
	if ($supportPhysicalRouteText -match [regex]::Escape($forbiddenSupportProgressShortcut)) {
		throw "Physical support stall recovery must use best-distance net closure, not lateral/last-sample movement: $forbiddenSupportProgressShortcut"
	}
}
foreach ($requiredSupportRouteResetEntry in @(
		"progress.m_fBestDistanceToTargetMeters = -1.0;",
		"progress.m_iLastSampleSecond = -1;",
		"progress.m_iLastProgressSecond = elapsedSeconds;",
		"progress.m_iLastRouteReissueSecond = -1;",
		"progress.m_iRouteReissueAttemptCount = 0;",
		"progress.m_iArrivalSampleCount = 0;"
	)) {
	if ($supportRouteResetMatch.Value -notmatch [regex]::Escape($requiredSupportRouteResetEntry)) {
		throw "Physical support route progress must reset per leg: $requiredSupportRouteResetEntry"
	}
}

foreach ($requiredLiveSupportDistanceEntry in @(
		"activeGroup.m_sSupportRequestId.IsEmpty()",
		"activeGroup.m_bSpawnedEntity",
		"ResolveActiveGroupLiveRuntimePosition(activeGroup, false)",
		"Math.Sqrt(DistanceSq2D(livePosition, targetPosition))",
		"float acceptedRadius = Math.Max(1.0, radiusMeters);",
		"return distanceMeters <= acceptedRadius;"
	)) {
	if ($supportLiveDistanceMatch.Value -notmatch [regex]::Escape($requiredLiveSupportDistanceEntry)) {
		throw "Physical support settlement must query live runtime distance: $requiredLiveSupportDistanceEntry"
	}
}
foreach ($supportSettlementProof in @(
		[pscustomobject]@{ Name = "arrival"; Text = $supportArrivalConfirmMatch.Value; Target = "request.m_vTargetPosition"; StaleStatus = 'group.m_sRuntimeStatus = "support_active";' },
		[pscustomobject]@{ Name = "recall exit"; Text = $supportRecallConfirmMatch.Value; Target = "request.m_vRecallExitPosition"; StaleStatus = 'group.m_sRuntimeStatus = "support_recalling";' }
	)) {
	foreach ($requiredSettlementEntry in @(
			"physicalWar.IsActiveSupportGroupPhysicallyWithinDistance",
			$supportSettlementProof.Target,
			$supportSettlementProof.StaleStatus,
			"group.m_iAssignedWaypointCount = 0;"
		)) {
		if ($supportSettlementProof.Text -notmatch [regex]::Escape($requiredSettlementEntry)) {
			throw "Physical support $($supportSettlementProof.Name) must fail closed on current live-distance evidence: $requiredSettlementEntry"
		}
	}
}

$supportBuildRouteText = $supportBuildRouteMatch.Value
if ($supportBuildRouteText -notmatch '(?s)if \(IsLiveConfirmedOperationRouteGroup\(state, activeGroup\)\)\s*return BuildDirectSupportRoutePositions\(activeGroup\);') {
	throw "Physical support must use a direct current-position route instead of a stale generated route"
}
$directSupportBranchIndex = $supportBuildRouteText.IndexOf("BuildDirectSupportRoutePositions(activeGroup)")
$generatedRouteGuardIndex = $supportBuildRouteText.IndexOf("if (!route)")
if ($directSupportBranchIndex -lt 0 -or $generatedRouteGuardIndex -lt 0 -or $directSupportBranchIndex -gt $generatedRouteGuardIndex) {
	throw "Physical support direct-route selection must occur before generated-route handling"
}
foreach ($requiredDirectSupportRouteEntry in @(
		"vector sourcePosition = activeGroup.m_vPosition;",
		"sourcePosition = activeGroup.m_vSourcePosition;",
		"vector targetPosition = activeGroup.m_vTargetPosition;",
		"AppendActiveGroupRoutePosition(positions, sourcePosition);",
		"AppendActiveGroupRoutePosition(positions, targetPosition);"
	)) {
	if ($supportDirectRouteMatch.Value -notmatch [regex]::Escape($requiredDirectSupportRouteEntry)) {
		throw "Physical support direct route must chain current live position to the active target: $requiredDirectSupportRouteEntry"
	}
}
foreach ($requiredRecallRouteEntry in @(
		"TryRefreshActiveSupportGroupLivePosition(activeGroup, livePositionEvidence);",
		"DeleteRuntimeGroupWaypoints(groupId);",
		"activeGroup.m_vSourcePosition = sourcePosition;",
		"activeGroup.m_vTargetPosition = exitPosition;",
		'activeGroup.m_sRouteId = "";',
		'activeGroup.m_sRuntimeStatus = "support_recalling";'
	)) {
	if ($supportRecallRouteMatch.Value -notmatch [regex]::Escape($requiredRecallRouteEntry)) {
		throw "Support recall must rebuild a direct route from current live position: $requiredRecallRouteEntry"
	}
}
$recallLiveRefreshIndex = $supportRecallRouteMatch.Value.IndexOf("TryRefreshActiveSupportGroupLivePosition(activeGroup, livePositionEvidence);")
$recallSourceCaptureIndex = $supportRecallRouteMatch.Value.IndexOf("vector sourcePosition = activeGroup.m_vPosition;")
$recallOldWaypointDeleteIndex = $supportRecallRouteMatch.Value.IndexOf("DeleteRuntimeGroupWaypoints(groupId);")
$recallTargetWriteIndex = $supportRecallRouteMatch.Value.IndexOf("activeGroup.m_vTargetPosition = exitPosition;")
if ($recallLiveRefreshIndex -lt 0 -or $recallSourceCaptureIndex -lt $recallLiveRefreshIndex -or $recallOldWaypointDeleteIndex -lt $recallSourceCaptureIndex -or $recallTargetWriteIndex -lt $recallOldWaypointDeleteIndex) {
	throw "Support recall must refresh and capture the live position before replacing the old route with the exit target"
}
foreach ($supportRecallCommand in @(
		[pscustomobject]@{ Name = "ordinary"; Text = $supportNormalRecallMatch.Value },
		[pscustomobject]@{ Name = "exact"; Text = $supportExactRecallMatch.Value }
	)) {
	$recallRefreshIndex = $supportRecallCommand.Text.IndexOf("TryRefreshActiveSupportGroupLivePosition(group, livePositionEvidence)")
	$recallExitResolveIndex = $supportRecallCommand.Text.IndexOf("ResolveSupportRecallExitPosition(state, request, group)")
	if ($recallRefreshIndex -lt 0 -or $recallExitResolveIndex -lt 0 -or $recallRefreshIndex -gt $recallExitResolveIndex) {
		throw "$($supportRecallCommand.Name) support recall must refresh live position before resolving the exit vector"
	}
}

foreach ($requiredBoundedRouteRecoveryEntry in @(
		"lastProgressAge >= ACTIVE_GROUP_ROUTE_STALL_REISSUE_SECONDS",
		"lastReissueAge >= ACTIVE_GROUP_ROUTE_REISSUE_COOLDOWN_SECONDS",
		"progress.m_iRouteReissueAttemptCount < ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS",
		"progress.m_iRouteReissueAttemptCount >= ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS",
		"progress.m_iLastRouteReissueSecond = state.m_iElapsedSeconds;",
		"progress.m_iRouteReissueAttemptCount++;",
		"progress.m_iRouteReissueAttemptCount = 1;",
		"bounded waypoint reissue attempts"
	)) {
	if ($supportPhysicalRouteText -notmatch [regex]::Escape($requiredBoundedRouteRecoveryEntry)) {
		throw "Physical support stalled-route recovery must stay cooldown-gated and capped: $requiredBoundedRouteRecoveryEntry"
	}
}
if ($supportPhysicalRouteText -notmatch '(?s)if \(!initialAssignment\)\s*progress\.m_iRouteReissueAttemptCount\+\+;') {
	throw "Physical support route retry count must increment only for a reissue"
}
if ($supportPhysicalRouteText -notmatch '(?s)bool canRetry = stalled\s*&& lastReissueAge >= ACTIVE_GROUP_ROUTE_REISSUE_COOLDOWN_SECONDS\s*&& progress\.m_iRouteReissueAttemptCount < ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS;') {
	throw "Physical support route reissue must require stall age, cooldown age, and remaining bounded attempts"
}
if ($supportPhysicalRouteText -notmatch '(?s)if \(stalled && progress\.m_iRouteReissueAttemptCount >= ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS\)\s*\{') {
	throw "Physical support route recovery must stop after the configured maximum reissue attempts"
}

$supportWaypointAssignText = $supportWaypointAssignMatch.Value
foreach ($requiredTrackedWaypointEntry in @(
		"array<IEntity> preparedEntities = {};",
		"array<AIWaypoint> preparedWaypoints = {};",
		"if (preparedWaypoints.Count() <= 1)",
		"SCR_EntityHelper.DeleteEntityAndChildren(preparedEntity)",
		"DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);",
		"group.AddWaypoint(preparedWaypoints[preparedIndex]);",
		"m_aRuntimeGroupWaypointIds.Insert(activeGroup.m_sGroupId);",
		"m_aRuntimeGroupWaypointEntities.Insert(preparedEntities[preparedIndex]);",
		'activeGroup.m_sRuntimeStatus != "support_recalling"'
	)) {
	if ($supportWaypointAssignText -notmatch [regex]::Escape($requiredTrackedWaypointEntry)) {
		throw "Physical support waypoint assignment must prepare, atomically replace, and track live handles: $requiredTrackedWaypointEntry"
	}
}
$preparedWaypointGuardIndex = $supportWaypointAssignText.IndexOf("if (preparedWaypoints.Count() <= 1)")
$deletePreviousWaypointsIndex = $supportWaypointAssignText.IndexOf("DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);")
if ($preparedWaypointGuardIndex -lt 0 -or $deletePreviousWaypointsIndex -lt 0 -or $deletePreviousWaypointsIndex -lt $preparedWaypointGuardIndex) {
	throw "Physical support must retain its previous waypoint chain until a replacement chain is fully prepared"
}
foreach ($requiredLiveWaypointReadinessEntry in @(
		"activeGroup.m_iAssignedWaypointCount > 1",
		'activeGroup.m_sSpawnFallbackMode.Contains("infantry_waypoints")',
		"CountRuntimeGroupWaypointEntities(activeGroup.m_sGroupId) > 1"
	)) {
	if ($supportWaypointReadyMatch.Value -notmatch [regex]::Escape($requiredLiveWaypointReadinessEntry)) {
		throw "Physical support route readiness must require retained live waypoint handles: $requiredLiveWaypointReadinessEntry"
	}
}
foreach ($requiredWaypointHandleCountEntry in @(
		"m_aRuntimeGroupWaypointIds[i] != groupId",
		"i >= m_aRuntimeGroupWaypointEntities.Count()",
		"waypointEntity && !waypointEntity.IsDeleted()"
	)) {
	if ($supportWaypointCountMatch.Value -notmatch [regex]::Escape($requiredWaypointHandleCountEntry)) {
		throw "Physical support waypoint readiness must count aligned, non-deleted handles: $requiredWaypointHandleCountEntry"
	}
}
$supportDeleteWaypointsText = $supportDeleteWaypointsMatch.Value
foreach ($requiredWaypointDeleteEntry in @(
		"group.RemoveWaypoint(waypoint);",
		"SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);",
		"m_aRuntimeGroupWaypointEntities.Remove(waypointIndex);",
		"m_aRuntimeGroupWaypointIds.Remove(waypointIndex);"
	)) {
	if ($supportDeleteWaypointsText -notmatch [regex]::Escape($requiredWaypointDeleteEntry)) {
		throw "Physical support waypoint cleanup must remove group membership, entity, and tracking handle: $requiredWaypointDeleteEntry"
	}
}
$removeWaypointIndex = $supportDeleteWaypointsText.IndexOf("group.RemoveWaypoint(waypoint);")
$deleteWaypointEntityIndex = $supportDeleteWaypointsText.IndexOf("SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);")
if ($removeWaypointIndex -lt 0 -or $deleteWaypointEntityIndex -lt 0 -or $removeWaypointIndex -gt $deleteWaypointEntityIndex) {
	throw "Physical support waypoint cleanup must detach the waypoint from the AIGroup before deleting it"
}

$supportServerGuardIndex = $supportWaypointAssignText.IndexOf("if (!Replication.IsServer())")
$supportMasterGuardIndex = $supportWaypointAssignText.IndexOf("if (groupReplication && !groupReplication.IsMaster())")
foreach ($authoritativeWaypointMutation in @(
		"SpawnActiveGroupRouteWaypoint(",
		"DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);",
		"group.AddWaypoint("
	)) {
	$authoritativeMutationIndex = $supportWaypointAssignText.IndexOf($authoritativeWaypointMutation)
	if ($supportServerGuardIndex -lt 0 -or $supportMasterGuardIndex -lt 0 -or $authoritativeMutationIndex -lt 0 -or $supportServerGuardIndex -gt $authoritativeMutationIndex -or $supportMasterGuardIndex -gt $authoritativeMutationIndex) {
		throw "Physical support waypoint mutation must occur only on the server/master: $authoritativeWaypointMutation"
	}
}
Write-Host "Physical support live-route authority and bounded recovery contract OK"

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
		"ResolveRuntimeEntityFactionKey",
		"ApplyCivilianAIGroupFaction",
		"{6985327711303920}Prefabs/Groups/CIV/HST_CivilianRuntimeEmptyGroup.et",
		"CountRuntimeEntityFactionMismatchesForZone",
		"phase20.civilian_population.civ_faction_mismatches",
		"IsTrueTownLocation",
		"IsMinorCivilianLocality",
		"IsCivilianProjectionEligible",
		"ResolveCivilianPedestrianTarget",
		"ResolveCivilianTrafficTarget",
		"HST_CivilianProjectionProofSummary",
		"BuildProjectionProofSummary",
		"RunCampaignDebugCivilianPopulationProbe",
		"CountUniqueRuntimeEntityPrefabsForZone",
		"CountCivilianActorAppearancesForZone",
		"CountUniqueCivilianActorPrefabsForZone",
		"SuppressAmbientTrafficHornInput",
		"CountAmbientTrafficDriversWithHornInput",
		"SetVehicleHorn(0)",
		"phase20.civilian_population.character_diversity",
		"phase20.civilian_population.traffic_horn_suppression",
		"phase20.civilian_population.inactive_zone_projection",
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
if ($civilianRuntimeServiceText -match 'AddAgentFromControlledEntity') {
	throw "Initial civilian AI composition must not broadcast the player-group member-state RPC"
}
foreach ($requiredCivilianInitialGroupAttachEntry in @(
		"scrGroup.AddAIEntityToGroup(memberEntity)",
		"if (agent.GetParentGroup() != group)",
		"group.AddAgent(agent)"
	)) {
	if ($civilianRuntimeServiceText -notmatch [regex]::Escape($requiredCivilianInitialGroupAttachEntry)) {
		throw "Civilian runtime must use the stock initial-AI group attach path with a direct fallback: $requiredCivilianInitialGroupAttachEntry"
	}
}
foreach ($requiredCivilianTrafficSeatingEntry in @(
		"protected bool AssignCivilianTrafficBehavior",
		"if (!AssignCivilianTrafficBehavior(state, balance, zone, trafficVehicle, i, seed))",
		"CleanupFailedCivilianTrafficRuntimeEntity(state, trafficVehicle, zone.m_sZoneId)",
		"protected void CleanupFailedCivilianTrafficRuntimeEntity",
		"DeleteRuntimeHelpersForOwner(vehicleEntity)",
		"MarkRuntimeVehicleDeleted(state, vehicleEntity)",
		"RemoveRuntimeEntityAt(runtimeIndex)",
		"ambient traffic route assignment failed",
		"if (!TryRegisterCivilianVehicleWithGroup(group, vehicleEntity))",
		"ambient traffic vehicle registration failed",
		"RplComponent driverReplication",
		"driverReplication.IsOwner()",
		"access.GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true)",
		"access.MoveInVehicle(vehicleEntity, ECompartmentType.PILOT, true, slot)",
		"authority-local driver entry accepted",
		"owner driver move-in request accepted"
	)) {
	if ($civilianRuntimeServiceText -notmatch [regex]::Escape($requiredCivilianTrafficSeatingEntry)) {
		throw "Civilian traffic must register its vehicle and prefer authority-local driver seating: $requiredCivilianTrafficSeatingEntry"
	}
}
$civilianTrafficSeatStart = $civilianRuntimeServiceText.IndexOf("protected bool TryMoveCivilianDriverIntoVehicle")
$civilianCompartmentResolverStart = -1
if ($civilianTrafficSeatStart -ge 0) {
	$civilianCompartmentResolverStart = $civilianRuntimeServiceText.IndexOf("protected BaseCompartmentManagerComponent ResolveCompartmentManager", $civilianTrafficSeatStart)
}
if ($civilianTrafficSeatStart -lt 0 -or $civilianCompartmentResolverStart -le $civilianTrafficSeatStart) {
	throw "Civilian traffic validator could not isolate TryMoveCivilianDriverIntoVehicle"
}
$civilianTrafficSeatText = $civilianRuntimeServiceText.Substring($civilianTrafficSeatStart, $civilianCompartmentResolverStart - $civilianTrafficSeatStart)
$civilianLocalSeatIndex = $civilianTrafficSeatText.IndexOf("access.GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true)")
$civilianOwnerSeatIndex = $civilianTrafficSeatText.IndexOf("access.MoveInVehicle(vehicleEntity, ECompartmentType.PILOT, true, slot)")
if ($civilianLocalSeatIndex -lt 0 -or $civilianOwnerSeatIndex -lt 0 -or $civilianLocalSeatIndex -gt $civilianOwnerSeatIndex) {
	throw "Server-owned civilian drivers must attempt authority-local forced seating before the owner-RPC fallback"
}
if ($civilianRuntimeServiceText -match '"Prefabs/Characters/Factions/CIV/Character_CIV' -or $configResourceText -match '"Prefabs/Characters/Factions/CIV/Character_CIV' -or $defaultCatalog -match '"Prefabs/Characters/Factions/CIV/Character_CIV') {
	throw "Civilian character runtime must not use path-only Character_CIV resources"
}
$civilianCharacterPoolEntries = [regex]::Matches($configResourceText + "`n" + $defaultCatalog, '"\{[0-9A-F]{16}\}Prefabs/Characters/Factions/CIV/[^"]+Character_CIV_[^"]+\.et"')
if ($civilianCharacterPoolEntries.Count -lt 40) {
	throw "Civilian character runtime must ship a broad concrete GUID-qualified stock CIV appearance pool in config and runtime defaults"
}
if ($configResourceText -match "Character_CIV_Randomized.et" -or $defaultCatalog -match "Character_CIV_Randomized.et") {
	throw "Civilian character runtime must not use editor-randomized entries for direct runtime spawning"
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
foreach ($requiredHQCompositionClearanceEntry in @(
		"HQ_IMMEDIATE_CLEARANCE_METERS = 150.0",
		"IsInsideHQImmediateClearance",
		"inside immediate HQ clearance"
	)) {
	if ($zoneCompositionServiceText -notmatch [regex]::Escape($requiredHQCompositionClearanceEntry)) {
		throw "Zone composition must use immediate HQ clearance without suppressing nearby locations: $requiredHQCompositionClearanceEntry"
	}
}
foreach ($requiredZoneCompositionRadioEntry in @(
		"ShouldSkipRadioTowerStaticForMissionTarget",
		"HasLiveMissionDestroyTarget",
		"active mission target owns radio tower",
		"ShouldUseExistingRadioTower",
		"existing world radio tower retained",
		"IsTransmitterTowerEntity",
		"EMapDescriptorType.MDT_TRANSMITTER",
		"RADIO_TOWER_PREFAB_TOKEN",
		"RecordSkip",
		"ApplyUprightEntityTransform(entity, slot.m_vPosition, slot.m_vAngles)"
	)) {
	if ($zoneCompositionServiceText -notmatch [regex]::Escape($requiredZoneCompositionRadioEntry)) {
		throw "Zone composition must keep radio mission duplicate-tower guard: $requiredZoneCompositionRadioEntry"
	}
}
foreach ($requiredAuthoredRadioMissionEntry in @(
		"TryBindExistingRadioTower",
		"FindExistingRadioTower",
		"m_aBorrowedWorldEntities",
		"bound existing radio tower",
		"!m_aBorrowedWorldEntities.Contains(entity)",
		"ForgetBorrowedWorldEntity(entity)"
	)) {
	if ($missionRuntimeServiceText -notmatch [regex]::Escape($requiredAuthoredRadioMissionEntry)) {
		throw "Radio destroy missions must bind and preserve an existing world tower when one is available: $requiredAuthoredRadioMissionEntry"
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

$schema53Paths = @(
	"Scripts/Game/HST/Services/HST_EnemyPatrolOperationService.c",
	"Scripts/Game/HST/Services/HST_EnemyPatrolOperationProofService.c",
	"Scripts/Game/HST/Services/HST_EnemyPatrolSaveValidationService.c",
	"Scripts/Game/HST/Services/HST_OperationRouteCursorService.c"
)
foreach ($schema53Path in $schema53Paths) {
	if (!(Test-Path $schema53Path)) {
		throw "Schema-53 exact enemy-patrol source is missing: $schema53Path"
	}
}

$schema53TypesText = Get-Content -Raw "Scripts/Game/HST/HST_Types.c"
$schema53StateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
$schema53SaveText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$schema53OperationText = Get-Content -Raw "Scripts/Game/HST/Services/HST_OperationService.c"
$schema53PatrolText = Get-Content -Raw $schema53Paths[0]
$schema53ProofText = Get-Content -Raw $schema53Paths[1]
$schema53ValidationText = Get-Content -Raw $schema53Paths[2]
$schema53RouteText = Get-Content -Raw $schema53Paths[3]
$schema53CommanderText = Get-Content -Raw "Scripts/Game/HST/Services/HST_EnemyCommanderService.c"
$schema53DirectorText = Get-Content -Raw "Scripts/Game/HST/Services/HST_EnemyDirectorService.c"
$schema53PlanningText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForcePlanningService.c"
$schema53PhysicalText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PhysicalWarService.c"
$schema53AdapterText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSpawnAdapterService.c"
$schema53PersistenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PersistenceService.c"
$schema53MarkerText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
$schema53CoordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"

$schema53StateCorpus = $schema53TypesText + "`n" + $schema53StateText + "`n" + $schema53SaveText
foreach ($schema53StateEntry in @(
		"SCHEMA_VERSION = 58",
		"HST_OPERATION_TYPE_ENEMY_PATROL",
		"int m_iRouteWaypointIndex = -1;",
		"int m_iRouteLapCount;",
		"int m_iRouteLegSequence;",
		"int m_iRouteLoopStartedAtSecond;",
		"int m_iRouteLoopCompletedAtSecond;",
		"target.m_iRouteWaypointIndex = source.m_iRouteWaypointIndex;",
		"target.m_iRouteLapCount = source.m_iRouteLapCount;",
		"schema53EnemyPatrolValidation.Normalize(this, restoredSchemaVersion);"
	)) {
	if ($schema53StateCorpus -notmatch [regex]::Escape($schema53StateEntry)) {
		throw "Schema-53 enemy-patrol state/migration contract is missing: $schema53StateEntry"
	}
}

foreach ($schema53RouteEntry in @(
		"class HST_OperationRouteCursorService",
		"BuildOrderedRoutePositions",
		"BuildRouteContractHash",
		"FreezePatrolRoute",
		"IsPatrolRouteContractValid",
		"StartPatrolLoop",
		"AdvanceLoopAfterArrival",
		"BeginReturnLeg",
		"AdvanceVirtualLeg",
		"SyncLegFromPosition"
	)) {
	if ($schema53RouteText -notmatch [regex]::Escape($schema53RouteEntry)) {
		throw "Schema-53 generated patrol-route cursor is missing: $schema53RouteEntry"
	}
}

foreach ($schema53PatrolEntry in @(
		"class HST_EnemyPatrolOperationService",
		"EXACT_CONTRACT_VERSION = 1",
		"QUARANTINED_CONTRACT_VERSION = -53",
		"REQUIRED_PATROL_LAPS = 1",
		"CanAdmitPreparedOrder",
		"AdmitPreparedOrder",
		"FindAdmissionIdentityCollision",
		"QuarantineUnsupportedPatrolAuthority",
		"PrepareOpenPhysicalAuthorityForSettlement",
		"TickOrder",
		"m_RouteCursor.AdvanceVirtualLeg",
		"m_RouteCursor.StartPatrolLoop",
		"m_RouteCursor.AdvanceLoopAfterArrival",
		"m_RouteCursor.BeginReturnLeg",
		"HasExactEnemyPatrolLiveContactEvidence",
		"RefundProactiveAttackResources",
		"ReconcileAfterRestore",
		"ReconcileSettledRuntimeCleanup",
		"SettleOpenOrdersForCampaignStop"
	)) {
	if ($schema53PatrolText -notmatch [regex]::Escape($schema53PatrolEntry)) {
		throw "Schema-53 exact enemy-patrol lifecycle is missing: $schema53PatrolEntry"
	}
}

foreach ($schema53PlanningEntry in @(
		"class HST_EnemyPatrolManifestResult",
		"PlanExactEnemyPatrol",
		"BuildExactEnemyPatrolManifest",
		"HST_EnemyPatrolOperationService.EXACT_FORCE_KIND",
		"HST_EnemyPatrolOperationService.EXACT_POLICY_ID"
	)) {
	if ($schema53PlanningText -notmatch [regex]::Escape($schema53PlanningEntry)) {
		throw "Schema-53 exact enemy-patrol planning is missing: $schema53PlanningEntry"
	}
}
if ($schema53DirectorText -notmatch [regex]::Escape("RefundProactiveAttackResources")) {
	throw "Schema-53 patrol settlement must use the proactive attack-resource refund boundary"
}

$schema53EnemyOrderRequiresBlock = Get-ScriptMethodBlock $schema53OperationText "static bool RequiresOperation(HST_EnemyOrderState order)"
$schema53DispatchBlock = Get-ScriptMethodBlock $schema53CommanderText "string ResolveRuntimeOwner(HST_EnemyOrderState order)"
if ([string]::IsNullOrEmpty($schema53EnemyOrderRequiresBlock) -or
	$schema53EnemyOrderRequiresBlock -notmatch [regex]::Escape("order.m_iOperationContractVersion != 0")) {
	throw "Schema-53 nonzero enemy-order contracts must remain outside legacy execution"
}
foreach ($schema53DispatchEntry in @(
		"RequiresExactEnemyPatrol",
		"RUNTIME_OWNER_EXACT_PATROL",
		"RUNTIME_OWNER_EXACT_QRF",
		"RUNTIME_OWNER_QUARANTINED",
		"RUNTIME_OWNER_UNSUPPORTED",
		"RUNTIME_OWNER_LEGACY"
	)) {
	if ([string]::IsNullOrEmpty($schema53DispatchBlock) -or $schema53DispatchBlock -notmatch [regex]::Escape($schema53DispatchEntry)) {
		throw "Schema-53 type/version enemy-order dispatch is missing: $schema53DispatchEntry"
	}
}
foreach ($schema53CommanderEntry in @(
		"SetExactEnemyPatrolAuthorityService",
		"PrepareExactEnemyPatrol",
		"QuarantineUnsupportedPatrolAuthority",
		"m_ExactEnemyPatrol.AdmitPreparedOrder",
		"m_ExactEnemyPatrol.TickOrder"
	)) {
	if ($schema53CommanderText -notmatch [regex]::Escape($schema53CommanderEntry)) {
		throw "Schema-53 enemy commander wiring is missing: $schema53CommanderEntry"
	}
}

foreach ($schema53RestoreEntry in @(
		"class HST_EnemyPatrolSaveValidationService",
		"migration_schema53_enemy_patrol_authority",
		"legacy_enemy_patrols_preserved_contract_zero",
		"HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION",
		"HST_OPERATION_TYPE_ENEMY_PATROL"
	)) {
	if ($schema53ValidationText -notmatch [regex]::Escape($schema53RestoreEntry)) {
		throw "Schema-53 exact enemy-patrol restore/quarantine contract is missing: $schema53RestoreEntry"
	}
}

foreach ($schema53PhysicalEntry in @(
		"RestartExactEnemyPatrolInfantryRoute",
		"IsExactEnemyPatrolRouteRecoveryExhausted",
		"TryResolveExactEnemyPatrolLivePosition",
		"HasExactEnemyPatrolLiveContactEvidence",
		"IsExactEnemyPatrolGroup"
	)) {
	if ($schema53PhysicalText -notmatch [regex]::Escape($schema53PhysicalEntry)) {
		throw "Schema-53 exact enemy-patrol physical projection is missing: $schema53PhysicalEntry"
	}
}
foreach ($schema53PersistenceEntry in @(
		"ReconcileExactInfantryAuthorityForPersistence",
		"ValidateExactLivingProjectionBindingsForPersistence"
	)) {
	if ($schema53AdapterText -notmatch [regex]::Escape($schema53PersistenceEntry) -or
		$schema53PersistenceText -notmatch [regex]::Escape($schema53PersistenceEntry)) {
		throw "Schema-53 pre-save roster reconciliation is missing: $schema53PersistenceEntry"
	}
}
foreach ($schema53PersistenceEntry in @(
		"PrepareStateForCapture",
		"NormalizeRetiredQuarantinedEnemyPatrolAuthority",
		"ValidatePhysicalEnemyPatrolSnapshots",
		"ValidateExactLivingProjectionBindingsForPersistence",
		"CountForceSpawnRuntimeMembers",
		"TryResolveExactEnemyPatrolLivePosition",
		"checkpoint deferred: exact patrol"
	)) {
	if ($schema53PersistenceText -notmatch [regex]::Escape($schema53PersistenceEntry)) {
		throw "Schema-53 pre-save physical patrol boundary is missing: $schema53PersistenceEntry"
	}
}

foreach ($schema53MarkerEntry in @(
		"AddExactEnemyPatrolOperationMarkers",
		"ShouldShowExactEnemyPatrolMarker",
		"hst_exact_enemy_patrol_",
		"enemy_patrol_exact"
	)) {
	if ($schema53MarkerText -notmatch [regex]::Escape($schema53MarkerEntry)) {
		throw "Schema-53 exact enemy-patrol marker projection is missing: $schema53MarkerEntry"
	}
}
foreach ($schema53CoordinatorEntry in @(
		"m_EnemyPatrolOperations = new HST_EnemyPatrolOperationService()",
		"SetExactEnemyPatrolAuthorityService",
		"m_EnemyPatrolOperations.ReconcileAfterRestore",
		"PrepareOpenPhysicalAuthorityForSettlement",
		"preserveOpenExactEnemyPatrol",
		"m_EnemyPatrolOperations.ReconcileSettledRuntimeCleanup",
		"AppendCampaignDebugEnemyPatrolOperationAssertions"
	)) {
	if ($schema53CoordinatorText -notmatch [regex]::Escape($schema53CoordinatorEntry)) {
		throw "Schema-53 coordinator wiring is missing: $schema53CoordinatorEntry"
	}
}

if ($schema53ProofText -notmatch [regex]::Escape("class HST_EnemyPatrolOperationProofService")) {
	throw "Schema-53 enemy-patrol proof service is missing"
}
foreach ($schema53AssertionId in @(
		"enemy_patrol.admission",
		"enemy_patrol.replay_refund",
		"enemy_patrol.route_loop",
		"enemy_patrol.projection_roster",
		"enemy_patrol.contact_hold",
		"enemy_patrol.settlement",
		"enemy_patrol.persistence",
		"enemy_patrol.corruption",
		"enemy_patrol.dispatch_isolation",
		"enemy_patrol.marker"
	)) {
	if ($schema53CoordinatorText -notmatch [regex]::Escape($schema53AssertionId)) {
		throw "Schema-53 enemy-patrol debug assertion is missing: $schema53AssertionId"
	}
}

$schema53DocumentationPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md",
	"docs/HST_ENFUSION_ENFORCE_NOTES.md",
	"docs/MIGRATIONS.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md"
)
foreach ($schema53DocumentationPath in $schema53DocumentationPaths) {
	$schema53DocumentationText = (Get-Content -Raw $schema53DocumentationPath).ToLowerInvariant()
	$mentionsSchema53 = $schema53DocumentationText.Contains("schema 53") -or $schema53DocumentationText.Contains("schema-53")
	if (!$mentionsSchema53 -or !$schema53DocumentationText.Contains("patrol")) {
		throw "$schema53DocumentationPath must describe the schema-53 enemy-patrol boundary"
	}
}
$schema53MigrationsText = (Get-Content -Raw "docs/MIGRATIONS.md").ToLowerInvariant()
foreach ($schema53MigrationNote in @(
		'contract version `0`',
		'version `-53`',
		"migration_schema53_enemy_patrol_authority",
		"packaged"
	)) {
	if (!$schema53MigrationsText.Contains($schema53MigrationNote)) {
		throw "Schema-53 migration documentation is missing: $schema53MigrationNote"
	}
}
Write-Host "Schema-53 exact enemy-patrol admission, route, projection, persistence, settlement, marker, migration, and proof contract OK"

$schema54Paths = @(
	"Scripts/Game/HST/Services/HST_GarrisonPatrolOperationService.c",
	"Scripts/Game/HST/Services/HST_GarrisonPatrolSaveValidationService.c",
	"Scripts/Game/HST/Services/HST_GarrisonPatrolOperationProofService.c"
)
foreach ($schema54Path in $schema54Paths) {
	if (!(Test-Path $schema54Path)) {
		throw "Schema-54 exact purchased-garrison patrol source is missing: $schema54Path"
	}
}

$schema54TypesText = Get-Content -Raw "Scripts/Game/HST/HST_Types.c"
$schema54StateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
$schema54SaveText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$schema54PlanningText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForcePlanningService.c"
$schema54IntegrityText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForcePlanningIntegrityService.c"
$schema54GarrisonText = Get-Content -Raw "Scripts/Game/HST/Services/HST_GarrisonService.c"
$schema54RecruitmentText = Get-Content -Raw "Scripts/Game/HST/Services/HST_RecruitmentService.c"
$schema54OperationText = Get-Content -Raw $schema54Paths[0]
$schema54ValidationText = Get-Content -Raw $schema54Paths[1]
$schema54ProofText = Get-Content -Raw $schema54Paths[2]
$schema54AdapterText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSpawnAdapterService.c"
$schema54RuntimeProofText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceRuntimeAuthorityProofService.c"
$schema54PhysicalText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PhysicalWarService.c"
$schema54PersistenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PersistenceService.c"
$schema54MarkerText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
$schema54UIServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
$schema54ArchiveText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSettlementArchiveService.c"
$schema54CoordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"

$schema54StateCorpus = $schema54TypesText + "`n" + $schema54StateText + "`n" + $schema54SaveText
foreach ($schema54StateEntry in @(
		"SCHEMA_VERSION = 58",
		"HST_OPERATION_TYPE_GARRISON_PATROL",
		"IsQuarantinedActiveGroup",
		"HST_GarrisonPatrolSaveValidationService schema54GarrisonPatrolValidation",
		"schema54GarrisonPatrolValidation.Normalize(this, restoredSchemaVersion);"
	)) {
	if ($schema54StateCorpus -notmatch [regex]::Escape($schema54StateEntry)) {
		throw "Schema-54 purchased-garrison state/migration contract is missing: $schema54StateEntry"
	}
}
$schema54OperationalGroupBlock = Get-ScriptMethodBlock $schema54StateText "bool IsOperationalActiveGroup("
$schema54QuarantinedGroupBlock = Get-ScriptMethodBlock $schema54StateText "bool IsQuarantinedActiveGroup("
if ([string]::IsNullOrEmpty($schema54OperationalGroupBlock) -or [string]::IsNullOrEmpty($schema54QuarantinedGroupBlock) -or
	$schema54OperationalGroupBlock -notmatch [regex]::Escape('if (IsQuarantinedActiveGroup(group))') -or
	$schema54QuarantinedGroupBlock -notmatch [regex]::Escape('exact_garrison_patrol_quarantined')) {
	throw "Schema-54 quarantined garrison patrols must be globally non-operational and non-combat-present"
}

foreach ($schema54PolicyEntry in @(
		'LEGACY_GARRISON_POLICY_ID = "garrison_exact_all_or_nothing_1"',
		'GARRISON_POLICY_ID = "garrison_exact_patrol_2"',
		"SelectGarrisonExecutionGroup",
		"NotSpawned",
		"manifest.m_iAcceptedMemberCount = requestedMemberCount;",
		"manifest.m_aGroups.Insert(groupElement);",
		"SetExactGarrisonPatrolAuthorityService",
		"CanAdmitPreparedPurchase",
		"AdmitPreparedPurchase"
	)) {
	if (($schema54PlanningText + "`n" + $schema54IntegrityText) -notmatch [regex]::Escape($schema54PolicyEntry)) {
		throw "Schema-54 policy-v2 empty-root/arbitrary-member planning is missing: $schema54PolicyEntry"
	}
}

foreach ($schema54GarrisonEntry in @(
		"LinkExecutableManifestExact",
		"UnlinkExecutableManifestExact",
		"CountExecutableManifestInfantry",
		"ResolveUniqueReciprocalExactPatrolOperation",
		"HST_GarrisonPatrolOperationService.EXACT_POLICY_ID",
		"Missing or ambiguous open authority remains capacity-reserved"
	)) {
	if ($schema54GarrisonText -notmatch [regex]::Escape($schema54GarrisonEntry)) {
		throw "Schema-54 exact purchased-garrison capacity/backlink boundary is missing: $schema54GarrisonEntry"
	}
}
$schema54GarrisonCapacityBlock = Get-ScriptMethodBlock $schema54GarrisonText "int CountExecutableManifestInfantry("
$schema54GarrisonReciprocalBlock = Get-ScriptMethodBlock $schema54GarrisonText "protected HST_OperationRecordState ResolveUniqueReciprocalExactPatrolOperation("
if ([string]::IsNullOrEmpty($schema54GarrisonCapacityBlock) -or
	[string]::IsNullOrEmpty($schema54GarrisonReciprocalBlock)) {
	throw "Schema-54 reciprocal purchased-garrison capacity authority is missing"
}
foreach ($schema54TerminalCapacityEntry in @(
		"ResolveUniqueReciprocalExactPatrolOperation",
		"EXACT_CONTRACT_VERSION",
		"EXACT_PROJECTION_CONTRACT_VERSION",
		"ASSIGNMENT_KIND",
		"SETTLEMENT_POLICY_ID",
		"HST_OPERATION_SETTLEMENT_SETTLED",
		"HST_OPERATION_MATERIALIZATION_RETIRED",
		"SETTLEMENT_KIND"
	)) {
	if ($schema54GarrisonCapacityBlock -notmatch [regex]::Escape($schema54TerminalCapacityEntry)) {
		throw "Schema-54 settled purchased-garrison capacity release is missing reciprocal terminal proof: $schema54TerminalCapacityEntry"
	}
}
foreach ($schema54ReciprocalCapacityEntry in @(
		"operation.m_sOperationId != manifest.m_sOperationId",
		"operation.m_sManifestId != manifest.m_sManifestId",
		"resolved.m_sQuoteId != manifest.m_sQuoteId",
		"resolved.m_sOwnerFactionKey != garrison.m_sFactionKey",
		"resolved.m_sAssignmentZoneId != garrison.m_sZoneId",
		"authorityMatches != 1"
	)) {
	if ($schema54GarrisonReciprocalBlock -notmatch [regex]::Escape($schema54ReciprocalCapacityEntry)) {
		throw "Schema-54 capacity must reserve malformed, foreign, or ambiguous exact operation backlinks: $schema54ReciprocalCapacityEntry"
	}
}

$schema54RecruitmentReportBlock = Get-ScriptMethodBlock $schema54RecruitmentText "string BuildRecruitmentReport("
$schema54RecruitmentAdmissionBlock = Get-ScriptMethodBlock $schema54RecruitmentText "protected HST_RecruitmentResult RecruitGarrisonForFaction("
$schema54RecruitmentCapacityBlock = Get-ScriptMethodBlock $schema54RecruitmentText "protected int ResolveRecruitOccupiedInfantry("
$schema54RecruitmentMapBlock = Get-ScriptMethodBlock $schema54CoordinatorText "protected bool IsRecruitableResistanceMapZone("
foreach ($schema54RecruitmentCapacityEntry in @(
	"CountExecutableManifestInfantry",
	"exact patrol infantry",
	"BuildRecruitCapacityLabel(state, capacityGarrisons, friendly, slots, activeInfantry)"
)) {
	if ([string]::IsNullOrEmpty($schema54RecruitmentReportBlock) -or
		$schema54RecruitmentReportBlock -notmatch [regex]::Escape($schema54RecruitmentCapacityEntry)) {
		throw "Schema-54 recruitment report does not expose exact patrol capacity: $schema54RecruitmentCapacityEntry"
	}
}
if ([string]::IsNullOrEmpty($schema54RecruitmentCapacityBlock) -or
	$schema54RecruitmentCapacityBlock -notmatch [regex]::Escape("garrisons.CountExecutableManifestInfantry(state, garrison)")) {
	throw "Schema-54 direct recruitment capacity must include exact living/reserved patrol infantry"
}
foreach ($schema54RecruitmentAdmissionEntry in @(
	"ResolveRecruitOccupiedInfantry",
	"if (requestedInfantry > infantryCapacity)",
	"direct recruitment is all-or-nothing",
	"addedInfantry != requestedInfantry || addedVehicles != requestedVehicles",
	"partial change rolled back before charge"
)) {
	if ([string]::IsNullOrEmpty($schema54RecruitmentAdmissionBlock) -or
		$schema54RecruitmentAdmissionBlock -notmatch [regex]::Escape($schema54RecruitmentAdmissionEntry)) {
		throw "Schema-54 direct recruitment exact-capacity admission is missing: $schema54RecruitmentAdmissionEntry"
	}
}
$schema54CapacityRejectIndex = $schema54RecruitmentAdmissionBlock.IndexOf("if (requestedInfantry > infantryCapacity)")
$schema54AddIndex = $schema54RecruitmentAdmissionBlock.IndexOf("garrisons.AddAbstractForces")
$schema54ExactDeltaIndex = $schema54RecruitmentAdmissionBlock.IndexOf("addedInfantry != requestedInfantry || addedVehicles != requestedVehicles")
$schema54SpendIndex = $schema54RecruitmentAdmissionBlock.IndexOf("economy.SpendFactionMoney")
if ($schema54CapacityRejectIndex -lt 0 -or $schema54AddIndex -le $schema54CapacityRejectIndex -or
	$schema54ExactDeltaIndex -le $schema54AddIndex -or $schema54SpendIndex -le $schema54ExactDeltaIndex) {
	throw "Schema-54 direct recruitment must reject insufficient capacity before mutation and verify exact admission before charging"
}
if ([string]::IsNullOrEmpty($schema54RecruitmentMapBlock) -or
	$schema54RecruitmentMapBlock -notmatch [regex]::Escape("m_Garrisons.CountExecutableManifestInfantry(m_State, garrison)")) {
	throw "Schema-54 resistance map recruitment gate must reserve exact garrison-patrol living infantry capacity"
}

foreach ($schema54OperationEntry in @(
		"class HST_GarrisonPatrolOperationService",
		"EXACT_CONTRACT_VERSION = 1",
		"QUARANTINED_CONTRACT_VERSION = -54",
		'EXACT_POLICY_ID = "garrison_exact_patrol_2"',
		'EXACT_GROUP_MODE = "exact_garrison_patrol"',
		'SETTLEMENT_KIND = "exact_garrison_patrol_terminal"',
		"HST_OPERATION_TYPE_GARRISON_PATROL",
		"BuildLocalPatrolRoute",
		"HoldPendingProjectionForStrategicSimulation",
		"AdvanceInfinitePatrolLoop",
		"LOOP_COUNTER_WRAP_LAPS",
		"ReleaseStrategicProjectionForMaterialization",
		"RequeueSuccessfulProjectionForStrategicHold",
		"ValidateExactLivingProjectionBindingsForPersistence",
		"RestartExactGarrisonPatrolInfantryRoute",
		"owner_changed",
		"SettleOpenOperationsForCampaignStop",
		"PrepareOpenPhysicalAuthorityForSettlement",
		"QuarantineOperationAuthority",
		"TryRetireQuarantinedSuccessfulProjection",
		"ResolveQuarantinedSuccessfulProjectionContext",
		"CompleteQuarantinedSuccessfulProjectionCancellation"
	)) {
	if ($schema54OperationText -notmatch [regex]::Escape($schema54OperationEntry)) {
		throw "Schema-54 exact purchased-garrison patrol lifecycle is missing: $schema54OperationEntry"
	}
}
$schema54RetireAndSettleBlock = Get-ScriptMethodBlock $schema54OperationText "protected bool RetireAndSettle("
if ([string]::IsNullOrEmpty($schema54RetireAndSettleBlock)) {
	throw "Schema-54 exact purchased-garrison terminal retirement boundary is missing"
}
foreach ($schema54TerminalRuntimeEntry in @(
		"establishedLiveAuthority",
		"CountDurableLivingMemberSlots",
		"!physicalRuntime && durableLiving > 0",
		"QuarantineOperationAuthority",
		"live runtime without successful batch authority"
	)) {
	if ($schema54RetireAndSettleBlock -notmatch [regex]::Escape($schema54TerminalRuntimeEntry)) {
		throw "Schema-54 live terminal transition may settle unresolved survivor authority: $schema54TerminalRuntimeEntry"
	}
}
$schema54MissingRuntimeGuardIndex = $schema54RetireAndSettleBlock.IndexOf('!physicalRuntime && durableLiving > 0')
$schema54SettleCallIndex = $schema54RetireAndSettleBlock.IndexOf('return SettleOperation')
if ($schema54MissingRuntimeGuardIndex -lt 0 -or $schema54SettleCallIndex -le $schema54MissingRuntimeGuardIndex) {
	throw "Schema-54 missing-live-runtime survivor quarantine must execute before terminal settlement"
}
$schema54QuarantineStatusBlock = Get-ScriptMethodBlock $schema54OperationText "protected bool ApplyQuarantineStatus("
$schema54QuarantineRetirementBlock = Get-ScriptMethodBlock $schema54OperationText "protected bool RetireQuarantinedSuccessfulProjectionWithRuntimeIdentity("
$schema54QuarantineBatchClaimBlock = Get-ScriptMethodBlock $schema54OperationText "protected bool BatchClaimsOperationAuthority("
$schema54QuarantineGroupClaimBlock = Get-ScriptMethodBlock $schema54OperationText "protected bool GroupClaimsOperationAuthority("
if ([string]::IsNullOrEmpty($schema54QuarantineStatusBlock) -or [string]::IsNullOrEmpty($schema54QuarantineRetirementBlock) -or
	[string]::IsNullOrEmpty($schema54QuarantineBatchClaimBlock) -or [string]::IsNullOrEmpty($schema54QuarantineGroupClaimBlock)) {
	throw "Schema-54 typed successful-projection quarantine retirement is missing"
}
if ($schema54QuarantineBatchClaimBlock -notmatch [regex]::Escape('batch.m_sOperationId != operation.m_sOperationId') -or
	$schema54QuarantineGroupClaimBlock -notmatch [regex]::Escape('group.m_sOperationId != operation.m_sOperationId')) {
	throw "Schema-54 quarantine may mutate a batch/group only from operation identity plus an independent runtime backlink"
}
$schema54QuarantineRetireCallIndex = $schema54QuarantineStatusBlock.IndexOf('TryRetireQuarantinedSuccessfulProjection')
$schema54QuarantineCancelIndex = $schema54QuarantineStatusBlock.IndexOf('m_SpawnQueue.RequestCancel')
if ($schema54QuarantineRetireCallIndex -lt 0 -or $schema54QuarantineCancelIndex -le $schema54QuarantineRetireCallIndex) {
	throw "Schema-54 successful physical quarantine must retire exact runtime before queue cancellation"
}
$schema54QuarantineReconcileIndex = $schema54QuarantineRetirementBlock.IndexOf('ReconcileQuarantinedExactInfantryProjectionAuthority')
$schema54QuarantineBindingIndex = $schema54QuarantineRetirementBlock.IndexOf('ValidateExactLivingProjectionBindingsForPersistence')
$schema54QuarantineRuntimeRetireIndex = $schema54QuarantineRetirementBlock.IndexOf('RetireProjectionRuntime')
$schema54QuarantineTerminalIndex = $schema54QuarantineRetirementBlock.IndexOf('CompleteQuarantinedSuccessfulProjectionCancellation')
if ($schema54QuarantineReconcileIndex -lt 0 -or $schema54QuarantineBindingIndex -le $schema54QuarantineReconcileIndex -or
	$schema54QuarantineRuntimeRetireIndex -le $schema54QuarantineBindingIndex -or
	$schema54QuarantineTerminalIndex -le $schema54QuarantineRuntimeRetireIndex -or
	$schema54QuarantineRetirementBlock -match 'CanRequeueSuccessfulProjectionForStrategicHold|RequeueSuccessfulProjectionForStrategicHold') {
	throw "Schema-54 physical quarantine must reconcile typed casualties, prove bindings, retire runtime, then terminalize without normal requeue capacity"
}
$schema54QuarantineAdapterBlock = Get-ScriptMethodBlock $schema54AdapterText "HST_ForceSpawnAdapterTickResult ReconcileQuarantinedExactInfantryProjectionAuthority("
$schema54QuarantineCasualtyBlock = Get-ScriptMethodBlock (Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSpawnQueueService.c") "HST_ForceSpawnQueueCallbackResult ConfirmQuarantinedRegisteredMemberCasualty("
$schema54QuarantineTerminalBlock = Get-ScriptMethodBlock (Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSpawnQueueService.c") "HST_ForceSpawnQueueCallbackResult CompleteQuarantinedSuccessfulProjectionCancellation("
if ([string]::IsNullOrEmpty($schema54QuarantineAdapterBlock) -or [string]::IsNullOrEmpty($schema54QuarantineCasualtyBlock) -or
	[string]::IsNullOrEmpty($schema54QuarantineTerminalBlock)) {
	throw "Schema-54 projection-key quarantine adapter/queue authority is missing"
}
if ($schema54QuarantineAdapterBlock -notmatch [regex]::Escape('ReconcileHandedOffMemberLifecycle') -or
	$schema54AdapterText -notmatch [regex]::Escape('ConfirmQuarantinedRegisteredMemberCasualty')) {
	throw "Schema-54 quarantine reconciliation must preserve mapped casualty tombstones without requiring a valid manifest backlink"
}
foreach ($schema54QuarantineTerminalEntry in @(
		'resultMatches != 1',
		'projectionMatches != 1',
		'HST_FORCE_SPAWN_SUCCEEDED',
		'HST_FORCE_SLOT_RETIRED',
		'm_bCasualtyConfirmed',
		'HST_FORCE_SLOT_CANCELLED',
		'HST_FORCE_SPAWN_CANCELLED'
	)) {
	if ($schema54QuarantineTerminalBlock -notmatch [regex]::Escape($schema54QuarantineTerminalEntry)) {
		throw "Schema-54 projection-key quarantine terminalizer is missing: $schema54QuarantineTerminalEntry"
	}
}
if ($schema54QuarantineTerminalBlock -match 'MAX_NONTERMINAL|CanEnqueue|Requeue') {
	throw "Schema-54 quarantine terminalization must not depend on normal admission or reprojection capacity"
}
$schema54RestoreRuntimeBlock = Get-ScriptMethodBlock $schema54OperationText "protected bool NormalizeRestoredOpenRuntime("
if ([string]::IsNullOrEmpty($schema54RestoreRuntimeBlock) -or
	$schema54RestoreRuntimeBlock -notmatch [regex]::Escape('CompleteQuarantinedSuccessfulProjectionCancellation')) {
	throw "Schema-54 restore must terminalize a proven-retired successful batch when normal strategic requeue cannot admit it"
}
$schema54SettlementBlock = Get-ScriptMethodBlock $schema54OperationText "protected bool SettleOperation("
if ([string]::IsNullOrEmpty($schema54SettlementBlock)) {
	throw "Schema-54 exact purchased-garrison terminal settlement method is missing"
}
foreach ($schema54SettlementEntry in @(
		"SETTLEMENT_KIND",
		"HST_OPERATION_SETTLEMENT_SETTLED",
		"FinalizeSettledRuntime"
	)) {
	if ($schema54SettlementBlock -notmatch [regex]::Escape($schema54SettlementEntry)) {
		throw "Schema-54 exact purchased-garrison terminal receipt is missing: $schema54SettlementEntry"
	}
}
if ($schema54SettlementBlock -match '(?i)refund|AddAbstractForces|m_iInfantryCount\s*\+') {
	throw "Schema-54 exact purchased-garrison operation settlement must not refund resources or transfer survivors to aggregate infantry"
}

$schema54AdapterRosterBlock = Get-ScriptMethodBlock $schema54AdapterText "protected string ValidateExactInfantryExecutionRoster("
$schema54AdapterBindingBlock = Get-ScriptMethodBlock $schema54AdapterText "bool ValidateExactLivingProjectionBindingsForPersistence("
foreach ($schema54AdapterEntry in @(
		"CountDurableLivingMemberSlots",
		"batch.m_iSuccessfulHandoffCount <= 0",
		"CountStrategicLivingMemberSlots",
		"activeGroup.m_iInfantryCount != livingCount"
	)) {
	if ([string]::IsNullOrEmpty($schema54AdapterRosterBlock) -or $schema54AdapterRosterBlock -notmatch [regex]::Escape($schema54AdapterEntry)) {
		throw "Schema-54 survivor-only rematerialization roster fix is missing: $schema54AdapterEntry"
	}
}
if ([string]::IsNullOrEmpty($schema54AdapterBindingBlock) -or
	$schema54AdapterBindingBlock -notmatch [regex]::Escape('physicalWar.IsForceSpawnRuntimeHandleRegistered(activeGroup, handle.m_Entity)')) {
	throw "Schema-54 exact persistence/retirement preflight must prove every adapter member belongs to the same PhysicalWar projection"
}
foreach ($schema54AdapterProofEntry in @(
		"first/survivor",
		"rematerialized survivor",
		"stale/original rejected"
	)) {
	if ($schema54RuntimeProofText -notmatch [regex]::Escape($schema54AdapterProofEntry)) {
		throw "Schema-54 survivor-only adapter proof is missing: $schema54AdapterProofEntry"
	}
}

foreach ($schema54PhysicalEntry in @(
		"RestartExactGarrisonPatrolInfantryRoute",
		"IsExactGarrisonPatrolRouteRecoveryExhausted",
		"TryResolveExactGarrisonPatrolLivePosition",
		"HasExactGarrisonPatrolLiveContactEvidence",
		"IsExactGarrisonPatrolGroup"
	)) {
	if ($schema54PhysicalText -notmatch [regex]::Escape($schema54PhysicalEntry)) {
		throw "Schema-54 exact purchased-garrison PhysicalWar isolation is missing: $schema54PhysicalEntry"
	}
}
foreach ($schema54LegacyIsolationEntry in @(
		"IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup)",
		"!IsExactGarrisonPatrolGroup(state, activeGroup)",
		"IsExactGarrisonPatrolActiveGroup"
	)) {
	if ($schema54PhysicalText -notmatch [regex]::Escape($schema54LegacyIsolationEntry)) {
		throw "Schema-54 exact purchased-garrison group is not isolated from a legacy PhysicalWar owner: $schema54LegacyIsolationEntry"
	}
}
$schema54GenericSurvivorBlock = Get-ScriptMethodBlock $schema54PhysicalText "protected bool UpdateRuntimeGroupSurvivors("
if ([string]::IsNullOrEmpty($schema54GenericSurvivorBlock)) {
	throw "Schema-54 generic survivor boundary is missing"
}
$schema54PatrolSurvivorSkipStart = $schema54GenericSurvivorBlock.IndexOf('if (IsExactEnemyPatrolGroup(state, activeGroup)')
$schema54PatrolSurvivorSkipEnd = $schema54GenericSurvivorBlock.IndexOf('bool missionConvoyGroup', $schema54PatrolSurvivorSkipStart)
if ($schema54PatrolSurvivorSkipStart -lt 0 -or $schema54PatrolSurvivorSkipEnd -le $schema54PatrolSurvivorSkipStart) {
	throw "Schema-54 exact patrol groups must be isolated before generic survivor mutation"
}
$schema54PatrolSurvivorSkip = $schema54GenericSurvivorBlock.Substring(
	$schema54PatrolSurvivorSkipStart,
	$schema54PatrolSurvivorSkipEnd - $schema54PatrolSurvivorSkipStart)
if ($schema54PatrolSurvivorSkip -notmatch [regex]::Escape('IsExactGarrisonPatrolGroup(state, activeGroup)') -or
	$schema54PatrolSurvivorSkip -notmatch 'continue\s*;') {
	throw "Schema-54 exact garrison patrol casualties must bypass the aggregate survivor/cleanup owner"
}

foreach ($schema54PersistenceEntry in @(
		"hasMaterializingExactInfantry",
		"checkpoint deferred: exact infantry materialization is in progress",
		"ValidatePhysicalGarrisonPatrolSnapshots",
		"DeferPhysicalGarrisonPatrolSnapshot",
		"TryResolveExactGarrisonPatrolLivePosition",
		"ValidateExactLivingProjectionBindingsForPersistence"
	)) {
	if ($schema54PersistenceText -notmatch [regex]::Escape($schema54PersistenceEntry)) {
		throw "Schema-54 pre-save exact purchased-garrison boundary is missing: $schema54PersistenceEntry"
	}
}
$schema54PrepareCaptureBlock = Get-ScriptMethodBlock $schema54PersistenceText "protected bool PrepareStateForCapture("
$schema54QuarantineDrainBlock = Get-ScriptMethodBlock $schema54PersistenceText "protected bool NormalizeRetiredQuarantinedGarrisonPatrolAuthority("
$schema54QuarantineBatchCleanupBlock = Get-ScriptMethodBlock $schema54PersistenceText "protected bool IsQuarantinedGarrisonBatchCleanupComplete("
if ([string]::IsNullOrEmpty($schema54PrepareCaptureBlock) -or [string]::IsNullOrEmpty($schema54QuarantineDrainBlock) -or
	[string]::IsNullOrEmpty($schema54QuarantineBatchCleanupBlock)) {
	throw "Schema-54 quarantined-runtime checkpoint boundary is missing"
}
if ($schema54PrepareCaptureBlock.IndexOf('NormalizeRetiredQuarantinedGarrisonPatrolAuthority') -lt 0 -or
	$schema54PrepareCaptureBlock.IndexOf('NormalizeRetiredQuarantinedGarrisonPatrolAuthority') -gt $schema54PrepareCaptureBlock.IndexOf('foreach (HST_OperationRecordState operation')) {
	throw "Schema-54 quarantine cleanup must run before current exact-operation checkpoint classification"
}
foreach ($schema54QuarantineDrainEntry in @(
		'HST_GarrisonPatrolOperationService.QUARANTINED_CONTRACT_VERSION',
		'CountHandlesForProjection',
		'CountHandlesForResultId',
		'GetForceSpawnGroupRoot',
		'CountForceSpawnRuntimeMembers',
		'IsQuarantinedGarrisonBatchCleanupComplete',
		'NormalizeQuarantinedGarrisonPatrolProcessResidue'
	)) {
	if ($schema54QuarantineDrainBlock -notmatch [regex]::Escape($schema54QuarantineDrainEntry)) {
		throw "Schema-54 quarantined-runtime checkpoint proof is missing: $schema54QuarantineDrainEntry"
	}
}
foreach ($schema54QuarantineBatchCleanupEntry in @(
		'HST_FORCE_SPAWN_CANCELLED',
		'HST_FORCE_SPAWN_FAILED_FINAL',
		'HST_FORCE_SLOT_REGISTERED',
		'HST_FORCE_SLOT_SPAWNING',
		'HST_FORCE_SLOT_CLEANUP_PENDING'
	)) {
	if ($schema54QuarantineBatchCleanupBlock -notmatch [regex]::Escape($schema54QuarantineBatchCleanupEntry)) {
		throw "Schema-54 quarantine queue-drain proof is missing: $schema54QuarantineBatchCleanupEntry"
	}
}

foreach ($schema54RestoreEntry in @(
		"class HST_GarrisonPatrolSaveValidationService",
		"migration_schema54_exact_garrison_patrol",
		"normalization_schema54_exact_garrison_patrol_conflict",
		"HST_GarrisonPatrolOperationService.QUARANTINED_CONTRACT_VERSION",
		"PreserveHistoricalGarrisons",
		"created no exact operation, roster, batch, group, route, casualty, or settlement identity"
	)) {
	if ($schema54ValidationText -notmatch [regex]::Escape($schema54RestoreEntry)) {
		throw "Schema-54 exact purchased-garrison migration/quarantine contract is missing: $schema54RestoreEntry"
	}
}
$schema54RestoreBatchClaimBlock = Get-ScriptMethodBlock $schema54ValidationText "static bool IsSchema54GarrisonPatrolBatchClaimant("
$schema54RestoreGroupClaimBlock = Get-ScriptMethodBlock $schema54ValidationText "static bool IsSchema54GarrisonPatrolGroupClaimant("
$schema54RestoreHoldBatchClaimBlock = Get-ScriptMethodBlock $schema54ValidationText "protected bool BatchClaimsAuthority("
$schema54RestoreHoldGroupClaimBlock = Get-ScriptMethodBlock $schema54ValidationText "protected bool GroupClaimsAuthority("
foreach ($schema54StrongClaimBlock in @(
		$schema54RestoreBatchClaimBlock,
		$schema54RestoreGroupClaimBlock,
		$schema54RestoreHoldBatchClaimBlock,
		$schema54RestoreHoldGroupClaimBlock
	)) {
	if ([string]::IsNullOrEmpty($schema54StrongClaimBlock) -or
		$schema54StrongClaimBlock -notmatch 'm_sOperationId\s*==\s*operation\.m_sOperationId|m_sOperationId\s*!=\s*operation\.m_sOperationId') {
		throw "Schema-54 restore quarantine must require operation identity plus an independent claimant backlink before mutating batch/group rows"
	}
}

foreach ($schema54MarkerEntry in @(
		"AddExactGarrisonPatrolOperationMarkers",
		"ShouldShowExactGarrisonPatrolMarker",
		"hst_exact_garrison_patrol_",
		"garrison_patrol_exact",
		"garrison patrol",
		"survivors"
	)) {
	if ($schema54MarkerText -notmatch [regex]::Escape($schema54MarkerEntry)) {
		throw "Schema-54 exact purchased-garrison marker is missing: $schema54MarkerEntry"
	}
}
foreach ($schema54UIEntry in @(
		"CountExecutableManifestInfantry",
		"exact patrol",
		"legacy fielded"
	)) {
	if ($schema54UIServiceText -notmatch [regex]::Escape($schema54UIEntry)) {
		throw "Schema-54 exact purchased-garrison Forces UI projection is missing: $schema54UIEntry"
	}
}

foreach ($schema54ArchiveEntry in @(
		"HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION",
		"HST_GarrisonPatrolOperationService.SETTLEMENT_KIND",
		"settled exact garrison patrol"
	)) {
	if ($schema54ArchiveText -notmatch [regex]::Escape($schema54ArchiveEntry)) {
		throw "Schema-54 exact purchased-garrison settlement archive contract is missing: $schema54ArchiveEntry"
	}
}

foreach ($schema54CoordinatorEntry in @(
		"m_GarrisonPatrolOperations = new HST_GarrisonPatrolOperationService()",
		"SetExactGarrisonPatrolAuthorityService",
		"m_GarrisonPatrolOperations.ReconcileAfterRestore",
		"m_GarrisonPatrolOperations.Tick",
		"PrepareOpenPhysicalAuthorityForSettlement",
		"SettleOpenOperationsForCampaignStop",
		"preserveOpenExactGarrisonPatrol",
		"ReconcileSettledRuntimeCleanup",
		"AppendCampaignDebugGarrisonPatrolOperationAssertions"
	)) {
	if ($schema54CoordinatorText -notmatch [regex]::Escape($schema54CoordinatorEntry)) {
		throw "Schema-54 exact purchased-garrison coordinator wiring is missing: $schema54CoordinatorEntry"
	}
}

foreach ($schema54ProofEntry in @(
		"class HST_GarrisonPatrolOperationProofService",
		"ProveAdmission",
		"ProveReplayCollisionAndRollback",
		"ProveRosterFoldAndReprojection",
		"ProveInfiniteRouteLoop",
		"ProveProjectionHysteresisAndCasualtyHold",
		"ProveTerminalSettlement",
		"ProveRestoreNormalization",
		"ProveCorruptGraphQuarantine",
		"ProveForeignSettlementCapacityReservation",
		"ProveTypedQuarantineTerminalization",
		"ProveMarkers"
	)) {
	if ($schema54ProofText -notmatch [regex]::Escape($schema54ProofEntry)) {
		throw "Schema-54 exact purchased-garrison source proof is missing: $schema54ProofEntry"
	}
}
foreach ($schema54AssertionId in @(
		"garrison_patrol.admission",
		"garrison_patrol.replay_rollback",
		"garrison_patrol.roster_projection",
		"garrison_patrol.route_loop",
		"garrison_patrol.projection_hold",
		"garrison_patrol.settlement",
		"garrison_patrol.restore",
		"garrison_patrol.corruption",
		"garrison_patrol.marker"
	)) {
	if ($schema54CoordinatorText -notmatch [regex]::Escape($schema54AssertionId)) {
		throw "Schema-54 exact purchased-garrison debug assertion is missing: $schema54AssertionId"
	}
}

$schema54DocumentationPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md",
	"docs/HST_ENFUSION_ENFORCE_NOTES.md",
	"docs/MIGRATIONS.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md"
)
foreach ($schema54DocumentationPath in $schema54DocumentationPaths) {
	$schema54DocumentationText = (Get-Content -Raw $schema54DocumentationPath).ToLowerInvariant()
	$mentionsSchema54 = $schema54DocumentationText.Contains("schema 54") -or $schema54DocumentationText.Contains("schema-54")
	$mentionsSchema54Policy = $schema54DocumentationText.Contains("policy-v2")
	$mentionsSchema54Garrison = $schema54DocumentationText.Contains("garrison")
	$mentionsSchema54PackagedGap = $schema54DocumentationText.Contains("packaged")
	if (!$mentionsSchema54 -or !$mentionsSchema54Policy -or !$mentionsSchema54Garrison -or !$mentionsSchema54PackagedGap) {
		throw "$schema54DocumentationPath must describe the schema-54 policy-v2 purchased-garrison boundary and packaged-runtime gap"
	}
}
$schema54MigrationsText = (Get-Content -Raw "docs/MIGRATIONS.md").ToLowerInvariant()
foreach ($schema54MigrationNote in @(
		"migration_schema54_exact_garrison_patrol",
		"garrison_exact_patrol_2",
		'version `-54`',
		"policy-v1",
		"zero refund",
		"packaged"
	)) {
	if (!$schema54MigrationsText.Contains($schema54MigrationNote)) {
		throw "Schema-54 migration documentation is missing: $schema54MigrationNote"
	}
}
Write-Host "Schema-54 exact purchased-garrison admission, infinite route, survivor projection, legacy isolation, persistence, no-refund settlement, marker/UI, migration, and proof contract OK"

# Schema 55: newly started officer-assassination guards are one exact, mission-owned,
# route-less infantry projection. Historical mission groups remain contract-zero.
$schema55RequiredPaths = @(
	"Scripts/Game/HST/Services/HST_MissionGuardOperationService.c",
	"Scripts/Game/HST/Services/HST_MissionGuardOperationProofService.c",
	"Scripts/Game/HST/Services/HST_AssassinationGuardSaveValidationService.c"
)
foreach ($schema55RequiredPath in $schema55RequiredPaths) {
	if (!(Test-Path $schema55RequiredPath)) {
		throw "Schema-55 exact officer-guard file is missing: $schema55RequiredPath"
	}
}

$schema55TypesText = Get-Content -Raw "Scripts/Game/HST/HST_Types.c"
$schema55StateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
$schema55CoordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$schema55OperationText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionGuardOperationService.c"
$schema55ProofText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionGuardOperationProofService.c"
$schema55RuntimeText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionRuntimeService.c"
$schema55PhysicalText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PhysicalWarService.c"
$schema55PersistenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PersistenceService.c"
$schema55SaveText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$schema55ValidationText = Get-Content -Raw "Scripts/Game/HST/Services/HST_AssassinationGuardSaveValidationService.c"
$schema55MarkerText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
$schema55CommandUIText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"

if ($schema55TypesText -notmatch [regex]::Escape('HST_OPERATION_TYPE_MISSION_GUARD')) {
	throw "Schema-55 typed mission-guard operation discriminator is missing"
}

foreach ($schema55OperationEntry in @(
	'class HST_MissionGuardOperationService',
	'EXACT_MISSION_ID = "assassinate_officer"',
	'EXACT_CONTRACT_VERSION = 1',
	'QUARANTINED_CONTRACT_VERSION = -55',
	'EXACT_POLICY_ID = "exact_assassinate_officer_guard_v1"',
	'EXACT_FORCE_KIND = "mission_guard"',
	'EXACT_INTENT_ID = "assassinate_officer_guard"',
	'EXACT_GROUP_MODE = "exact_mission_guard"',
	'SETTLEMENT_POLICY_ID = "mission_owned_no_refund"',
	'PrepareNewMissionContract',
	'CanAdmitNewMission',
	'AdmitNewMission',
	'RollbackAdmission',
	'TickBeforePhysical',
	'ReconcileAfterMissionOutcomes',
	'ReconcileAfterRestore',
	'PrepareOpenPhysicalAuthorityForPersistence',
	'PrepareQuarantinedAuthorityForPersistence',
	'SettleOpenOperationsForCampaignStop',
	'ReconcileSettledRuntimeCleanup',
	'BuildGuardStatusText'
)) {
	if ($schema55OperationText -notmatch [regex]::Escape($schema55OperationEntry)) {
		throw "Schema-55 exact officer-guard operation boundary is missing: $schema55OperationEntry"
	}
}
foreach ($schema55AuthorityEntry in @(
	'group.m_sMissionAssetId = ""',
	'operation.m_vTacticalTargetPosition = hvtPosition',
	'operation.m_sCurrentRouteId = ""',
	'manifest.m_iMoneyCost = 0',
	'manifest.m_iHRCost = 0',
	'manifest.m_iAttackResourceCost = 0',
	'manifest.m_iSupportResourceCost = 0',
	'manifest.m_aAssets.Count() != 0',
	'HST_OPERATION_TERMINAL_DESTROYED',
	'HST_OPERATION_TERMINAL_COMPLETED',
	'HST_OPERATION_TERMINAL_CANCELLED',
	'HST_OPERATION_TERMINAL_INVALIDATED',
	'HST_OPERATION_TERMINAL_SPAWN_FAILED',
	'CompleteQuarantinedSuccessfulProjectionCancellation',
	'exact mission guard authority quarantined without refund or legacy conversion'
)) {
	if ($schema55OperationText -notmatch [regex]::Escape($schema55AuthorityEntry)) {
		throw "Schema-55 route-less HVT/roster/settlement authority is missing: $schema55AuthorityEntry"
	}
}
if ($schema55OperationText -notmatch [regex]::Escape('return hvtPosition + "10 0 0"') -or
	$schema55OperationText -notmatch [regex]::Escape('return hvtPosition + "7 0 7"')) {
	throw "Schema-55 guard anchor must remain a deterministic non-zero offset from the mission-owned HVT"
}

foreach ($schema55CoordinatorEntry in @(
	'm_MissionGuardOperations = new HST_MissionGuardOperationService()',
	'SetMissionGuardOperationService',
	'm_MissionGuardOperations.ReconcileAfterRestore',
	'm_MissionGuardOperations.TickBeforePhysical',
	'm_MissionGuardOperations.ReconcileAfterMissionOutcomes',
	'm_MissionGuardOperations.PrepareOpenPhysicalAuthorityForSettlement',
	'm_MissionGuardOperations.SettleOpenOperationsForCampaignStop',
	'm_MissionGuardOperations.PrepareNewMissionContract',
	'm_MissionGuardOperations.AdmitNewMission',
	'AppendCampaignDebugMissionGuardOperationAssertions'
)) {
	if ($schema55CoordinatorText -notmatch [regex]::Escape($schema55CoordinatorEntry)) {
		throw "Schema-55 exact officer-guard coordinator wiring is missing: $schema55CoordinatorEntry"
	}
}
foreach ($schema55AssertionId in @(
	'mission_guard.admission_isolation',
	'mission_guard.projection_lifecycle',
	'mission_guard.settlement',
	'mission_guard.restore_migration',
	'mission_guard.corruption_quarantine',
	'mission_guard.marker_status'
)) {
	if ($schema55CoordinatorText -notmatch [regex]::Escape($schema55AssertionId)) {
		throw "Schema-55 exact officer-guard debug assertion is missing: $schema55AssertionId"
	}
}

if ($schema55RuntimeText -notmatch [regex]::Escape('HST_MissionGuardOperationService.IsExactOrQuarantinedMission(mission)')) {
	throw "Schema-55 exact/quarantined officer guards must bypass legacy mission hostile-group creation"
}
foreach ($schema55PhysicalEntry in @(
	'IsExactOrQuarantinedMissionGuardGroup',
	'IsExactMissionGuardGroup',
	'HasExactMissionGuardRuntime',
	'TryResolveExactMissionGuardLivePosition',
	'HasExactMissionGuardLiveContactEvidence',
	'RestartExactMissionGuardInfantryAssignment',
	'HST_OPERATION_MATERIALIZATION_DEMATERIALIZING'
)) {
	if ($schema55PhysicalText -notmatch [regex]::Escape($schema55PhysicalEntry)) {
		throw "Schema-55 exact officer-guard PhysicalWar isolation is missing: $schema55PhysicalEntry"
	}
}

foreach ($schema55PersistenceEntry in @(
	'SetMissionGuardOperationService',
	'HasQuarantinedMissionGuardAuthority',
	'ValidateQuarantinedMissionGuardCleanup',
	'PrepareQuarantinedAuthorityForPersistence',
	'PrepareOpenPhysicalAuthorityForPersistence',
	'ValidatePhysicalMissionGuardSnapshots',
	'TryResolveExactMissionGuardLivePosition',
	'hasMaterializingExactInfantry'
)) {
	if ($schema55PersistenceText -notmatch [regex]::Escape($schema55PersistenceEntry)) {
		throw "Schema-55 exact officer-guard checkpoint boundary is missing: $schema55PersistenceEntry"
	}
}
foreach ($schema55StateEntry in @(
	'HST_MissionGuardOperationService.IsMissionGuardGroupClaimant',
	'HST_MissionGuardOperationService.IsExactMissionGuardGroup',
	'HST_MissionGuardOperationService.QUARANTINE_STATUS'
)) {
	if ($schema55StateText -notmatch [regex]::Escape($schema55StateEntry)) {
		throw "Schema-55 campaign operational/quarantine classification is missing: $schema55StateEntry"
	}
}

foreach ($schema55SaveEntry in @(
	'class HST_AssassinationGuardSaveValidationService',
	'IsSchema55MissionGuardMissionClaimant',
	'IsSchema55MissionGuardOperationClaimant',
	'IsSchema55MissionGuardManifestClaimant',
	'IsSchema55MissionGuardBatchClaimant',
	'IsSchema55MissionGuardGroupClaimant',
	'migration_schema55_exact_mission_guard',
	'normalization_schema55_exact_mission_guard_conflict',
	'PreserveHistoricalMissionFamily',
	'inferred no manifest, roster, batch, casualty, projection, or settlement authority',
	'ValidateHVTBoundary',
	'ValidateCurrentCatalogRoster',
	'BuildGroupCatalog',
	'HST_OPERATION_TERMINAL_DESTROYED'
)) {
	if (($schema55ValidationText + $schema55SaveText) -notmatch [regex]::Escape($schema55SaveEntry)) {
		throw "Schema-55 exact officer-guard migration/restore boundary is missing: $schema55SaveEntry"
	}
}
if ($schema55SaveText -notmatch 'schema5[567]MissionGuardValidation\.Normalize\(this, restoredSchemaVersion\)') {
	throw "Schema-55 exact officer-guard validator is not called from campaign save normalization"
}

foreach ($schema55UIEntry in @('BuildGuardStatusText', 'guards neutralized', 'guard authority unavailable')) {
	if (($schema55OperationText + $schema55MarkerText + $schema55CommandUIText) -notmatch [regex]::Escape($schema55UIEntry)) {
		throw "Schema-55 existing HVT marker/UI status boundary is missing: $schema55UIEntry"
	}
}
foreach ($schema55ProofEntry in @(
	'class HST_MissionGuardOperationProofService',
	'ProveAdmissionIsolation',
	'ProveProjectionLifecycle',
	'ProveSettlement',
	'ProveRestoreMigration',
	'ProveCorruptionQuarantine',
	'ProveMarkerStatus',
	'ProveCompactSettledRestore',
	'packaged gates not claimed'
)) {
	if ($schema55ProofText -notmatch [regex]::Escape($schema55ProofEntry)) {
		throw "Schema-55 exact officer-guard source proof is missing: $schema55ProofEntry"
	}
}

$schema55DocumentationPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md",
	"docs/HST_ENFUSION_ENFORCE_NOTES.md",
	"docs/MIGRATIONS.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md"
)
foreach ($schema55DocumentationPath in $schema55DocumentationPaths) {
	$schema55DocumentationText = (Get-Content -Raw $schema55DocumentationPath).ToLowerInvariant()
	if (!$schema55DocumentationText.Contains("schema 55") -and !$schema55DocumentationText.Contains("schema-55")) {
		throw "$schema55DocumentationPath must identify the Schema-55 officer-guard boundary"
	}
	foreach ($schema55DocumentationTerm in @("officer", "guard", "packaged")) {
		if (!$schema55DocumentationText.Contains($schema55DocumentationTerm)) {
			throw "$schema55DocumentationPath must describe the Schema-55 officer-guard packaged-runtime boundary: $schema55DocumentationTerm"
		}
	}
}
$schema55MigrationsText = (Get-Content -Raw "docs/MIGRATIONS.md").ToLowerInvariant()
foreach ($schema55MigrationNote in @(
	'migration_schema55_exact_mission_guard',
	'normalization_schema55_exact_mission_guard_conflict',
	'exact_assassinate_officer_guard_v1',
	'zero refund',
	'packaged'
)) {
	if (!$schema55MigrationsText.Contains($schema55MigrationNote)) {
		throw "Schema-55 migration documentation is missing: $schema55MigrationNote"
	}
}
if (!$schema55MigrationsText.Contains('contract zero') -and
	!$schema55MigrationsText.Contains('contract-zero')) {
	throw "Schema-55 migration documentation must preserve historical mission guards on contract zero"
}
Write-Host "Schema-55 exact officer-guard admission, HVT isolation, survivor projection, zero-refund settlement, restore/quarantine, marker/UI, migration, and proof contract OK"

# Schema 56: newly started traitor-assassination guards opt into contract 2
# without changing Schema-55 officer authority or historical mission rows.
$schema56ProofPath = "Scripts/Game/HST/Services/HST_TraitorGuardOperationProofService.c"
if (!(Test-Path $schema56ProofPath)) {
	throw "Schema-56 traitor-guard proof file is missing"
}
$schema56StateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
$schema56SaveText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$schema56OperationText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionGuardOperationService.c"
$schema56ValidationText = Get-Content -Raw "Scripts/Game/HST/Services/HST_AssassinationGuardSaveValidationService.c"
$schema56PersistenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PersistenceService.c"
$schema56CoordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$schema56ProofText = Get-Content -Raw $schema56ProofPath

if ($schema56StateText -notmatch 'SCHEMA_VERSION\s*=\s*(56|57|58)\s*;') {
	throw "Schema-56 campaign schema lineage is missing"
}
foreach ($schema56CoreEntry in @(
	'OFFICER_CONTRACT_VERSION = 1',
	'OFFICER_QUARANTINED_CONTRACT_VERSION = -55',
	'OFFICER_MISSION_ID = "assassinate_officer"',
	'OFFICER_POLICY_ID = "exact_assassinate_officer_guard_v1"',
	'TRAITOR_CONTRACT_VERSION = 2',
	'TRAITOR_QUARANTINED_CONTRACT_VERSION = -56',
	'TRAITOR_MISSION_ID = "assassinate_traitor"',
	'TRAITOR_POLICY_ID = "exact_assassinate_traitor_guard_v1"',
	'TRAITOR_INTENT_ID = "assassinate_traitor_guard"',
	'IsSupportedExactMissionId',
	'IsSupportedExactContractVersion',
	'IsQuarantinedOperationContractVersion',
	'ResolveExpectedContractVersion',
	'ResolveExpectedPolicyId',
	'ResolveExpectedIntentId',
	'ResolveQuarantinedContractVersion',
	'IsExactOfficerMission',
	'IsExactTraitorMission',
	'IsQuarantinedTraitorMission',
	'ResolveQuarantineVersionForOperation',
	'ValidateExactCatalogRoster'
)) {
	if ($schema56OperationText -notmatch [regex]::Escape($schema56CoreEntry)) {
		throw "Schema-56 traitor-guard core contract is missing: $schema56CoreEntry"
	}
}
$schema56SupportedIdBlock = Get-ScriptMethodBlock $schema56OperationText "static bool IsSupportedExactMissionId("
if ([string]::IsNullOrEmpty($schema56SupportedIdBlock) -or
	$schema56SupportedIdBlock -notmatch [regex]::Escape('OFFICER_MISSION_ID') -or
	$schema56SupportedIdBlock -notmatch [regex]::Escape('TRAITOR_MISSION_ID')) {
	throw "Schema-56 lineage must retain officer and traitor guard mission IDs"
}
$schema56PlanBlock = Get-ScriptMethodBlock $schema56OperationText "protected string BuildAdmissionPlan("
$schema56ManifestBlock = Get-ScriptMethodBlock $schema56OperationText "protected HST_ForceManifestState BuildManifest("
$schema56OperationBuildBlock = Get-ScriptMethodBlock $schema56OperationText "protected HST_OperationRecordState BuildOperation("
foreach ($schema56DynamicPair in @(
	'ResolveExpectedContractVersion(mission.m_sMissionId)',
	'ResolveExpectedPolicyId(mission.m_sMissionId)',
	'ResolveExpectedIntentId(mission.m_sMissionId)',
	'definition.m_sMissionId != mission.m_sMissionId'
)) {
	if (($schema56PlanBlock + $schema56ManifestBlock + $schema56OperationBuildBlock) -notmatch [regex]::Escape($schema56DynamicPair)) {
		throw "Schema-56 traitor admission is not mission-policy dynamic: $schema56DynamicPair"
	}
}
foreach ($schema56UnchangedAuthorityEntry in @(
	'group.m_sMissionAssetId = ""',
	'operation.m_vTacticalTargetPosition = hvtPosition',
	'operation.m_sCurrentRouteId = ""',
	'manifest.m_iMoneyCost = 0',
	'manifest.m_iHRCost = 0',
	'manifest.m_iAttackResourceCost = 0',
	'manifest.m_iSupportResourceCost = 0',
	'SETTLEMENT_POLICY_ID = "mission_owned_no_refund"'
)) {
	if ($schema56OperationText -notmatch [regex]::Escape($schema56UnchangedAuthorityEntry)) {
		throw "Schema-56 traitor guard changed the route-less HVT/zero-refund boundary: $schema56UnchangedAuthorityEntry"
	}
}

foreach ($schema56CoordinatorEntry in @(
	'IsSupportedExactMissionId(mission.m_sMissionId)',
	'ResolveExpectedContractVersion(mission.m_sMissionId)',
	'ResolveExpectedPolicyId(mission.m_sMissionId)',
	'AppendCampaignDebugTraitorGuardOperationAssertions',
	'HST_TraitorGuardOperationProofService',
	'RunTraitor()'
)) {
	if ($schema56CoordinatorText -notmatch [regex]::Escape($schema56CoordinatorEntry)) {
		throw "Schema-56 traitor-guard coordinator wiring is missing: $schema56CoordinatorEntry"
	}
}
foreach ($schema56AssertionId in @(
	'traitor_guard.admission_isolation',
	'traitor_guard.projection_lifecycle',
	'traitor_guard.settlement',
	'traitor_guard.restore_migration',
	'traitor_guard.corruption_quarantine',
	'traitor_guard.marker_status'
)) {
	if ($schema56CoordinatorText -notmatch [regex]::Escape($schema56AssertionId)) {
		throw "Schema-56 traitor-guard debug assertion is missing: $schema56AssertionId"
	}
}

foreach ($schema56PersistenceEntry in @(
	'IsSupportedExactContractVersion',
	'IsQuarantinedOperationContractVersion',
	'ValidatePhysicalMissionGuardSnapshots',
	'PrepareQuarantinedAuthorityForPersistence'
)) {
	if ($schema56PersistenceText -notmatch [regex]::Escape($schema56PersistenceEntry)) {
		throw "Schema-56 traitor-guard persistence boundary is missing: $schema56PersistenceEntry"
	}
}
foreach ($schema56SaveEntry in @(
	'IsSchema56MissionGuardMissionClaimant',
	'IsSchema56MissionGuardOperationClaimant',
	'IsSchema56MissionGuardManifestClaimant',
	'IsSchema56MissionGuardBatchClaimant',
	'IsSchema56MissionGuardGroupClaimant',
	'PreserveHistoricalMissionFamily',
	'migration_schema56_exact_traitor_guard',
	'normalization_schema56_exact_traitor_guard_conflict',
	'TRAITOR_QUARANTINED_CONTRACT_VERSION',
	'ValidateCurrentCatalogRoster'
)) {
	if (($schema56ValidationText + $schema56SaveText) -notmatch [regex]::Escape($schema56SaveEntry)) {
		throw "Schema-56 traitor-guard migration/restore boundary is missing: $schema56SaveEntry"
	}
}
$schema56GroupClaimBlock = Get-ScriptMethodBlock $schema56ValidationText "protected static bool IsMissionGuardGroupClaimantForFamily("
if ([string]::IsNullOrEmpty($schema56GroupClaimBlock) -or
	$schema56GroupClaimBlock -match [regex]::Escape('"mission_group_" + mission.m_sInstanceId')) {
	throw "Schema-56 exact claimant classification must not adopt ordinary legacy mission_group rows"
}
foreach ($schema56GuardedSkipSuffix in @(
	'MissionClaimant',
	'ManifestClaimant',
	'BatchClaimant',
	'GroupClaimant'
)) {
	$schema56GuardedSkipPattern = 'restoredSchemaVersion\s*>=\s*55\s*&&\s*HST_AssassinationGuardSaveValidationService\.IsSchema5[67]MissionGuard' + $schema56GuardedSkipSuffix
	if ($schema56SaveText -notmatch $schema56GuardedSkipPattern) {
		throw "Schema-56 generic-normalizer skip lacks the pre-55 legacy boundary: $schema56GuardedSkipSuffix"
	}
}

foreach ($schema56ProofEntry in @(
	'class HST_TraitorGuardOperationProofReport',
	'class HST_TraitorGuardOperationProofService',
	'RunTraitor',
	'ProveAdmissionIsolation',
	'ProveProjectionLifecycle',
	'ProveSettlement',
	'ProveRestoreMigration',
	'ProveCorruptionQuarantine',
	'ProveMarkerStatus',
	'IsExactOfficerMission',
	'IsExactTraitorMission',
	'migration_schema56_exact_traitor_guard',
	'normalization_schema56_exact_traitor_guard_conflict',
	'unclaimed engine/package gates'
)) {
	if ($schema56ProofText -notmatch [regex]::Escape($schema56ProofEntry)) {
		throw "Schema-56 traitor-guard source proof is missing: $schema56ProofEntry"
	}
}

$schema56DocumentationPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md",
	"docs/HST_ENFUSION_ENFORCE_NOTES.md",
	"docs/MIGRATIONS.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md"
)
foreach ($schema56DocumentationPath in $schema56DocumentationPaths) {
	$schema56DocumentationText = (Get-Content -Raw $schema56DocumentationPath).ToLowerInvariant()
	$mentionsSchema56 = $schema56DocumentationText.Contains("schema 56") -or $schema56DocumentationText.Contains("schema-56")
	if (!$mentionsSchema56 -or !$schema56DocumentationText.Contains("traitor") -or
		!$schema56DocumentationText.Contains("guard") -or
		!$schema56DocumentationText.Contains("packaged")) {
		throw "$schema56DocumentationPath must describe the Schema-56 traitor-guard and packaged-runtime boundary"
	}
}
$schema56MigrationsText = (Get-Content -Raw "docs/MIGRATIONS.md").ToLowerInvariant()
foreach ($schema56MigrationNote in @(
	'migration_schema56_exact_traitor_guard',
	'normalization_schema56_exact_traitor_guard_conflict',
	'exact_assassinate_traitor_guard_v1',
	'contract `2`',
	'version `-56`',
	'contract zero',
	'zero refund',
	'packaged'
)) {
	if (!$schema56MigrationsText.Contains($schema56MigrationNote)) {
		throw "Schema-56 migration documentation is missing: $schema56MigrationNote"
	}
}
Write-Host "Schema-56 traitor-guard contract-2 admission, officer coexistence, HVT isolation, survivor projection, zero-refund settlement, migration/quarantine, marker/UI, and proof contract OK"

# Schema 57: newly started spec-ops-assassination guards opt into contract 3
# without changing Schema-55 officer, Schema-56 traitor, or historical rows.
$schema57ProofPath = "Scripts/Game/HST/Services/HST_SpecOpsGuardOperationProofService.c"
if (!(Test-Path $schema57ProofPath)) {
	throw "Schema-57 spec-ops-guard proof file is missing"
}
$schema57StateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
$schema57SaveText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$schema57OperationText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionGuardOperationService.c"
$schema57ValidationText = Get-Content -Raw "Scripts/Game/HST/Services/HST_AssassinationGuardSaveValidationService.c"
$schema57PersistenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PersistenceService.c"
$schema57CoordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$schema57ProofText = Get-Content -Raw $schema57ProofPath

if ($schema57StateText -notmatch 'SCHEMA_VERSION\s*=\s*(57|58)\s*;') {
	throw "Schema-57 campaign schema is missing"
}
foreach ($schema57CoreEntry in @(
	'OFFICER_CONTRACT_VERSION = 1',
	'OFFICER_QUARANTINED_CONTRACT_VERSION = -55',
	'TRAITOR_CONTRACT_VERSION = 2',
	'TRAITOR_QUARANTINED_CONTRACT_VERSION = -56',
	'SPECOPS_CONTRACT_VERSION = 3',
	'SPECOPS_QUARANTINED_CONTRACT_VERSION = -57',
	'SPECOPS_MISSION_ID = "assassinate_specops"',
	'SPECOPS_POLICY_ID = "exact_assassinate_specops_guard_v1"',
	'SPECOPS_INTENT_ID = "assassinate_specops_guard"',
	'IsSupportedExactMissionId',
	'IsSupportedExactContractVersion',
	'IsQuarantinedOperationContractVersion',
	'ResolveExpectedContractVersion',
	'ResolveExpectedPolicyId',
	'ResolveExpectedIntentId',
	'ResolveQuarantinedContractVersion',
	'ResolveQuarantinedContractVersionForExactContract',
	'IsExactOfficerMission',
	'IsExactTraitorMission',
	'IsExactSpecOpsMission',
	'IsQuarantinedSpecOpsMission',
	'ResolveQuarantineVersionForOperation',
	'SelectExactExecutionGroupPlan',
	'ValidateExactCatalogRoster'
)) {
	if ($schema57OperationText -notmatch [regex]::Escape($schema57CoreEntry)) {
		throw "Schema-57 spec-ops-guard core contract is missing: $schema57CoreEntry"
	}
}
$schema57SupportedIdBlock = Get-ScriptMethodBlock $schema57OperationText "static bool IsSupportedExactMissionId("
if ([string]::IsNullOrEmpty($schema57SupportedIdBlock) -or
	$schema57SupportedIdBlock -notmatch [regex]::Escape('OFFICER_MISSION_ID') -or
	$schema57SupportedIdBlock -notmatch [regex]::Escape('TRAITOR_MISSION_ID') -or
	$schema57SupportedIdBlock -notmatch [regex]::Escape('SPECOPS_MISSION_ID')) {
	throw "Schema-57 opt-in must retain officer/traitor and add only the named spec-ops guard family"
}
foreach ($schema57ResolverContract in @(
	@('static int ResolveExpectedContractVersion(', 'SPECOPS_CONTRACT_VERSION'),
	@('static string ResolveExpectedPolicyId(', 'SPECOPS_POLICY_ID'),
	@('static string ResolveExpectedIntentId(', 'SPECOPS_INTENT_ID'),
	@('static int ResolveQuarantinedContractVersion(', 'SPECOPS_QUARANTINED_CONTRACT_VERSION'),
	@('static int ResolveQuarantinedContractVersionForExactContract(', 'SPECOPS_QUARANTINED_CONTRACT_VERSION')
)) {
	$schema57ResolverBlock = Get-ScriptMethodBlock $schema57OperationText $schema57ResolverContract[0]
	if ([string]::IsNullOrEmpty($schema57ResolverBlock) -or
		$schema57ResolverBlock -notmatch [regex]::Escape('SPECOPS') -or
		$schema57ResolverBlock -notmatch [regex]::Escape($schema57ResolverContract[1])) {
		throw "Schema-57 mission-family resolver is missing spec-ops pairing: $($schema57ResolverContract[0])"
	}
}
$schema57PlanBlock = Get-ScriptMethodBlock $schema57OperationText "protected string BuildAdmissionPlan("
$schema57ManifestBlock = Get-ScriptMethodBlock $schema57OperationText "protected HST_ForceManifestState BuildManifest("
$schema57OperationBuildBlock = Get-ScriptMethodBlock $schema57OperationText "protected HST_OperationRecordState BuildOperation("
foreach ($schema57DynamicPair in @(
	'ResolveExpectedContractVersion(mission.m_sMissionId)',
	'ResolveExpectedPolicyId(mission.m_sMissionId)',
	'ResolveExpectedIntentId(mission.m_sMissionId)',
	'definition.m_sMissionId != mission.m_sMissionId'
)) {
	if (($schema57PlanBlock + $schema57ManifestBlock + $schema57OperationBuildBlock) -notmatch [regex]::Escape($schema57DynamicPair)) {
		throw "Schema-57 spec-ops admission is not mission-policy dynamic: $schema57DynamicPair"
	}
}
$schema57ExactGroupSelectionBlock = Get-ScriptMethodBlock $schema57OperationText "protected HST_GroupSpawnPlan SelectExactExecutionGroupPlan("
if ([string]::IsNullOrEmpty($schema57ExactGroupSelectionBlock) -or
	$schema57ExactGroupSelectionBlock -notmatch [regex]::Escape('candidate.m_iManpower > selected.m_iManpower') -or
	$schema57ExactGroupSelectionBlock -notmatch [regex]::Escape('candidate.m_bSkipped') -or
	$schema57PlanBlock -notmatch [regex]::Escape('SelectExactExecutionGroupPlan(composition)') -or
	$schema57PlanBlock -match [regex]::Escape('executableGroupCount != 1')) {
	throw "Schema-57 exact single-root planning must deterministically select one strongest executable group from a multi-group composition"
}
foreach ($schema57UnchangedAuthorityEntry in @(
	'group.m_sMissionAssetId = ""',
	'operation.m_vTacticalTargetPosition = hvtPosition',
	'operation.m_sCurrentRouteId = ""',
	'manifest.m_iMoneyCost = 0',
	'manifest.m_iHRCost = 0',
	'manifest.m_iAttackResourceCost = 0',
	'manifest.m_iSupportResourceCost = 0',
	'SETTLEMENT_POLICY_ID = "mission_owned_no_refund"'
)) {
	if ($schema57OperationText -notmatch [regex]::Escape($schema57UnchangedAuthorityEntry)) {
		throw "Schema-57 spec-ops guard changed the route-less HVT/zero-refund boundary: $schema57UnchangedAuthorityEntry"
	}
}

foreach ($schema57CoordinatorEntry in @(
	'IsSupportedExactMissionId(mission.m_sMissionId)',
	'ResolveExpectedContractVersion(mission.m_sMissionId)',
	'ResolveExpectedPolicyId(mission.m_sMissionId)',
	'AppendCampaignDebugSpecOpsGuardOperationAssertions',
	'HST_SpecOpsGuardOperationProofService',
	'RunSpecOps()'
)) {
	if ($schema57CoordinatorText -notmatch [regex]::Escape($schema57CoordinatorEntry)) {
		throw "Schema-57 spec-ops-guard coordinator wiring is missing: $schema57CoordinatorEntry"
	}
}
foreach ($schema57AssertionId in @(
	'specops_guard.admission_isolation',
	'specops_guard.projection_lifecycle',
	'specops_guard.settlement',
	'specops_guard.restore_migration',
	'specops_guard.corruption_quarantine',
	'specops_guard.marker_status'
)) {
	if ($schema57CoordinatorText -notmatch [regex]::Escape($schema57AssertionId)) {
		throw "Schema-57 spec-ops-guard debug assertion is missing: $schema57AssertionId"
	}
}

foreach ($schema57PersistenceEntry in @(
	'IsSupportedExactContractVersion',
	'IsQuarantinedOperationContractVersion',
	'ValidatePhysicalMissionGuardSnapshots',
	'PrepareQuarantinedAuthorityForPersistence'
)) {
	if ($schema57PersistenceText -notmatch [regex]::Escape($schema57PersistenceEntry)) {
		throw "Schema-57 spec-ops-guard persistence boundary is missing: $schema57PersistenceEntry"
	}
}
foreach ($schema57SaveEntry in @(
	'IsSchema57MissionGuardMissionClaimant',
	'IsSchema57MissionGuardOperationClaimant',
	'IsSchema57MissionGuardManifestClaimant',
	'IsSchema57MissionGuardBatchClaimant',
	'IsSchema57MissionGuardGroupClaimant',
	'PreserveHistoricalMissionFamily',
	'migration_schema57_exact_specops_guard',
	'normalization_schema57_exact_specops_guard_conflict',
	'SPECOPS_QUARANTINED_CONTRACT_VERSION',
	'ValidateCurrentCatalogRoster'
)) {
	if (($schema57ValidationText + $schema57SaveText) -notmatch [regex]::Escape($schema57SaveEntry)) {
		throw "Schema-57 spec-ops-guard migration/restore boundary is missing: $schema57SaveEntry"
	}
}
$schema57GroupClaimBlock = Get-ScriptMethodBlock $schema57ValidationText "protected static bool IsMissionGuardGroupClaimantForFamily("
if ([string]::IsNullOrEmpty($schema57GroupClaimBlock) -or
	$schema57GroupClaimBlock -match [regex]::Escape('"mission_group_" + mission.m_sInstanceId')) {
	throw "Schema-57 exact claimant classification must not adopt ordinary legacy mission_group rows"
}
foreach ($schema57GuardedSkipSuffix in @(
	'MissionClaimant',
	'ManifestClaimant',
	'BatchClaimant',
	'GroupClaimant'
)) {
	$schema57GuardedSkipPattern = 'restoredSchemaVersion\s*>=\s*55\s*&&\s*HST_AssassinationGuardSaveValidationService\.IsSchema57MissionGuard' + $schema57GuardedSkipSuffix
	if ($schema57SaveText -notmatch $schema57GuardedSkipPattern) {
		throw "Schema-57 generic-normalizer skip lacks the pre-55 legacy boundary: $schema57GuardedSkipSuffix"
	}
}

foreach ($schema57ProofEntry in @(
	'class HST_SpecOpsGuardOperationProofReport',
	'class HST_SpecOpsGuardOperationProofService',
	'RunSpecOps',
	'ProveAdmissionIsolation',
	'ProveProjectionLifecycle',
	'ProveSettlement',
	'ProveRestoreMigration',
	'ProveCorruptionQuarantine',
	'ProveMarkerStatus',
	'IsExactOfficerMission',
	'IsExactTraitorMission',
	'IsExactSpecOpsMission',
	'migration_schema57_exact_specops_guard',
	'normalization_schema57_exact_specops_guard_conflict',
	'unclaimed engine/package gates'
)) {
	if ($schema57ProofText -notmatch [regex]::Escape($schema57ProofEntry)) {
		throw "Schema-57 spec-ops-guard source proof is missing: $schema57ProofEntry"
	}
}

$schema57DocumentationPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md",
	"docs/HST_ENFUSION_ENFORCE_NOTES.md",
	"docs/MIGRATIONS.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md"
)
foreach ($schema57DocumentationPath in $schema57DocumentationPaths) {
	$schema57DocumentationText = (Get-Content -Raw $schema57DocumentationPath).ToLowerInvariant()
	$mentionsSchema57 = $schema57DocumentationText.Contains("schema 57") -or $schema57DocumentationText.Contains("schema-57")
	if (!$mentionsSchema57 -or !$schema57DocumentationText.Contains("spec-ops") -or
		!$schema57DocumentationText.Contains("guard") -or
		!$schema57DocumentationText.Contains("packaged")) {
		throw "$schema57DocumentationPath must describe the Schema-57 spec-ops guard and packaged-runtime boundary"
	}
}
$schema57MigrationsText = (Get-Content -Raw "docs/MIGRATIONS.md").ToLowerInvariant()
foreach ($schema57MigrationNote in @(
	'migration_schema57_exact_specops_guard',
	'normalization_schema57_exact_specops_guard_conflict',
	'exact_assassinate_specops_guard_v1',
	'contract `3`',
	'version `-57`',
	'contract zero',
	'zero refund',
	'packaged'
)) {
	if (!$schema57MigrationsText.Contains($schema57MigrationNote)) {
		throw "Schema-57 migration documentation is missing: $schema57MigrationNote"
	}
}
Write-Host "Schema-57 spec-ops-guard contract-3 admission, officer/traitor coexistence, HVT isolation, survivor projection, zero-refund settlement, migration/quarantine, marker/UI, and proof contract OK"

# Schema 58: newly started rescue_pows missions opt into a separate exact
# composite guard/captive authority without adopting historical POW/refugee rows.
$schema58Paths = @(
	"Scripts/Game/HST/Services/HST_RescuePOWOperationService.c",
	"Scripts/Game/HST/Services/HST_RescuePOWSaveValidationService.c",
	"Scripts/Game/HST/Services/HST_RescuePOWOperationProofService.c",
	"Scripts/Game/HST/Components/HST_RescueCaptiveLifecycleComponent.c",
	"Scripts/Game/HST/Services/HST_RescuePOWExternalAssetPolicy.c"
)
foreach ($schema58Path in $schema58Paths) {
	if (!(Test-Path $schema58Path)) {
		throw "Schema-58 exact POW-rescue source is missing: $schema58Path"
	}
}
$schema58StateText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignState.c"
$schema58TypesText = Get-Content -Raw "Scripts/Game/HST/HST_Types.c"
$schema58SaveText = Get-Content -Raw "Scripts/Game/HST/State/HST_CampaignSaveData.c"
$schema58ForceAuthorityText = Get-Content -Raw "Scripts/Game/HST/Data/HST_ForceAuthority.c"
$schema58OperationText = Get-Content -Raw $schema58Paths[0]
$schema58ValidationText = Get-Content -Raw $schema58Paths[1]
$schema58ProofText = Get-Content -Raw $schema58Paths[2]
$schema58LifecycleText = Get-Content -Raw $schema58Paths[3]
$schema58ExternalAssetPolicyText = Get-Content -Raw $schema58Paths[4]
$schema58QueueText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSpawnQueueService.c"
$schema58AdapterText = Get-Content -Raw "Scripts/Game/HST/Services/HST_ForceSpawnAdapterService.c"
$schema58RuntimeText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MissionRuntimeService.c"
$schema58PersistenceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_PersistenceService.c"
$schema58CoordinatorText = Get-Content -Raw "Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c"
$schema58MarkerText = Get-Content -Raw "Scripts/Game/HST/Services/HST_MapMarkerService.c"
$schema58UIServiceText = Get-Content -Raw "Scripts/Game/HST/Services/HST_CommandUIService.c"
$schema58MissionAssetComponentText = Get-Content -Raw "Scripts/Game/HST/Components/HST_MissionAssetComponent.c"
$schema58CaptiveActionText = Get-Content -Raw "Scripts/Game/HST/Components/HST_MissionCaptiveUserActions.c"
$schema58MissionActionFilterText = Get-Content -Raw "Scripts/Game/HST/Components/HST_MissionCargoUserActions.c"

foreach ($schema58StateEntry in @(
	'SCHEMA_VERSION = 58',
	'HST_OPERATION_TYPE_MISSION_RESCUE',
	'enum HST_ERescueCaptiveDisposition',
	'int m_iRescueGraceUntilSecond;',
	'bool m_bRescueExtractionGrace;',
	'int m_iRescueContractVersion;',
	'int m_iRescueOrdinal = -1;',
	'HST_ERescueCaptiveDisposition m_eRescueDisposition',
	'string m_sRescueEscortIdentityId;',
	'string m_sRescueCarrierVehicleId;',
	'string m_sRescueCarrierSeatToken;',
	'string m_sRescueLastCommandRequestId;',
	'string m_sRescueCasualtyReceiptId;',
	'string m_sRescueExtractionReceiptId;',
	'string m_sRescueProjectionId;',
	'bool m_bExternalAssetAuthority;'
)) {
	if (($schema58StateText + "`n" + $schema58TypesText + "`n" + $schema58ForceAuthorityText) -notmatch [regex]::Escape($schema58StateEntry)) {
		throw "Schema-58 rescue durable state is missing: $schema58StateEntry"
	}
}

foreach ($schema58CoreEntry in @(
	'EXACT_CONTRACT_VERSION = 1',
	'QUARANTINED_CONTRACT_VERSION = -58',
	'EXACT_CAPTIVE_COUNT = 3',
	'EXTRACTION_GRACE_SECONDS = 300',
	'EXACT_MISSION_ID = "rescue_pows"',
	'EXACT_FORCE_KIND = "mission_rescue"',
	'EXACT_POLICY_ID = "exact_rescue_pows_v1"',
	'EXACT_INTENT_ID = "rescue_pows_guard"',
	'PrepareNewMissionContract',
	'CanAdmitNewMission',
	'AdmitNewMission',
	'HandleCaptiveCommand',
	'MarkCaptiveDeathObserved',
	'CanEnterExtractionGrace',
	'TickBeforeMissionRuntime',
	'TickAfterMissionRuntime',
	'FindCompletedActiveMissionId',
	'FindFailedActiveMissionId',
	'ReconcileAfterMissionOutcomes',
	'PrepareOpenPhysicalAuthorityForPersistence',
	'PrepareQuarantinedAuthorityForPersistence'
)) {
	if ($schema58OperationText -notmatch [regex]::Escape($schema58CoreEntry)) {
		throw "Schema-58 rescue operation contract is missing: $schema58CoreEntry"
	}
}

foreach ($schema58ExecutionEntry in @(
	'm_bExternalAssetAuthority',
	'CountExecutableManifestSlots',
	'IsExternallyManagedAssetDescriptor',
	'external asset authority is reserved for the exact rescue contract',
	'HST_RescuePOWExternalAssetPolicy.ValidateManifestShape(manifest)',
	'HST_RescuePOWExternalAssetPolicy.ValidateAdapterAuthorityGraph(',
	'if (!durableBatch || !durableBatch.m_bExternalAssetAuthority)',
	'asset slots present without exact rescue authority'
)) {
	if (($schema58QueueText + "`n" + $schema58AdapterText + "`n" + $schema58ExternalAssetPolicyText) -notmatch [regex]::Escape($schema58ExecutionEntry)) {
		throw "Schema-58 guard/external-captive execution boundary is missing: $schema58ExecutionEntry"
	}
}
$schema58QueueExternalAssetShapeCallCount = ([regex]::Matches(
	$schema58QueueText,
	[regex]::Escape('HST_RescuePOWExternalAssetPolicy.ValidateManifestShape(manifest)'))).Count
if ($schema58QueueExternalAssetShapeCallCount -lt 3) {
	throw "Schema-58 queue admission, replay, and durable-batch validation must all enforce the shared external-asset manifest shape"
}
foreach ($schema58QueueShapeMethodSignature in @(
	'protected string ValidateEnqueueRequest(',
	'protected HST_ForceSpawnQueueEnqueueResult ResolveExistingEnqueue(',
	'protected string ValidateBatchManifest('
)) {
	$schema58QueueShapeMethodBlock = Get-ScriptMethodBlock $schema58QueueText $schema58QueueShapeMethodSignature
	if ([string]::IsNullOrEmpty($schema58QueueShapeMethodBlock) -or
		$schema58QueueShapeMethodBlock.IndexOf('HST_RescuePOWExternalAssetPolicy.ValidateManifestShape(manifest)') -lt 0) {
		throw "Schema-58 queue path does not enforce the shared external-asset manifest shape: $schema58QueueShapeMethodSignature"
	}
}
$schema58AdapterSupportMethodBlock = Get-ScriptMethodBlock $schema58AdapterText 'protected string ValidateSupportedManifest('
if ([string]::IsNullOrEmpty($schema58AdapterSupportMethodBlock) -or
	$schema58AdapterSupportMethodBlock.IndexOf('HST_RescuePOWExternalAssetPolicy.ValidateAdapterAuthorityGraph(') -lt 0) {
	throw "Schema-58 adapter validation does not enforce the state-aware external-asset authority graph"
}
$schema58AdapterRootMethodBlock = Get-ScriptMethodBlock $schema58AdapterText 'protected void SpawnGroupRoot('
if ([string]::IsNullOrEmpty($schema58AdapterRootMethodBlock) -or
	$schema58AdapterRootMethodBlock.IndexOf('if (!durableBatch || !durableBatch.m_bExternalAssetAuthority)') -lt 0) {
	throw "Schema-58 adapter root registration must preserve the exact rescue durable mode discriminator"
}
foreach ($schema58ExternalAssetShapeEntry in @(
	'class HST_RescuePOWExternalAssetPolicy',
	'manifest.m_bFrozen',
	'integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash',
	'manifest.m_sForceKind != HST_RescuePOWOperationService.EXACT_FORCE_KIND',
	'manifest.m_sPolicyId != HST_RescuePOWOperationService.EXACT_POLICY_ID',
	'manifest.m_sIntentId != HST_RescuePOWOperationService.EXACT_INTENT_ID',
	'manifest.m_aGroups.Count() != 1',
	'manifest.m_aVehicles.Count() != 0',
	'manifest.m_aAssets.Count() != HST_RescuePOWOperationService.EXACT_CAPTIVE_COUNT',
	'HST_RescuePOWOperationService.BuildCaptiveSlotId(missionInstanceId, ordinal)',
	'asset.m_sKind != HST_RescuePOWOperationService.CAPTIVE_KIND',
	'asset.m_sRole != HST_RescuePOWOperationService.CAPTIVE_ROLE',
	'asset.m_sPrefab != HST_RescuePOWOperationService.CAPTIVE_PREFAB',
	'asset.m_iOrdinal != ordinal',
	'asset.m_iQuantity != 1',
	'!asset.m_bRequired',
	'!asset.m_sAssignedVehicleSlotId.IsEmpty()',
	'foreign exact rescue asset-slot semantics rejected',
	'HST_OPERATION_TYPE_MISSION_RESCUE',
	'operation.m_iContractVersion != HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION',
	'exact rescue external-asset operation/mission relationship is not reciprocal',
	'ValidateCaptiveAuthorityRows(',
	'state.m_aMissionAssets',
	'HST_RescuePOWOperationService.IsExactRescueCaptiveAsset(state, asset)',
	'asset.m_sRescueProjectionId == HST_RescuePOWOperationService.BuildCaptiveProjectionId(',
	'!asset.m_sAssignedVehicleSlotId.IsEmpty()',
	'!asset.m_sConvoyElementId.IsEmpty()',
	'exact rescue external-asset durable captive authority requires exactly three rows',
	'candidateOperation.m_sMissionInstanceId == mission.m_sInstanceId',
	'candidateBatch.m_sManifestId == manifest.m_sManifestId',
	'candidateGroup.m_sMissionInstanceId == mission.m_sInstanceId',
	'HasUniqueAuthorityRows(state, mission, operation, manifest, batch, group)'
)) {
	if ($schema58ExternalAssetPolicyText -notmatch [regex]::Escape($schema58ExternalAssetShapeEntry)) {
		throw "Schema-58 external-asset defense-in-depth is missing: $schema58ExternalAssetShapeEntry"
	}
}

foreach ($schema58RuntimeEntry in @(
	'TickExactRescueCaptiveActuators',
	'EnsureExactRescueCaptiveProjection',
	'FoldExactRescueCaptiveProjection',
	'TryResolveExactRescueCaptiveDeathEvidence',
	'RebindExactRescueEscortProjection',
	'ResolveExactRescueEscortCarrierEvidence',
	'ResolveExactRescueCarrierEvidence',
	'exact rescue captive projection is absent; absence is not death evidence',
	'vehicle_local_'
)) {
	if ($schema58RuntimeText -notmatch [regex]::Escape($schema58RuntimeEntry)) {
		throw "Schema-58 captive projection/evidence boundary is missing: $schema58RuntimeEntry"
	}
}
foreach ($schema58ClientActionProjectionEntry in @(
	'[RplProp()]',
	'm_bRescueActionProjectionEvaluated',
	'm_bRescueActionProjectionConfigured',
	'm_iRescueActionContractVersion',
	'm_eRescueActionDisposition',
	'm_bRescueActionMissionActive',
	'm_bRescueActionAuthorityQuarantined',
	'm_iRescueActionRevision',
	'ConfigureRescueActionProjection',
	'RefreshRescueActionProjectionFromAuthority',
	'HasRescueActionProjection',
	'IsRescueActionProjectionEvaluated',
	'IsRescueActionMissionActive',
	'IsRescueActionAuthorityQuarantined',
	'missionAsset.RefreshRescueActionProjectionFromAuthority()',
	'HST_MissionCaptiveActionPolicy.ResolveProjection(owner)',
	'HST_MissionCaptiveActionPolicy.IsProjectionPending(projection)',
	'projection.GetRescueActionDisposition()'
)) {
	if (($schema58MissionAssetComponentText + "`n" + $schema58CaptiveActionText + "`n" + $schema58MissionActionFilterText) -notmatch [regex]::Escape($schema58ClientActionProjectionEntry)) {
		throw "Schema-58 client/JIP captive action projection is missing: $schema58ClientActionProjectionEntry"
	}
}
$schema58ConfigureIdentityBlock = Get-ScriptMethodBlock $schema58MissionAssetComponentText 'void ConfigureMissionAsset('
if ([string]::IsNullOrEmpty($schema58ConfigureIdentityBlock) -or
	$schema58ConfigureIdentityBlock.IndexOf('m_sAssetId == assetId') -lt 0 -or
	$schema58ConfigureIdentityBlock.IndexOf('return;') -lt 0 -or
	$schema58ConfigureIdentityBlock.IndexOf('return;') -gt $schema58ConfigureIdentityBlock.IndexOf('Replication.BumpMe()')) {
	throw "Schema-58 replicated mission-asset identity must avoid unchanged per-tick replication bumps"
}
$schema58CanExtractBlock = Get-ScriptMethodBlock $schema58CaptiveActionText 'static bool CanExtract('
if ([string]::IsNullOrEmpty($schema58CanExtractBlock) -or
	$schema58CanExtractBlock.IndexOf('HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING') -lt 0 -or
	$schema58CanExtractBlock.IndexOf('HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED') -lt 0 -or
	$schema58CanExtractBlock.IndexOf('HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING') -ge 0) {
	throw "Schema-58 exact captive actions must hide extraction during the transient BOARDING state"
}
$schema58InteractionBlock = Get-ScriptMethodBlock $schema58CoordinatorText 'protected bool TryHandleExactRescueMissionInteraction('
$schema58ReplayIndex = $schema58InteractionBlock.IndexOf('TryReplayCaptiveCommandReceipt(')
$schema58InactiveIndex = $schema58InteractionBlock.IndexOf('rescue mission is no longer active')
if ([string]::IsNullOrEmpty($schema58InteractionBlock) -or $schema58ReplayIndex -lt 0 -or
	$schema58InactiveIndex -lt 0 -or $schema58ReplayIndex -gt $schema58InactiveIndex) {
	throw "Schema-58 durable captive receipt replay must precede terminal mission-status rejection"
}
if ($schema58LifecycleText -notmatch [regex]::Escape('EDamageState.DESTROYED') -or
	$schema58LifecycleText -notmatch [regex]::Escape('RequestServerMissionAssetDestroyed')) {
	throw "Schema-58 captive lifecycle component lacks authoritative damage reporting"
}

foreach ($schema58ValidationEntry in @(
	'class HST_RescuePOWSaveValidationService',
	'SCHEMA_VERSION = 58',
	'migration_schema58_exact_rescue_pows',
	'normalization_schema58_exact_rescue_pows_conflict',
	'IsSchema58RescuePOWMissionClaimant',
	'IsSchema58RescuePOWOperationClaimant',
	'IsSchema58RescuePOWManifestClaimant',
	'IsSchema58RescuePOWBatchClaimant',
	'IsSchema58RescuePOWGroupClaimant',
	'IsSchema58RescuePOWAssetClaimant',
	'ValidateCurrentAggregate',
	'ValidateCompactAggregate',
	'PreservePreSchema58Authority',
	'ValidateLifecycle',
	'QuarantineAggregate'
)) {
	if (($schema58ValidationText + "`n" + $schema58SaveText) -notmatch [regex]::Escape($schema58ValidationEntry)) {
		throw "Schema-58 rescue migration/restore boundary is missing: $schema58ValidationEntry"
	}
}
foreach ($schema58StrongSkip in @(
	'IsSchema58RescuePOWMissionClaimant(this, mission)',
	'IsSchema58RescuePOWAssetClaimant(this, asset)',
	'IsSchema58RescuePOWManifestClaimant(this, manifest)',
	'IsSchema58RescuePOWBatchClaimant(this, spawnResult)',
	'IsSchema58RescuePOWGroupClaimant(this, group)',
	'!HST_RescuePOWSaveValidationService.HasSchema58RescuePOWMissionClaimant(this)'
)) {
	if ($schema58SaveText -notmatch [regex]::Escape($schema58StrongSkip)) {
		throw "Schema-58 pre-normalization strong-claimant isolation is missing: $schema58StrongSkip"
	}
}

foreach ($schema58WiringEntry in @(
	'm_RescuePOWOperations = new HST_RescuePOWOperationService()',
	'SetRescuePOWOperationService',
	'PrepareNewMissionContract(mission)',
	'AdmitNewMission(',
	'TryHandleExactRescueMissionInteraction',
	'FindFailedActiveMissionId(m_State)',
	'FindCompletedActiveMissionId(m_State)',
	'AppendCampaignDebugRescuePOWOperationAssertions',
	'rescue_pow.admission_isolation',
	'rescue_pow.composite_authority',
	'rescue_pow.captive_transitions',
	'rescue_pow.guard_independence',
	'rescue_pow.outcome_grace',
	'rescue_pow.restore_quarantine'
)) {
	if (($schema58CoordinatorText + "`n" + $schema58PersistenceText) -notmatch [regex]::Escape($schema58WiringEntry)) {
		throw "Schema-58 coordinator/persistence/proof wiring is missing: $schema58WiringEntry"
	}
}

foreach ($schema58ProofEntry in @(
	'class HST_RescuePOWOperationProofReport',
	'class HST_RescuePOWOperationProofService',
	'ProveAdmissionIsolation',
	'ProveCompositeAuthority',
	'ProveCaptiveTransitions',
	'ProveGuardIndependence',
	'ProveOutcomeGrace',
	'ProveRestoreQuarantine',
	'PACKAGED_GATES'
)) {
	if ($schema58ProofText -notmatch [regex]::Escape($schema58ProofEntry)) {
		throw "Schema-58 rescue source proof is missing: $schema58ProofEntry"
	}
}

if ($schema58MarkerText -notmatch [regex]::Escape('BuildExactRescueMissionMarkerLabel') -or
	$schema58MarkerText -notmatch [regex]::Escape('POW Extraction | HQ') -or
	$schema58UIServiceText -notmatch [regex]::Escape('BuildExactRescueNextStepText') -or
	$schema58UIServiceText -notmatch [regex]::Escape('CountExactRescueCaptiveStates')) {
	throw "Schema-58 state-derived rescue marker/UI projection is missing"
}

$schema58DocumentationPaths = @(
	"README.md",
	"docs/ARCHITECTURE.md",
	"docs/FEATURE_CHECKLIST.md",
	"docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md",
	"docs/HST_ENFUSION_ENFORCE_NOTES.md",
	"docs/MIGRATIONS.md",
	"docs/PARITY.md",
	"docs/PHASE_PLAN.md"
)
foreach ($schema58DocumentationPath in $schema58DocumentationPaths) {
	$schema58DocumentationText = (Get-Content -Raw $schema58DocumentationPath).ToLowerInvariant()
	$mentionsSchema58 = $schema58DocumentationText.Contains("schema 58") -or $schema58DocumentationText.Contains("schema-58")
	if (!$mentionsSchema58 -or !$schema58DocumentationText.Contains("rescue_pows") -or
		!$schema58DocumentationText.Contains("packaged")) {
		throw "$schema58DocumentationPath must describe the Schema-58 POW-rescue and packaged-runtime boundary"
	}
}
$schema58MigrationsText = (Get-Content -Raw "docs/MIGRATIONS.md").ToLowerInvariant()
foreach ($schema58MigrationNote in @(
	'migration_schema58_exact_rescue_pows',
	'normalization_schema58_exact_rescue_pows_conflict',
	'exact_rescue_pows_v1',
	'rescue_pows_guard',
	'contract `1`',
	'version `-58`',
	'contract `0`',
	'packaged'
)) {
	if (!$schema58MigrationsText.Contains($schema58MigrationNote)) {
		throw "Schema-58 migration documentation is missing: $schema58MigrationNote"
	}
}
Write-Host "Schema-58 exact POW-rescue admission, composite guard/captive authority, transition/idempotency, fold/evidence, outcome/grace, migration/quarantine, UI/marker, and proof contract OK"

Write-Host "h-istasi foundation validation passed"
