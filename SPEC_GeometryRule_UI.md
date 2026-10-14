# Spécification UI — Espace de travail GeometryRule (BonsaiViewer)

Document local (ne pas committer). Spécification de l'interface telle qu'elle a été
conçue et implémentée dans `src/bonsaiviewer/` et `src/bonsaiviewer/modules/rule/`.

## 1. Vue d'ensemble

L'onglet **GeometryRule** du ruban est un espace de travail dédié à la
construction de jeux de règles de géométrie. Il réorganise l'interface :

```
┌──────────────────────────────────────────────────────────────────┐
│ Ruban : Home | Navigate | Inspect | Panels | GeometryRule        │
│        page GeometryRule → groupe COLOR → bouton « Color Red »   │
├──────────────┬──────────────────────────────┬────────────────────┤
│ Rule Library │                              │ Properties         │
│ (haut)       │                              │   ↓ (splitter)     │
├──────────────┤        Vue 3D               │ Rule               │
│ Rule Config  │                              │  (arborescence)    │
│ (bas)        │                              │                    │
└──────────────┴──────────────────────────────┴────────────────────┘
```

- **Colonne gauche** : Rule Library (en haut) et Rule Config (en bas), séparés
  par un splitter. Ces deux panneaux n'existent que dans l'onglet GeometryRule.
- **Colonne droite** : Properties (haut) et Rule (bas), séparés par un splitter.
- **Sélection de l'onglet GeometryRule** : les panneaux Models et Spatial
  Hierarchy disparaissent ; Rule Library et Rule Config prennent leur place
  (même position, même empilement vertical).
- **Retour sur un autre onglet** : Models / Spatial réapparaissent ;
  Rule Library / Rule Config se masquent.

## 2. Ruban — page GeometryRule

- Même squelette que les autres pages (frame `ribbonPage`, groupes de boutons).
- Contient un seul groupe **COLOR** avec le bouton **Color Red** :
  - bouton à bascule (checkable), icône `:/icons/paint.svg` ;
  - activé : tous les objets de tous les modèles chargés sont colorés en rouge ;
  - désactivé : les couleurs d'origine (vertex colors) sont restaurées.
- Le bouton n'apparaît pas ailleurs (retiré de la page Inspect).

## 3. Panneau Rule Library

Bibliothèque de règles, organisée en **dossiers refermables** (composant
`Section` : en-tête cliquable, flèche haut/bas, corps repliable) :

| Dossier | Entrées |
|---|---|
| OneObjectRule | Name Contains, Minimum Volume, Maximum Height |
| TwoObjectRule | Minimum Distance, Overlap Check |
| ComplexRule | Rule Chain, Group Filter |
| IDS Facet | Entity Facet, Attribute Facet, Property Facet, Classification Facet, Material Facet |

Comportement des entrées :

- **Clic** sur une entrée → la définition (Nom, Description, propriétés,
  valeurs par défaut) s'affiche dans Rule Config **en lecture seule**.
- **Glisser** une entrée → source d'un drag & drop vers l'arborescence du
  panneau Rule.

## 4. Panneau Rule

### 4.1 Barre de boutons

| Bouton | Comportement |
|---|---|
| Save | popup « Not Working » (en attente de spécification de persistance) |
| Load | popup « Not Working » |
| New RuleFile | crée un RuleFile « Rule File #N » à la racine de l'arbre |
| Add Folder | crée un dossier « Folder #N » dans l'élément sélectionné (RuleFile ou dossier), ou à la racine si rien n'est sélectionné |

### 4.2 Arborescence

Types de nœuds :

| Type | Règle de contenu |
|---|---|
| **RuleFile** | contient une à plusieurs règles (et des dossiers) |
| **Folder** | refermable ; peut contenir un dossier et 0 à n règles |
| **Rule** | règle ou facette instanciée depuis la bibliothèque |
| **Slot** | emplacement vide sous une règle (`source`, `target`, `context`), affiché en gris |

### 4.3 Emplacements (slots) par règle

Chaque règle instanciée expose ses emplacements enfants selon sa famille :

| Famille | Emplacements |
|---|---|
| OneObjectRule | `source` |
| TwoObjectRule | `source`, `target` |
| ComplexRule | `source`, `target`, `context` |
| Facettes IDS de requirement (Attribute, Property, Classification, Material) | `source` |
| Entity Facet (applicabilité) | `context` |

