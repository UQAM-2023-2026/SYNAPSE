# 🚀 Démarrage Rapide - Résidences EDM3003

## Étape 1: Ouvrir l'application

```bash
python3 serveur.py
```

Ou double-cliquez sur `residences.html`

## Étape 2: Importer l'horaire EDM3003

1. Ouvrez `import-horaire.html` dans votre navigateur
2. Cliquez sur **"Importer l'horaire dans l'application"**
3. Cliquez sur **"Ouvrir l'application"**

✅ Toutes les sessions PM/SOIR sont maintenant importées avec leurs types d'activités!

## Étape 3: Marquer les disponibilités

1. Allez dans l'onglet **"Disponibilités"**
2. Cliquez sur votre nom
3. Sélectionnez vos dates de disponibilité
4. Cliquez sur **"Enregistrer la disponibilité"**

## Étape 4: Assigner les personnes aux résidences

1. Allez dans l'onglet **"Résidences"**
2. Trouvez une résidence importée (elles ont une barre colorée à gauche selon le type)
3. Cliquez sur **"Éditer"** *(à implémenter)* ou créez une nouvelle résidence pour une date existante
4. Sélectionnez les personnes qui doivent venir
5. Organisez-les par priorité avec les flèches ↑↓

## Étape 5: Voir qui est là aujourd'hui

1. Allez dans l'onglet **"Tableau de bord"**
2. Sélectionnez la date du jour
3. Voyez toutes les sessions (PM/SOIR) avec:
   - Le type d'activité coloré
   - Les personnes attendues par priorité
   - Les projets sur lesquels chacun travaille

## Types d'activités et couleurs

- 🔵 **Planification** - Bleu
- 🟢 **Résidence de création** - Vert
- 🟡 **Test** - Jaune
- 🔴 **Revue** - Rouge
- 🟣 **Montage** - Violet
- 🟠 **Générale** - Orange
- 🎀 **Lancement** - Rose
- ⚫ **Mise en place / Démontage** - Gris
- 🔵 **Présentation** - Cyan
- 🟢 **Post-mortem** - Teal

## Sessions PM vs SOIR

Les résidences importées ont automatiquement:
- **PM**: 13h00 - 17h00
- **SOIR**: 18h00 - 22h00

Vous pouvez ajuster les heures au besoin.

## Workflow recommandé

### Pour les étudiants:
1. Marquez vos disponibilités à l'avance
2. Consultez le tableau de bord pour voir quand vous êtes attendus
3. Mettez à jour votre projet du jour

### Pour l'organisateur:
1. Importez l'horaire au début de la session
2. Assignez les personnes aux résidences selon les disponibilités
3. Utilisez le système de priorités pour indiquer qui doit absolument venir
4. Consultez le calendrier pour une vue d'ensemble hebdomadaire

## Astuces

- 💾 **Les données sont sauvegardées automatiquement** dans le navigateur
- 📤 **Exportez régulièrement** via l'onglet "Personnes"
- 🎨 **Les couleurs indiquent le type d'activité** - facile de distinguer un test d'une résidence
- 📱 **Fonctionne sur mobile** - consultez le tableau de bord depuis votre téléphone
- 🔍 **Utilisez la recherche** pour trouver rapidement une personne

## Accès réseau (pour toute l'équipe)

Si vous voulez que tout le monde puisse accéder:

1. Lancez le serveur: `python3 serveur.py`
2. Trouvez votre adresse IP:
   - Mac: `ifconfig | grep "inet " | grep -v 127.0.0.1`
   - Linux: `ip addr show | grep "inet " | grep -v 127.0.0.1`
   - Windows: `ipconfig`
3. Les autres accèdent via: `http://[VOTRE-IP]:8000/residences.html`

## Besoin d'aide?

- Consultez le `README.md` pour plus de détails
- Les fichiers sont dans: `/Organisation - Résidences/`
- Vos données sont dans le localStorage du navigateur

## Dates importantes EDM3003

- **12 janv** - Mise en place
- **14 janv** - PLANIFICATION S5
- **26 janv** - TEST INTÉRIEUR 1
- **28 janv** - REVUE S5
- **2 fév** - PLANIFICATION S6
- **11 fév** - TEST INTÉRIEUR 2
- **16 fév** - REVUE S6 (avec public invité!)
- **18 fév** - RÉTRO S6 / PLANIFICATION S7
- **23-25 fév** - Montage Synapse
- **25 fév** - Générale Synapse
- **26 fév - 1 mars** - Lancement Synapse
- **2-5 mars** - Démontage et rangement
- **16 mars** - Post-mortem

Bon courage avec vos résidences! 🎭✨
