# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.

def onOffToOn(channel, sampleIndex, val, prev):
	return

def whileOn(channel, sampleIndex, val, prev):
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
    if val == 1 and prev == 0:
        targets = op('targets')  # ton Constant CHOP
        if channel.name == 'kx':
            targets.par.value0 = 0.08
            targets.par.value1 = 1
        elif channel.name == 'kz':
            targets.par.value0 = 1
            targets.par.value1 = 1
            
    elif channel.name == 'rhizome':
        targets = op('targets')  # ton Constant CHOP

        if (op('Etat_Infra_out')[N1_RZID] == 0 & op('Etat_Infra_out')[N1_Level] == 0) or (op('Etat_Infra_out')[N2_RZID] == 0 & op('Etat_Infra_out')[N2_Level] == 0) # rhizome branché 
            targets.par.value0 = 0.08
            targets.par.value1 = 1

        if (op('Etat_Infra_out')[N1_RZID] == 1 & op('Etat_Infra_out')[N1_Level] == 0) or (op('Etat_Infra_out')[N2_RZID] == 1 & op('Etat_Infra_out')[N2_Level] == 0) # rhizome branché
            targets.par.value0 = 1
            targets.par.value1 = 1
    return
	