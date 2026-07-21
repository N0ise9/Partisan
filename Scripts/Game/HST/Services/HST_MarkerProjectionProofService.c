#ifdef ENABLE_DIAG
class HST_MarkerProjectionProofReport
{
	bool m_bInitialSnapshotJIPExact;
	bool m_bStableRebuildExact;
	bool m_bOrderedDeltaRevisionExact;
	bool m_bDuplicateIdempotencyExact;
	bool m_bGapResyncExact;
	bool m_bReconnectSnapshotExact;
	bool m_bAcknowledgePruningExact;
	bool m_bMalformedFailClosedExact;
	bool m_bSchemaMigrationExact;
	string m_sInitialSnapshotJIPEvidence;
	string m_sStableRebuildEvidence;
	string m_sOrderedDeltaRevisionEvidence;
	string m_sDuplicateIdempotencyEvidence;
	string m_sGapResyncEvidence;
	string m_sReconnectSnapshotEvidence;
	string m_sAcknowledgePruningEvidence;
	string m_sMalformedFailClosedEvidence;
	string m_sSchemaMigrationEvidence;

	bool AllExact()
	{
		return m_bInitialSnapshotJIPExact
			&& m_bStableRebuildExact
			&& m_bOrderedDeltaRevisionExact
			&& m_bDuplicateIdempotencyExact
			&& m_bGapResyncExact
			&& m_bReconnectSnapshotExact
			&& m_bAcknowledgePruningExact
			&& m_bMalformedFailClosedExact
			&& m_bSchemaMigrationExact;
	}

	string BuildReport()
	{
		string report = string.Format(
			"marker projection proof | all exact %1 | snapshot/JIP %2 | stable %3 | deltas %4 | duplicate %5",
			AllExact(),
			m_bInitialSnapshotJIPExact,
			m_bStableRebuildExact,
			m_bOrderedDeltaRevisionExact,
			m_bDuplicateIdempotencyExact);
		report = report + string.Format(
			" | gap/resync %1 | reconnect %2 | ACK/prune %3 | malformed %4 | migration %5",
			m_bGapResyncExact,
			m_bReconnectSnapshotExact,
			m_bAcknowledgePruningExact,
			m_bMalformedFailClosedExact,
			m_bSchemaMigrationExact);
		return report;
	}
}

class HST_MarkerProjectionProofService
{
	static const int PRIMARY_PLAYER_ID = 6101;
	static const int JIP_PLAYER_ID = 6102;
	static const int SNAPSHOT_CATCHUP_PLAYER_ID = 6103;
	static const int RAPID_MUTATION_PLAYER_ID = 6104;
	static const int MULTI_PACKET_PLAYER_ID = 6105;
	static const int EPOCH_RESET_PLAYER_ID = 6106;
	static const int LOST_ACK_PLAYER_ID = 6107;
	static const int GAP_PLAYER_ID = 6199;

	HST_MarkerProjectionProofReport Run()
	{
		HST_MarkerProjectionProofReport report = new HST_MarkerProjectionProofReport();
		ProveStreamLifecycle(report);
		ProveDroppedDeltaResync(report);
		ProveMalformedPacketFailClosed(report);
		ProveSchemaMigration(report);
		return report;
	}

	protected void ProveStreamLifecycle(HST_MarkerProjectionProofReport report)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iMarkerProjectionEpoch = 7;
		state.m_iMarkerProjectionSequence = 2;
		HST_MapMarkerState alpha = BuildMarker(
			"proof_alpha",
			"Alpha | Headquarters",
			"zone_alpha",
			"location",
			"FIA",
			"1000 20 1200",
			1,
			1);
		HST_MapMarkerState bravo = BuildMarker(
			"proof_bravo",
			"Bravo % Depot",
			"zone_bravo",
			"location",
			"US",
			"2200 15 1800",
			1,
			2);
		state.m_aMapMarkers.Insert(alpha);
		state.m_aMapMarkers.Insert(bravo);

		HST_ClientProjectionService server = new HST_ClientProjectionService();
		server.Synchronize(state, "projection proof initial state");
		HST_ClientMarkerProjectionRegistry primary = new HST_ClientMarkerProjectionRegistry();
		HST_ClientMarkerProjectionRegistry jip = new HST_ClientMarkerProjectionRegistry();

		HST_MarkerProjectionDispatch primarySnapshot = server.RegisterReady(
			PRIMARY_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"projection proof primary ready");
		HST_MarkerProjectionApplyResult primarySnapshotResult = ApplyDispatch(primary, primarySnapshot);
		string primarySnapshotAck = Acknowledge(server, PRIMARY_PLAYER_ID, primarySnapshotResult);

		HST_MarkerProjectionDispatch jipSnapshot = server.RegisterReady(
			JIP_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"projection proof JIP ready");
		HST_MarkerProjectionApplyResult jipSnapshotResult = ApplyDispatch(jip, jipSnapshot);
		string jipSnapshotAck = Acknowledge(server, JIP_PLAYER_ID, jipSnapshotResult);

