# Spécification UI — Espace de travail Clash Filter (BonsaiViewer)

Document local (ne pas committer). Spécification de l'interface telle qu'elle a été
conçue et implémentée dans `src/bonsaiviewer/` et `src/bonsaiviewer/modules/rule/`.

## 1. Vue d'ensemble

L'onglet **Clash Filter** du ruban est un espace de travail dédié au tri des
résultats des règles. Il réorganise l'interface :

```
┌──────────────────────────────────────────────────────────────────┐
│ Ruban : Home | Navigate | Inspect | Panels | GeometryRule |     │
│        Clash Filter                                               │
├──────────────┬──────────────────────────────┬────────────────────┤
│ Result       │                              │ Properties         │
│ (haut)       │                              │   ↓ (splitter)     │
├──────────────┤        Vue 3D               │ Rule               │
│ Result Info  │                              │  (arborescence)    │
│ (bas)        │                              │                    │
└──────────────┴──────────────────────────────┴────────────────────┘
```

- **Colonne gauche** : Result (en haut) et Result Info (en bas), séparés par
  un splitter. Ces deux panneaux n'existent que dans l'onglet Clash Filter.
- **Colonne droite** : Properties (haut) et Rule (bas), séparés par un
  splitter. Le panneau Rule n'est visible que dans GeometryRule et ici.
- **Sélection de l'onglet Clash Filter** : les panneaux Models et Spatial
  Hierarchy disparaissent ; Result et Result Info prennent leur place
  (même position, même empilement vertical).
- **Retour sur un autre onglet** : Models / Spatial réapparaissent ;
  Result, Result Info et Rule se masquent.



## 2. Ruban — page Clash Filter

Même squelette que les autres pages (frame `ribbonPage`, groupes de boutons).

### Groupe Context View

| Bouton | Comportement |
|---|---|
| **Save Context** (floppy-disk) | capture l'ensemble des objets actuellement affichés (objets des modèles visibles non masqués au niveau élément) dans un contexte de session ; message du nombre capturé |
| **Show Context** (eye) | ré-affiche les objets du contexte sauvé ; sans contexte sauvé, message d'information |
| **Always Display Context** (checkable) | une fois activé, le contexte sauvé reste visible en toutes circonstances : lors de l'isolement d'un résultat (double-clic), la paire est isolée **puis** le contexte est ré-affiché par-dessus ; seul le résultat change dans la vue. Le bouton doit être facilement de-checkable. |
| **Color Result** (paint) | colore en rouge tous les objets présents dans les résultats courants (sources et cibles) |
| **Display Everything** | Affiche tous les objets de la maquette, cela ne change pas la couleurs des objets |

Le contexte est **en mémoire de session** : il n'est pas persisté et est
perdu à la fermeture de l'application.

### Groupe Regroup Result

| Bouton | Comportement |
|---|---|
| **Afficher les results** | Ce bouton vient réafficher la totalité des resutats de la règle. Il retire les filtre et degroupes les resultats.  |
| **Filter Only Non Determined** | filtre le panneau Result pour ne montrer que les résultats au statut Non Déterminé.  |
| **Group by source entity** (checkable) | regroupe les résultats en arborescence, un nœud par entité source (libellé + nombre de résultats) ; mutuellement exclusif avec Group by target ; décocher repasse en liste à plat |
| **Group by target entity** (checkable) | idem par entité cible ; **no-op si aucun résultat n'a de cible** (le mode ne change pas) |
| **Group by source model** (checkable) | regroupe les résultats en arborescence, un nœud par model source (libellé + nombre de résultats) ; mutuellement exclusif avec Group by target ; décocher repasse en liste à plat ; **no-op si aucun résultat n'a de cible** (le mode ne change pas) |

## 3. Panneau Result

Le panneau Result contient les résultats de la règle (ou des règles) lancée :
bouton Run Rule (toutes les règles OBB Above du RuleFile), clic droit
Lancer (le fichier, le dossier ou la règle), ou double-clic sur une règle
du panneau Rule.

