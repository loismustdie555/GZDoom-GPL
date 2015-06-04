DEFINE_ACTION_FUNCTION(AActor, A_WakeOracleSpectre)
{
	TThinkerIterator<AActor> it(NAME_AlienSpectre3);
	AActor *spectre = it.Next();

	if (spectre != NULL && spectre->health > 0 && self->target != spectre)
	{
		spectre->Sector->SoundTarget = spectre->LastHeard = self->LastHeard;
		spectre->target = self->target;
		spectre->SetState (spectre->SeeState);
	}
}
