"""
CHOP Execute DAT

me - this DAT

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""

def onOffToOn(channel: Channel, sampleIndex: int, val: float, 
              prev: float):
	prct_supra = op('../Energy_System/out1')[0].eval()
	prct_infra = op('../Energy_System/out1')[1].eval()
	
	energie_supra = op('../Energy_System/Counter/SUPRA_Total').par.const0value
	energie_infra = op('../Energy_System/Counter/INFRA_Total').par.const0value
	
	extreme_prct = op('../Controles').par.Extrme.eval() - 1
	appel_prct = op('../Controles').par.Appel.eval()


	

	if prct_infra > extreme_prct:
		total = energie_supra.eval() + energie_infra.eval()
		target_infra = total * (extreme_prct / 100.0)
		diff = energie_infra.eval() - target_infra
		
		energie_infra.val = target_infra
		energie_supra.val += diff

	return


