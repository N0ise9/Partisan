[ComponentEditorProps(category: "h-istasi", description: "Explosive-only mission demolition target")]
class HST_MissionDestroyTargetComponentClass : ScriptComponentClass
{
}

class HST_MissionDestroyTargetComponent : ScriptComponent
{
	[Attribute(defvalue: "300", uiwidget: UIWidgets.EditBox, desc: "Explosive score required before the mission target is destroyed.", category: "HST Mission")]
	protected float m_fRequiredExplosiveDamage;

	[Attribute(defvalue: "100", uiwidget: UIWidgets.EditBox, desc: "Score for RPG, AT, and rocket hits.", category: "HST Mission")]
	protected float m_fRocketDamageScore;

	[Attribute(defvalue: "160", uiwidget: UIWidgets.EditBox, desc: "Score for satchel and demolition charge hits.", category: "HST Mission")]
	protected float m_fDemoChargeDamageScore;

	[Attribute(defvalue: "35", uiwidget: UIWidgets.EditBox, desc: "Score for 40mm HE and small explosive hits.", category: "HST Mission")]
	protected float m_fSmallExplosiveDamageScore;

	[Attribute(defvalue: "0.35", uiwidget: UIWidgets.EditBox, desc: "Seconds before the same explosive source can score again.", category: "HST Mission")]
	protected float m_fDuplicateHitWindowSeconds;

	[Attribute(defvalue: "45", uiwidget: UIWidgets.EditBox, desc: "Radius used to detect short-lived explosive projectile and blast entities around the target.", category: "HST Mission")]
	protected float m_fExplosiveWitnessRadius;

	[Attribute(defvalue: "0.05", uiwidget: UIWidgets.EditBox, desc: "Seconds between nearby explosive witness scans.", category: "HST Mission")]
	protected float m_fExplosiveWitnessPollIntervalSeconds;

	[Attribute(defvalue: "0.75", uiwidget: UIWidgets.EditBox, desc: "Seconds before the same nearby explosive witness source can score again.", category: "HST Mission")]
	protected float m_fExplosiveWitnessSourceCooldownSeconds;

	[Attribute(defvalue: "96", uiwidget: UIWidgets.EditBox, desc: "Maximum entities collected by each nearby explosive witness scan.", category: "HST Mission")]
	protected int m_iMaxExplosiveWitnessScanEntities;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Legacy debug fallback. Keep disabled for radio tower demolition missions.", category: "HST Mission")]
	protected bool m_bAllowStockDamageDestroyedCompletion;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Print nearby explosive witness candidates and damage callback scores.", category: "HST Mission Debug")]
	protected bool m_bDebugExplosiveWitnesses;

