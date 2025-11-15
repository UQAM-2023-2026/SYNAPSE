"""
RZ ENERGY SCRIPT — version clean & autonome
Chaque rhizome gère sa propre énergie et émet un trigger quand il est plein.
"""

import random

# --- paramètres généraux ---
MAX_ENERGY  = op('../Rz_Units')[0]
rand_min = op('../Rz3').par.Randmin
rand_max = op('../Rz3').par.Randmax
ENERGY_RATE = random.uniform(rand_min, rand_max)  # vitesse de recharge aléatoire (facultatif)


def whileOn(channel, sampleIndex, val, prev):

    rz_data = op('RZ_Data')
    if not rz_data:
        return

    # lecture du tick
    tick_val = op('Rz_Tick')[0] if op('Rz_Tick') else 0.0

    # lecture/écriture énergie
    current = float(rz_data.par.const5value)
    if current < MAX_ENERGY:
        new_val = min(current + ENERGY_RATE * tick_val * 0.05, MAX_ENERGY)
        rz_data.par.const5value = new_val

    if rz_data.par.const5value >= MAX_ENERGY:
        # envoi trigger
        rz_data.par.const6value = 0

    return
