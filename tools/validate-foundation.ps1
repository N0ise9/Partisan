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
	'SCR_MenuSpawnLogic',
	'm_sForcedFaction "PLAYERS"',
	'Prefabs/Characters/Core/DefaultPlayerControllerMP_ScenarioFramework.et',
	'SCR_MapConfigComponent',
	'Configs/Map/MapSpawnConflict.conf',
	'SCR_PlayerSpawnPointManagerComponent',
	'SCR_SpawnProtectionComponent',
	'SCR_TimedSpawnPointComponent',
	'Prefabs/MP/Managers/Factions/FactionManager_USxUSSR.et',
	'SCR_FactionAliasComponent',
	'm_sAlias "PLAYERS"',
	'm_sFactionKey "RHS_USAF"',
	'm_sAlias "OPFOR"',
	'm_sFactionKey "RHS_AFRF"',
	'Configs/Factions/FIA_Campaign.conf',
	'Configs/Factions/CIV.conf',
	'Prefabs/MP/Managers/Loadouts/LoadoutManager_Base.et',
	'm_aPlayerLoadouts',
	'SCR_FactionPlayerLoadout',
	'm_sAffiliatedFaction "RHS_USAF"',
	'Prefabs/Systems/Radio/RadioManager.et',
	'Prefabs/MP/ScriptedChatEntity.et'
)
$requiredBootstrapLoadouts = @(
	'Loadout_USAF_Rifleman.conf',
	'Loadout_USAF_AR.conf',
	'Loadout_USAF_GL.conf',
	'Loadout_USAF_Medic.conf',
	'Loadout_USAF_LAT.conf',
	'Loadout_USAF_Sniper.conf'
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

	if ($text -match "\bSCR_PlayerArsenalLoadout\b" -or $text -match 'm_sAffiliatedFaction "FIA"') {
		throw "Runtime layer must use RHS_USAF role-selection loadouts for the temporary deploy harness: $runtimeLayer"
	}

	foreach ($requiredLoadout in $requiredBootstrapLoadouts) {
		if ($text -notmatch [regex]::Escape($requiredLoadout)) {
			throw "Missing bootstrap player loadout in ${runtimeLayer}: $requiredLoadout"
		}
	}

	$bootstrapLoadoutCount = ([regex]::Matches($text, "Loadout_USAF_[A-Za-z0-9_]+\.conf")).Count
	if ($bootstrapLoadoutCount -lt $requiredBootstrapLoadouts.Count) {
		throw "Bootstrap loadout list is empty or incomplete in ${runtimeLayer}"
	}

	$bootstrapAffiliationCount = ([regex]::Matches($text, 'm_sAffiliatedFaction "RHS_USAF"')).Count
	if ($bootstrapAffiliationCount -lt $requiredBootstrapLoadouts.Count) {
		throw "Bootstrap player loadouts must be explicitly affiliated with RHS_USAF in ${runtimeLayer}"
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
	if ($text -match '"faction affiliation" "FIA"') {
		throw "Starting point layer must keep FIA out of the temporary deploy harness: $startingPointLayer"
	}

	foreach ($requiredEntry in @(
		"Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et",
		"SCR_CampaignMilitaryBaseComponent",
		"SCR_CampaignSpawnPointGroup",
		'"faction affiliation" "RHS_USAF"',
		"Prefabs/Systems/ScenarioFramework/Components/Area.et",
		"Prefabs/Systems/ScenarioFramework/Components/Layer.et",
		"Prefabs/Systems/ScenarioFramework/Components/Slot.et",
		"SCR_ScenarioFrameworkPluginSpawnPoint",
		'PrefabsEditable/SpawnPoints/E_SpawnPoint_US_USMC.et',
		'm_sFactionKey "PLAYERS"',
		'm_sFactionKey "RHS_USAF"',
		'm_sFaction "RHS_USAF"',
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
	"HST_PlayerLifecycleService",
	"HST_TownService",
	"HST_GarrisonService",
	"HST_RecruitmentService",
	"HST_ZoneCaptureService"
)) {
	if ($requiredService -notin $definedSymbols) {
		throw "Missing Antistasi framework service: $requiredService"
	}
}

foreach ($requiredCoordinatorEntry in @(
	"RegisterConnectedPlayer",
	"CaptureZoneForResistance",
	"CompleteMission",
	"FailMission",
	"ApplyIncomeNow",
	"AddAbstractGarrison",
	"FoldGarrisonSurvivors",
	"TrainTroops",
	"RecruitResistanceGarrison",
	"AwardFactionResources",
	"AwardPlayerResources"
)) {
	if ($scriptText -notmatch [regex]::Escape($requiredCoordinatorEntry)) {
		throw "Missing coordinator dev/framework entry point: $requiredCoordinatorEntry"
	}
}
Write-Host "Antistasi framework service spine OK"

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