- **Barre de filtre** : une ligne de texte filtre les résultats dont le nom
  de la source **ou** de la cible contient le texte (insensible à la casse).
- **Tableau** (8 colonnes) sous la barre : **Source**, **Target**, **Model**
  (« Source → Target » quand la paire relie deux maquettes différentes),
  **Rule** (règle productrice), **Measure** (mesure nominale du bloc `info`
  du moteur, vide si la règle n'en produit pas), puis **Valide**,
  **Non Déterminé**, **Invalide**. Chaque ligne porte 3 cases à cocher
  exclusives (cocher l'une décoche les deux autres).
- Les lignes viennent du moteur Python IfcClash_Plus (requête `run` IPC) :
  les résultats portent cross-references (modèle + step_id), règle,
  criticité, acteur et bloc `info` brut ; le viewer les mappe sur son
  `ElementRegistry` et colore les sources en rouge, les cibles en orange.
- **Statut par défaut : Non Déterminé** (le choix neutre).
- **Clic** sur une ligne : la sélection met à jour le panneau Result Info.
- **Clic** sur une case de statut : change le statut du résultat.
- **Double-clic** sur une ligne : le viewer isole la paire (masque tout le
  reste) et zoome à proximité ; si **Always Display Context** est activé et
  qu'un contexte est sauvé, le contexte reste visible en plus de la paire.
  L'opération est répétable d'une ligne à l'autre ; **Show All** (onglet
  Inspect) restaure la visibilité.
- **Groupement** (boutons du ruban) : en mode groupé, les nœuds de groupe
  sont repliés/dépliés et leur libellé porte le nombre de résultats.

Le clic droit sur **un résultat, un groupe, ou plusieurs résultats** ouvre :

| Action | Effet |
|---|---|
| **Afficher les objets** | ré-affiche tout (Show All) et sélectionne les objets des résultats choisis |
| **Validé les results** | statut Valide pour tous les résultats de la sélection |
| **Invalidé les results** | statut Invalide pour tous |
| **Non déterminé les résults** | statut Non Déterminé pour tous |

Sur un nœud de groupe, l'action s'applique à tous les résultats du groupe.

## 4. Panneau Result Info

Le panneau Result Info affiche les informations du résultat sélectionné dans
le panneau Result :

- le nom de la **règle** ayant produit le résultat ;
- le **name** de la source et le **name** de la target ;
- le modèle source, et les modèles source/cible séparés quand la paire
  relie deux maquettes ;
- la **mesure** du résultat (nom + valeur du bloc `info` du moteur) ;
- la **criticité** et l'**acteur** quand la règle les renseigne ;
- le **statut** courant.

Sans sélection (« No result selected »).

## 5. Panneau Rule

Le panneau Rule est exactement le même que dans l'onglet GeometryRule :
même arborescence, même clic droit, même Rule Config. Il est visible
uniquement dans ces deux onglets.

## 6. Ce qui n'est pas encore couvert (non-goals actuels)

- **Statuts non persistés** : les verdicts Valide / Non Déterminé / Invalide
  vivent dans le panneau Result ; un nouveau run les remet à Non Déterminé
  et rien n'est écrit sur disque (seul l'export BCF, à venir, les emporte).
- Le filtre par nom et « Filter Only Non Determined » sont des filtres
  d'affichage : ils ne modifient pas les résultats.
- « Afficher les objets » ré-affiche tout et sélectionne ; il n'isole pas et
  ne zoome pas (seul le double-clic isole + zoome).
- Le contexte n'est ni persisté ni ré-évalué : les objets sont capturés à
  l'instant du Save Context (objets supprimés d'un modèle retiré = ids
  morts, ignorés par le viewer).
- Les blocs `info` avancés des résultats (points de contact, faces, OBB,
  raycast, zone) sont transportés par l'IPC mais pas encore rendus dans
  Result Info ; pas de regroupement par criticité/règle (les colonnes
  existent) ; pas d'export BCF (phase 5 de la spec d'intégration).
