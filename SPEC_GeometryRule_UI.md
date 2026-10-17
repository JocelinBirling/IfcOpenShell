# Spécification UI — Espace de travail GeometryRule (BonsaiViewer)

Document local (ne pas committer). Spécification de l'interface telle qu'elle a été
conçue et implémentée dans `src/bonsaiviewer/` et `src/bonsaiviewer/modules/rule/`.

## 1. Vue d'ensemble

L'onglet **GeometryRule** du ruban est un espace de travail dédié à la
construction de jeux de règles de géométrie. Il réorganise l'interface :

```
┌──────────────────────────────────────────────────────────────────┐
│ Ruban : Home | Navigate | Inspect | Panels | GeometryRule |     │
│        Clash Filter                                               │
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
  Rule Library / Rule Config se masquent. Le panneau Rule n'est visible
  que dans GeometryRule et Clash Filter.

## 2. Ruban — page GeometryRule

- Même squelette que les autres pages (frame `ribbonPage`, groupes de boutons).
- Groupe **RUN** : boutons **Run Rule** (icône `:/icons/intersect.svg`) et
  **Show Results** (icône `:/icons/check.svg`) :
  - Run Rule sérialise le RuleFile ouvert (`engineproxy/RuleFileJson`, format
    exact de `ifcclash_plus/serialization.py`) et l'envoie au moteur de règles
    Python **IfcClash_Plus** dans un processus externe (JSON Lines sur
    stdin/stdout, `engineproxy/EngineClient`) ; `list_ifc_path` = chemins des
    modèles chargés dans la session (les modèles sans chemin sont skippés et
    comptés dans le message de fin) ;
  - le moteur exécute **toutes les règles complètes du fichier** (le catalogue
    complet, pas seulement OBB Above), avec la géométrie de son côté ; le
    viewer ne bloque jamais (signaux Qt), la barre d'état affiche les
    événements `progress` ; un seul job à la fois ;
  - à la fin : les sources des paires en échec sont colorées en rouge, les
    cibles en orange (`ViewportWindow::colorElements` → `setObjectsColor`),
    les paires remplissent le panneau Result du workspace Clash Filter et
    l'application **bascule automatiquement sur l'onglet Clash Filter** ;
  - Show Results bascule sur l'onglet Clash Filter (les résultats y vivent).
- **Prérequis moteur** : Settings > General, **Rule Engine Interpreter**
  (chemin de l'exécutable Python de l'environnement conda) et **Rule Engine
  Script** (`ifcclash_plus/ipc.py`). Vides par défaut : les fonctions moteur
  sont désactivées. Le flat layout du paquet impose `python …/ipc.py`
  (le répertoire du script est le working directory), pas `python -m`.
- Groupe **COLOR** : bouton **Color Red** :
  - bouton à bascule (checkable), icône `:/icons/paint.svg` ;
  - activé : tous les objets de tous les modèles chargés sont colorés en rouge ;
  - désactivé : les couleurs d'origine (vertex colors) sont restaurées.
- Le bouton Color Red n'apparaît pas ailleurs (retiré de la page Inspect).

## 3. Panneau Rule Library

Bibliothèque de règles, organisée en **dossiers refermables** (composant
`Section` : en-tête cliquable, flèche haut/bas, corps repliable) :

| Dossier | Entrées |
|---|---|
| OneObjectRule | Volume, Area, TopOrBottomSurface, LateralSurface, ProjectedSurface, ObbHigh, ObbLength, Orientation, OneObjectFace |
| TwoObjectRule | AngleBetween, Intersection, Clearance, ClearanceAxisToAxis, Collision, Above, Below, OBB_Above, OBB_Below, OBB_Front_And_Back, OBB_Custom, SurfaceRecover, TwoObjectsFace, ClearanceForDoors |
| ComplexRule | Alignement, Ray_Check, DirectView, FreeSpace |
| IDS Facet | Entity Facet, Attribute Facet, Property Facet, Classification Facet, Material Facet, PartOf Facet |

Le catalogue vient du moteur : la réponse `catalogue` de l'IPC
(`ifcclash_plus/registry.py`, 27 règles + 6 facettes) remplit les sections
via `Catalogue::setCatalogueFromEngine()` ; `Catalogue.cpp` embarque une
copie statique du même registre en repli (démarrage sans moteur). Les
paramètres portent les noms canoniques du moteur (`tolerance`,
`volume_min`, `predefinedType`…) : la map de valeurs de l'instance sérialise
directement, sans traduction.

Comportement des entrées :

- **Clic** sur une entrée → la définition (Nom, Description, propriétés,
  valeurs par défaut) s'affiche dans Rule Config **en lecture seule**.
- **Glisser** une entrée → source d'un drag & drop vers l'arborescence du
  panneau Rule.

## 4. Panneau Rule

### 4.1 Barre de boutons

| Bouton | Comportement |
|---|---|
| Save | boîte de dialogue fichier (`.ifcrule.json`) : sérialise l'arbre au format exact de `ifcclash_plus/serialization.py` (règles, dossiers, slots, restrictions), avec un bloc `viewer` {label, description} par nœud — extension ignorée par la CLI, round-trip garanti |
| Load | boîte de dialogue fichier (`.ifcrule.json`) : reconstruit l'arbre (RuleFile, dossiers, règles, slots remplis) depuis un fichier sauvé par le viewer **ou par la CLI Python** |
| New RuleFile | crée un RuleFile « Rule File #N » **à la place de l'ancien** : l'arbre est vidé, le nouveau fichier est le seul ouvert |
| Add Folder | crée un dossier « Folder #N » dans l'élément sélectionné (RuleFile ou dossier) ; sans sélection, s'attache au RuleFile ouvert (créé au besoin). Un dossier ne peut jamais vivre à la racine |

### 4.2 Arborescence

Types de nœuds :

| Type | Règle de contenu |
|---|---|
| **RuleFile** | contient une à plusieurs règles (et des dossiers). **Une seule RuleFile ouverte en même temps.** |
| **Folder** | refermable ; peut contenir un dossier et 0 à n règles. Le Folder est forcément sous un RuleFile, il ne peut pas être seul. |
| **Rule** | règle ou facette instanciée depuis la bibliothèque |
| **Slot** | emplacement vide sous une règle (`source`, `target`, `context`), affiché en gris. Un slot rempli est **souligné** (utilisé). |

### 4.3 Emplacements (slots) par règle

Chaque règle instanciée expose ses emplacements enfants selon sa famille :

| Famille | Emplacements |
|---|---|
| OneObjectRule | `source` |
| TwoObjectRule | `source`, `target` |
| ComplexRule | `source`, `target`, `context` |
| Facettes IDS (Entity, Attribute, Property, Classification, Material, PartOf) | **Aucun emplacement** |

### 4.4 Matrice du drag & drop

| Cible du dépôt | Résultat |
|---|---|
| Zone vide de l'arbre | si un RuleFile est ouvert, la règle y est ajoutée ; sinon un nouveau RuleFile est créé |
| RuleFile ou Folder | la règle est ajoutée à l'intérieur |
| Rule (règle instanciée) | la nouvelle règle est ajoutée en sibling juste après |
| Slot vide | l'emplacement est **rempli sur place** : texte `source : NomDeLaRegle`, libellé souligné ; la règle expose à son tour ses propres emplacements (récursif) |
| Déplacement interne, entre les éléments | réorganisation libre par le comportement standard de Qt |
| Déplacement interne, **sur** un élément existant (Rule ou Folder) | l'élément est **remplacé** : l'élément draggué prend sa place, hérite du rôle de slot (`source : …`) si la cible était un slot rempli |

Le libellé de rôle (`source`/`target`/`context`) est mémorisé dans un rôle de
données (`kSlotNameRole`) et survit au renommage, aux déplacements internes et
aux remplacements.

Dans l'arborescence, une IDS Facet est uniquement déposable dans un emplacement. En aucun cas, je peux avoir une IDS facet directement sous un RuleFile ou bien un Folder.

### 4.5 Effet du clic droit

| Type | Fonction du Clic Droit |
|---|---|
| **RuleFile** | **Lancer le fichier de règle** : requête `run` IPC sur le RuleFile sérialisé (toutes les règles complètes qu'il contient, récursivement dans les dossiers) |
| **Folder** | **Lancer les règles du dossier** : idem, restreint au sous-arbre (scope sérialisé en RuleFile temporaire) ; **Coller la Rule** : insère la règle copiée (avec sa configuration et ses emplacements) dans le dossier ; désactivé si rien n'est copié |
| **Rule** | **Lancer la règle** (sous-arbre de la règle via `run`) ; **Afficher le display Input** : requête `preview` IPC — le moteur renvoie des items de dessin (contrat d'affichage : OBB de zones magenta pointillé, OBB d'objets jaune plein, arêtes/faces/axes/segments) rendus en wireframe par `setOverlayLines` ; **Copier la rule, sa configuration et les emplacements** : clone profond dans un presse-papier interne au panneau |
| **Facet** | **Afficher la sélection en lançant l'entity** : requête `select` IPC — la sélection (ifctester) est évaluée par le moteur sur tous les modèles de la session, les éléments renvoyés (références croisées) sont affichés à l'écran. La page doit se vider, et seul les éléments sélectioner par la facet doivent appraitre à l'écran. |

Sur tous les éléments de l'arborescence, on doit ajouter la fonction "Supprimer". Cette fonction retire de l'arborscence l'objet en question. Il est aussi important de retirer de l'arborescence les éléments fils de l'élement.

Le **double-clic** sur une règle la lance ; sur une facette, il affiche sa
sélection (raccourci des actions du clic droit).

## 5. Panneau Rule Config

Éditeur de propriétés de la sélection courante. Le formulaire est reconstruit
à chaque changement de sélection (un seul conteneur, remplacé intégralement).

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

Éditeurs par type de propriété (le `kind` du registre choisit l'éditeur) :
texte = QLineEdit, float = QDoubleSpinBox, float nullable = champ texte
(vide = absent), entier = QSpinBox, booléen = QCheckBox, enum = combo
éditable, liste de chaînes = une valeur par ligne, direction = 3 spinbox
XYZ. Tous les éléments de l'arbre (RuleFile, Folder, Rule, Rule en slot)
sont éditables, description comprise.

### Facet

Pour les facettes, chaque propriété texte (hors `Instructions`, pure
documentation) peut être une **simple valeur** ou une **propriété complexe**.
Par défaut : simple valeur (QLineEdit). Une checkbox « Complex » remplace le
champ par une liste déroulante : **Enumeration**, **Pattern**, **Bounds**,
**Length**.

- **Enumeration** : une pile de zones texte avec boutons **+** (ajoute une
  ligne) et **-** (retire la dernière) ;
- **Pattern**, **Bounds**, **Length** : un champ texte unique.

Stockage dans la map de l'instance :

- simple : `QString` ;
- complexe : `QVariantMap { "restriction": <kind>, "values": QStringList }`
  (une entrée par ligne pour Enumeration, le texte seul à [0] pour les autres).

Interprétation par le moteur (facette Entity) : Enumeration = l'élément
correspond s'il correspond à **une** des valeurs ; Pattern = expression
régulière (recherche partielle, insensible à la casse) ; Bounds = `min,max`
comparés à la valeur numérique ; Length = `min,max` comparés à la longueur du
texte.

## 6. Catalogue de règles

Source de vérité : le registre du moteur (`ifcclash_plus/registry.py`,
réponse IPC `catalogue`). `modules/rule/Catalogue.cpp` en est le conteneur
miroir (copie statique + remplacement dynamique
`setCatalogueFromEngine()`), avec label d'affichage, famille, description,
propriétés (nom, kind, défaut, nullable, enum, doc, unité) et emplacements.

### 6.1 Panneau Clash (résultats) — remplacé par Result / Result Info

Le panneau Clash a été retiré : les résultats vivent désormais dans le
workspace **Clash Filter** (`modules/rule/ResultPanel` + `ResultInfoPanel`,
voir SPEC_Clash_Filter_UI.md). Run Rule et le double-clic sur une règle y
affichent les paires en échec ; l'application bascule sur l'onglet Clash
Filter à la fin d'un run.

### 6.2 Propriétés des règles

Les paramètres des règles portent les noms canoniques du moteur (ex. OBB
Above : `tolerance`, float, défaut 1.0, hauteur de la bande de détection
au-dessus de la source, en unités du fichier IFC). La liste complète vit dans
le registre (27 règles) : chaque règle expose ses paramètres avec kind
(float, bool, string, enum, string_list, direction…), défaut, unité et doc.

Propriétés des facettes IDS (issues de `src/ifctester/ifctester/facet.py`,
paramètres réels de la spécification IDS) :

| Facette | Propriétés | Défauts |
|---|---|---|
| Entity | Name, Predefined Type, Instructions | IFCWALL, "", "" |
| Attribute | Name, Value, Cardinality, Instructions | Name, "", required, "" |
| Property | Property Set, Base Name, Value, Data Type, Cardinality | Property_Set, PropertyName, "", "", required |
| Classification | Value, System, Cardinality | "", "", required |
| Material | Value, Cardinality | "", required |
| PartOf | Name, Predefined Type, Relation, Cardinality | "", "", "", required |

Cardinality accepte les valeurs `required`, `optional`, `prohibited`.
Seuls les attributs de facettes sont traduits en clés de sérialisation
(`predefinedType`→`predefined_type`, `propertySet`→`property_set`,
`baseName`→`base_name`, `dataType`→`data_type`).

## 7. Ce qui n'est pas encore couvert (non-goals actuels)

- **Fonctions avancées non éditables** (phase 4 de la spec d'intégration) :
  exceptions par paire (`booleanrule`), grouping, criticité/acteur, must-check
  absolu/relatif, activation conditionnelle des dossiers, cascade
  `SelectRule` — le moteur les exécute si le JSON les porte (serialization.py
  les sérialise) mais le viewer ne sait pas encore les éditer : elles partent
  absentes du JSON produit. L'état `State` d'une règle est toujours `Final`
  côté viewer (pas de Select/Display éditable).
- **Export BCF** (phase 5) : requête IPC à ajouter côté moteur + bouton
  viewer. Les blocs `info` des résultats (points/faces/OBB/raycast/zone) sont
  transportés mais Result Info ne les rend pas encore.
- **Display Input** : les items sont dessinés avec le pipeline des lignes de
  mesure — utiliser un outil de mesure les efface (pas de fusion
  mesuré/zones). Un run avec état Display_Input produit les items mais
  n'affiche pas les résultats de ces règles.
- **Statuts non persistés** : les verdicts Valide / Non Déterminé / Invalide
  vivent dans le panneau Result ; un nouveau run les remet à Non Déterminé.
  Le presse-papier Copier/Coller et le contexte Clash Filter sont en mémoire
  de session uniquement.
- Le moteur travaille en coordonnées monde du fichier par modèle ; les
  transformations de fédération (ModelTransformation) ne sont pas appliquées.
  Les sources et cibles de tous les modèles sont mises en commun avant
  l'appariement : les clashs intra-modèle et inter-modèles sont détectés,
  dans la limite où les maquettes partagent le même référentiel.
  Un modèle chargé sans chemin disque n'est pas clashable (skippé).
- **Cardinality** libre (champ texte, non contrainte aux trois valeurs).
- Un slot rempli ne peut pas être vidé (remplacé, ou supprimé avec
  **Supprimer**) ; les dossiers ne sont pas renommables via l'arbre.
  **Supprimer** est disponible sur tous les éléments (RuleFile, Folder,
  Rule, Rule en slot) et emporte l'élément avec tous ses fils ; supprimer
  le RuleFile laisse l'arbre vide.
- Règles fragiles côté moteur (Rapport.md) : le survit par règle est géré
  (log + run continue), mais certaines règles restent à corriger
  (coordonnées locales LateralSurface/ProjectedSurface, Above/Below, Ray_Check,
  OBB_Front_And_Back, ObbHigh, run_actor).
