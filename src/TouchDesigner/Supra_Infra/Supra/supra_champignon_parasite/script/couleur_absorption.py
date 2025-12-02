# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.

symbioseB = 15
symbioseM = 45
symbioseMax = 50
absorptionMSupra = 20
absorptionMInfra = 20
saturationMSupra = 80
saturationMInfra = 80
saturationMaxSupra = 100
saturationMaxInfra = 100
absorptionMaxSupra = 0
absorptionMaxInfra = 0
Idle = 0.8


def onOffToOn(channel, sampleIndex, val, prev):
	return

def whileOn(channel, sampleIndex, val, prev):
	#op('switch1').par.index = '0'
	if val <= symbioseB:
		op('constant1').par.const0value = '0.8'
		op('constant1').par.name0 = 'symbioseB'
	elif symbioseB < val <= symbioseM:
		op('constant1').par.const0value = '1'
		op('constant1').par.name0 = 'symbioseM'
	elif symbioseM < symbioseMax:
		op('constant1').par.const0value = '1.3'
		op('constant1').par.name0 = 'symbioseMax'
	elif symbioseMax < absorptionMax:
		op('constant1').par.const0value = '1.3'
		op('constant1').par.name0 = 'absorptionMax'
	elif absorptionM < absorptionMax:
		op('constant1').par.const0value = '1.3'
		op('constant1').par.name0 = 'absorptionMax'
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	return
	