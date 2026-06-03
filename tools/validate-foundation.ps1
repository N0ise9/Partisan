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

$project = Get-Content -Raw "addon.gproj"
if ($project -notmatch '"58D0FB3206B6F859"' -or $project -notmatch '"595F2BF2F44836FB"') {
	throw "addon.gproj must depend on base Reforger and RHS Status Quo"
}
Write-Host "Project dependencies OK"

$files = Get-ChildItem -Recurse -File -Include *.c,*.conf,*.ent,*.layer,*.gproj,*.md,.gitignore |
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
	'Prefabs/Characters/Core/DefaultPlayerControllerMP_ScenarioFramework.et',
	'm_bUseSpawnPreload 0',
	'SCR_MapConfigComponent',
	'Configs/Map/MapSpawnConflict.conf',
	'SCR_PlayerSpawnPointManagerComponent',
	'SCR_SpawnProtectionComponent',
	'SCR_TimedSpawnPointComponent',
	'Prefabs/MP/Managers/Factions/FactionManager_USxUSSR.et',
	'SCR_FactionAliasComponent',
	'm_sAlias "PLAYERS"',
	'm_sFactionKey "FIA"',
	'm_sAlias "OPFOR"',
	'm_sFactionKey "RHS_AFRF"',
	'Configs/Factions/FIA_Campaign.conf',
	'Configs/Factions/CIV.conf',
	'Prefabs/MP/Managers/Loadouts/LoadoutManager_Base.et',
	'm_aPlayerLoadouts',
	'SCR_FactionPlayerLoadout',
	'Configs/Loadouts/FIA/Loadout_FIA_Base.conf',
	'm_sAffiliatedFaction "FIA"',
	'Prefabs/Systems/Radio/RadioManager.et',
	'Prefabs/MP/ScriptedChatEntity.et'
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
	"HST_PhysicalWarService"
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
	"m_iQrfCooldownUntilSecond"
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
	"RequestCommanderStartMission",
	"RequestCommanderRecruitGarrison",
	"RequestMemberManualCheckpoint",
	"RequestAdminSetZoneActive",
	"RequestAdminCaptureZone"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCoordinatorEntry)) {
		throw "Missing coordinator dev/framework entry point: $requiredCoordinatorEntry"
	}
}
Write-Host "Antistasi framework service spine OK"

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
Write-Host "Native spawn request contract OK"

foreach ($requiredPhysicalWarEntry in @(
	"UpdateZoneActivation",
	"HST_ActiveGroupState",
	"HST_QRFState",
	"CreateFactionTemplate",
	"TrySpawnActiveGroup",
	"FoldActiveGroup",
	"UpdateQRF",
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
