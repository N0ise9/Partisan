// Session-only live binding for durable field vehicles. Registered bindings are
// exact; the bounded same-prefab position lookup is recovery-only and fails
// closed when more than one durable row is plausible.
class HST_PersistentFieldVehicleRuntimeService
{
	protected ref array<IEntity> m_aEntities = {};
	protected ref array<string> m_aRuntimeIds = {};
	protected bool m_bBindingFault;

	bool Track(IEntity entity, HST_RuntimeVehicleState record)
	{
		if (!entity || !IsDurableRecord(record))
			return false;
		if (m_aEntities.Count() != m_aRuntimeIds.Count())
		{
			m_bBindingFault = true;
			return false;
		}
		int entityIndex = m_aEntities.Find(entity);
		int idIndex = m_aRuntimeIds.Find(record.m_sVehicleRuntimeId);
		if (entityIndex >= 0)
		{
			if (m_aRuntimeIds[entityIndex] != record.m_sVehicleRuntimeId
				|| idIndex != entityIndex)
			{
				m_bBindingFault = true;
				Print("h-istasi vehicle persistence | rejected durable root ID rebind/collision", LogLevel.ERROR);
				return false;
			}
			return true;
		}
		if (idIndex >= 0)
		{
			IEntity existingEntity = m_aEntities[idIndex];
			if (existingEntity && existingEntity != entity
				&& IsLivingEntity(existingEntity))
			{
				m_bBindingFault = true;
				Print("h-istasi vehicle persistence | rejected durable ID/live-root collision", LogLevel.ERROR);
				return false;
			}
			m_aEntities[idIndex] = entity;
			return true;
		}
		m_aEntities.Insert(entity);
		m_aRuntimeIds.Insert(record.m_sVehicleRuntimeId);
		return true;
	}

	bool Untrack(IEntity entity = null, string runtimeId = "")
	{
		bool changed;
		for (int index = m_aRuntimeIds.Count() - 1; index >= 0; index--)
		{
			if ((!entity || index >= m_aEntities.Count()
					|| m_aEntities[index] != entity)
				&& (runtimeId.IsEmpty()
					|| m_aRuntimeIds[index] != runtimeId))
				continue;
			RemoveAt(index);
			changed = true;
		}
		m_bBindingFault = m_bBindingFault || HasBindingCollision();
		return changed;
	}

	HST_RuntimeVehicleState ResolveForEntity(
		HST_CampaignState state,
		IEntity entity)
	{
		if (!state || !entity)
			return null;
		int trackedIndex = m_aEntities.Find(entity);
		if (trackedIndex >= 0 && trackedIndex < m_aRuntimeIds.Count())
		{
			HST_RuntimeVehicleState tracked = state.FindRuntimeVehicle(
				m_aRuntimeIds[trackedIndex]);
			if (IsDurableRecord(tracked))
				return tracked;
			// A registered root whose durable row vanished is an authority fault,
			// not permission to borrow a nearby row under a different ID.
			return null;
		}

		string prefab;
		if (entity.GetPrefabData())
			prefab = entity.GetPrefabData().GetPrefabName();
		vector origin = entity.GetOrigin();
		HST_RuntimeVehicleState positionalMatch;
		foreach (HST_RuntimeVehicleState candidate : state.m_aRuntimeVehicles)
		{
			if (!IsDurableRecord(candidate) || candidate.m_sPrefab != prefab)
				continue;
			if (DistanceSq2D(candidate.m_vPosition, origin) > 64.0)
				continue;
			int candidateBindingIndex
				= m_aRuntimeIds.Find(candidate.m_sVehicleRuntimeId);
			if (candidateBindingIndex >= 0
				&& candidateBindingIndex < m_aEntities.Count()
				&& m_aEntities[candidateBindingIndex] != entity)
				continue;
			// Positional lookup is migration/recovery fallback only. Two plausible
			// rows are ambiguous, so fail closed instead of binding the first one.
			if (positionalMatch && positionalMatch != candidate)
				return null;
			positionalMatch = candidate;
		}
		return positionalMatch;
	}

	IEntity ResolveEntityForRuntimeId(string runtimeId)
	{
		if (runtimeId.IsEmpty()
			|| m_aEntities.Count() != m_aRuntimeIds.Count())
			return null;
		int index = m_aRuntimeIds.Find(runtimeId);
		if (index < 0 || index >= m_aEntities.Count())
			return null;
		IEntity entity = m_aEntities[index];
		if (!IsLivingEntity(entity))
			return null;
		return entity;
	}

