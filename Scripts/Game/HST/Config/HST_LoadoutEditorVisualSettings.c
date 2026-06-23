class HST_LoadoutEditorVisualSettings
{
	static const int SCHEMA_VERSION = 1;

	int m_iSchemaVersion = SCHEMA_VERSION;
	float m_fPreviewLightLV = 17.0;
	int m_iPanelPreset;
	int m_iAccentPreset;
	int m_iRowPreset;
	int m_iWorldPreset;

	void Normalize()
	{
		if (m_iSchemaVersion <= 0)
			m_iSchemaVersion = SCHEMA_VERSION;

		m_fPreviewLightLV = Math.Clamp(m_fPreviewLightLV, 8.0, 24.0);
		m_iPanelPreset = ClampPreset(m_iPanelPreset, 3);
		m_iAccentPreset = ClampPreset(m_iAccentPreset, 4);
		m_iRowPreset = ClampPreset(m_iRowPreset, 3);
		m_iWorldPreset = ClampPreset(m_iWorldPreset, 3);
	}

	protected int ClampPreset(int value, int count)
	{
		if (count <= 0)
			return 0;
		if (value < 0)
			return 0;
		if (value >= count)
			return count - 1;

		return value;
	}
}

class HST_LoadoutEditorVisualSettingsService
{
	static const string SETTINGS_DIRECTORY = "$profile:h-istasi";
	static const string SETTINGS_FILE = "$profile:h-istasi/HST_LoadoutEditorSettings.json";

	HST_LoadoutEditorVisualSettings LoadOrCreate()
	{
		HST_LoadoutEditorVisualSettings settings = new HST_LoadoutEditorVisualSettings();
		if (!FileIO.FileExists(SETTINGS_FILE))
		{
			FileIO.MakeDirectory(SETTINGS_DIRECTORY);
			Save(settings);
			return settings;
		}

		array<string> lines = ReadLines(SETTINGS_FILE);
		foreach (string line : lines)
		{
			ApplyInt(line, "schemaVersion", settings.m_iSchemaVersion);
			ApplyFloat(line, "previewLightLV", settings.m_fPreviewLightLV);
			ApplyInt(line, "panelPreset", settings.m_iPanelPreset);
			ApplyInt(line, "accentPreset", settings.m_iAccentPreset);
			ApplyInt(line, "rowPreset", settings.m_iRowPreset);
			ApplyInt(line, "worldPreset", settings.m_iWorldPreset);
		}

		settings.Normalize();
		if (settings.m_iSchemaVersion < HST_LoadoutEditorVisualSettings.SCHEMA_VERSION)
		{
			settings.m_iSchemaVersion = HST_LoadoutEditorVisualSettings.SCHEMA_VERSION;
			Save(settings);
		}

		return settings;
	}

	bool Save(notnull HST_LoadoutEditorVisualSettings settings)
	{
		settings.Normalize();
		FileIO.MakeDirectory(SETTINGS_DIRECTORY);

		array<string> lines = {};
		lines.Insert("{");
		lines.Insert(string.Format("  \"schemaVersion\": %1,", settings.m_iSchemaVersion));
		lines.Insert(string.Format("  \"previewLightLV\": %1,", settings.m_fPreviewLightLV));
		lines.Insert(string.Format("  \"panelPreset\": %1,", settings.m_iPanelPreset));
		lines.Insert(string.Format("  \"accentPreset\": %1,", settings.m_iAccentPreset));
		lines.Insert(string.Format("  \"rowPreset\": %1,", settings.m_iRowPreset));
		lines.Insert(string.Format("  \"worldPreset\": %1", settings.m_iWorldPreset));
		lines.Insert("}");
		return WriteLines(SETTINGS_FILE, lines);
	}

	protected void ApplyInt(string line, string key, out int target)
	{
		if (!LineHasKey(line, key))
			return;

		target = ExtractValue(line).ToInt();
	}

	protected void ApplyFloat(string line, string key, out float target)
	{
		if (!LineHasKey(line, key))
			return;

		target = ExtractValue(line).ToFloat();
	}

	protected bool LineHasKey(string line, string key)
	{
		return line.Contains("\"" + key + "\"");
	}

	protected string ExtractValue(string line)
	{
		int colon = line.IndexOf(":");
		if (colon < 0)
			return "";

		string value = line.Substring(colon + 1, line.Length() - colon - 1);
		value.Replace(",", "");
		value.Replace("\"", "");
		value = value.Trim();
		return value;
	}

	protected array<string> ReadLines(string fileName)
	{
		array<string> lines = {};
		FileHandle file = FileIO.OpenFile(fileName, FileMode.READ);
		if (!file)
			return lines;

		string line;
		while (file.ReadLine(line) >= 0)
			lines.Insert(line);

		file.Close();
		return lines;
	}

	protected bool WriteLines(string fileName, notnull array<string> lines)
	{
		FileHandle file = FileIO.OpenFile(fileName, FileMode.WRITE);
		if (!file)
			return false;

		foreach (string line : lines)
			file.WriteLine(line);

		file.Close();
		return true;
	}
}
