# SYNAPSE Dashboard - Quick Start

## 🚀 Launch en Un Click (Méthode Recommandée)

### Option 1 : Double-click sur macOS
Double-cliquez sur le fichier :
```
LAUNCH_SYNAPSE.command
```

### Option 2 : Via Terminal
```bash
cd /Users/cexiistudiom4max/Desktop/SYNAPSE/src/TouchDesigner/Routing/UI
./launch.py
# ou
python3 launch.py
```

**Le launcher fait automatiquement :**
- ✅ Démarre le serveur OSC/WebSocket (port 6970 + 8765)
- ✅ Démarre le serveur HTTP (port 8000)
- ✅ Ouvre le dashboard dans votre navigateur
- ✅ Gère proprement l'arrêt avec Ctrl+C

## 🛑 Arrêt

Appuyez sur `Ctrl+C` dans le terminal pour arrêter **tous** les services proprement.

## 📊 URLs d'Accès

- **Dashboard** : http://localhost:8000/dashboard.html
- **WebSocket** : ws://localhost:8765
- **OSC Port** : 6970 (UDP)

## 🔧 Configuration TouchDesigner

Dans TouchDesigner, configurez votre **OSC Out CHOP** :
- **Network Address** : `127.0.0.1` (local) ou `10.0.2.245` (réseau)
- **Network Port** : `6970`
- **Protocol** : `UDP`
- **Active** : `On`

## 📱 Résolution

Le dashboard est optimisé pour **1280×720 pixels** (sans scroll).

---

## 🧪 Méthode Manuelle (Alternative)

Si vous préférez lancer les composants séparément :

**Terminal 1** - Serveur OSC/WebSocket :
```bash
python3 debug_server.py
```

**Terminal 2** - Serveur HTTP :
```bash
python3 -m http.server 8000
```

**Navigateur** - Ouvrir manuellement :
```
http://localhost:8000/dashboard.html
```

---

## ⚠️ Troubleshooting

### Port 8000 déjà utilisé
```bash
# Trouver le processus
lsof -i :8000

# Tuer le processus
kill -9 <PID>
```

### Port 6970 (OSC) déjà utilisé
```bash
# Trouver le processus
lsof -i :6970

# Tuer le processus
kill -9 <PID>
```

### Serveur ne démarre pas
Vérifiez que les dépendances sont installées :
```bash
pip install -r requirements.txt
```

### TouchDesigner n'envoie rien
- Vérifier que le OSC Out CHOP est **Active**
- Vérifier le port (doit être `6970`)
- Vérifier l'adresse IP
- Regarder les logs dans TouchDesigner

### L'interface web ne se connecte pas
- Vérifier que le serveur OSC/WebSocket tourne
- Ouvrir la console du navigateur (F12) pour voir les erreurs
- Vérifier le status dans le header (doit être "Connected")

### Dashboard ne s'affiche pas correctement
- Utiliser un navigateur moderne (Chrome, Firefox, Safari récent)
- Vérifier la résolution de la fenêtre (1280×720 recommandé)
- Désactiver les extensions de navigateur qui pourraient interférer

---

## 📁 Structure des Fichiers

```
UI/
├── LAUNCH_SYNAPSE.command     ← Double-click pour démarrer (macOS)
├── launch.py                  ← Launcher Python intelligent
├── debug_server.py            ← Serveur OSC/WebSocket
├── dashboard.html             ← Interface web (1280×720)
├── requirements.txt           ← Dépendances Python
├── QUICKSTART.md              ← Guide de démarrage
├── README.md                  ← Documentation
└── TOUCHDESIGNER_CONFIG.md    ← Configuration TouchDesigner
```

## 🎯 Workflow Recommandé

1. **Double-click** sur `LAUNCH_SYNAPSE.command`
2. Attendre que le navigateur s'ouvre automatiquement
3. Vérifier que le status est "Connected" (LED verte)
4. Lancer TouchDesigner et configurer OSC Out
5. Les données devraient apparaître en temps réel
6. `Ctrl+C` dans le terminal pour tout arrêter
