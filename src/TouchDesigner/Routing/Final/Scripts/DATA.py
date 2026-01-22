"""
CHOP Execute DAT

me - this DAT

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""

def onOffToOn(channel: Channel, sampleIndex: int, val: float, 
			  prev: float):
	"""
	Called when a channel changes from 0 to non-zero.

	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def whileOn(channel: Channel, sampleIndex: int, val: float, 
			prev: float):
	"""
	Called every frame while a channel is non-zero.

	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def onOnToOff(channel: Channel, sampleIndex: int, val: float, 
			  prev: float):
	"""
	Called when a channel changes from non-zero to 0.

	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def whileOff(channel: Channel, sampleIndex: int, val: float, 
			 prev: float):
	"""
	Called every frame while a channel is 0.

	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def onValueChange(channel: Channel, sampleIndex: int, val: float, 
				  prev: float):
	toggles = op('par1')      # CHOP avec les 0/1

	# --- STATE ---
	if op('par1')[1] == True:
		op('State_Supra').bypass = 0
	
	else :
		op('State_Supra').bypass = 1
	
	if op('par1')[2] == True:
		op('State_Infra').bypass = 0

	else :
		op('State_Infra').bypass = 1
	
	if op('par1')[4] == True:
		op('Total').bypass = 0

	else :
		op('Total').bypass = 1
	
	if op('par1')[5] == True:
		op('EnergieSupra').bypass = 0
	
	else :
		op('EnergieSupra').bypass = 1

	if op('par1')[6] == True:
		op('EnergieInfra').bypass = 0
	
	else :
		op('EnergieInfra').bypass = 1

	if op('par1')[8] == True:
		op('Noeud1_RzEnergie').bypass = 0

	else :
		op('Noeud1_RzEnergie').bypass = 1

	if op('par1')[9] == True:
		op('Noeud1_RzID').bypass = 0

	else :
		op('Noeud1_RzID').bypass = 1

	if op('par1')[10] == True:
		op('Noeud1_ID').bypass = 0

	else :
		op('Noeud1_ID').bypass = 1

	if op('par1')[11] == True:
		op('Noeud1_Level').bypass = 0

	else :
		op('Noeud1_Level').bypass = 1

	if op('par1')[13] == True:
		op('Noeud2_RzEnergie').bypass = 0

	else :
		op('Noeud2_RzEnergie').bypass = 1

	if op('par1')[14] == True:
		op('Noeud2_RzID').bypass = 0

	else :
		op('Noeud2_RzID').bypass = 1

	if op('par1')[15] == True:
		op('Noeud2_ID').bypass = 0

	else :
		op('Noeud2_ID').bypass = 1

	if op('par1')[16] == True:
		op('Noeud2_Level').bypass = 0

	else :
		op('Noeud2_Level').bypass = 1

	if op('par1')[18] == True:
		op('Breathing_Supra').bypass = 0

	else :
		op('Breathing_Supra').bypass = 1

	if op('par1')[19] == True:
		op('Breathing_Infra').bypass = 0
	else :
		op('Breathing_Infra').bypass = 1

	if op('par1')[21] == True:
		op('RZ1_Target_Node').bypass = 0

	else :
		op('RZ1_Target_Node').bypass = 1

	if op('par1')[22] == True:
		op('RZ1_Connected').bypass = 0

	else :
		op('RZ1_Connected').bypass = 1

	if op('par1')[24] == True:
		op('RZ2_Connected').bypass = 0

	else :
		op('RZ2_Connected').bypass = 1

	if op('par1')[25] == True:
		op('RZ2_Target_Node').bypass = 0

	else :
		op('RZ2_Target_Node').bypass = 1

	if op('par1')[27] == True:
		op('Prct_Supra').bypass = 0

	else :
		op('Prct_Supra').bypass = 1

	if op('par1')[28] == True:
		op('Prct_Infra').bypass = 0

	else :
		op('Prct_Infra').bypass = 1

	return

