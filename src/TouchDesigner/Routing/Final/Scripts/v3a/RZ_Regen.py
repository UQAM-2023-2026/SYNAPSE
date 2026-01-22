# RZ_Regen — gère la régénération automatique du rhizome
# Hypothèses:
# - Global_Config : op('../Global_Config') (Constant CHOP)
#   channels: RZ_capacity, RZ_min_percent, RZ_regen_time, Tick_rate
# - RZ_State      : op('RZ_State')         (Constant CHOP)
#   channels: energy, connected, target_index, is_regen, regen_t

def get_cfg():
    return op('../Global_Config')

def get_rz():
    return op('RZ_State')

def set_chop_channel(chop, chan_name, value):
    """
    Met à jour un channel d'un Constant CHOP en écrivant
    dans le paramètre ValueX / constXvalue correspondant.
    On matche par index: index du channel -> index du param 'value*'.
    """
    chans = chop.chans()
    pars  = chop.pars('*value*')

    idx = None
    for i, ch in enumerate(chans):
        if ch.name == chan_name:
            idx = i
            break

    if idx is None or idx >= len(pars):
        return

    pars[idx].val = value


def whileOff(channel, sampleIndex, val, prev):
    """
    Appelé chaque frame tant que le CHOP RZ_State est OFF (dépend du param du CHOP Execute),
    mais on se base surtout sur:
      - connected == 0
      - is_regen == 1

    Idée:
      - si is_regen == 1 et connected == 0 -> on remonte de 0 -> E_min sur RZ_regen_time
      - une fois E_min atteint -> is_regen repasse à 0, et le rhizome attend une nouvelle connexion
    """

    cfg = get_cfg()
    rz  = get_rz()

    if cfg is None or rz is None:
        return

    # état du rhizome
    energy    = float(rz['energy'])
    connected = float(rz['connected'])
    is_regen  = float(rz['is_regen'])
    regen_t   = float(rz['regen_t'])

    # si pas en mode regen -> on ne fait rien
    if is_regen < 0.5:
        return

    # on veut régénérer seulement quand il n'est connecté à rien
    if connected > 0.5:
        return

    # lecture des paramètres globaux
    cap        = float(cfg['RZ_capacity'])
    min_pct    = float(cfg['RZ_min_percent'])
    regen_time = float(cfg['RZ_regen_time'])
    try:
        dt = float(cfg['Tick_rate'])
    except:
        dt = 1.0 / 60.0

    e_min = cap * min_pct

    # avance le timer
    regen_t += dt

    # progression de 0 -> 1 sur regen_time secondes
    if regen_time <= 0:
        progress = 1.0
    else:
        progress = min(1.0, regen_t / regen_time)

    # nouvelle énergie = interpolation 0 -> e_min
    new_energy = e_min * progress

    # write back
    set_chop_channel(rz, 'regen_t', regen_t)
    #set_chop_channel(rz, 'energy', new_energy)

    # si on a atteint la fin de la regen -> on fixe au min et on sort du mode regen
    if progress >= 1.0:
        set_chop_channel(rz, 'energy', e_min)
        set_chop_channel(rz, 'is_regen', 0.0)
        # on laisse connected à 0 : le rhizome attend un nouveau lien

    return


def onOffToOn(channel, sampleIndex, val, prev):
    # rien de spécial pour l'instant
    return
