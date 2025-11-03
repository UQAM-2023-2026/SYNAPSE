"""
RZ ENERGY CONTROL SCRIPT
"""

import random

# --- Variables watchers ---
Rz_ConnectNode     = op('Rz_ConnectNode')[0]
Rz_State           = op('Rz_State')[0]
Rz_TargetNode      = op('Rz_TargetNode')[0]
Rz_Rate_R          = op('RZ_Rate_R')[0]
Rz_Rate_T          = op('RZ_Rate_T')[0]
Rz_Energy          = op('Rz_Energy')[0]
Rz_ConnectRhizome  = op('Rz_ConnectRhizome')[0]
Rz_TargetRhizome   = op('Rz_TargetRhizome')[0]
Rz_Tick            = op('Rz_Tick')[0]  

# paramètres du rhizome (ajustables)
MAX_ENERGY  = 30
ENERGY_RATE = random.uniform(1, 5)  # vitesse d’ajout d’énergie (unités par tick complet)


def onOffToOn(channel, sampleIndex, val, prev):
    """
    Called when le channel relié passe de 0 → 1
    Ici, on déclenche la génération d’énergie (dépôt/recolte)
    """

    current_energy = float(op('RZ_Data').par.const5value.eval())

    if current_energy < MAX_ENERGY:
        tick_val = Rz_Tick
        add_energy = ENERGY_RATE * tick_val
        new_energy = current_energy + add_energy

        # clamp
        if new_energy > MAX_ENERGY:
            new_energy = MAX_ENERGY

        op('RZ_Data').par.const5value = new_energy

    return


def whileOn(channel, sampleIndex, val, prev):
    """
    Si tu veux que l’énergie monte en continu pendant que le channel reste ON.
    Ça te donne un effet de “pulsation” ou recharge graduelle selon ton LFO.
    """

    current_energy = float(op('RZ_Data').par.const5value.eval())
    tick_val = Rz_Tick

    if current_energy < MAX_ENERGY:
        add_energy = ENERGY_RATE * tick_val * 0.05
        new_energy = min(current_energy + add_energy, MAX_ENERGY)
        op('RZ_Data').par.const5value = new_energy

    # Si on atteint le max, on peut déclencher un reset ou autre action
    if current_energy >= MAX_ENERGY:
        op('RZ_Data').par.const6value = 0  # ex: reset, stop, ou signal visuel

    return
