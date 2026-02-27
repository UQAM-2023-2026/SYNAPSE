Thresholds = op('Thresholds')

absorption_threshold = Thresholds['Absorption'].eval()
equilibre_threshold = Thresholds['Equilibre'].eval()
appel_threshold = Thresholds['Appel'].eval()
extreme_threshold = Thresholds['Extreme'].eval()

def get_state_index(ratio):
	if ratio == 0:
		return 0 # NUL
	elif ratio < absorption_threshold:
		return 0 # ABSORPTION
	elif ratio < equilibre_threshold:
		return 1 # EQUILIBRE
	else:
		return 2 # SATURATION


def get_special_state(prct):
	if prct == 0:
		return 0 # NUL
	elif prct < appel_threshold:
		return 1 # APPEL
	elif prct > extreme_threshold:
		return 2 # EXTREME
	else:
		return 0 # NUL (No special event)


def onValueChange(channel: Channel, sampleIndex: int, val: float, 
                  prev: float):

	systeme = op('Systeme')

	prct_supra = systeme.par.const0value 
	prct_infra = systeme.par.const2value
	
	total = systeme['total'].eval()

	# Calcul des nouveaux etats
	if total == 0:
		new_state_infra = 0
		new_state_supra = 0
		new_special_state_infra = 0
		new_special_state_supra = 0
	else:
		new_state_infra = get_state_index(prct_infra)
		new_state_supra = get_state_index(prct_supra)

		new_special_state_infra = get_special_state(prct_infra)
		new_special_state_supra = get_special_state(prct_supra)

	# Mise a jour des operateurs d'etat
	op('State_Infra').par.const0value = new_state_infra
	op('State_Supra').par.const0value = new_state_supra

	op('Special_Infra').par.const0value = new_special_state_infra
	op('Special_Supra').par.const0value = new_special_state_supra


	
	return
