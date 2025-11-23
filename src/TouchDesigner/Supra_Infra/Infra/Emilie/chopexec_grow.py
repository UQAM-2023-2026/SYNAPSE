"""
CHOP Execute DAT

me - this DAT

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""

def onOffToOn(channel: Channel, sampleIndex: int, val: float, 
              prev: float):
	op('lumablur1').par.blackvalue = 0
	op('feedback1').par.resetpulse.pulse() 
	return

def whileOn(channel: Channel, sampleIndex: int, val: float, 
            prev: float):
	return

def onOnToOff(channel: Channel, sampleIndex: int, val: float, 
              prev: float):
	op('lumablur1').par.blackvalue = 1
	op('feedback1').par.resetpulse.pulse() 
	return

def whileOff(channel: Channel, sampleIndex: int, val: float, 
             prev: float):
	return

def onValueChange(channel: Channel, sampleIndex: int, val: float, 
                  prev: float):
	return