	protected bool m_bReportedDestroyed;
	protected float m_fLocalExplosiveDamage;
	protected float m_fDuplicateHitRemainingSeconds;
	protected float m_fExplosiveWitnessPollRemainingSeconds;
	protected float m_fWitnessDebugSummaryRemainingSeconds;
	protected int m_iLastWitnessQueryCount;
	protected int m_iLastWitnessPotentialCount;
	protected string m_sLastAcceptedSource;
	protected ref array<IEntity> m_aExplosiveWitnessCandidates = {};
	protected ref array<string> m_aRecentExplosiveWitnessSourceKeys = {};
	protected ref array<float> m_aRecentExplosiveWitnessSourceSeconds = {};

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		NormalizeConfig();
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		NormalizeConfig();
		RegisterDamageCallbacks(owner);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_fDuplicateHitRemainingSeconds > 0.0)
			m_fDuplicateHitRemainingSeconds = Math.Max(0.0, m_fDuplicateHitRemainingSeconds - timeSlice);

		TickRecentExplosiveWitnessSources(timeSlice);

		if (!m_bReportedDestroyed && Replication.IsServer() && owner)
			TickExplosiveWitnessScan(owner, timeSlice);

		if (m_bDebugExplosiveWitnesses && Replication.IsServer())
			TickExplosiveWitnessDebugSummary(timeSlice);

		if (!m_bAllowStockDamageDestroyedCompletion)
			return;

		if (m_bReportedDestroyed || !Replication.IsServer() || !owner)
			return;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || damageManager.GetState() != EDamageState.DESTROYED)
			return;

		ReportDestroyedByLegacyDamageState(owner);
	}

	void OnDamageReceived(IEntity owner, float rawDamage, IEntity sourceEntity, string sourcePrefab, string damageTypeText)
	{
		if (m_bReportedDestroyed || !Replication.IsServer() || !owner)
			return;

		float score = ResolveExplosiveDamageScore(rawDamage, sourceEntity, sourcePrefab, damageTypeText);
		string sourceKey = BuildDamageSourceKey(sourceEntity, sourcePrefab, damageTypeText);
		Print(string.Format("h-istasi mission target | damage callback | raw=%1 | sourcePrefab=%2 | type=%3 | sourceKey=%4 | score=%5", rawDamage, sourcePrefab, damageTypeText, sourceKey, score));
		if (score <= 0.0)
			return;

		TryApplyExplosiveDamageScore(owner, score, sourceKey, false);
	}

	bool DebugApplyRocketScore(IEntity owner)
	{
		if (!Replication.IsServer() || !owner)
			return false;

		return TryApplyExplosiveDamageScore(owner, m_fRocketDamageScore, "debug:rpg_test_hit", false);
	}

	protected void TickExplosiveWitnessScan(IEntity owner, float timeSlice)
	{
		if (m_fExplosiveWitnessRadius <= 0.0)
			return;

		m_fExplosiveWitnessPollRemainingSeconds -= timeSlice;
		if (m_fExplosiveWitnessPollRemainingSeconds > 0.0)
			return;

		m_fExplosiveWitnessPollRemainingSeconds = m_fExplosiveWitnessPollIntervalSeconds;
		PollNearbyExplosiveWitnesses(owner);
	}

	protected void PollNearbyExplosiveWitnesses(IEntity owner)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		m_iLastWitnessQueryCount = 0;
		m_iLastWitnessPotentialCount = 0;
		m_aExplosiveWitnessCandidates.Clear();
		world.QueryEntitiesBySphere(owner.GetOrigin(), m_fExplosiveWitnessRadius, AddExplosiveWitnessCandidate, null, EQueryEntitiesFlags.ALL);

		foreach (IEntity candidate : m_aExplosiveWitnessCandidates)
		{
			if (!candidate || candidate == owner)
				continue;

			if (IsEntityOwnedByTarget(candidate, owner))
				continue;

			string witnessText = BuildEntityIdentityText(candidate);
			float score = ResolveExplosiveWitnessDamageScore(witnessText);
			if (m_bDebugExplosiveWitnesses)
				Print(string.Format("h-istasi mission target | witness candidate | text=%1 | score=%2", witnessText, score));

			if (score <= 0.0)
				continue;

			string sourceKey = BuildExplosiveWitnessSourceKey(candidate, witnessText);
			if (TryApplyExplosiveDamageScore(owner, score, sourceKey, true))
			{
				Print(string.Format("h-istasi mission target | explosive witness accepted | source %1 | score %2", sourceKey, score));
				return;
			}
		}
	}

	protected bool AddExplosiveWitnessCandidate(IEntity entity)
	{
		m_iLastWitnessQueryCount++;

		if (!entity)
			return true;

		string text = BuildEntityIdentityText(entity);
		string lowered = text;
		lowered.ToLower();

		if (!IsPotentialExplosiveWitnessText(lowered))
			return true;

		m_iLastWitnessPotentialCount++;

		if (m_aExplosiveWitnessCandidates.Count() < m_iMaxExplosiveWitnessScanEntities)
			m_aExplosiveWitnessCandidates.Insert(entity);

		return m_aExplosiveWitnessCandidates.Count() < m_iMaxExplosiveWitnessScanEntities;
	}

	protected void TickExplosiveWitnessDebugSummary(float timeSlice)
	{
		m_fWitnessDebugSummaryRemainingSeconds -= timeSlice;
		if (m_fWitnessDebugSummaryRemainingSeconds > 0.0)
			return;

		m_fWitnessDebugSummaryRemainingSeconds = 1.0;
		Print(string.Format("h-istasi mission target | witness scan summary | queried=%1 | potential=%2 | kept=%3", m_iLastWitnessQueryCount, m_iLastWitnessPotentialCount, m_aExplosiveWitnessCandidates.Count()));
	}

	protected bool TryApplyExplosiveDamageScore(IEntity owner, float score, string sourceKey, bool isWitness)
	{
		if (m_bReportedDestroyed || !Replication.IsServer() || !owner || score <= 0.0)
			return false;

		if (sourceKey.IsEmpty())
			sourceKey = "unknown explosive";

		if (IsDuplicateExplosiveHit(sourceKey))
			return false;

		if (isWitness && IsRecentExplosiveWitnessSource(sourceKey))
			return false;

		HST_MissionAssetComponent asset = ResolveMissionAssetComponent(owner);
		if (!asset)
			return false;

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return false;

		m_sLastAcceptedSource = sourceKey;
		m_fDuplicateHitRemainingSeconds = m_fDuplicateHitWindowSeconds;
		if (isWitness)
			RememberExplosiveWitnessSource(sourceKey);

		m_fLocalExplosiveDamage += score;
		string result = coordinator.RequestServerMissionAssetExplosiveDamage(asset.GetAssetId(), ResolveReportPosition(owner, asset), score, sourceKey);
		Print(result);

		string loweredResult = result;
		loweredResult.ToLower();
		if (loweredResult.Contains("demolished") || loweredResult.Contains("already destroyed"))
			m_bReportedDestroyed = true;

		return true;
	}

	protected void NormalizeConfig()
	{
		if (m_fRequiredExplosiveDamage <= 0.0)
			m_fRequiredExplosiveDamage = 300.0;
		if (m_fRocketDamageScore <= 0.0)
			m_fRocketDamageScore = 100.0;
		if (m_fDemoChargeDamageScore <= 0.0)
			m_fDemoChargeDamageScore = 160.0;
		if (m_fSmallExplosiveDamageScore <= 0.0)
			m_fSmallExplosiveDamageScore = 35.0;
		if (m_fDuplicateHitWindowSeconds <= 0.0)
			m_fDuplicateHitWindowSeconds = 0.35;
		if (m_fExplosiveWitnessRadius <= 0.0)
			m_fExplosiveWitnessRadius = 45.0;
		if (m_fExplosiveWitnessPollIntervalSeconds <= 0.0)
			m_fExplosiveWitnessPollIntervalSeconds = 0.05;
		if (m_fExplosiveWitnessSourceCooldownSeconds <= 0.0)
			m_fExplosiveWitnessSourceCooldownSeconds = 0.75;
		if (m_iMaxExplosiveWitnessScanEntities <= 0)
			m_iMaxExplosiveWitnessScanEntities = 96;
	}

	protected void RegisterDamageCallbacks(IEntity owner)
	{
		if (!Replication.IsServer() || !owner)
			return;

		// Wire the Workbench damage/hit-zone callback to OnDamageReceived().
		// Mission completion must come from explosive score, not generic destroyed state.
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager)
		{
			Print("h-istasi mission target | no SCR_DamageManagerComponent on demolition target/proxy; using explosive witness fallback", LogLevel.WARNING);
			return;
		}

		Print("h-istasi mission target | damage manager found, but damage callback bridge still needs Workbench hit-zone invoker wiring; using explosive witness fallback", LogLevel.WARNING);
	}

	protected void ReportDestroyedByLegacyDamageState(IEntity owner)
	{
		HST_MissionAssetComponent asset = ResolveMissionAssetComponent(owner);
		if (!asset)
			return;

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return;

		m_bReportedDestroyed = true;
		Print(coordinator.RequestServerMissionAssetDestroyed(asset.GetAssetId(), ResolveReportPosition(owner, asset)));
	}

	protected HST_MissionAssetComponent ResolveMissionAssetComponent(IEntity owner)
	{
		IEntity cursor = owner;
		while (cursor)
		{
			HST_MissionAssetComponent asset = HST_MissionAssetComponent.Cast(cursor.FindComponent(HST_MissionAssetComponent));
			if (asset)
				return asset;

			cursor = cursor.GetParent();
		}

		return null;
	}

	protected vector ResolveReportPosition(IEntity owner, HST_MissionAssetComponent asset)
	{
		IEntity cursor = owner;
		while (cursor)
		{
			if (asset == HST_MissionAssetComponent.Cast(cursor.FindComponent(HST_MissionAssetComponent)))
				return cursor.GetOrigin();

			cursor = cursor.GetParent();
		}

		return owner.GetOrigin();
	}

	protected bool IsDuplicateExplosiveHit(string sourceKey)
	{
		if (sourceKey.IsEmpty())
			return false;

		if (sourceKey != m_sLastAcceptedSource)
			return false;

		return m_fDuplicateHitRemainingSeconds > 0.0;
	}

	protected void TickRecentExplosiveWitnessSources(float timeSlice)
	{
		for (int i = m_aRecentExplosiveWitnessSourceKeys.Count() - 1; i >= 0; i--)
		{
			if (i >= m_aRecentExplosiveWitnessSourceSeconds.Count())
			{
				m_aRecentExplosiveWitnessSourceKeys.Remove(i);
				continue;
			}

			m_aRecentExplosiveWitnessSourceSeconds[i] = Math.Max(0.0, m_aRecentExplosiveWitnessSourceSeconds[i] - timeSlice);
			if (m_aRecentExplosiveWitnessSourceSeconds[i] <= 0.0)
			{
				m_aRecentExplosiveWitnessSourceKeys.Remove(i);
				m_aRecentExplosiveWitnessSourceSeconds.Remove(i);
			}
		}
	}

	protected bool IsRecentExplosiveWitnessSource(string sourceKey)
	{
		if (sourceKey.IsEmpty())
			return false;

		return m_aRecentExplosiveWitnessSourceKeys.Find(sourceKey) >= 0;
	}

	protected void RememberExplosiveWitnessSource(string sourceKey)
	{
		if (sourceKey.IsEmpty())
			return;

		float cooldownSeconds = ResolveExplosiveWitnessSourceCooldownSeconds(sourceKey);
		int existingIndex = m_aRecentExplosiveWitnessSourceKeys.Find(sourceKey);
		if (existingIndex >= 0)
		{
			m_aRecentExplosiveWitnessSourceSeconds[existingIndex] = cooldownSeconds;
			return;
		}

		m_aRecentExplosiveWitnessSourceKeys.Insert(sourceKey);
		m_aRecentExplosiveWitnessSourceSeconds.Insert(cooldownSeconds);
	}

	protected float ResolveExplosiveWitnessSourceCooldownSeconds(string sourceKey)
	{
		if (sourceKey.Contains("generic-warhead"))
			return Math.Max(m_fExplosiveWitnessSourceCooldownSeconds, 12.0);

		return m_fExplosiveWitnessSourceCooldownSeconds;
	}

	protected bool IsEntityOwnedByTarget(IEntity candidate, IEntity owner)
	{
		IEntity cursor = candidate;
		while (cursor)
		{
			if (cursor == owner)
				return true;

			cursor = cursor.GetParent();
		}

		return false;
	}

	protected string BuildEntityIdentityText(IEntity entity)
	{
		if (!entity)
			return "";

		string text;
		if (entity.GetPrefabData())
			text = entity.GetPrefabData().GetPrefabName();

		text = text + " " + entity.GetName();
		text.Trim();
		return text;
	}

	protected string BuildExplosiveWitnessSourceKey(IEntity entity, string witnessText)
	{
		string key;
		if (entity && entity.GetPrefabData())
			key = entity.GetPrefabData().GetPrefabName();

		if (key.IsEmpty() && entity)
			key = entity.GetName();

		if (key.IsEmpty())
			key = witnessText;

		string lowered = witnessText;
		lowered.ToLower();
		if (IsGenericWarheadWitnessText(lowered))
			return "witness:generic-warhead:" + key;

		return "witness:" + key;
	}

	protected string BuildRoundedWitnessPositionKey(IEntity entity)
	{
		if (!entity)
			return "unknown";

		vector position = entity.GetOrigin();
		return string.Format("%1:%2:%3", Math.Round(position[0]), Math.Round(position[1]), Math.Round(position[2]));
	}

	protected string BuildDamageSourceKey(IEntity sourceEntity, string sourcePrefab, string damageTypeText)
	{
		string key = sourcePrefab;
		if (key.IsEmpty() && sourceEntity && sourceEntity.GetPrefabData())
			key = sourceEntity.GetPrefabData().GetPrefabName();

		if (key.IsEmpty() && sourceEntity)
			key = sourceEntity.GetName();

		if (key.IsEmpty())
			key = damageTypeText;

		return key;
	}

	protected float ResolveExplosiveDamageScore(float rawDamage, IEntity sourceEntity, string sourcePrefab, string damageTypeText)
	{
		string text = sourcePrefab + " " + damageTypeText;
		if (text.Trim().IsEmpty() && sourceEntity)
		{
			if (sourceEntity.GetPrefabData())
				text = sourceEntity.GetPrefabData().GetPrefabName();
			text = text + " " + sourceEntity.GetName();
		}

		text.ToLower();

		if (IsSmallArmsDamageText(text))
			return 0.0;

		if (IsDemoChargeDamageText(text))
			return m_fDemoChargeDamageScore;

		if (IsRocketDamageText(text))
			return m_fRocketDamageScore;

		if (IsMineDamageText(text))
			return Math.Max(m_fRocketDamageScore, 120.0);

		if (IsSmallExplosiveDamageText(text))
			return m_fSmallExplosiveDamageScore;

		if (text.Contains("explosion") || text.Contains("explosive") || text.Contains("blast"))
			return Math.Max(m_fSmallExplosiveDamageScore, Math.Min(rawDamage, m_fRocketDamageScore));

		return 0.0;
	}

	protected float ResolveExplosiveWitnessDamageScore(string witnessText)
	{
		string text = witnessText;
		text.ToLower();

		if (text.IsEmpty() || IsSmallArmsDamageText(text))
			return 0.0;

		if (IsWeaponOrVehicleWitnessText(text))
			return 0.0;

		if (!IsProjectileOrBlastWitnessText(text))
			return 0.0;

		if (IsGenericWarheadWitnessText(text))
			return m_fSmallExplosiveDamageScore;

		if (IsRocketWitnessDamageText(text))
			return m_fRocketDamageScore;

		if (IsMineDamageText(text))
			return Math.Max(m_fRocketDamageScore, 120.0);

		if (IsSmallExplosiveDamageText(text))
			return m_fSmallExplosiveDamageScore;

		if (IsExplosiveShellWitnessText(text))
			return Math.Max(m_fSmallExplosiveDamageScore, 45.0);

		if (text.Contains("explosion") || text.Contains("blast") || text.Contains("explosive"))
			return m_fSmallExplosiveDamageScore;

		return 0.0;
	}

	protected bool IsSmallArmsDamageText(string text)
	{
		if (text.IsEmpty())
			return false;

		if (text.Contains("bullet"))
			return true;
		if (text.Contains("cartridge"))
			return true;
		if (text.Contains("ball"))
			return true;
		if (text.Contains("tracer"))
			return true;
		if (text.Contains("fmj"))
			return true;
		if (text.Contains("smallarms"))
			return true;
		if (text.Contains("rifle"))
			return true;
		if (text.Contains("machinegun"))
			return true;
		if (text.Contains("mg_"))
			return true;
		if (text.Contains("556") || text.Contains("5.56"))
			return true;
		if (text.Contains("762") || text.Contains("7.62"))
			return true;
		if (text.Contains("9x19") || text.Contains("45acp"))
			return true;
		if (text.Contains("buckshot") || text.Contains("slug"))
			return true;

		return false;
	}

	protected bool IsRocketDamageText(string text)
	{
		if (text.Contains("rpg"))
			return true;
		if (text.Contains("pg7"))
			return true;
		if (text.Contains("rocket"))
			return true;
		if (text.Contains("launcher"))
			return true;
		if (text.Contains("heat"))
			return true;
		if (text.Contains("maaws"))
			return true;
		if (text.Contains("m72"))
			return true;
		if (text.Contains("at4"))
			return true;
		if (text.Contains("m136"))
			return true;
		if (text.Contains("law"))
			return true;

		return false;
	}

	protected bool IsRocketWitnessDamageText(string text)
	{
		if (text.Contains("rocket"))
			return true;
		if (text.Contains("missile"))
			return true;
		if (text.Contains("heat"))
			return true;
		if (text.Contains("rpg") || text.Contains("pg7"))
			return true;
		if (text.Contains("maaws"))
			return true;
		if (text.Contains("m72") || text.Contains("at4") || text.Contains("m136"))
			return true;

		return false;
	}

	protected bool IsGenericWarheadWitnessText(string text)
	{
		if (!text.Contains("warhead"))
			return false;
		if (text.Contains("rocket"))
			return false;
		if (text.Contains("missile"))
			return false;
		if (text.Contains("heat"))
			return false;
		if (text.Contains("rpg"))
			return false;
		if (text.Contains("pg7"))
			return false;
		if (text.Contains("maaws"))
			return false;
		if (text.Contains("m72"))
			return false;
		if (text.Contains("at4"))
			return false;
		if (text.Contains("m136"))
			return false;

		return true;
	}

	protected bool IsDemoChargeDamageText(string text)
	{
		if (text.Contains("satchel"))
			return true;
		if (text.Contains("demo"))
			return true;
		if (text.Contains("demolition"))
			return true;
		if (text.Contains("charge"))
			return true;
		if (text.Contains("explosive_charge"))
			return true;
		if (text.Contains("plastic"))
			return true;

		return false;
	}

	protected bool IsMineDamageText(string text)
	{
		if (text.Contains("mine"))
			return true;
		if (text.Contains("ied"))
			return true;

		return false;
	}

	protected bool IsSmallExplosiveDamageText(string text)
	{
		if (text.Contains("grenade"))
			return true;
		if (text.Contains("he_"))
			return true;
		if (text.Contains("_he"))
			return true;
		if (text.Contains(" he"))
			return true;
		if (text.Contains("-he"))
			return true;
		if (text.Contains("m433"))
			return true;
		if (text.Contains("40mm"))
			return true;

		return false;
	}

	protected bool IsProjectileOrBlastWitnessText(string text)
	{
		if (text.Contains("projectile"))
			return true;
		if (text.Contains("munition"))
			return true;
		if (text.Contains("ammo"))
			return true;
		if (text.Contains("rocket"))
			return true;
		if (text.Contains("missile"))
			return true;
		if (text.Contains("warhead"))
			return true;
		if (text.Contains("rpg"))
			return true;
		if (text.Contains("pg7"))
			return true;
		if (text.Contains("maaws"))
			return true;
		if (text.Contains("m72"))
			return true;
		if (text.Contains("at4"))
			return true;
		if (text.Contains("m136"))
			return true;
		if (text.Contains("heat"))
			return true;
		if (text.Contains("grenade"))
			return true;
		if (text.Contains("shell"))
			return true;
		if (text.Contains("m433"))
			return true;
		if (text.Contains("40mm"))
			return true;
		if (text.Contains("25mm"))
			return true;
		if (text.Contains("m242"))
			return true;
		if (text.Contains("mine") || text.Contains("ied"))
			return true;
		if (text.Contains("explosion") || text.Contains("blast") || text.Contains("explosive"))
			return true;

		return false;
	}

	protected bool IsPotentialExplosiveWitnessText(string text)
	{
		if (text.IsEmpty())
			return false;

		if (text.Contains("projectile"))
			return true;
		if (text.Contains("munition"))
			return true;
		if (text.Contains("ammo"))
			return true;
		if (text.Contains("rocket"))
			return true;
		if (text.Contains("missile"))
			return true;
		if (text.Contains("warhead"))
			return true;
		if (text.Contains("rpg"))
			return true;
		if (text.Contains("pg7"))
			return true;
		if (text.Contains("maaws"))
			return true;
		if (text.Contains("m72"))
			return true;
		if (text.Contains("at4"))
			return true;
		if (text.Contains("m136"))
			return true;
		if (text.Contains("heat"))
			return true;
		if (text.Contains("grenade"))
			return true;
		if (text.Contains("shell"))
			return true;
		if (text.Contains("m433"))
			return true;
		if (text.Contains("40mm"))
			return true;
		if (text.Contains("25mm"))
			return true;
		if (text.Contains("m242"))
			return true;
		if (text.Contains("mine"))
			return true;
		if (text.Contains("ied"))
			return true;
		if (text.Contains("explosion"))
			return true;
		if (text.Contains("blast"))
			return true;
		if (text.Contains("explosive"))
			return true;

		return false;
	}

	protected bool IsWeaponOrVehicleWitnessText(string text)
	{
		if (text.IsEmpty())
			return false;

		bool looksLikeProjectileOrAmmo = IsProjectileOrAmmoWitnessText(text);
		if (looksLikeProjectileOrAmmo)
			return false;

		if (text.Contains("vehicle"))
			return true;
		if (text.Contains("turret"))
			return true;
		if (text.Contains("lav25") || text.Contains("lav_"))
			return true;

		if (text.Contains("launcher"))
			return true;

		if (text.Contains("weapon"))
			return true;

		return false;
	}

	protected bool IsProjectileOrAmmoWitnessText(string text)
	{
		if (text.Contains("projectile"))
			return true;
		if (text.Contains("ammo"))
			return true;
		if (text.Contains("munition"))
			return true;
		if (text.Contains("rocket"))
			return true;
		if (text.Contains("missile"))
			return true;
		if (text.Contains("warhead"))
			return true;
		if (text.Contains("rpg"))
			return true;
		if (text.Contains("pg7"))
			return true;
		if (text.Contains("maaws"))
			return true;
		if (text.Contains("m72"))
			return true;
		if (text.Contains("at4"))
			return true;
		if (text.Contains("m136"))
			return true;
		if (text.Contains("heat"))
			return true;
		if (text.Contains("grenade"))
			return true;
		if (text.Contains("40mm"))
			return true;
		if (text.Contains("shell"))
			return true;
		if (text.Contains("mine"))
			return true;
		if (text.Contains("ied"))
			return true;

		return false;
	}

	protected bool ShouldDebugExplosiveWitnessText(string witnessText)
	{
		string lowered = witnessText;
		lowered.ToLower();

		if (lowered.Contains("rocket"))
			return true;
		if (lowered.Contains("rpg") || lowered.Contains("pg7"))
			return true;
		if (lowered.Contains("missile") || lowered.Contains("warhead"))
			return true;
		if (lowered.Contains("grenade"))
			return true;
		if (lowered.Contains("explosion") || lowered.Contains("blast"))
			return true;
		if (lowered.Contains("projectile"))
			return true;
		if (lowered.Contains("munition") || lowered.Contains("ammo"))
			return true;

		return false;
	}

	protected bool IsExplosiveShellWitnessText(string text)
	{
		if (!text.Contains("shell") && !text.Contains("25mm") && !text.Contains("m242"))
			return false;

		if (text.Contains("explosive"))
			return true;
		if (text.Contains("blast"))
			return true;
		if (text.Contains(" he") || text.Contains("_he") || text.Contains("he_") || text.Contains("-he"))
			return true;

		return false;
	}

}

