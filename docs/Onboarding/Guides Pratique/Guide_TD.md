# Guide d'utilisation TouchDesigner
*ici se trouve les directives concernant la méthodologie de TouchDesigner afin de maximiser le brain-power de tous*

## Nomenclatures
### Patches (*.toe*)
suivre cette nomenclature : 
```

    équipe_NomPatch_versionXY_STATUT_TonNom.toe

```
  où :
  ```

  - *équipe* = Noeud / Rhiz / Supra / Infra / LX / Back / etc

  - *NomPatch* = Nom facile à comprendre qui décrit essentiellement la raison d'être de la patch

  - *versionXY* = numéro de la version, remplacer *X*  en fonction du sprint et *Y* pour la version
            Exemple
                version1a --> version A de la patch au Sprint #1

  - *STATUT* = Statut de la patch [PROD, BUG, REV, GO]
            Où :
              - *PROD* = Production (on going changes)
              - *BUG* = Bug à corriger / non-fonctionnelle
              - *REV* = Peer-review en cours / Prêt à réviser
              - *GO* = Fichier approuvé techniquement et prêt à être déployé

  - TonNom = Ton nom

```
**NE JAMAIS UTILISER DE CHARACTÈRE SPÉCIAUX DANS VOTRE NOMENCLATURE**

### TOX
suivre cette nomenclature :
```

    équipe_FonctionTOX_versionXY_TAG.tox

```
  où :
  ```

  - *équipe* = Noeud / Rhiz / Supra / Infra / LX / Back / etc

  - *FonctionTOX* = Décrit la fonction principale du TOX, simple et concis.

  - *versionXY* = numéro de la version, remplacer *X*  en fonction du sprint et *Y* pour la version
            Exemple
                version1a --> version A de la patch au Sprint #1

  - *TAG* = Catégorie du tox [EFFET, DMX, OSC, TRIGGER, BASE, etc]
            Où :
              - *EFFET* = Visuel
              - *DMX* = Communication DMX
              - *OSC* = Communication OSC
              - *TRIGGER* = Qui trigger un événement
              - *BASE* = TOX de base

```
**NE JAMAIS UTILISER DE CHARACTÈRE SPÉCIAUX DANS VOTRE NOMENCLATURE**
<br>

## Bonnes Pratiques
SVP suivre autant que possible ces consignes, le 2min de plus va nous sauver des heures plus tard.
```

Toujours utiliser un TextDAT à chaque niveau (../) de votre patch qui contient :
  - Explication en détails des éléments
  - Explication de la logique employée
  - Explication des liens entre les données
  - Explication de cette patch dans l'écosystème.
  - Personne Ressource

Toujours utiliser des Null avant d'exporter des données, même si c'est vers un opérateur au même niveau.

Toujours créer des paramètres custom lorsque c'est applicable. Utiliser plusieurs pages de réglages au besoin, en autant que ce soit clair. 
Normaliser la nomenclature des paramêtres.

Utiliser des annotations (SHIFT + A) avec une belle couleur unique bien titré afin de "zoner" les composantes de programmation

Prendre le temps de rendre la patch esthétiquement belle (c'est mieux pour tout le monde)

Utiliser des Select pour éviter les longs branchements laids au besoin

Renommer les opérateurs d'importance et leur attribuer une couleur.

Faire en sorte que n'importe qui peut comprendre la logique et l'emplacement des opérateurs.

```


# Pour tout type de question, parlez à la personne technique de votre équipe ou à Sasha

<br>
<br>

