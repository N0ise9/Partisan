modded class SCR_StaminaBlurEffect
{
	protected static bool s_bHSTInfiniteStaminaVisualSuppressed;

	static void SetHistasiInfiniteStaminaVisualSuppressed(bool suppressed)
	{
		s_bHSTInfiniteStaminaVisualSuppressed = suppressed;
	}

	override protected void FindStaminaValues()
	{
		if (s_bHSTInfiniteStaminaVisualSuppressed)
		{
			ClearHistasiStaminaVisuals();
			return;
		}

		super.FindStaminaValues();
	}

	override protected void StaminaEffects(bool repeat)
	{
		if (s_bHSTInfiniteStaminaVisualSuppressed)
		{
			ClearHistasiStaminaVisuals();
			return;
		}

		super.StaminaEffects(repeat);
	}

	override protected void ClearStaminaEffect(bool repeat)
	{
		if (s_bHSTInfiniteStaminaVisualSuppressed)
			repeat = false;

		super.ClearStaminaEffect(repeat);
	}

	override protected void DisplayOnResumed()
	{
		if (s_bHSTInfiniteStaminaVisualSuppressed)
		{
			ClearHistasiStaminaVisuals();
			return;
		}

		super.DisplayOnResumed();
	}

	override protected void UpdateEffect(float timeSlice)
	{
		if (s_bHSTInfiniteStaminaVisualSuppressed)
		{
			ClearHistasiStaminaVisuals();
			return;
		}

		super.UpdateEffect(timeSlice);
	}

	protected void ClearHistasiStaminaVisuals()
	{
		GetGame().GetCallqueue().Remove(StaminaEffects);
		GetGame().GetCallqueue().Remove(ClearStaminaEffect);
		super.DisplayOnSuspended();
		super.ClearStaminaEffect(false);
		ClearEffects();
	}
}
