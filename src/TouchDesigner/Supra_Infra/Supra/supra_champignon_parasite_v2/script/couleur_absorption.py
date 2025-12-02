# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.

absorptionB = 15
absorptionM = 45
absorptionMax = 50

def onOffToOn(channel, sampleIndex, val, prev):
	return

def whileOn(channel, sampleIndex, val, prev):
	#op('switch1').par.index = '0'
	if val <= absorptionB:
		op('constant2').par.const0value = '0.8'
	elif absorptionB < val <= absorptionM:
		op('constant2').par.const0value = '1'
	elif absorptionM < absorptionMax:
		op('constant2').par.const0value = '1.3'
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	return
	