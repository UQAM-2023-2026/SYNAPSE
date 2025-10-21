# Conventions de Nommage (Nomenclature)

La cohérence dans les noms de fichiers est essentielle pour faciliter le travail d’équipe et la maintenance du projet **Mycorhize**.  
Ce guide résume les conventions de nommage définies pour ce projet, en s’alignant sur son vocabulaire thématique (*rhizome*, *nœud*, *supra*, *infra*, *dépôt*, *récolte*, etc.) et sur le ton clair et concis des autres guides d’onboarding.  
L’objectif est de rendre les fichiers aisément identifiables et **rapidement scannables** par tout nouvel arrivant.

---

## Principes Généraux

- **Clarté et concision :** Choisissez des noms de fichiers courts mais explicites.  
  Évitez les termes génériques ou les abréviations obscures.  
  Chaque nom doit indiquer clairement le contenu ou le rôle du fichier (ex. préférez `schema_reseau.png` à `image1.png`).

- **Vocabulaire du projet :** Réutilisez les termes propres à Mycorhize pour refléter la structure du projet.  
  Par exemple :
  - le module central utilise le préfixe **Rhizome** ;
  - les fichiers liés aux capteurs de terrain commencent par **Noeud** ;
  - les éléments de la zone supérieure utilisent **Supra** ;
  - ceux de la zone inférieure utilisent **Infra**.  
  Cette métaphore commune facilite la compréhension du rôle de chaque fichier au sein du « mycélium » du projet.

- **Séparateurs et caractères spéciaux :**  
  N’utilisez **pas d’espaces** ni de caractères accentués dans les noms de fichiers.  
  Remplacez les espaces par des tirets bas `_` et retirez les accents (`Supra` au lieu de `Suprâ`).  
  Évitez les caractères spéciaux et les majuscules dans les extensions.

- **Préfixes par catégorie :**  
  Adoptez un système de **préfixes** pour regrouper les fichiers par type ou fonction :  
  `Rapport_`, `Noeud_`, `Infra_`, `Supra_`, `Rhizome_`, etc.  
  Cela facilite le tri alphabétique et la recherche rapide dans le dépôt.

- **Informations en suffixe :**  
  Ajoutez des détails en fin de nom pour distinguer les versions ou contextes :  
  `_v1`, `_v2`, ou une date au format ISO `AAAA-MM-JJ`.

- **Extensions standard :**  
  Conservez les extensions appropriées (`.pdf`, `.ino`, `.png`, `.wav`, `.mp4`, etc.)  
  sans les répéter dans le nom.

---

## Conventions par Type de Fichier

### Documents PDF (Rapports, Guides, etc.)

- **Nom descriptif + type de doc :**  
  Commencez par un préfixe comme `Rapport_`, `Guide_`, `CompteRendu_`, suivi d’un titre clair.
- **Version et date :**  
  Utilisez un suffixe pour distinguer les versions (`_v2`) ou les dates.
- **Exemples :**
  - `Rapport_Mycorhize_v1.pdf`
  - `Guide_Installation_2025-05.pdf`
  - `Doc_Budget_Supra_2025.pdf`

---

### Code Arduino (Sketches des nœuds et du rhizome)

- **Préfixe du système :**  
  Utilisez **Noeud** pour les capteurs/dispositifs et **Rhizome** pour le module central.  
- **Descriptif fonctionnel :**  
  Ajoutez ensuite la fonction du code (capteurs, transfert, communication…).
- **Exemples :**
  - `Noeud7_CapteursInfra.ino`
  - `Rhizome_TransfertEnergie.ino`
  - `Noeud_Supra_LEDsync_v2.ino`

---

### Fichiers Visuels (Images, Schémas)

- **Contexte et contenu :**  
  Indiquez le module ou la zone (`Supra`, `Infra`, `Noeud`, `Rhizome`) et le contenu (`schema`, `prototype`, `rendu`…).
- **Date ou numéro de version :**  
  Ajoutez une date (`2025-10-16`) ou une version (`v2`).
- **Exemples :**
  - `Infra_Mapping_v1.png`
  - `Schema_Rhizome_v2.svg`
  - `Supra_Installation_2025-10-16.jpg`

---

### Fichiers Audio

- **Source et contexte :**  
  Utilisez un préfixe clair (`Infra`, `Supra`, `NoeudX`, etc.) et un mot-clé décrivant le son.
- **Date d’enregistrement :**  
  Ajoutez la date si plusieurs versions existent.
- **Exemples :**
  - `Audio_Supra_Ambiance_2025-10-16.wav`
  - `Audio_Noed3_Recolte_v1.mp3`
  - `Audio_Infra_Pulse_v2.wav`

---

### Fichiers Vidéo

- **Sujet et emplacement :**  
  Commencez par la zone ou le module (`Infra`, `Supra`, `Rhizome`).
- **Date et séquence :**  
  Ajoutez la date (`AAAA-MM-JJ`) ou un numéro de séquence.
- **Exemples :**
  - `Supra_Timelapse_2025-10-16.mp4`
  - `Infra_Recolte_v2.mp4`
  - `Rhizome_Demo_2025-05-02.mov`

---

## Tableau Récapitulatif des Préfixes/Suffixes

| **Type de fichier**        | **Préfixes recommandés**               | **Suffixes**                          |
|----------------------------|----------------------------------------|---------------------------------------|
| **Documents PDF**          | `Rapport_`, `Guide_`, `Doc_`          | `_vX` / date (`AAAA-MM-JJ`)           |
| **Code Arduino (.ino)**    | `NoeudX_`, `Rhizome_`                 | *(extension uniquement)*              |
| **Visuels (Images)**       | `Supra_`, `Infra_`, `NoeudX_`, `Rhizome_` | `_vX` / date (`AAAA-MM-JJ`)           |
| **Audio (Sons)**           | `Audio_Supra_`, `Audio_Infra_`, `Audio_NoeudX_` | `_vX` / date (`AAAA-MM-JJ`)           |
| **Vidéo**                  | `Supra_`, `Infra_`, `Rhizome_`        | `_vX` / date (`AAAA-MM-JJ`)           |

---

## Bonnes Pratiques Résumées

✅ Utilisez toujours **des noms en minuscules** et **sans accents**.  
✅ Séparez les mots avec `_`.  
✅ Employez le **vocabulaire du projet** (`rhizome`, `infra`, `supra`, `noeud`).  
✅ Ajoutez une **version ou une date** pour assurer le suivi.  
✅ Soyez **cohérent·e** — le respect collectif de cette nomenclature garantit un dépôt clair et durable.

---

*Fichier : `/docs/onboarding/nomenclature.md`*  
*Version : 1.0 — dernière mise à jour : 2025-10-16*