[ComponentEditorProps(category: "h-istasi", description: "Relays damage from a demolition hit proxy to the mission target root")]
class HST_MissionDestroyTargetProxyComponentClass : ScriptComponentClass
{
}

class HST_MissionDestroyTargetProxyComponent : ScriptComponent
{
	protected HST_MissionDestroyTargetComponent m_Target;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_Target = ResolveTarget(owner);
		RegisterProxyDamageCallbacks(owner);
	}

	protected HST_MissionDestroyTargetComponent ResolveTarget(IEntity owner)
	{
		IEntity cursor = owner;
		while (cursor)
		{
			HST_MissionDestroyTargetComponent target = HST_MissionDestroyTargetComponent.Cast(cursor.FindComponent(HST_MissionDestroyTargetComponent));
			if (target)
				return target;

			cursor = cursor.GetParent();
		}

		return null;
	}

	protected void RegisterProxyDamageCallbacks(IEntity owner)
	{
		if (!Replication.IsServer() || !owner)
			return;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager)
		{
			Print("h-istasi mission target proxy | no damage manager on proxy", LogLevel.WARNING);
			return;
		}

		Print("h-istasi mission target proxy | damage manager found; bind proxy hit-zone damage invoker to RelayDamage");
	}

	void RelayDamage(IEntity proxyOwner, float rawDamage, IEntity sourceEntity, string sourcePrefab, string damageTypeText)
	{
		if (!m_Target)
			m_Target = ResolveTarget(proxyOwner);

		if (!m_Target)
			return;

		IEntity root = ResolveMissionRoot(proxyOwner);
		if (!root)
			root = proxyOwner;

		m_Target.OnDamageReceived(root, rawDamage, sourceEntity, sourcePrefab, damageTypeText);
	}

	protected IEntity ResolveMissionRoot(IEntity owner)
	{
		IEntity cursor = owner;
		IEntity best = owner;
		while (cursor)
		{
			if (HST_MissionAssetComponent.Cast(cursor.FindComponent(HST_MissionAssetComponent)))
				return cursor;

			best = cursor;
			cursor = cursor.GetParent();
		}

		return best;
	}
}

class HST_MissionDestroyTargetSabotageAction : HST_MissionUserActionBase
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;
		if (!pOwnerEntity)
			return;

		HST_MissionDestroyTargetComponent demolition = HST_MissionDestroyTargetComponent.Cast(pOwnerEntity.FindComponent(HST_MissionDestroyTargetComponent));
		if (!demolition)
		{
			Print("h-istasi mission | demolition debug failed: component missing", LogLevel.WARNING);
			return;
		}

		bool applied = demolition.DebugApplyRocketScore(pOwnerEntity);
		Print(string.Format("h-istasi mission | demolition debug rocket score applied %1", applied));
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "DEBUG: Apply demolition hit";
		return true;
	}
}
