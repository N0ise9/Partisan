class HST_ProfilePathService
{
	protected static const int FILE_COMPARE_CHUNK_SIZE = 4096;
	protected static const int ARCHIVE_SUFFIX_LIMIT = 10000;
	protected static const int MIGRATION_FAILED = 0;
	protected static const int MIGRATION_COPIED = 1;
	protected static const int MIGRATION_ARCHIVED = 2;
	protected static const int MIGRATION_DEDUPLICATED = 3;

	static const string PROFILE_DIRECTORY = "$profile:Partisan";
	static const string SETTINGS_FILE = "$profile:Partisan/HST_Settings.json";
	static const string CAMPAIGN_SAVE_FILE = "$profile:Partisan/HST_CampaignSaveData.json";
	static const string CAMPAIGN_RECOVERY_FILE = "$profile:Partisan/HST_CampaignSaveData.recovery.json";
	static const string VISUAL_SETTINGS_FILE = "$profile:Partisan/HST_LoadoutEditorSettings.json";
	static const string LOADOUT_DIRECTORY = "$profile:Partisan/loadouts";
	static const string LOADOUT_DIRECTORY_V2 = "$profile:Partisan/loadouts/v2";
	static const string DEBUG_DIRECTORY = "$profile:Partisan/debug";
	static const string LEGACY_ARCHIVE_DIRECTORY = "$profile:Partisan/legacy-profile-archive";
	static const string LEGACY_DIRECTORY_CONFLICT_ARCHIVE = "$profile:Partisan/legacy-profile-archive/directory-conflicts";
	static const string MIGRATION_STAGING_DIRECTORY = "$profile:Partisan/.profile-migration-staging";

	static const string LEGACY_PROFILE_DIRECTORY = "$profile:h-istasi";
	static const string LEGACY_SETTINGS_FILE = "$profile:h-istasi/HST_Settings.json";
	static const string LEGACY_CAMPAIGN_SAVE_FILE = "$profile:h-istasi/HST_CampaignSaveData.json";
	static const string LEGACY_CAMPAIGN_RECOVERY_FILE = "$profile:h-istasi/HST_CampaignSaveData.recovery.json";
	static const string LEGACY_VISUAL_SETTINGS_FILE = "$profile:h-istasi/HST_LoadoutEditorSettings.json";
	static const string LEGACY_LOADOUT_DIRECTORY = "$profile:h-istasi/loadouts";
	static const string LEGACY_LOADOUT_DIRECTORY_V2 = "$profile:h-istasi/loadouts/v2";

	protected static bool s_bLegacyMigrationInProgress;
	protected static bool s_bLegacyMigrationComplete;

	static bool MigrateLegacyProfileTree()
	{
		if (s_bLegacyMigrationComplete)
			return true;
		if (s_bLegacyMigrationInProgress)
			return false;

		if (!FileIO.FileExists(LEGACY_PROFILE_DIRECTORY))
		{
			s_bLegacyMigrationComplete = true;
			return true;
		}

		s_bLegacyMigrationInProgress = true;
		bool migrated = MigrateLegacyProfileTreeInternal();
		s_bLegacyMigrationInProgress = false;
		if (migrated)
			s_bLegacyMigrationComplete = true;

		return migrated;
	}

	protected static bool MigrateLegacyProfileTreeInternal()
	{
		if (!FileIO.MakeDirectory(PROFILE_DIRECTORY))
		{
			Print("Partisan profile migration | canonical profile directory is unavailable; retired data was left untouched", LogLevel.WARNING);
			return false;
		}

		array<ref SCR_FileInfo> entries = SCR_FileIOHelper.GetDirectoryContent(LEGACY_PROFILE_DIRECTORY);
		if (!entries)
		{
			Print("Partisan profile migration | retired profile tree could not be enumerated; retired data was left untouched", LogLevel.WARNING);
			return false;
		}

		array<string> files = {};
		array<string> directories = {};
		array<string> retainedSourceDirectories = {};
		foreach (SCR_FileInfo entry : entries)
		{
			if (!entry)
				continue;

			string sourcePath = NormalizePath(entry.m_sFilePath);
			string relativePath;
			if (!TryGetLegacyRelativePath(sourcePath, relativePath))
			{
				Print("Partisan profile migration | ignored an entry outside the retired profile root", LogLevel.WARNING);
				continue;
			}

			if ((entry.m_eAttributes | FileAttribute.DIRECTORY) == entry.m_eAttributes)
				directories.Insert(sourcePath);
			else
				files.Insert(sourcePath);
		}

		files.Sort();
		directories.Sort();

		// Preserve empty directory structure where it does not conflict with a
		// canonical file. Structural conflicts are handled per file below by
		// moving their contents into the archive subtree.
		foreach (string legacyDirectory : directories)
		{
			string relativeDirectory;
			if (!TryGetLegacyRelativePath(legacyDirectory, relativeDirectory))
				continue;

			string canonicalDirectory = PROFILE_DIRECTORY + "/" + relativeDirectory;
			if (!FileIO.MakeDirectory(canonicalDirectory))
			{
				string conflictArchiveDirectory
					= LEGACY_DIRECTORY_CONFLICT_ARCHIVE + "/" + relativeDirectory;
				if (FileIO.MakeDirectory(conflictArchiveDirectory))
				{
					Print(
						"Partisan profile migration | archived a legacy directory conflict: "
							+ relativeDirectory,
						LogLevel.WARNING);
				}
				else
				{
					retainedSourceDirectories.Insert(legacyDirectory);
					Print(
						"Partisan profile migration | retained an unverified legacy directory: "
							+ relativeDirectory,
						LogLevel.WARNING);
				}
			}
		}

		int copiedCount;
		int archivedCount;
		int deduplicatedCount;
		int failedFileCount;
		foreach (string sourceFile : files)
		{
			string relativeFile;
			if (!TryGetLegacyRelativePath(sourceFile, relativeFile))
			{
				failedFileCount++;
				continue;
			}

			int disposition = MigrateLegacyFile(sourceFile, relativeFile);
			if (disposition == MIGRATION_COPIED)
				copiedCount++;
			else if (disposition == MIGRATION_ARCHIVED)
				archivedCount++;
			else if (disposition == MIGRATION_DEDUPLICATED)
				deduplicatedCount++;
			else
			{
				failedFileCount++;
				Print("Partisan profile migration | retained an unverified legacy file: " + relativeFile, LogLevel.WARNING);
			}
		}

		SortDirectoriesDeepestFirst(directories);
		int retainedDirectoryCount;
		foreach (string sourceDirectory : directories)
		{
			if (!FileIO.FileExists(sourceDirectory))
				continue;
			if (retainedSourceDirectories.Contains(sourceDirectory))
			{
				retainedDirectoryCount++;
				continue;
			}

			if (!FileIO.DeleteFile(sourceDirectory) && FileIO.FileExists(sourceDirectory))
				retainedDirectoryCount++;
		}

		bool retiredTreeRemoved = !FileIO.FileExists(LEGACY_PROFILE_DIRECTORY);
		if (!retiredTreeRemoved)
		{
			if (!FileIO.DeleteFile(LEGACY_PROFILE_DIRECTORY) && FileIO.FileExists(LEGACY_PROFILE_DIRECTORY))
				retainedDirectoryCount++;

			retiredTreeRemoved = !FileIO.FileExists(LEGACY_PROFILE_DIRECTORY);
		}

		string summary = string.Format(
			"Partisan profile migration | copied %1 | archived conflicts %2 | deduplicated %3 | failed files %4 | retained directories %5 | retired tree removed %6",
			copiedCount,
			archivedCount,
			deduplicatedCount,
			failedFileCount,
			retainedDirectoryCount,
			retiredTreeRemoved);
		if (failedFileCount == 0 && retiredTreeRemoved)
			Print(summary);
		else
			Print(summary, LogLevel.WARNING);

		return failedFileCount == 0 && retiredTreeRemoved;
	}

	protected static int MigrateLegacyFile(string sourceFile, string relativeFile)
	{
		if (!FileIO.FileExists(sourceFile))
			return MIGRATION_DEDUPLICATED;

		string canonicalFile = PROFILE_DIRECTORY + "/" + relativeFile;
		if (FileIO.FileExists(canonicalFile))
		{
			if (FilesMatchExact(sourceFile, canonicalFile))
			{
				if (DeleteVerifiedSourceFile(sourceFile, canonicalFile))
					return MIGRATION_DEDUPLICATED;

				return MIGRATION_FAILED;
			}
			if (canonicalFile == CAMPAIGN_SAVE_FILE
				|| canonicalFile == CAMPAIGN_RECOVERY_FILE)
			{
				// Two differing campaign saves are competing authorities, not an
				// ordinary profile-file conflict. Retain the retired source so the
				// startup authority guard fails closed instead of archiving it and
				// silently proceeding with the canonical campaign.
				return MIGRATION_FAILED;
			}

			return ArchiveLegacyFile(sourceFile, relativeFile);
		}

		string canonicalDirectory = NormalizePath(FilePath.StripFileName(canonicalFile));
		if (canonicalDirectory.IsEmpty() || !FileIO.MakeDirectory(canonicalDirectory))
			return ArchiveLegacyFile(sourceFile, relativeFile);

		if (CopyNewFileAndVerify(sourceFile, canonicalFile))
		{
			if (DeleteVerifiedSourceFile(sourceFile, canonicalFile))
				return MIGRATION_COPIED;

			return MIGRATION_FAILED;
		}

		// A copy or content-verification failure always leaves the source in the
		// retired tree. A later startup can retry without trusting partial output.
		return MIGRATION_FAILED;
	}

	protected static int ArchiveLegacyFile(string sourceFile, string relativeFile)
	{
		if (!FileIO.MakeDirectory(LEGACY_ARCHIVE_DIRECTORY))
			return MIGRATION_FAILED;

		string archiveBase = LEGACY_ARCHIVE_DIRECTORY + "/" + relativeFile;
		for (int suffix; suffix <= ARCHIVE_SUFFIX_LIMIT; suffix++)
		{
			string archiveFile = archiveBase;
			if (suffix > 0)
				archiveFile += ".legacy-" + suffix.ToString();

			if (FileIO.FileExists(archiveFile))
			{
				if (!FilesMatchExact(sourceFile, archiveFile))
					continue;

				if (DeleteVerifiedSourceFile(sourceFile, archiveFile))
					return MIGRATION_DEDUPLICATED;

				return MIGRATION_FAILED;
			}

			if (!CopyNewFileAndVerify(sourceFile, archiveFile))
				return MIGRATION_FAILED;

			if (DeleteVerifiedSourceFile(sourceFile, archiveFile))
				return MIGRATION_ARCHIVED;

			return MIGRATION_FAILED;
		}

		return MIGRATION_FAILED;
	}

	protected static bool CopyNewFileAndVerify(string sourceFile, string destinationFile)
	{
		if (!FileIO.FileExists(sourceFile) || FileIO.FileExists(destinationFile))
			return false;

		string destinationDirectory = NormalizePath(FilePath.StripFileName(destinationFile));
		if (destinationDirectory.IsEmpty() || !FileIO.MakeDirectory(destinationDirectory))
			return false;

		string stagingFile;
		if (!PrepareVerifiedMigrationStage(
			sourceFile,
			destinationFile,
			stagingFile))
			return false;

		// Re-check after staging. A canonical file that appeared concurrently is
		// never deleted or overwritten by failed-copy cleanup. Matching content is
		// already a successful deduplication; differing content remains canonical
		// and leaves the legacy source available for conflict archival on retry.
		if (FileIO.FileExists(destinationFile))
		{
			bool existingExact = FilesMatchExact(sourceFile, destinationFile);
			DeleteVerifiedMigrationStage(stagingFile, sourceFile);
			return existingExact;
		}

		FileIO.CopyFile(stagingFile, destinationFile);
		bool promotedExact = FileIO.FileExists(destinationFile)
			&& FilesMatchExact(sourceFile, destinationFile);
		DeleteVerifiedMigrationStage(stagingFile, sourceFile);
		return promotedExact;
	}

	protected static bool PrepareVerifiedMigrationStage(
		string sourceFile,
		string destinationFile,
		out string stagingFile)
	{
		stagingFile = "";
		if (!FileIO.MakeDirectory(MIGRATION_STAGING_DIRECTORY))
			return false;

		string stagingBase = MIGRATION_STAGING_DIRECTORY
			+ "/migration_" + sourceFile.Hash().ToString()
			+ "_" + destinationFile.Hash().ToString();
		for (int suffix; suffix <= ARCHIVE_SUFFIX_LIMIT; suffix++)
		{
			string candidate = stagingBase + ".tmp";
			if (suffix > 0)
				candidate = candidate + "-" + suffix.ToString();

			if (FileIO.FileExists(candidate))
			{
				if (FilesMatchExact(sourceFile, candidate))
				{
					stagingFile = candidate;
					return true;
				}
				continue;
			}

			FileIO.CopyFile(sourceFile, candidate);
			if (!FileIO.FileExists(candidate)
				|| !FilesMatchExact(sourceFile, candidate))
				continue;

			stagingFile = candidate;
			return true;
		}
		return false;
	}

	protected static void DeleteVerifiedMigrationStage(
		string stagingFile,
		string sourceFile)
	{
		if (stagingFile.IsEmpty() || !FileIO.FileExists(stagingFile)
			|| !FileIO.FileExists(sourceFile)
			|| !FilesMatchExact(sourceFile, stagingFile))
			return;
		FileIO.DeleteFile(stagingFile);
		FileIO.DeleteFile(MIGRATION_STAGING_DIRECTORY);
	}

	protected static bool FilesMatchExact(string firstFile, string secondFile)
	{
		FileHandle firstHandle = FileIO.OpenFile(firstFile, FileMode.READ);
		FileHandle secondHandle = FileIO.OpenFile(secondFile, FileMode.READ);
		if (!firstHandle || !secondHandle)
		{
			if (firstHandle)
				firstHandle.Close();
			if (secondHandle)
				secondHandle.Close();
			return false;
		}

		int firstLength = firstHandle.GetLength();
		int secondLength = secondHandle.GetLength();
		if (firstLength != secondLength)
		{
			firstHandle.Close();
			secondHandle.Close();
			return false;
		}

		array<int> firstBytes = {};
		array<int> secondBytes = {};
		int comparedBytes;
		bool exact = true;
		while (comparedBytes < firstLength)
		{
			int chunkSize = Math.Min(FILE_COMPARE_CHUNK_SIZE, firstLength - comparedBytes);
			int firstRead = firstHandle.ReadArray(firstBytes, 1, chunkSize);
			int secondRead = secondHandle.ReadArray(secondBytes, 1, chunkSize);
			if (firstRead != chunkSize || secondRead != chunkSize || firstBytes.Count() != secondBytes.Count())
			{
				exact = false;
				break;
			}

			for (int i; i < firstBytes.Count(); i++)
			{
				if (firstBytes[i] == secondBytes[i])
					continue;

				exact = false;
				break;
			}

			if (!exact)
				break;

			comparedBytes += chunkSize;
		}

		firstHandle.Close();
		secondHandle.Close();
		return exact;
	}

	protected static bool DeleteVerifiedSourceFile(string sourceFile, string verifiedDestinationFile)
	{
		if (!FileIO.FileExists(sourceFile))
			return true;
		if (!FileIO.FileExists(verifiedDestinationFile) || !FilesMatchExact(sourceFile, verifiedDestinationFile))
			return false;

		if (!FileIO.DeleteFile(sourceFile) && FileIO.FileExists(sourceFile))
			return false;

		return !FileIO.FileExists(sourceFile);
	}

	protected static bool TryGetLegacyRelativePath(string sourcePath, out string relativePath)
	{
		relativePath = "";
		string legacyRoot = NormalizePath(LEGACY_PROFILE_DIRECTORY);
		sourcePath = NormalizePath(sourcePath);
		string prefix = legacyRoot + "/";
		if (!sourcePath.StartsWith(prefix))
			return false;

		relativePath = sourcePath.Substring(prefix.Length(), sourcePath.Length() - prefix.Length());
		if (relativePath.IsEmpty()
			|| relativePath == ".."
			|| relativePath.StartsWith("../")
			|| relativePath.Contains("/../"))
		{
			relativePath = "";
			return false;
		}

		return true;
	}

	protected static string NormalizePath(string path)
	{
		int asciiBackslash = 92;
		string backslash = asciiBackslash.AsciiToString();
		path.Replace(backslash, "/");
		while (path.Contains("//"))
			path.Replace("//", "/");

		while (path.Length() > 0 && path.EndsWith("/"))
			path = path.Substring(0, path.Length() - 1);

		return path;
	}

	protected static void SortDirectoriesDeepestFirst(array<string> directories)
	{
		if (!directories)
			return;

		for (int i; i < directories.Count(); i++)
		{
			for (int j = i + 1; j < directories.Count(); j++)
			{
				if (directories[j].Length() <= directories[i].Length())
					continue;

				string swap = directories[i];
				directories[i] = directories[j];
				directories[j] = swap;
			}
		}
	}

	static string ResolveReadableFile(string currentPath, string legacyPath)
	{
		// Every settings/save reader enters through this resolver, so a client
		// component that initializes before the request bridge still migrates the
		// retired tree before it consumes profile data.
		MigrateLegacyProfileTree();

		if (FileIO.FileExists(currentPath))
			return currentPath;
		if (FileIO.FileExists(legacyPath))
			return legacyPath;
		return currentPath;
	}

	static bool IsLegacyPath(string path)
	{
		return !path.IsEmpty()
			&& (path == LEGACY_PROFILE_DIRECTORY
				|| path.StartsWith(LEGACY_PROFILE_DIRECTORY + "/"));
	}

	static bool HasUnresolvedLegacyCampaignAuthority()
	{
		bool canonicalConflict = FileIO.FileExists(LEGACY_CAMPAIGN_SAVE_FILE)
			&& (!FileIO.FileExists(CAMPAIGN_SAVE_FILE)
				|| !FilesMatchExact(
				LEGACY_CAMPAIGN_SAVE_FILE,
				CAMPAIGN_SAVE_FILE));
		bool recoveryConflict
			= FileIO.FileExists(LEGACY_CAMPAIGN_RECOVERY_FILE)
				&& (!FileIO.FileExists(CAMPAIGN_RECOVERY_FILE)
					|| !FilesMatchExact(
						LEGACY_CAMPAIGN_RECOVERY_FILE,
						CAMPAIGN_RECOVERY_FILE));
		return canonicalConflict || recoveryConflict;
	}

	static void EnsureProfileDirectory()
	{
		FileIO.MakeDirectory(PROFILE_DIRECTORY);
	}

	static void EnsureLoadoutDirectories()
	{
		EnsureProfileDirectory();
		FileIO.MakeDirectory(LOADOUT_DIRECTORY);
		FileIO.MakeDirectory(LOADOUT_DIRECTORY_V2);
	}

	static void EnsureDebugDirectory()
	{
		EnsureProfileDirectory();
		FileIO.MakeDirectory(DEBUG_DIRECTORY);
	}
}
