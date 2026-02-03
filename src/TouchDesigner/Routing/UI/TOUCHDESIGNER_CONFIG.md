# Configuration TouchDesigner pour OSC

## Configuration OSC Out dans TouchDesigner

Pour envoyer des données vers le moniteur UI, configurez votre OSC Out CHOP:

1. **Network Address**: `10.0.2.245`
2. **Network Port**: `6970`
3. **Protocol**: `UDP`

## Exemple de configuration

```
OSC Out CHOP:
- Active: On
- Network Address: 10.0.2.245
- Network Port: 6970
- Protocol: UDP
- OSC Address: /synapse/[nom_du_canal]
```

## Test depuis TouchDesigner

Vous pouvez tester en créant un simple setup:

1. Créer un **Constant CHOP** avec quelques canaux
2. Créer un **OSC Out CHOP**
3. Connecter le Constant au OSC Out
4. Configurer l'OSC Out:
   - Address: 10.0.2.245
   - Port: 6970
   - Vérifier que "Active" est coché

## Vérification des ports

Si vous utilisez déjà le port 6970 pour autre chose dans TouchDesigner:

### Option 1: Changer le port du moniteur
Éditez `osc_server.py` ligne 91:
```python
osc_port = 6970  # Changer pour un autre port
```

### Option 2: Changer le port dans TouchDesigner
Modifiez simplement le port dans votre OSC Out CHOP

## Debugging

1. Vérifier que le serveur OSC est lancé:
```bash
lsof -i :6970
```

2. Envoyer des messages de test:
```bash
python3 test_osc.py
```

3. Vérifier les logs du serveur dans le terminal où `osc_server.py` tourne

## Format des messages OSC

Le serveur accepte tous les formats de messages OSC:
- `/chemin/vers/data` [valeur1, valeur2, ...]
- Supporte: float, int, string, bool

Exemple:
- `/synapse/position/x 0.5`
- `/synapse/velocity 1.2 3.4 5.6`
- `/synapse/status "active"`