		bool primarySnapshotExact = primarySnapshot.m_bSnapshot
			&& primarySnapshot.HasPackets()
			&& primarySnapshotResult
			&& primarySnapshotResult.m_bAccepted
			&& primarySnapshotResult.m_bSnapshotCommitted
			&& primarySnapshotResult.m_bNeedsAcknowledge
			&& primarySnapshotAck.Contains("accepted:")
			&& RegistryMatchesServer(primary, server);
		bool jipSnapshotExact = jipSnapshot.m_bSnapshot
			&& jipSnapshot.HasPackets()
			&& jipSnapshotResult
			&& jipSnapshotResult.m_bAccepted
			&& jipSnapshotResult.m_bSnapshotCommitted
			&& jipSnapshotAck.Contains("accepted:")
			&& RegistryMatchesServer(jip, server)
			&& RegistriesEqual(primary, jip);
		string snapshotCatchupEvidence;
		string lostAcknowledgeEvidence;
		bool snapshotCatchupExact = ProveMutationWhileSnapshotPending(snapshotCatchupEvidence);
		bool lostAcknowledgeExact = ProveReadinessRecoversLostAcknowledge(lostAcknowledgeEvidence);
		report.m_bInitialSnapshotJIPExact = primarySnapshotExact && jipSnapshotExact && snapshotCatchupExact && lostAcknowledgeExact;
		report.m_sInitialSnapshotJIPEvidence = string.Format(
			"primary/JIP packets %1/%2 | records %3/%4 | watermark %5/%6 | hash equal %7",
			primarySnapshot.m_aPackets.Count(),
			jipSnapshot.m_aPackets.Count(),
			primary.GetRecordCount(),
			jip.GetRecordCount(),
			primary.GetWatermark(),
			jip.GetWatermark(),
			primary.GetRegistryHash() == jip.GetRegistryHash());
		report.m_sInitialSnapshotJIPEvidence = report.m_sInitialSnapshotJIPEvidence + " | " + snapshotCatchupEvidence + " | " + lostAcknowledgeEvidence;

		string stableHash = server.GetCurrentRegistryHash();
		int stableSequence = server.GetCurrentSequence();
		int stableJournal = server.GetJournalCount();
		server.Synchronize(state, "projection proof unchanged rebuild");
		HST_MarkerProjectionDispatch primaryStable = server.BuildPendingDispatch(PRIMARY_PLAYER_ID, "projection proof unchanged primary");
		HST_MarkerProjectionDispatch jipStable = server.BuildPendingDispatch(JIP_PLAYER_ID, "projection proof unchanged JIP");
		report.m_bStableRebuildExact = server.GetCurrentRegistryHash() == stableHash
			&& server.GetCurrentSequence() == stableSequence
			&& server.GetJournalCount() == stableJournal
			&& !primaryStable.HasPackets()
			&& !jipStable.HasPackets()
			&& alpha.m_iRevision == 1
			&& alpha.m_iStreamSequence == 1
			&& bravo.m_iRevision == 1
			&& bravo.m_iStreamSequence == 2;
		report.m_sStableRebuildEvidence = string.Format(
			"sequence %1 -> %2 | journal %3 -> %4 | pending packets %5/%6",
			stableSequence,
			server.GetCurrentSequence(),
			stableJournal,
			server.GetJournalCount(),
			primaryStable.m_aPackets.Count(),
			jipStable.m_aPackets.Count());

		HST_MapMarkerState charlie = BuildMarker(
			"proof_charlie",
			"Charlie Observation Post",
			"operation_charlie",
			"operation",
			"FIA",
			"3100 35 2750",
			1,
			3);
		state.m_aMapMarkers.Insert(charlie);
		state.m_iMarkerProjectionSequence = 3;
		server.Synchronize(state, "projection proof create");
		HST_MarkerProjectionDispatch createDispatch = server.BuildPendingDispatch(PRIMARY_PLAYER_ID, "projection proof create delivery");
		HST_MarkerProjectionApplyResult createResult = ApplyDispatch(primary, createDispatch);
		int duplicateWatermarkBefore = primary.GetWatermark();
		int duplicateCountBefore = primary.GetRecordCount();
		string duplicateHashBefore = primary.GetRegistryHash();
		HST_MarkerProjectionApplyResult duplicateResult;
		if (createDispatch.m_aPackets.Count() == 1)
			duplicateResult = primary.ApplyPacket(createDispatch.m_aPackets[0]);
		string createAck = Acknowledge(server, PRIMARY_PLAYER_ID, createResult);

		report.m_bDuplicateIdempotencyExact = duplicateResult
			&& duplicateResult.m_bAccepted
			&& !duplicateResult.m_bRegistryChanged
			&& duplicateResult.m_bNeedsAcknowledge
			&& !duplicateResult.m_bNeedsResync
			&& primary.GetWatermark() == duplicateWatermarkBefore
			&& primary.GetRecordCount() == duplicateCountBefore
			&& primary.GetRegistryHash() == duplicateHashBefore;
		report.m_sDuplicateIdempotencyEvidence = string.Format(
			"accepted/changed/resync %1/%2/%3 | watermark %4 -> %5 | records %6 -> %7",
			duplicateResult && duplicateResult.m_bAccepted,
			duplicateResult && duplicateResult.m_bRegistryChanged,
			duplicateResult && duplicateResult.m_bNeedsResync,
			duplicateWatermarkBefore,
			primary.GetWatermark(),
			duplicateCountBefore,
			primary.GetRecordCount());

		alpha.m_sLabel = "Alpha Headquarters Updated";
		alpha.m_iRevision = 2;
		alpha.m_iStreamSequence = 4;
		state.m_iMarkerProjectionSequence = 4;
		server.Synchronize(state, "projection proof update");
		HST_MarkerProjectionDispatch updateDispatch = server.BuildPendingDispatch(PRIMARY_PLAYER_ID, "projection proof update delivery");
		HST_MarkerProjectionApplyResult updateResult = ApplyDispatch(primary, updateDispatch);
		string updateAck = Acknowledge(server, PRIMARY_PLAYER_ID, updateResult);

		charlie.m_bVisible = false;
		charlie.m_bTombstone = true;
		charlie.m_iRevision = 2;
		charlie.m_iStreamSequence = 5;
		charlie.m_iTombstonedAtSecond = 900;
		state.m_iMarkerProjectionSequence = 5;
		server.Synchronize(state, "projection proof delete");
		HST_MarkerProjectionDispatch deleteDispatch = server.BuildPendingDispatch(PRIMARY_PLAYER_ID, "projection proof delete delivery");
		HST_MarkerProjectionApplyResult deleteResult = ApplyDispatch(primary, deleteDispatch);
		string deleteAck = Acknowledge(server, PRIMARY_PLAYER_ID, deleteResult);

