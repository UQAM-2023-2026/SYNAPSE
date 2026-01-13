# Gestion des Résidences de Création

Application web pour gérer les disponibilités, planifier les résidences de création et faciliter l'organisation à l'école.

## 🚀 Démarrage rapide

### Option 1: Serveur Python (Recommandé pour accès réseau)

```bash
python3 serveur.py
```

Le navigateur s'ouvrira automatiquement à `http://localhost:8000/residences.html`

**Pour accéder depuis d'autres ordinateurs sur le même réseau:**
1. Trouvez l'adresse IP de votre ordinateur:
   - Mac/Linux: `ifconfig` ou `ip addr`
   - Windows: `ipconfig`
2. Les autres peuvent accéder à `http://[VOTRE-IP]:8000/residences.html`

### Option 2: Ouvrir directement le fichier

Double-cliquez simplement sur `residences.html` - ça fonctionne hors ligne!

## 📋 Fonctionnalités

### 1. Tableau de bord
- Voir qui est présent aujourd'hui
- Afficher les projets sur lesquels chacun travaille
- Statistiques rapides

### 2. Disponibilités
- Chaque personne peut marquer ses périodes de disponibilité
- Ajouter des notes et des plages horaires spécifiques
- Voir toutes ses disponibilités enregistrées

### 3. Calendrier
- Vue hebdomadaire des disponibilités
- Voir rapidement qui est disponible chaque jour
- Navigation facile entre les semaines

### 4. Résidences
- Planifier une résidence pour une date spécifique
- Sélectionner les personnes qui devraient venir
- Définir un ordre de priorité (qui est prioritaire pour venir)
- Voir toutes les résidences planifiées

### 5. Gestion des personnes
- Ajouter/supprimer des personnes
- Exporter toutes les données en JSON
- Importer des données sauvegardées

## 💾 Sauvegarde des données

Les données sont automatiquement sauvegardées dans le **localStorage** du navigateur.

⚠️ **Important:** Les données sont stockées localement sur chaque ordinateur/navigateur. Pour partager les données entre plusieurs appareils:

1. Utilisez l'option **"Exporter les données"** dans l'onglet "Personnes"
2. Partagez le fichier JSON
3. Importez-le sur les autres appareils

### Pour une utilisation collaborative

Si vous voulez que tout le monde travaille sur les mêmes données en temps réel, vous avez deux options:

#### Option A: Ordinateur partagé à l'école
- Installez l'application sur un ordinateur fixe
- Tout le monde l'utilise sur place

#### Option B: Serveur accessible en ligne
- Hébergez le fichier sur un serveur web
- Utilisez Tailscale, ngrok ou un hébergement cloud
- Tout le monde peut y accéder à distance

## 🎨 Personnalisation

Le fichier `residences.html` est autonome - tout le CSS et JavaScript est inclus. Vous pouvez:
- Modifier les couleurs dans la section `<style>`
- Ajouter des champs personnalisés dans le JavaScript
- Adapter l'interface à vos besoins

## 🔧 Workflow recommandé

1. **En début de session:**
   - Chaque personne marque ses disponibilités dans l'onglet "Disponibilités"

2. **Planification:**
   - L'organisateur va dans "Résidences"
   - Crée une résidence pour une date
   - Sélectionne les personnes (en ordre de priorité)

3. **Jour de résidence:**
   - Ouvrir le "Tableau de bord"
   - Sélectionner la date du jour
   - Voir qui doit venir et sur quoi chacun travaille
   - Mettre à jour les projets au besoin

4. **Sauvegarde régulière:**
   - Exporter les données régulièrement
   - Garder une copie de backup

## 📱 Utilisation mobile

L'interface est responsive et fonctionne sur mobile/tablette!

## 🆘 Support

Pour des questions ou des améliorations, modifiez directement le code HTML ou contactez le développeur.

## 📊 Liste des personnes initiales

L'application est pré-configurée avec les 28 personnes de votre fichier Excel:

Sasha, Dom, Rachel, Florence, Tom, Aldric, Émilie, Maria, Maude, Noum, Med, Thierry, No, JC, Justin, Raton, Anthonny, Sarah, Ugo, Clément, Max, Mery, Laura, Emmanuel, Alexandre, Katianna, Louis-David, Léa

Vous pouvez ajouter ou supprimer des personnes dans l'onglet "Personnes".