	bool PrepareForCapture(HST_CampaignState state)
	{
		if (!state || m_bBindingFault
			|| m_aEntities.Count() != m_aRuntimeIds.Count())
			return false;
		for (int index = m_aEntities.Count() - 1; index >= 0; index--)
		{
			IEntity entity = m_aEntities[index];
			string runtimeId = m_aRuntimeIds[index];
			HST_RuntimeVehicleState record = state.FindRuntimeVehicle(runtimeId);
			if (!record)
			{
				if (IsLivingEntity(entity))
					return false;
				RemoveAt(index);
				continue;
			}
			if (!IsDurableRecord(record))
			{
				if (IsLivingEntity(entity))
					return false;
				RemoveAt(index);
				continue;
			}
			if (!entity || !entity.GetWorld())
			{
				UpdateCargoPosition(state, runtimeId, record.m_vPosition);
				record.m_bDeleted = true;
				RemoveAt(index);
				continue;
			}

			record.m_vPosition = entity.GetOrigin();
			record.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(
				entity.GetYawPitchRoll());
			UpdateCargoPosition(state, runtimeId, record.m_vPosition);
			if (!IsLivingEntity(entity))
			{
				record.m_bDeleted = true;
				RemoveAt(index);
				continue;
			}
			record.m_bDeleted = false;
		}
		return true;
	}

	bool CleanupForNewCampaignReset(
		HST_CampaignState state,
		array<string> retainedRuntimeIds)
	{
		if (!state || !PrepareForCapture(state))
			return false;
		if (retainedRuntimeIds)
		{
			foreach (string retainedId : retainedRuntimeIds)
			{
				if (retainedId.IsEmpty() || m_aRuntimeIds.Find(retainedId) < 0)
					return false;
			}
		}

		// Player-safety preflight occurs before any deletion.
		for (int index = m_aRuntimeIds.Count() - 1; index >= 0; index--)
		{
			bool retained = retainedRuntimeIds
				&& retainedRuntimeIds.Find(m_aRuntimeIds[index]) >= 0;
			IEntity entity = m_aEntities[index];
			if (retained && (!entity || !HasLivingPlayerOccupant(entity)))
				return false;
			if (!retained && entity && HasLivingPlayerOccupant(entity))
				return false;
		}

		for (int index = m_aRuntimeIds.Count() - 1; index >= 0; index--)
		{
			string runtimeId = m_aRuntimeIds[index];
			if (retainedRuntimeIds && retainedRuntimeIds.Find(runtimeId) >= 0)
				continue;
			HST_RuntimeVehicleState record = state.FindRuntimeVehicle(runtimeId);
			if (record)
				record.m_bDeleted = true;
			IEntity entity = m_aEntities[index];
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			RemoveAt(index);
		}
		return true;
	}

	protected bool IsDurableRecord(HST_RuntimeVehicleState record)
	{
		if (!record || record.m_bDeleted || record.m_bDetached
			|| record.m_sVehicleRuntimeId.IsEmpty())
			return false;
		return record.m_sRuntimeKind == "loot_vehicle"
			|| record.m_sRuntimeKind == "field_vehicle"
			|| record.m_sRuntimeKind == "garage_redeploy";
	}

	protected void RemoveAt(int index)
	{
		if (index < 0 || index >= m_aEntities.Count()
			|| index >= m_aRuntimeIds.Count())
			return;
		m_aEntities.Remove(index);
		m_aRuntimeIds.Remove(index);
	}

	protected bool HasBindingCollision()
	{
		if (m_aEntities.Count() != m_aRuntimeIds.Count())
			return true;
		for (int first = 0; first < m_aRuntimeIds.Count(); first++)
		{
			for (int second = first + 1; second < m_aRuntimeIds.Count(); second++)
			{
				if (m_aRuntimeIds[first] == m_aRuntimeIds[second]
					|| (m_aEntities[first]
						&& m_aEntities[first] == m_aEntities[second]))
					return true;
			}
		}
		return false;
	}

	protected void UpdateCargoPosition(
		HST_CampaignState state,
		string runtimeId,
		vector position)
	{
		if (!state || runtimeId.IsEmpty())
			return;
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (cargoItem && cargoItem.m_sVehicleRuntimeId == runtimeId)
				cargoItem.m_vLastVehiclePosition = position;
		}
	}

	protected bool HasLivingPlayerOccupant(IEntity vehicleEntity)
	{
		if (!vehicleEntity || !IsLivingEntity(vehicleEntity))
			return false;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				playerEntity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
			if (!playerEntity || !IsLivingEntity(playerEntity))
				continue;
			if (playerEntity == vehicleEntity)
				return true;
			SCR_CompartmentAccessComponent access
				= SCR_CompartmentAccessComponent.Cast(
					playerEntity.FindComponent(SCR_CompartmentAccessComponent));
			if (access && access.IsInCompartment()
				&& access.GetVehicle() == vehicleEntity)
				return true;
		}
		return false;
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity || !entity.GetWorld())
			return false;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent controller
				= character.GetCharacterController();
			return controller
				&& controller.GetLifeState() != ECharacterLifeState.DEAD;
		}
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager
			|| damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}
}