		HST_MapMarkerState projectedAlpha = primary.FindRecord("proof_alpha");
		HST_MapMarkerState projectedCharlie = primary.FindRecord("proof_charlie");
		bool dispatchOrderExact = IsSingleSequenceDelta(createDispatch, 3)
			&& IsSingleSequenceDelta(updateDispatch, 4)
			&& IsSingleSequenceDelta(deleteDispatch, 5);
		bool revisionExact = projectedAlpha
			&& projectedAlpha.m_iRevision == 2
			&& projectedAlpha.m_iStreamSequence == 4
			&& projectedAlpha.m_sLabel == alpha.m_sLabel
			&& projectedCharlie
			&& projectedCharlie.m_iRevision == 2
			&& projectedCharlie.m_iStreamSequence == 5
			&& projectedCharlie.m_bTombstone
			&& !projectedCharlie.m_bVisible;
		bool mutationAcksExact = createResult
			&& createResult.m_bAccepted
			&& createAck.Contains("accepted:")
			&& updateResult
			&& updateResult.m_bAccepted
			&& updateAck.Contains("accepted:")
			&& deleteResult
			&& deleteResult.m_bAccepted
			&& deleteAck.Contains("accepted:");
		string rapidMutationEvidence;
		string multiPacketEvidence;
		bool rapidMutationExact = ProveRapidMutationInFlight(rapidMutationEvidence);
		bool multiPacketExact = ProveMultiPacketRuntimeAcknowledgement(multiPacketEvidence);
		report.m_bOrderedDeltaRevisionExact = dispatchOrderExact
			&& revisionExact
			&& mutationAcksExact
			&& rapidMutationExact
			&& multiPacketExact
			&& primary.GetWatermark() == 5
			&& primary.GetLiveRecordCount() == 2
			&& RegistryMatchesServer(primary, server);
		report.m_sOrderedDeltaRevisionEvidence = string.Format(
			"dispatch ranges %1-%2/%3-%4/%5-%6",
			createDispatch.m_iFromSequence,
			createDispatch.m_iToSequence,
			updateDispatch.m_iFromSequence,
			updateDispatch.m_iToSequence,
			deleteDispatch.m_iFromSequence,
			deleteDispatch.m_iToSequence);
		report.m_sOrderedDeltaRevisionEvidence = report.m_sOrderedDeltaRevisionEvidence + string.Format(
			" | alpha r%1@%2 | tombstone r%3@%4 | live %5",
			projectedAlpha && projectedAlpha.m_iRevision,
			projectedAlpha && projectedAlpha.m_iStreamSequence,
			projectedCharlie && projectedCharlie.m_iRevision,
			projectedCharlie && projectedCharlie.m_iStreamSequence,
			primary.GetLiveRecordCount());
		report.m_sOrderedDeltaRevisionEvidence = report.m_sOrderedDeltaRevisionEvidence + " | " + rapidMutationEvidence + " | " + multiPacketEvidence;

		int retainedJournal = server.GetJournalCount();
		HST_MarkerProjectionDispatch jipCatchup = server.BuildPendingDispatch(JIP_PLAYER_ID, "projection proof delayed JIP catchup");
		HST_MarkerProjectionApplyResult jipCatchupResult = ApplyDispatch(jip, jipCatchup);
		string jipCatchupAck = Acknowledge(server, JIP_PLAYER_ID, jipCatchupResult);
		report.m_bAcknowledgePruningExact = retainedJournal == 3
			&& jipCatchup.m_iFromSequence == 3
			&& jipCatchup.m_iToSequence == 5
			&& jipCatchupResult
			&& jipCatchupResult.m_bAccepted
			&& jipCatchupAck.Contains("accepted:")
			&& server.GetJournalCount() == 0
			&& RegistryMatchesServer(jip, server)
			&& RegistriesEqual(primary, jip);
		report.m_sAcknowledgePruningEvidence = string.Format(
			"retained before lagging ACK %1 | catchup %2-%3 in %4 packet(s) | journal after ACK %5",
			retainedJournal,
			jipCatchup.m_iFromSequence,
			jipCatchup.m_iToSequence,
			jipCatchup.m_aPackets.Count(),
			server.GetJournalCount());

