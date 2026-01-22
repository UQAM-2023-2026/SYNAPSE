"""
CHOP Execute DAT

0 = ABSORPTION
1 = SYMBIOSE
2 = SATURATION
3 = SYMBIOSE MAXIMALE (infra & supra dans le sweet spot)
4 = SHUTDOWN (énergie totale >= seuil global)

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""

def onOffToOn(channel: 'Channel', sampleIndex: int, val: float, prev: float):
    return


def whileOn(channel: 'Channel', sampleIndex: int, val: float, prev: float):
    return


def onOnToOff(channel: 'Channel', sampleIndex: int, val: float, prev: float):
    return


def whileOff(channel: 'Channel', sampleIndex: int, val: float, prev: float):
    return


def onValueChange(channel: 'Channel', sampleIndex: int, val: float, prev: float):
    """
    Calcule l'état global en fonction du % d'énergie
    et de quelques seuils globaux (symbiose max, shutdown).

    Règle de base (par niveau, via ratio):
      - 0–40%  → ABSORPTION  (state = 0)
      - 40–60% → SYMBIOSE    (state = 1)
      - 60–100%→ SATURATION  (state = 2)

    Overrides :
      - si E_total >= shutdown_threshold        → SHUTDOWN (4)
      - si infra et supra entre [symb_min, symb_max]
        → SYMBIOSE MAX (3) pour les deux
    """

    # ---------------- LECTURE SUPRA / INFRA ----------------
    supra = float(op('supra_state')[0])   # énergie supra
    infra = float(op('infra_state')[0])   # énergie infra

    stateSupra = op('STATE_SUPRA')  # Constant CHOP pour l'index d'état
    stateInfra = op('STATE_INFRA')  # Constant CHOP pour l'index d'état

    total = supra + infra

    # ---------------- LECTURE CONFIG GLOBALE ----------------
    cfg = op('../Global_Config')

    # Sécurité si la config n'est pas présente
    if cfg is not None:
        shutdown_threshold = float(cfg[12])
        symb_min = float(cfg[13])
        symb_max = float(cfg[14])
    # else:
    #     # fallback safe
    #     shutdown_threshold = 1000.0
    #     symb_min = 400.0
    #     symb_max = 600.0

    # ---------------- CAS 1 : système mort / vide ----------------
    if total <= 0:
        # Absorption par défaut (les deux niveaux)
        stateSupra.par.const0value = 0
        stateInfra.par.const0value = 0
        return

    # ---------------- CAS 2 : SHUTDOWN GLOBAL ----------------
    # On utilise >= pour être safe si ça overshoot un peu.
    if total >= shutdown_threshold:
        stateSupra.par.const0value = 4   # SHUTDOWN
        stateInfra.par.const0value = 4
        op.RZ1.par.Connected = False
        op.RZ2.par.Connected = False
        return

    # ---------------- CAS 3 : SYMBIOSE MAXIMALE ----------------
    # Les deux niveaux dans le range config -> état 3
    # if (symb_min <= supra <= symb_max) and (symb_min <= infra <= symb_max):
    #     stateSupra.par.const0value = 3   # SYMBIOSE MAXIMALE
    #     stateInfra.par.const0value = 3
    #     return

    # ---------------- LOGIQUE 3 ÉTATS (ratio) ----------------
    ratioSupra = supra / total   # 0–1
    ratioInfra = infra / total   # 0–1

    # --- SUPRA ---
    if ratioSupra < 0.4:
        stateSupra.par.const0value = 0  # ABSORPTION
    elif ratioSupra <= 0.6:
        stateSupra.par.const0value = 1  # SYMBIOSE
    else:
        stateSupra.par.const0value = 2  # SATURATION

    # --- INFRA ---
    if ratioInfra < 0.4:
        stateInfra.par.const0value = 0  # ABSORPTION
    elif ratioInfra <= 0.6:
        stateInfra.par.const0value = 1  # SYMBIOSE
    else:
        stateInfra.par.const0value = 2  # SATURATION

    return
