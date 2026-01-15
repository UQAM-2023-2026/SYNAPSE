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
	#op('switch1').par.index = '1'
	op('constant1').par.const0value = '1'
	op('constant5').par.const0value = '1.3'
	op('constant4').par.const0value = '1'
	op('constant6').par.const0value = '0'
	op('select3').par.chop = 'etat_absorption3'
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	return
	