### 4.4 Matrice du drag & drop

| Cible du dépôt | Résultat |
|---|---|
| Zone vide de l'arbre | nouveau RuleFile créé, la règle y est ajoutée |
| RuleFile ou Folder | la règle est ajoutée à l'intérieur |
| Rule (règle instanciée) | la nouvelle règle est ajoutée en sibling juste après |
| Slot vide | l'emplacement est **rempli sur place** : texte `source : NomDeLaRegle` ; la règle expose à son tour ses propres emplacements (récursif) |
| Déplacement interne (drag dans l'arbre) | réorganisation libre par le comportement standard de Qt |

Le libellé de rôle (`source`/`target`/`context`) est mémorisé dans un rôle de
données (`kSlotNameRole`) et survit au renommage et aux déplacements internes.

## 5. Panneau Rule Config

Éditeur de propriétés de la sélection courante. Le formulaire est reconstruit à
chaque changement de sélection (un seul conteneur, remplacé intégralement).

| Sélection | Mode | Contenu |
|---|---|---|
| Règle de Rule Library | lecture seule | Nom, Description, propriétés avec valeurs par défaut |
| RuleFile de l'arbre | éditable | Nom, Description |
| Folder de l'arbre | éditable | Nom, Description |
| Rule de l'arbre | éditable | Nom, Description, propriétés (valeurs propres à l'instance) |
| Rule dans un slot | éditable | Nom édité sans le préfixe (`source :`), préfixe restitué à l'écriture |
| Slot vide | lecture seule | nom de l'emplacement |
| Rien | — | « No selection » |

Écriture immédiate dans l'arbre (pas de bouton Apply) :

- **Nom** → modifie le libellé du nœud (préfixe de slot préservé) ;
- **Description** → stockée sur le nœud ;
- **Propriétés** → stockées dans une map (nom → valeur) propre à l'instance.

Éditeurs par type de propriété : texte = QLineEdit, float = QDoubleSpinBox,
booléen = QCheckBox.

## 6. Catalogue de règles

Définitions partagées (`modules/rule/Catalogue.cpp`) — nom, famille,
description (les facettes IDS reprennent les requirement templates de la
spécification IDS), propriétés et emplacements.

Propriétés des règles « inventées » :

| Règle | Propriétés |
|---|---|
| Name Contains | Search Text (texte), Case Sensitive (bool) |
| Minimum Volume | Threshold (float), Unit (texte) |
| Maximum Height | Limit (float), Unit (texte), Warn Only (bool) |
| Minimum Distance | Distance (float), Unit (texte) |
| Overlap Check | Tolerance (float), Report First Only (bool) |
| Rule Chain | Separator (texte), Steps (float) |
| Group Filter | Group Name (texte), Negate (bool) |

Propriétés des facettes IDS (issues de `src/ifctester/ifctester/facet.py`,
paramètres réels de la spécification IDS) :

| Facette | Propriétés | Défauts |
|---|---|---|
| Entity | Name, Predefined Type, Instructions | IFCWALL, "", "" |
| Attribute | Name, Value, Cardinality, Instructions | Name, "", required, "" |
| Property | Property Set, Base Name, Value, Data Type, Cardinality | Property_Set, PropertyName, "", "", required |
| Classification | Value, System, Cardinality | "", "", required |
| Material | Value, Cardinality | "", required |

Cardinality accepte les valeurs `required`, `optional`, `prohibited`.

## 7. Ce qui n'est pas encore couvert (non-goals actuels)

- **Persistence** : Save/Load en « Not Working » ; rien n'est écrit sur disque.
- **Exécution** : les règles ne sont pas évaluées (pas de lien avec le moteur
  ifctester ni avec la coloration du viewport).
- **Cardinality** libre (champ texte, non contrainte aux trois valeurs).
- Un slot rempli ne peut pas être vidé ou réassigné ; un RuleFile ne peut pas
  être supprimé ; les dossiers ne sont pas renommables via l'arbre.
- La facette IDS **PartOf** (name, predefinedType, relation, cardinality)
  existe dans ifctester mais n'est pas dans la bibliothèque.
