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

	static int LayoutToRawPx(WorkspaceWidget workspace, float value)
	{
		if (!workspace)
			return Math.Round(value);

		return Math.Round(workspace.DPIScale(value));
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

	static int GetLayoutWidth(WorkspaceWidget workspace)
	{
		if (!workspace)
			return 1;

		return Math.Max(1, Math.Round(workspace.DPIUnscale(workspace.GetWidth())));
	}

	static int GetLayoutHeight(WorkspaceWidget workspace)
	{
		if (!workspace)
			return 1;

		return Math.Max(1, Math.Round(workspace.DPIUnscale(workspace.GetHeight())));
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

	static int CenteredLeft(int layoutWidth, int width)
	{
		return Math.Max(0, (layoutWidth - width) / 2);
	}

	static int CenteredTop(int layoutHeight, int height)
	{
		return Math.Max(0, (layoutHeight - height) / 2);
	}

	static int ClampLeft(int left, int width, int layoutWidth, int margin = 0)
	{
		int maxLeft = Math.Max(margin, layoutWidth - width - margin);
		return ClampInt(left, margin, maxLeft);
	}

	static int ClampTop(int top, int height, int layoutHeight, int margin = 0)
	{
		int maxTop = Math.Max(margin, layoutHeight - height - margin);
		return ClampInt(top, margin, maxTop);
	}

	static bool IsPointInsideRect(int x, int y, int left, int top, int width, int height)
	{
		if (width <= 0 || height <= 0)
			return false;
		if (x < left || y < top)
			return false;
		if (x > left + width || y > top + height)
			return false;

		return true;
	}

	static void GetNativePointCandidates(WorkspaceWidget workspace, float nativeX, float nativeY, out int rawX, out int rawY, out int unscaledX, out int unscaledY)
	{
		rawX = Math.Round(nativeX);
		rawY = Math.Round(nativeY);
		unscaledX = rawX;
		unscaledY = rawY;
		if (!workspace)
			return;

		unscaledX = Math.Round(workspace.DPIUnscale(nativeX));
		unscaledY = Math.Round(workspace.DPIUnscale(nativeY));
	}

	static bool IsEitherNativeCandidateInsideRect(WorkspaceWidget workspace, float nativeX, float nativeY, int left, int top, int width, int height)
	{
		int rawX;
		int rawY;
		int unscaledX;
		int unscaledY;
		GetNativePointCandidates(workspace, nativeX, nativeY, rawX, rawY, unscaledX, unscaledY);

		return IsPointInsideRect(rawX, rawY, left, top, width, height)
			|| IsPointInsideRect(unscaledX, unscaledY, left, top, width, height);
	}
}
