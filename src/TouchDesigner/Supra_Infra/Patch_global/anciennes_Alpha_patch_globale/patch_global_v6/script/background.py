# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.

absorptionM = 0.5
absorptionMax = 0
Idle = 0.8
symbioseM = 1
symbioseMax = 1.2
saturationM = 1.5
saturationMax = 2

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

        if channel.name == 'k1' or channel.name == 'phase1':  # symbiose
            targets.par.value0  = 1
            targets.par.value1  = 1
            targets.par.value2  = 0
            targets.par.value3  = 0.7
            targets.par.value4  = 0.7
            targets.par.value5  = 0.664
            targets.par.value6  = 0.039
            targets.par.value7  = 0.75
            targets.par.value8  = 0.33
            targets.par.value9  = 0.6
            targets.par.value10 = 0.546
            targets.par.value11 = 0
            targets.par.value12 = 0

        elif channel.name == 'k2' or channel.name == 'phase2':  # absorption
            targets.par.value0  = 1
            targets.par.value1  = 1
            targets.par.value2  = 0
            targets.par.value3  = 1
            targets.par.value4  = 0.67
            targets.par.value5  = 0.322
            targets.par.value6  = 0.6
            targets.par.value7  = 0.993
            targets.par.value8  = 0.678
            targets.par.value9  = 0.368
            targets.par.value10 = 0.099
            targets.par.value11 = 210
            targets.par.value12 = 179

        elif channel.name == 'k3' or channel.name == 'phase3':  # saturation
            targets.par.value0  = 1.2
            targets.par.value1  = 1
            targets.par.value2  = 1
            targets.par.value3  = 0.32
            targets.par.value4  = 0.7
            targets.par.value5  = 0.2
            targets.par.value6  = 0
            targets.par.value7  = 0.3
            targets.par.value8  = 0.332
            targets.par.value9  = 0.6
            targets.par.value10 = 1
            targets.par.value11 = 0
            targets.par.value12 = 0

       # elif op('base1').par.Menu.eval() == 'Idle' or 'symbioseM' or 'symbioseMax'
       # elif op('base1').par.Menu.eval() == 'Idle' or op('base1').par.Menu.eval() == 'symbioseM' or op('base1').par.Menu.eval() == 'symbioseMax'
       # elif op('base1').par.Menu == 2 or 3 or 4
    return
	