class HST_UIWorkspaceMetrics
{
	static const float BASE_WIDTH = 1920.0;
	static const float BASE_HEIGHT = 1080.0;

	static void GetRawWorkspaceSize(WorkspaceWidget workspace, out int width, out int height)
	{
		width = 1;
		height = 1;

		if (!workspace)
			return;

		width = Math.Max(1, Math.Round(workspace.GetWidth()));
		height = Math.Max(1, Math.Round(workspace.GetHeight()));
	}

	static void GetLayoutSize(WorkspaceWidget workspace, out int width, out int height)
	{
		width = 1;
		height = 1;

		if (!workspace)
			return;

		width = Math.Max(1, Math.Round(workspace.DPIUnscale(workspace.GetWidth())));
		height = Math.Max(1, Math.Round(workspace.DPIUnscale(workspace.GetHeight())));
	}

	static int LayoutToRawPx(WorkspaceWidget workspace, float value)
	{
		if (!workspace)
			return Math.Round(value);

		return Math.Round(workspace.DPIScale(value));
	}

	static int RawToLayoutPx(WorkspaceWidget workspace, float value)
	{
		if (!workspace)
			return Math.Round(value);

		return Math.Round(workspace.DPIUnscale(value));
	}

	static float GetScale(int width, int height, float minScale = 0.70, float maxScale = 1.15)
	{
		float sx = width / BASE_WIDTH;
		float sy = height / BASE_HEIGHT;
		return Math.Clamp(Math.Min(sx, sy), minScale, maxScale);
	}

	static int ScalePx(float value, float scale)
	{
		return Math.Round(value * scale);
	}

	static int ScaleFont(float value, float scale, int minSize = 9)
	{
		int scaled = Math.Round(value * scale);
		return ClampInt(scaled, minSize, Math.Round(value * 1.15));
	}

	static int ClampInt(int value, int minimum, int maximum)
	{
		if (maximum < minimum)
			return minimum;
		if (value < minimum)
			return minimum;
		if (value > maximum)
			return maximum;

		return value;
	}
}
