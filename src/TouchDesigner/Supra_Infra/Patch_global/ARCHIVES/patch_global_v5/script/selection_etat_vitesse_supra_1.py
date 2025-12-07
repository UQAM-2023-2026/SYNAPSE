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
	chop_source = op('select4')
	etat_saturationM = int(chop_source['saturationM'][0])
	etat_saturationMax = int(chop_source['saturationMax'][0])
	if etat_saturationM > 1:
		op('select3').par.chop = 'saturationM'
	if etat_saturationMax > 1:
		op('select3').par.chop = 'saturationMax'
	return
	