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
	"""
	Called when a channel value changes.
	
	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	if op('null1')[0].eval() == 1:
		op('../Controles').par.Supraappel = 0
		op('../Controles').par.Supraabsorption = 0
		op('../Controles').par.Supraquilibre = 0
		op('../Controles').par.Suprasaturation = 0
		op('../Controles').par.Supraextrme = 0
		
		op('../Controles').par.Infraappel = 0
		op('../Controles').par.Infraabsorption = 0
		op('../Controles').par.Infraquilibre = 0
		op('../Controles').par.Infrasaturation = 0
		op('../Controles').par.Infraextrme = 0
		
	else:
		
		op('../Controles').par.Supraabsorption = op('../State_Watcher/Supra_Stater')[0].eval()
		op('../Controles').par.Supraquilibre = op('../State_Watcher/Supra_Stater')[1].eval()
		op('../Controles').par.Suprasaturation = op('../State_Watcher/Supra_Stater')[2].eval()
		
		
		
		op('../Controles').par.Infraabsorption = op('../State_Watcher/Infra_Stater')[0].eval()
		op('../Controles').par.Infraquilibre = op('../State_Watcher/Infra_Stater')[1].eval()
		op('../Controles').par.Infrasaturation = op('../State_Watcher/Infra_Stater')[2].eval()
		

		op('../Controles').par.Supraappel = op('../State_Watcher/Supra_Special')[1].eval()
		op('../Controles').par.Supraextrme = op('../State_Watcher/Supra_Special')[2].eval()

		op('../Controles').par.Infraappel = op('../State_Watcher/Infra_Special')[1].eval()
		op('../Controles').par.Infraextrme = op('../State_Watcher/Infra_Special')[2].eval()
	return