		server.RemovePlayer(PRIMARY_PLAYER_ID);
		HST_ClientMarkerProjectionRegistry reconnect = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionDispatch reconnectSnapshot = server.RegisterReady(
			PRIMARY_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"projection proof reconnect");
		HST_MarkerProjectionApplyResult reconnectResult = ApplyDispatch(reconnect, reconnectSnapshot);
		string reconnectAck = Acknowledge(server, PRIMARY_PLAYER_ID, reconnectResult);
		string epochResetEvidence;
		bool epochResetExact = ProveEpochResetAfterHighWatermark(epochResetEvidence);
		report.m_bReconnectSnapshotExact = reconnectSnapshot.m_bSnapshot
			&& reconnectSnapshot.HasPackets()
			&& reconnectResult
			&& reconnectResult.m_bAccepted
			&& reconnectResult.m_bSnapshotCommitted
			&& reconnectAck.Contains("accepted:")
			&& RegistryMatchesServer(reconnect, server)
			&& RegistriesEqual(primary, reconnect)
			&& epochResetExact;
		report.m_sReconnectSnapshotEvidence = string.Format(
			"snapshot %1 | packets %2 | epoch/watermark %3/%4 | records/live %5/%6",
			reconnectSnapshot.m_sSnapshotId,
			reconnectSnapshot.m_aPackets.Count(),
			reconnect.GetEpoch(),
			 reconnect.GetWatermark(),
			 reconnect.GetRecordCount(),
			 reconnect.GetLiveRecordCount());
		report.m_sReconnectSnapshotEvidence = report.m_sReconnectSnapshotEvidence + " | " + epochResetEvidence;
	}

	protected bool ProveMutationWhileSnapshotPending(out string evidence)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iMarkerProjectionEpoch = 17;
		state.m_iMarkerProjectionSequence = 1;
		HST_MapMarkerState marker = BuildMarker("snapshot_catchup", "Snapshot Catch-up", "catchup_zone", "location", "FIA", "400 10 450", 1, 1);
		state.m_aMapMarkers.Insert(marker);

		HST_ClientProjectionService server = new HST_ClientProjectionService();
		server.Synchronize(state, "snapshot catch-up initial");
		HST_ClientMarkerProjectionRegistry client = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionDispatch snapshot = server.RegisterReady(
			SNAPSHOT_CATCHUP_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"snapshot catch-up ready");

		marker.m_sLabel = "Snapshot Catch-up Updated";
		marker.m_iRevision = 2;
		marker.m_iStreamSequence = 2;
		state.m_iMarkerProjectionSequence = 2;
		server.Synchronize(state, "snapshot catch-up mutation");
		HST_MarkerProjectionDispatch blocked = server.BuildPendingDispatch(SNAPSHOT_CATCHUP_PLAYER_ID, "snapshot still pending");
		HST_MarkerProjectionApplyResult snapshotResult = ApplyDispatch(client, snapshot);
		string snapshotAck = Acknowledge(server, SNAPSHOT_CATCHUP_PLAYER_ID, snapshotResult);
		HST_MarkerProjectionDispatch catchup = server.BuildPendingDispatch(SNAPSHOT_CATCHUP_PLAYER_ID, "post-snapshot ACK catch-up");
		HST_MarkerProjectionApplyResult catchupResult = ApplyDispatch(client, catchup);
		string catchupAck = Acknowledge(server, SNAPSHOT_CATCHUP_PLAYER_ID, catchupResult);

		bool exact = snapshot.m_bSnapshot
			&& !blocked.HasPackets()
			&& snapshotAck.Contains("accepted:")
			&& IsSingleSequenceDelta(catchup, 2)
			&& catchupAck.Contains("accepted:")
			&& RegistryMatchesServer(client, server);
		evidence = string.Format(
			"snapshot-pending mutation | blocked packets %1 | catch-up %2-%3 | final watermark %4 | exact %5",
			blocked.m_aPackets.Count(),
			catchup.m_iFromSequence,
			catchup.m_iToSequence,
			client.GetWatermark(),
			exact);
		return exact;
	}

	protected bool ProveReadinessRecoversLostAcknowledge(out string evidence)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iMarkerProjectionEpoch = 18;
		state.m_iMarkerProjectionSequence = 1;
		HST_MapMarkerState marker = BuildMarker("lost_ack", "Lost ACK", "lost_ack_zone", "location", "FIA", "550 10 575", 1, 1);
		state.m_aMapMarkers.Insert(marker);
		HST_ClientProjectionService server = new HST_ClientProjectionService();
		server.Synchronize(state, "lost ACK initial");
		HST_ClientMarkerProjectionRegistry client = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionDispatch snapshot = server.RegisterReady(
			LOST_ACK_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"lost ACK ready");
		HST_MarkerProjectionApplyResult snapshotResult = ApplyDispatch(client, snapshot);

		marker.m_sLabel = "Lost ACK Updated";
		marker.m_iRevision = 2;
		marker.m_iStreamSequence = 2;
		state.m_iMarkerProjectionSequence = 2;
		server.Synchronize(state, "lost ACK mutation");
		HST_MarkerProjectionDispatch heartbeatRecovery = server.RegisterReady(
			LOST_ACK_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			client.GetEpoch(),
			client.GetWatermark(),
			client.GetRegistryHash(),
			"lost ACK heartbeat");
		HST_MarkerProjectionApplyResult recoveryResult = ApplyDispatch(client, heartbeatRecovery);
		string recoveryAck = Acknowledge(server, LOST_ACK_PLAYER_ID, recoveryResult);

		bool exact = snapshotResult
			&& snapshotResult.m_bSnapshotCommitted
			&& IsSingleSequenceDelta(heartbeatRecovery, 2)
			&& recoveryAck.Contains("accepted:")
			&& RegistryMatchesServer(client, server);
		evidence = string.Format(
			"lost-ACK heartbeat | recovered %1-%2 | final watermark %3 | exact %4",
			heartbeatRecovery.m_iFromSequence,
			heartbeatRecovery.m_iToSequence,
			client.GetWatermark(),
			exact);
		return exact;
	}

	protected bool ProveRapidMutationInFlight(out string evidence)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iMarkerProjectionEpoch = 19;
		state.m_iMarkerProjectionSequence = 1;
		HST_MapMarkerState marker = BuildMarker("rapid_mutation", "Rapid Mutation", "rapid_zone", "operation", "US", "700 10 750", 1, 1);
		state.m_aMapMarkers.Insert(marker);

		HST_ClientProjectionService server = new HST_ClientProjectionService();
		server.Synchronize(state, "rapid mutation initial");
		HST_ClientMarkerProjectionRegistry client = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionDispatch snapshot = server.RegisterReady(
			RAPID_MUTATION_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"rapid mutation ready");
		HST_MarkerProjectionApplyResult snapshotResult = ApplyDispatch(client, snapshot);
		string snapshotAck = Acknowledge(server, RAPID_MUTATION_PLAYER_ID, snapshotResult);

		marker.m_sLabel = "Rapid Mutation Two";
		marker.m_iRevision = 2;
		marker.m_iStreamSequence = 2;
		state.m_iMarkerProjectionSequence = 2;
		server.Synchronize(state, "rapid mutation two");
		HST_MarkerProjectionDispatch first = server.BuildPendingDispatch(RAPID_MUTATION_PLAYER_ID, "rapid mutation first delivery");

		marker.m_sLabel = "Rapid Mutation Three";
		marker.m_iRevision = 3;
		marker.m_iStreamSequence = 3;
		state.m_iMarkerProjectionSequence = 3;
		server.Synchronize(state, "rapid mutation three");
		HST_MarkerProjectionDispatch overlap = server.BuildPendingDispatch(RAPID_MUTATION_PLAYER_ID, "rapid mutation overlapping delivery");
		HST_MarkerProjectionApplyResult firstResult = ApplyDispatch(client, first);
		string firstAck = Acknowledge(server, RAPID_MUTATION_PLAYER_ID, firstResult);
		HST_MarkerProjectionDispatch second = server.BuildPendingDispatch(RAPID_MUTATION_PLAYER_ID, "rapid mutation post-ACK delivery");
		HST_MarkerProjectionApplyResult secondResult = ApplyDispatch(client, second);
		string secondAck = Acknowledge(server, RAPID_MUTATION_PLAYER_ID, secondResult);

		bool exact = snapshotAck.Contains("accepted:")
			&& IsSingleSequenceDelta(first, 2)
			&& !overlap.HasPackets()
			&& firstAck.Contains("accepted:")
			&& IsSingleSequenceDelta(second, 3)
			&& secondAck.Contains("accepted:")
			&& RegistryMatchesServer(client, server);
		evidence = string.Format(
			"rapid in-flight mutation | first %1-%2 | overlap packets %3 | second %4-%5 | exact %6",
			first.m_iFromSequence,
			first.m_iToSequence,
			overlap.m_aPackets.Count(),
			second.m_iFromSequence,
			second.m_iToSequence,
			exact);
		return exact;
	}

	protected bool ProveMultiPacketRuntimeAcknowledgement(out string evidence)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iMarkerProjectionEpoch = 23;
		state.m_iMarkerProjectionSequence = 1;
		state.m_aMapMarkers.Insert(BuildMarker("multi_base", "Multi Base", "multi_zone", "location", "FIA", "900 10 950", 1, 1));
		HST_ClientProjectionService server = new HST_ClientProjectionService();
		server.Synchronize(state, "multi-packet initial");
		HST_ClientMarkerProjectionRegistry client = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionDispatch snapshot = server.RegisterReady(
			MULTI_PACKET_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"multi-packet ready");
		HST_MarkerProjectionApplyResult snapshotResult = ApplyDispatch(client, snapshot);
		string snapshotAck = Acknowledge(server, MULTI_PACKET_PLAYER_ID, snapshotResult);

		for (int i; i < HST_MarkerProjectionCodec.MAX_RECORDS_PER_PACKET + 1; i++)
		{
			int sequence = i + 2;
			state.m_aMapMarkers.Insert(BuildMarker(
				string.Format("multi_event_%1", i),
				string.Format("Multi Event %1", i),
				"multi_zone",
				"operation",
				"US",
				Vector(1000 + i * 5, 10, 1000 + i * 5),
				1,
				sequence));
		}
		state.m_iMarkerProjectionSequence = HST_MarkerProjectionCodec.MAX_RECORDS_PER_PACKET + 2;
		server.Synchronize(state, "multi-packet burst");
		HST_MarkerProjectionDispatch burst = server.BuildPendingDispatch(MULTI_PACKET_PLAYER_ID, "multi-packet delivery");
		HST_MarkerProjectionApplyResult firstResult;
		HST_MarkerProjectionApplyResult finalResult;
		if (burst.m_aPackets.Count() == 2)
		{
			firstResult = client.ApplyPacket(burst.m_aPackets[0]);
			finalResult = client.ApplyPacket(burst.m_aPackets[1]);
		}
		string finalAck = Acknowledge(server, MULTI_PACKET_PLAYER_ID, finalResult);
		bool exact = snapshotAck.Contains("accepted:")
			&& burst.m_aPackets.Count() == 2
			&& firstResult
			&& firstResult.m_bAccepted
			&& !firstResult.m_bNeedsAcknowledge
			&& finalResult
			&& finalResult.m_bAccepted
			&& finalResult.m_bNeedsAcknowledge
			&& finalAck.Contains("accepted:")
			&& RegistryMatchesServer(client, server);
		evidence = string.Format(
			"multi-packet runtime ACK | packets %1 | intermediate/final ACK %2/%3 | final watermark %4 | exact %5",
			burst.m_aPackets.Count(),
			firstResult && firstResult.m_bNeedsAcknowledge,
			finalResult && finalResult.m_bNeedsAcknowledge,
			client.GetWatermark(),
			exact);
		return exact;
	}

	protected bool ProveEpochResetAfterHighWatermark(out string evidence)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iMarkerProjectionEpoch = 31;
		state.m_iMarkerProjectionSequence = 40;
		state.m_aMapMarkers.Insert(BuildMarker("old_epoch_marker", "Old Epoch", "old_zone", "location", "US", "1200 10 1250", 5, 40));
		HST_ClientProjectionService server = new HST_ClientProjectionService();
		server.Synchronize(state, "epoch reset high watermark");
		HST_ClientMarkerProjectionRegistry client = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionDispatch oldSnapshot = server.RegisterReady(
			EPOCH_RESET_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"epoch reset ready");
		HST_MarkerProjectionApplyResult oldResult = ApplyDispatch(client, oldSnapshot);
		string oldAck = Acknowledge(server, EPOCH_RESET_PLAYER_ID, oldResult);

		state.m_iMarkerProjectionEpoch = 32;
		state.m_iMarkerProjectionSequence = 2;
		state.m_aMapMarkers.Clear();
		state.m_aMapMarkers.Insert(BuildMarker("new_epoch_marker", "New Epoch", "new_zone", "location", "FIA", "1300 10 1350", 1, 2));
		server.Synchronize(state, "epoch reset lower watermark");
		HST_MarkerProjectionDispatch resetSnapshot = server.BuildPendingDispatch(EPOCH_RESET_PLAYER_ID, "epoch reset delivery");
		HST_MarkerProjectionApplyResult resetResult = ApplyDispatch(client, resetSnapshot);
		string resetAck = Acknowledge(server, EPOCH_RESET_PLAYER_ID, resetResult);

		bool exact = oldAck.Contains("accepted:")
			&& resetSnapshot.m_bSnapshot
			&& resetResult
			&& resetResult.m_bSnapshotCommitted
			&& resetAck.Contains("accepted:")
			&& client.GetEpoch() == 32
			&& client.GetWatermark() == 2
			&& !client.FindRecord("old_epoch_marker")
			&& client.FindRecord("new_epoch_marker")
			&& RegistryMatchesServer(client, server);
		evidence = string.Format(
			"epoch reset | old/new watermark 40/%1 | epoch %2 | reset ACK %3 | exact %4",
			client.GetWatermark(),
			client.GetEpoch(),
			resetAck.Contains("accepted:"),
			exact);
		return exact;
	}

	protected void ProveDroppedDeltaResync(HST_MarkerProjectionProofReport report)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iMarkerProjectionEpoch = 11;
		state.m_iMarkerProjectionSequence = 1;
		state.m_aMapMarkers.Insert(BuildMarker(
			"gap_base",
			"Gap Base",
			"gap_zone",
			"location",
			"FIA",
			"500 10 500",
			1,
			1));

		HST_ClientProjectionService server = new HST_ClientProjectionService();
		server.Synchronize(state, "gap proof initial state");
		HST_ClientMarkerProjectionRegistry client = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionDispatch snapshot = server.RegisterReady(
			GAP_PLAYER_ID,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			0,
			0,
			"",
			"gap proof ready");
		HST_MarkerProjectionApplyResult snapshotResult = ApplyDispatch(client, snapshot);
		string snapshotAck = Acknowledge(server, GAP_PLAYER_ID, snapshotResult);
		bool initialReady = snapshotResult
			&& snapshotResult.m_bAccepted
			&& snapshotAck.Contains("accepted:")
			&& client.GetWatermark() == 1;

		for (int i; i < HST_MarkerProjectionCodec.MAX_RECORDS_PER_PACKET + 1; i++)
		{
			int sequence = i + 2;
			state.m_aMapMarkers.Insert(BuildMarker(
				string.Format("gap_event_%1", i),
				string.Format("Gap Event %1", i),
				"gap_zone",
				"operation",
				"US",
				Vector(600 + i * 10, 10, 650 + i * 5),
				1,
				sequence));
		}
		state.m_iMarkerProjectionSequence = HST_MarkerProjectionCodec.MAX_RECORDS_PER_PACKET + 2;
		server.Synchronize(state, "gap proof burst");
		HST_MarkerProjectionDispatch burst = server.BuildPendingDispatch(GAP_PLAYER_ID, "gap proof burst delivery");
		HST_MarkerProjectionPacket droppedFirstFollower;
		HST_MarkerProjectionApplyResult gapResult;
		if (burst.m_aPackets.Count() >= 2)
		{
			droppedFirstFollower = HST_MarkerProjectionCodec.DecodePacket(burst.m_aPackets[1]);
			gapResult = client.ApplyPacket(burst.m_aPackets[1]);
		}
		bool gapDetected = initialReady
			&& burst.m_aPackets.Count() == 2
			&& droppedFirstFollower
			&& droppedFirstFollower.m_iFromSequence > client.GetWatermark() + 1
			&& gapResult
			&& !gapResult.m_bAccepted
			&& gapResult.m_bNeedsResync
			&& client.GetWatermark() == 1
			&& client.GetRecordCount() == 1;

		server.RequestResync(GAP_PLAYER_ID, "gap proof dropped first delta packet");
		HST_MarkerProjectionDispatch resync = server.BuildPendingDispatch(GAP_PLAYER_ID, "gap proof resync");
		int stagedEpochBefore = client.GetEpoch();
		int stagedWatermarkBefore = client.GetWatermark();
		int stagedRecordsBefore = client.GetRecordCount();
		string stagedHashBefore = client.GetRegistryHash();
		int intermediateEpoch;
		int intermediateWatermark;
		int intermediateRecords;
		string intermediateHash;
		HST_MarkerProjectionApplyResult firstSnapshotChunk;
		HST_MarkerProjectionApplyResult resyncResult;
		if (resync.m_aPackets.Count() == 2)
		{
			firstSnapshotChunk = client.ApplyPacket(resync.m_aPackets[0]);
			intermediateEpoch = client.GetEpoch();
			intermediateWatermark = client.GetWatermark();
			intermediateRecords = client.GetRecordCount();
			intermediateHash = client.GetRegistryHash();
			resyncResult = client.ApplyPacket(resync.m_aPackets[1]);
		}
		bool stagingAtomic = firstSnapshotChunk
			&& firstSnapshotChunk.m_bAccepted
			&& !firstSnapshotChunk.m_bSnapshotCommitted
			&& !firstSnapshotChunk.m_bRegistryChanged
			&& intermediateEpoch == stagedEpochBefore
			&& intermediateWatermark == stagedWatermarkBefore
			&& intermediateRecords == stagedRecordsBefore
			&& intermediateHash == stagedHashBefore;
		string resyncAck = Acknowledge(server, GAP_PLAYER_ID, resyncResult);
		bool resynchronized = resync.m_bSnapshot
			&& resync.m_aPackets.Count() == 2
			&& stagingAtomic
			&& resyncResult
			&& resyncResult.m_bAccepted
			&& resyncResult.m_bSnapshotCommitted
			&& resyncAck.Contains("accepted:")
			&& RegistryMatchesServer(client, server);
		report.m_bGapResyncExact = gapDetected && resynchronized;
		report.m_sGapResyncEvidence = string.Format(
			"burst packets %1 | skipped to %2 from watermark 1 | rejected/resync %3/%4 | snapshot packets %5 atomic %6 | final watermark %7",
			burst.m_aPackets.Count(),
			droppedFirstFollower && droppedFirstFollower.m_iFromSequence,
			gapResult && !gapResult.m_bAccepted,
			gapResult && gapResult.m_bNeedsResync,
			resync.m_aPackets.Count(),
			stagingAtomic,
			client.GetWatermark());
	}

	protected void ProveMalformedPacketFailClosed(HST_MarkerProjectionProofReport report)
	{
		map<string, ref HST_MapMarkerState> records = new map<string, ref HST_MapMarkerState>();
		HST_MapMarkerState marker = BuildMarker(
			"malformed_guard",
			"Malformed Guard",
			"guard_zone",
			"location",
			"FIA",
			"800 12 900",
			1,
			1);
		records.Set(marker.m_sMarkerId, marker);
		string registryHash = HST_MarkerProjectionCodec.BuildLiveRegistryHash(records);
		array<string> snapshotPackets = {};
		bool snapshotBuilt = HST_MarkerProjectionCodec.BuildSnapshotPackets(records, 13, "malformed_guard_snapshot", 1, registryHash, snapshotPackets);
		HST_ClientMarkerProjectionRegistry client = new HST_ClientMarkerProjectionRegistry();
		HST_MarkerProjectionApplyResult readyResult;
		if (!snapshotPackets.IsEmpty())
			readyResult = client.ApplyPacket(snapshotPackets[0]);
		int watermarkBefore = client.GetWatermark();
		int recordCountBefore = client.GetRecordCount();
		string hashBefore = client.GetRegistryHash();
		string malformedPayload = "HST_MARKER_DELTA|1|13|2|2|1|invalid\nM|2|0";
		HST_MarkerProjectionPacket decoded = HST_MarkerProjectionCodec.DecodePacket(malformedPayload);
		string invalidBooleanLine = HST_MarkerProjectionCodec.EncodeRecord(marker);
		invalidBooleanLine.Replace("M|1|1|0|", "M|1|1|invalid|");
		HST_MarkerProjectionPacket invalidBooleanDecoded = HST_MarkerProjectionCodec.DecodePacket("HST_MARKER_DELTA|1|13|1|1|1|\n" + invalidBooleanLine);
		string oversizedLabel;
		string oversizedChunk = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
		while (oversizedLabel.Length() <= HST_MarkerProjectionCodec.MAX_PACKET_CHARACTERS)
			oversizedLabel = oversizedLabel + oversizedChunk;
		HST_MapMarkerState oversizedMarker = BuildMarker("oversized", oversizedLabel, "oversized_zone", "location", "FIA", "1 1 1", 1, 1);
		map<string, ref HST_MapMarkerState> oversizedRecords = new map<string, ref HST_MapMarkerState>();
		oversizedRecords.Set(oversizedMarker.m_sMarkerId, oversizedMarker);
		array<string> oversizedSnapshotPackets = {};
		bool oversizedSnapshotBuilt = HST_MarkerProjectionCodec.BuildSnapshotPackets(oversizedRecords, 13, "oversized_snapshot", 1, "oversized", oversizedSnapshotPackets);
		array<ref HST_MapMarkerState> oversizedEvents = { oversizedMarker };
		array<string> oversizedDeltaPackets = {};
		bool oversizedDeltaBuilt = HST_MarkerProjectionCodec.BuildDeltaPackets(oversizedEvents, 13, "oversized", oversizedDeltaPackets);
		HST_MarkerProjectionApplyResult malformedResult = client.ApplyPacket(malformedPayload);
		bool decoderExact = readyResult
			&& snapshotBuilt
			&& readyResult.m_bAccepted
			&& readyResult.m_bSnapshotCommitted
			&& !decoded
			&& !invalidBooleanDecoded;
		bool boundsExact = !oversizedSnapshotBuilt
			&& oversizedSnapshotPackets.IsEmpty()
			&& !oversizedDeltaBuilt
			&& oversizedDeltaPackets.IsEmpty();
		bool registryStable = malformedResult
			&& !malformedResult.m_bAccepted
			&& malformedResult.m_bNeedsResync
			&& client.IsReady()
			&& client.GetWatermark() == watermarkBefore
			&& client.GetRecordCount() == recordCountBefore
			&& client.GetRegistryHash() == hashBefore;
		report.m_bMalformedFailClosedExact = decoderExact && boundsExact && registryStable;
		report.m_sMalformedFailClosedEvidence = string.Format(
			"truncated/strict-bool decode null %1/%2 | oversize snapshot/delta rejected %3/%4 | accepted/resync %5/%6",
			!decoded,
			!invalidBooleanDecoded,
			!oversizedSnapshotBuilt,
			!oversizedDeltaBuilt,
			malformedResult && malformedResult.m_bAccepted,
			malformedResult && malformedResult.m_bNeedsResync);
		report.m_sMalformedFailClosedEvidence = report.m_sMalformedFailClosedEvidence + string.Format(
			" | watermark %1 -> %2 | records %3 -> %4 | hash stable %5",
			watermarkBefore,
			client.GetWatermark(),
			recordCountBefore,
			client.GetRecordCount(),
			hashBefore == client.GetRegistryHash());
	}

	protected void ProveSchemaMigration(HST_MarkerProjectionProofReport report)
	{
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.m_iSchemaVersion = 60;
		saveData.m_iMarkerProjectionEpoch = 0;
		saveData.m_iMarkerProjectionSequence = 77;
		saveData.m_iCampaignSeed = 6160;
		HST_MapMarkerState legacy = new HST_MapMarkerState();
		legacy.m_sMarkerId = "legacy_projection_row";
		legacy.m_sLabel = "Legacy Projection Row";
		legacy.m_bVisible = true;
		saveData.m_aMapMarkers.Insert(legacy);

		HST_MarkerProjectionSaveValidationService validation = new HST_MarkerProjectionSaveValidationService();
		validation.Normalize(saveData, 60);
		int migrationEventsAfterFirstPass = CountCampaignEvents(saveData, HST_MarkerProjectionSaveValidationService.MIGRATION_EVENT_ID);
		validation.Normalize(saveData, 60);
		int migrationEventsAfterSecondPass = CountCampaignEvents(saveData, HST_MarkerProjectionSaveValidationService.MIGRATION_EVENT_ID);
		report.m_bSchemaMigrationExact = saveData.m_aMapMarkers.IsEmpty()
			&& saveData.m_iMarkerProjectionEpoch == 1
			&& saveData.m_iMarkerProjectionSequence == 0
			&& saveData.m_iCampaignSeed == 6160
			&& migrationEventsAfterFirstPass == 1
			&& migrationEventsAfterSecondPass == 1;
		report.m_sSchemaMigrationEvidence = string.Format(
			"rows %1 | epoch/sequence %2/%3 | migration events first/second %4/%5 | seed %6",
			saveData.m_aMapMarkers.Count(),
			saveData.m_iMarkerProjectionEpoch,
			saveData.m_iMarkerProjectionSequence,
			migrationEventsAfterFirstPass,
			migrationEventsAfterSecondPass,
			saveData.m_iCampaignSeed);
	}

	protected HST_MapMarkerState BuildMarker(
		string markerId,
		string label,
		string linkedId,
		string category,
		string ownerFactionKey,
		vector position,
		int revision,
		int sequence)
	{
		HST_MapMarkerState marker = new HST_MapMarkerState();
		marker.m_sMarkerId = markerId;
		marker.m_sLinkedId = linkedId;
		marker.m_sLabel = label;
		marker.m_sCallsign = markerId;
		marker.m_sCategory = category;
		marker.m_sOwnerFactionKey = ownerFactionKey;
		marker.m_sIconHint = category;
		marker.m_sColorHint = ownerFactionKey;
		marker.m_sTextColorHint = "white";
		marker.m_sStyleHint = "proof";
		marker.m_vPosition = position;
		marker.m_bVisible = true;
		marker.m_bRuntimeNative = false;
		marker.m_iRevision = revision;
		marker.m_iStreamSequence = sequence;
		return marker;
	}

	protected HST_MarkerProjectionApplyResult ApplyDispatch(
		HST_ClientMarkerProjectionRegistry registry,
		HST_MarkerProjectionDispatch dispatch)
	{
		if (!registry || !dispatch || !dispatch.HasPackets())
			return null;

		HST_MarkerProjectionApplyResult result;
		foreach (string payload : dispatch.m_aPackets)
		{
			result = registry.ApplyPacket(payload);
			if (!result || !result.m_bAccepted)
				return result;
		}
		return result;
	}

	protected string Acknowledge(
		HST_ClientProjectionService server,
		int playerId,
		HST_MarkerProjectionApplyResult result)
	{
		if (!server || !result || !result.m_bNeedsAcknowledge)
			return "not acknowledged";

		return server.Acknowledge(
			playerId,
			HST_MarkerProjectionCodec.PROTOCOL_VERSION,
			result.m_iEpoch,
			result.m_sSnapshotId,
			result.m_iAcknowledgeSequence,
			result.m_sRegistryHash);
	}

	protected bool RegistryMatchesServer(
		HST_ClientMarkerProjectionRegistry registry,
		HST_ClientProjectionService server)
	{
		if (!registry || !server || !registry.IsReady())
			return false;
		if (registry.GetEpoch() != server.GetEpoch()
			|| registry.GetWatermark() != server.GetCurrentSequence()
			|| registry.GetRegistryHash() != server.GetCurrentRegistryHash())
			return false;

		map<string, ref HST_MapMarkerState> serverRecords = new map<string, ref HST_MapMarkerState>();
		map<string, ref HST_MapMarkerState> clientRecords = new map<string, ref HST_MapMarkerState>();
		server.CopyCurrentRecords(serverRecords);
		registry.CopyRecords(clientRecords);
		return RecordMapsEqual(serverRecords, clientRecords);
	}

	protected bool RegistriesEqual(
		HST_ClientMarkerProjectionRegistry left,
		HST_ClientMarkerProjectionRegistry right)
	{
		if (!left || !right)
			return false;
		if (left.IsReady() != right.IsReady()
			|| left.GetEpoch() != right.GetEpoch()
			|| left.GetWatermark() != right.GetWatermark()
			|| left.GetRegistryHash() != right.GetRegistryHash())
			return false;

		map<string, ref HST_MapMarkerState> leftRecords = new map<string, ref HST_MapMarkerState>();
		map<string, ref HST_MapMarkerState> rightRecords = new map<string, ref HST_MapMarkerState>();
		left.CopyRecords(leftRecords);
		right.CopyRecords(rightRecords);
		return RecordMapsEqual(leftRecords, rightRecords);
	}

	protected bool RecordMapsEqual(
		map<string, ref HST_MapMarkerState> left,
		map<string, ref HST_MapMarkerState> right)
	{
		if (!left || !right || left.Count() != right.Count())
			return false;
		foreach (string markerId, HST_MapMarkerState leftMarker : left)
		{
			HST_MapMarkerState rightMarker = right.Get(markerId);
			if (!rightMarker || !HST_MarkerProjectionCodec.TransportEquals(leftMarker, rightMarker))
				return false;
		}
		return true;
	}

	protected bool IsSingleSequenceDelta(HST_MarkerProjectionDispatch dispatch, int sequence)
	{
		if (!dispatch || dispatch.m_bSnapshot || dispatch.m_aPackets.Count() != 1)
			return false;
		if (dispatch.m_iFromSequence != sequence || dispatch.m_iToSequence != sequence)
			return false;
		HST_MarkerProjectionPacket packet = HST_MarkerProjectionCodec.DecodePacket(dispatch.m_aPackets[0]);
		return packet
			&& packet.m_sKind == HST_MarkerProjectionCodec.DELTA_HEADER
			&& packet.m_iFromSequence == sequence
			&& packet.m_iToSequence == sequence
			&& packet.m_aRecords.Count() == 1
			&& packet.m_aRecords[0].m_iStreamSequence == sequence;
	}

	protected int CountCampaignEvents(HST_CampaignSaveData saveData, string eventId)
	{
		if (!saveData)
			return 0;
		int count;
		foreach (HST_CampaignEventState eventState : saveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				count++;
		}
		return count;
	}
}
#endif
