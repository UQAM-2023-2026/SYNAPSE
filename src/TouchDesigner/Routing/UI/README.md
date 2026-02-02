# SYNAPSE OSC UI Monitor

Interface web pour visualiser en temps réel les données OSC streamées sur le port 6970.

## Installation

1. Installer les dépendances Python:
```bash
pip install -r requirements.txt
```

## Utilisation

1. Démarrer le serveur OSC/WebSocket:
```bash
python3 osc_server.py
```

2. Ouvrir l'interface web dans un navigateur:
```bash
python3 -m http.server 8000
```

3. Naviguer vers: http://10.0.2.245:8000

## Architecture

- **osc_server.py**: Écoute les messages OSC sur le port 6970 et les redistribue via WebSocket
- **index.html**: Interface web qui affiche les messages en temps réel
- Port OSC: 6970 (local)
- Port WebSocket: 8765
- Port HTTP: 8000

## Fonctionnalités

- Visualisation en temps réel des messages OSC
- Statistiques (messages/sec, total, adresses uniques)
- Pause/Resume du flux
- Export des messages en JSON
- Historique des 1000 derniers messages
- Interface moderne avec gradient et animations
