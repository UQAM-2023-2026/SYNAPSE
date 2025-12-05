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
	chop_source = op('select2')
	etat_absorptionM = int(chop_source['absorptionM'][0])
	etat_absorptionMax = int(chop_source['absorptionMax'][0])
	if etat_absorptionM > 0:
		op('select3').par.renameto = 'absorptionM'
	if etat_absorptionMax > 0:
		op('select3').par.renameto = 'absorptionMax'
	return
	