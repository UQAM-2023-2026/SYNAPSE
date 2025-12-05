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

        if channel.name == 'kx': # PAS DE rhizome 
            targets.par.value0 = 0.08
            targets.par.value1 = 1

        elif channel.name == 'kz': # rhizome connexion
            targets.par.value0 = 1
            targets.par.value1 = 1

    return
