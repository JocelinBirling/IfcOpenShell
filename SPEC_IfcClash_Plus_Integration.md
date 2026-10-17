# Spécification d'intégration — moteur IfcClash_Plus dans BonsaiViewer

Document local (ne pas committer). Spécification de l'intégration du moteur de
règles Python **IfcClash_Plus** dans **BonsaiViewer** (`src/bonsaiviewer/`).
Complète `SPEC_GeometryRule_UI.md` et `SPEC_Clash_Filter_UI.md` : ces deux
documents décrivent l'interface existante ; le présent document spécifie le
remplacement du moteur C++ par le moteur Python, et les évolutions d'UI qui en
découlent.

Sources de référence :
- Moteur : `IfcClash_Plus/ifcclash_plus/` (RuleClass.py, Rules.py,
  CustomOBB.py, clash_utils.py, booleanrule.py, serialization.py)
- Contrat de résultats : `IfcClash_Plus/doc/specification_ResultData.md`
  (ClashResultInfo)
- État des règles : `IfcClash_Plus/Rapport.md` (bugs bloquants, règles ⚠️)
- UI existante : `SPEC_GeometryRule_UI.md`, `SPEC_Clash_Filter_UI.md`

## 0. Décisions actées

| Sujet | Décision |
|---|---|
| Exécution du moteur | **Processus externe + IPC** (pas d'interpréteur embarqué) |
| Distribution Python | **Environnement détecté/configuré par l'utilisateur** (réglage chemin d'interpréteur dans BonsaiViewer, pas d'env embarquée) |
| Port C++ existant (`modules/rule/engine/ObbAbove.{h,cpp}`, `Obb.{h,cpp}`) | **Supprimé** — toute la détection passe par le moteur Python |
| Périmètre de règles | **Catalogue complet** de `Rules.py` (~28 règles), règles ⚠️ à finaliser côté moteur |
| Source des modèles | **Chemins de session** : le moteur re-ouvre les IFC depuis le disque |
| Display Input / Display Result | **Contrat d'affichage IPC** : le moteur renvoie des données de dessin JSON, le viewer les rend |
| Fonctionnalités avancées | **Toutes dans le périmètre** : exceptions, grouping, criticité/acteur, must-check, cascade SelectRule, activation de dossiers, export BCF |
| Schéma de résultats | **Liste complète des ClashResult** renvoyée au viewer, incluant le bloc `ClashResultInfo` |
| Persistance | **Format JSON de `serialization.py`** (round-trip garanti avec la CLI Python) |

## 1. Objectif et périmètre

BonsaiViewer héberge aujourd'hui deux espaces de travail — **GeometryRule** et
**Clash Filter** — pilotés par un port C++ limité à la seule règle OBB Above,
avec des facettes IDS interprétées à la main. Cette spécification remplace ce
port par le moteur Python complet IfcClash_Plus, exécuté dans un processus
séparé, et étend les deux pages pour couvrir tout le catalogue de règles et
les fonctionnalités avancées du moteur.

Dans le périmètre :
- le catalogue complet des règles (OneObjectRule, TwoObjectRule, ComplexRule)
  et des facettes IDS ;
- les exceptions par paire (`booleanrule`), le grouping, la criticité, l'acteur,
  le must-check absolu/relatif, la cascade `SelectRule`, l'activation
  conditionnelle des dossiers, l'export BCF ;
- la persistance Save/Load au format JSON `serialization.py` ;
- Display Input / Display Result via un contrat de données d'affichage.

Hors périmètre (section 13) : transformations de fédération du viewer,
persistance des statuts de revue (Valide/ND/Invalide), moteur C++.

## 2. Architecture générale

```
┌─────────────────────────── BonsaiViewer (C++/Qt) ───────────────────────────┐
│  pages GeometryRule / Clash Filter                                          │
│  modules/rule/ : RulePanel, RuleConfigPanel, RuleLibraryPanel,             │
│                  ResultPanel, ResultInfoPanel, Catalogue (conteneur)        │
│  modules/rule/engineproxy/  ← remplace modules/rule/engine/               │
│    EngineProcess (QProcess, cycle de vie, crash/respawn)                    │
│    EngineClient (requêtes/réponses, files d'événements)                     │
│    EngineSettings (interpréteur, diagnostic)                               │
└──────────┬────────────────────────────────────────────────▲────────────────┘
           │ JSON Lines sur stdin                           │ JSON Lines sur stdout
           ▼                                                │
┌───────────────────── processus moteur (Python) ───────────┴─────────────────┐
│  python -m ifcclash_plus.ipc       (nouveau point d'entrée)                 │
│  boucle de requêtes : catalogue | run | preview | select | cancel | ...     │
│  ifcclash_plus : RuleFile/RuleFolder, SelectFacet/SelectRule, Rules.py,     │
│                  booleanrule, serialization, ClashResult/ClashResultInfo   │
│  ifcopenshell.geom (multiprocessing interne), ifctester, pythonocc          │
└─────────────────────────────────────────────────────────────────────────────┘
```

Principes :

1. **Un seul moteur par session.** Le processus moteur démarre au premier
   besoin (bouton Run, ouverture de l'onglet GeometryRule) et vit tant que
   BonsaiViewer est ouvert. `EngineProcess` le relance s'il meurt ; un run en
   cours est déclarée interrompu et l'utilisateur est informé.
2. **Découplage total.** Aucun lien binaire entre le viewer et le moteur ;
   seul le protocole JSON (section 4) est partagé. Le moteur reste un paquet
   Python installable et utilisable seul (CLI), identique pour les deux
   surfaces.
3. **Un job à la fois.** Le viewer n'envoie qu'une requête `run` à la fois ;
   les actions suivantes sont mises en file côté C++. Le moteur gère seul son
   parallélisme interne (itérateurs géométriques multi-cœurs).
4. **L'UI ne bloque jamais.** `EngineClient` est asynchrone (signaux Qt) ;
   la barre d'état affiche la progression des événements `progress`.
5. **Le catalogue est introspecté, pas codé deux fois.** La Rule Library, les
   formulaires de Rule Config et les slots sont construits depuis la réponse
   `catalogue` du moteur (section 5). `Catalogue.cpp` devient un conteneur de
   cette réponse, plus une copie statique des définitions.

## 3. Environnement Python

### 3.1 Configuration

- Nouvel onglet **Rule Engine** dans les Settings de BonsaiViewer :
  - **Interpreter path** : chemin de l'exécutable Python de l'environnement
    (conda ou venv) ;
  - **Engine package** : chemin du dépôt/paquet `ifcclash_plus`
    (installé ou en développement), par défaut auto-détecté depuis
    l'interpréteur (`python -c "import ifcclash_plus"`).
- Persistance dans les réglages de l'application (ViewerSettings).
- Aucune valeur par défaut implicite : sans interpréteur configuré, les
  actions moteurs sont désactivées et le premier accès affiche un dialogue de
  configuration avec un lien vers la documentation d'installation
  (conda-forge `pythonocc-core=*=novtk*` + pip `ifcopenshell==0.8.4.post1`,
  `ifctester`, `numpy`, `shapely`, `trimesh` — cf. `IfcClash_Plus/agent.md`).

### 3.2 Diagnostic

Au démarrage du processus, `EngineProcess` envoie `hello` et vérifie :
l'import de chaque dépendance et sa version, la disponibilité du module IPC,
la version du moteur. Un rapport lisible (dépendance manquante, wheel
ifcopenshell incompatible avec le Python de l'env, pythonocc absent) est
affiché dans un dialogue ; le moteur passe à l'état `unavailable` tant que
le diagnostic échoue. Le message d'erreur du moteur (stderr) est affiché
tel quel.

## 4. Protocole IPC

### 4.1 Transport

- **JSON Lines** : une requête/réponse/événement par ligne, UTF-8, sur les
  flux standard du processus (`stdin` du moteur = requêtes, `stdout` = 
  réponses et événements, `stderr` = logs non structurés, redirigés vers la
  console de debug du viewer).
- Chaque message : `{"id": "<uuid>", "type": "<type>", ...}`. Les réponses
  reprennent l'id de la requête ; les événements non sollicités
  (`progress`, `log`, `rule_result`...) portent l'id du job en cours.

### 4.2 Requêtes (viewer → moteur)

Charge utile **plate** : les champs métier vivent directement dans le message
(pas de bloc `job` imbriqué). Chaque message : `{"id": "<uuid>", "type":
"<type>", ...}`.

| Type | Charge utile | Réponse |
|---|---|---|
| `hello` | — | `reply` : `engine` (nom, version), `versions` (ifcopenshell, OCC, ifctester, python) |
| `catalogue` | — | `reply` : `catalogue` {`catalogue_version`, `families`, `states`, `rules`, `facets`} |
| `run` | `rule_file` (dict `serialize_rule_file`), `list_ifc_path` (chemins absolus des modèles de la session) | événements `progress`/`rule_start`/`rule_result`/`display`, puis `run_done` |
| `preview` | idem `run` (même charge) | événements `display` pour chaque règle, puis `run_done` ; un `run` dont une règle a l'état `Display_Input`/`Display_Result` produit aussi un `display` (et pas de `rule_result`) |
| `select` | un Select sérialisé (clé `select`) + `list_ifc_path` | `reply` : liste des éléments correspondants (références croisées, section 7.1) — pour « Afficher la sélection en lançant l'entity » |
| `cancel` | id du job | le moteur termine la règle en cours et émet `run_done` avec `cancelled: true` |
| `shutdown` | — | le moteur s'arrête proprement (à la fermeture de BonsaiViewer) |

### 4.3 Événements (moteur → viewer)

| Type | Charge utile | Usage UI |
|---|---|---|
| `progress` | `stage` (nom de règle/d'étape), `percent`, `message` | barre d'état |
| `rule_start` | `node_id`, `rule_type` | arbre : règle en cours |
| `rule_result` | `node_id`, `rule_type`, `results: [Résultat]` (section 7), `grouping` (groupes produits par le moteur), `state` (Final/Select/Display_*) | panneau Result ; un événement par règle terminée |
| `display` | `node_id`, `items: [DisplayItem]` (section 8) | overlays du viewport |
| `log` | `level`, `message` | console de debug |

`run_done` porte : `cancelled`, `error` (éventuellement), le nombre de
résultats par règle, et les `select` produits (pour la cascade — voir 6.2).

### 4.4 Cycle de vie et erreurs

- Processus mort pendant un job → `EngineProcess` émet un événement d'erreur,
  le job est déclaré échoué, l'utilisateur peut relancer (respawn implicite au
  job suivant).
- Timeout de requête (`hello`, `catalogue`, `select`) : 30 s ; les runs n'ont
  pas de timeout, l'annulation passe par `cancel` (SIGTERM en dernier recours).
- Messages malformés : ignorés et loggés des deux côtés ; le protocole ne
  prévoit pas de négociation de version au-delà de `hello` (incompatibilité →
  message utilisateur demandant la mise à jour du paquet moteur).

## 5. Registre de règles (introspection du catalogue)

Nouveau module côté moteur : `ifcclash_plus/registry.py` — une entrée par
règle/facette/sélection, source unique de vérité partagée par l'UI et la CLI :

```json
{
  "type": "OBB_Above",
  "family": "TwoObjectRule",
  "class": "ifcclash_plus.Rules.OBB_Above",
  "doc": "Détecte une cible pénétrant la bande de tolerance au-dessus de la source",
  "slots": ["source", "target"],
  "params": [
    {"name": "tolerance", "kind": "float", "default": 0.85,
     "unit": "model", "doc": "hauteur de la zone de détection au-dessus de la source"}
  ],
  "supports_states": ["Final", "Select", "Display_Input", "Display_Result"],
  "supports_exceptions": true, "supports_grouping": true,
  "supports_criticity": true, "supports_actor": true
}
```

- `kind` ∈ {`float`, `bool`, `string`, `enum`, `string_list`, `direction`} —
  c'est lui qui choisit l'éditeur de Rule Config (QDoubleSpinBox, QCheckBox,
  QLineEdit, combo, pile de lignes, triplet XYZ).
- Les facettes IDS (Entity, Attribute, Property, Classification, Material,
  PartOf) et les sélections (Facet Select, Rule Select — section 10.3) sont
  décrites par le même format.
- Familles et catalogues initiaux (liste indicative — la source de vérité est
  `Rules.py`, exposée via le registre) :
  - **OneObjectRule** : Volume, Area, TopOrBottomSurface, LateralSurface,
    ProjectedSurface, ObbHigh, ObbLength, Orientation, OneObjectFace,
    Alignement, FreeSpace (source + context) ;
  - **TwoObjectRule** : AngleBetween, Intersection, Clearance,
    ClearanceAxisToAxis, Collision, Ray_Check (context), Above, Below,
    OBB_Above, OBB_Below, OBB_Front_And_Back, OBB_Custom, SurfaceRecover,
    DirectView (context), TwoObjectsFace, ClearanceForDoors ;
  - **IDS Facet** : les six facettes de la spécification IDS.

## 6. Cycle d'exécution

### 6.1 Portées de run

Les déclencheurs existants sont conservés (Run Rule au ruban, clic droit sur
RuleFile/Folder/Rule, double-clic) mais tous passent par la même requête `run` :
le viewer sérialise le sous-arbre sélectionné au format
`serialize_rule_file` (le scope `folder`/`rule` devient un RuleFile
temporaire contenant ce sous-arbre). `list_ifc_path` est **toujours** réinjecté
par le viewer depuis les modèles chargés dans la session — les chemins
enregistrés dans le fichier de règles sont ignorés au run (section 9).

Le moteur exécute les règles dans l'ordre de l'arbre (cascade incluse),
et émet un `rule_result` par règle. À la fin d'un run réussi,
l'application bascule sur l'onglet Clash Filter (comportement actuel
conservé).

### 6.2 Cascade et états

- `SelectRule` produit des sélections intermédiaires consommées par les
  règles suivantes : ces états vivent dans le moteur pendant le job, jamais
  dans le viewer.
- Le viewer peut demander un run avec `state: "Select"` (résultat = sélection)
  via le même `run` ; le champ `state` de chaque règle est une propriété
  sérialisée du nœud (par défaut `Final`), éditable dans Rule Config.
- L'**activation des dossiers** (`activation_rule`) est évaluée par le moteur
  en début de dossier : un dossier non activé est sauté et signalé dans
  `progress`.

### 6.3 Progression et annulation

- `progress` émis par étape de pipeline (chargement IFC, sélection, géométrie,
  run par règle) ; le viewer l'affiche sans traitement fin.
- `cancel` : le moteur termine la règle en cours (point d'arrêt propre entre
  deux règles), renvoie les résultats déjà acquis avec `cancelled: true`.
  Les résultats d'un run annulé sont affichés normalement.

## 7. Schéma des résultats

### 7.1 Références croisées des éléments

Un résultat ne transporte **jamais** d'objet OCC ni d'entité Python : le
moteur exporte chaque élément sous forme sérialisable, que le viewer mappe
sur son `ElementRegistry` :

```json
"source": {
  "model": 0,                // index dans list_ifc_path du job
  "step_id": 1234,           // #id STEP
  "global_id": "3ElkXn...",
  "name": "Basic Wall:...",
  "ifc_type": "IfcWallStandardCase"
}
```

Le couple (index modèle, step_id) est la clé de jointure viewer ; les autres
champs sont de l'affichage.

### 7.2 Contrat ClashResult → JSON

Chaque résultat suit `doc/specification_ResultData.md` (le moteur fournit
`to_dict()` sur ClashResult/ClashResultInfo — à finaliser côté moteur si
absent) :

```json
{
  "id": "<uuid>",
  "type": "TwoObjectsResult",        // OneObjectResult | TwoObjectsResult | ComplexResult
  "rule": "OBB_Above",              // type de la règle productrice
  "status": true,                    // verdict, mis à False par les exceptions
  "criticity": ["Structural"],       // rempli par run_criticity
  "actor": ["Architecte"],          // rempli par run_actor
  "source": {...}, "target": {...}, // ComplexResult : "sources": [...], "targets": [...], "context": [...]
  "info": {
    "distance": 0.42, "penetration_depth": null, "contact_area": null,
    "ratio": null, "offset": null, "hit_ratio": null, "angle": null,
    "volume_intersection": null, "volume_ratio": null,
    "containment_ratio": null, "clearance_violated_volume": null,
    "source_points": [...], "target_points": [...],   // PointEntry
    "source_faces": [...], "target_faces": [...],    // FaceEntry
    "source_obbs": [...], "target_obbs": [...],       // ObbData
    "axis_mode": null, "axis_degraded": null,          // ClearanceAxisToAxis
    "raycast": {...},                                  // DirectView
    "zone": {...},                                     // ClearanceForDoors
    "primary_measure": "distance"
  }
}
```

Règles : seuls les champs renseignés sont exportés (`null` toléré), aucune
validation croisée à l'exécution — le contrat est garanti par les tests du
moteur.

### 7.3 Consommation côté viewer

- **Panneau Result** : colonnes existantes (Source, Target, Model, statuts de
  revue) **+** `Rule`, `Measure` (valeur de `primary_measure` + unité du
  vocabulaire), `Criticity`, `Actor`. Le regroupement existant (source/target
  entity, model) s'applique tel quel ; nouvelles options de regroupement
  miroir du moteur : `Group by criticity`, `Group by rule`.
- **Panneau Result Info** : rendu générique des blocs `info` — les scalaires en
  table (KeyValueTable), les paires de points/faces en listes numérotées,
  les ObbData/raycast/zone en sections avec actions d'affichage (section 8).
  Un bloc inconnu du viewer est affiché en JSON brut (extensibilité sans
  recompilation).
- Le viewer marque `status: false` (invalidé par exception) visuellement dans
  la ligne, indépendamment du statut de revue (Valide/ND/Invalide) qui reste
  une donnée de session du viewer.

## 8. Contrat d'affichage (Display Input / Display Result)

L'affichage OCC du moteur est remplacé par un rendu viewer. Le moteur convertit
ses `DisplayItem` (`RuleCheck._item_shape`) et les blocs `info` sérialisables
en items de dessin JSON ; le viewer les rend avec ses pipelines existants
(`setOverlayLines` pour les arêtes, extension mineure pour les faces pleines).

```json
{
  "node_id": "...", "category": "detection_zone",  // catégorie DISPLAY_CATEGORY du moteur
  "element": {"model": 0, "step_id": 1234},        // optionnel : rattachement
  "shape": {
    "kind": "edge",    "start": [x,y,z], "end": [x,y,z]
  // ou
    "kind": "face",    "corners": [[x,y,z], ...]          // polygone
  // ou
    "kind": "axis",    "center": [x,y,z], "direction": [x,y,z], "length": 2.0
  // ou
    "kind": "obb",     "center": [...], "axes": [...3 dirs], "half_sizes": [...], "label": "zone"
  // ou
    "kind": "segments","segments": [[[x,y,z],[x,y,z]], ...]  // raycast : hit/blocked
  // ou
    "kind": "zone",    "side": "Front", "leaf": "SingleSwingLeft",
                       "width": 0.9, "depth": 1.2, "height": 2.1  // ZoneData, solide reconstruit par le viewer
  },
  "color": [1.0, 0.0, 1.0], "transparency": 0.6, "line_style": "dashed" | "solid"
}
```

- Le mappage catégorie → couleur/style est côté viewer (table par défaut,
  extensible dans les Settings) : les catégories du moteur
  (`DISPLAY_CATEGORY` dans RuleClass.py) sont transportées telles quelles.
- « Afficher le display Input » et « Display_Result » envoient `preview` /
  `run` avec l'état correspondant ; les items reçus remplacent les overlays
  courants ; toute nouvelle mesure/recherche les efface (comportement
  actuel).
- Le double-clic sur une ligne de Result peut afficher les items du résultat
  sélectionné (points de contact, OBB, zone) via les blocs `info` — action
  « Afficher la géométrie du résultat » dans Result Info.

## 9. Persistance (Save / Load)

- **Format** : exactement celui de `serialization.py`
  (`serialize_rule_file` / `save_to_json`), extension `.ifcrule.json`. Les
  boutons Save / Load du panneau Rule remplacent les popups « Not Working ».
- **Round-trip garanti** : un fichier sauvé par BonsaiViewer se charge dans la
  CLI Python (`load_from_json`) et réciproquement — même schéma, y compris
  exceptions, grouping, criticité, acteur, must-check, activation, `SelectRule`
  (règles référencées sérialisées imbriquées par valeur).
- **Modèles** : `list_ifc_path` est sauvé pour référence, mais au run le viewer
  réinjecte toujours les chemins de la session courante ; le chargement
  n'exige pas que les fichiers existent.
- **Undo du New RuleFile** : la fermeture d'un RuleFile modifié non sauvé
  demande confirmation (le comportement « New remplace l'ancien » reste).
- Les statuts de revue (Valide/ND/Invalide) restent en mémoire de session ;
  ils sortent par l'export BCF (section 11.3).

## 10. Évolutions UI — page GeometryRule

L'existant (SPEC_GeometryRule_UI.md) est conservé ; évolutions :

### 10.1 Rule Library

Trois dossiers de règles — **OneObjectRule**, **TwoObjectRule**, **ComplexRule**
— alimentés par le registre (section 5), plus **IDS Facet** et **Select**
(Facet Select / Rule Select). Le glisser-déposer existant est inchangé.

### 10.2 Rule Config

- Les éditeurs sont pilotés par `kind` du registre, plus les propriétés
  complexes de facettes (Enumeration/Pattern/Bounds/Length) existantes.
- Nouvelles sections éditables pour une règle (repliables, même panneau) :
  - **State** : Final / Select / Display_Input / Display_Result (combo) ;
  - **Exceptions** : liste d'exceptions ; built-ins par relation
    (`Same_IfcSystem`, `Same_Building`, `Same_Storey`, `Same_Space`) en combo ;
    exception booléenne (`BooleanLeaf`/And/Or/Not) via un mini-éditeur
    d'arbre avec référence de règle ;
  - **Grouping** : mode (Entity, Property, Attribute, Source/Target... — les
    modes du moteur) + clés ;
  - **Criticité** : liste de paires {SelectFacet, label} — éditeur de facette
    réduit + texte ;
  - **Acteur** : idem paires {SelectFacet, label} ;
  - **Must-check** : activable, mode (`Absolute_Number`, `Absolute_Quantity`,
    `Relative_Number`, `Relative_Quantity`) + paramètres de référence.
- Un Folder éditable expose en plus son **Activation** (facette ou règle
  booléenne).

### 10.3 Slots et sélections

Le contenu d'un slot `source`/`target`/`context` devient un **Select** :
- déposer une facette (ou un groupe de facettes) → `SelectFacet`
  (applicabilité, avec `and_`/`or_` si plusieurs) ;
- déposer une **règle de l'arbre** → `SelectRule` référençant cette règle
  (cascade waterfall) ; la référence est sérialisée imbriquée par valeur ;
- un slot affiche son type de Select en préfixe secondaire
  (`source [facet]`, `source [rule]`).

### 10.4 Ruban

Inchangé (Run Rule, Show Results, Color Red), mais Run Rule passe par
l'`EngineClient`. Le bouton Color Red reste un effet viewer pur.

## 11. Évolutions UI — page Clash Filter

L'existant (SPEC_Clash_Filter_UI.md) est conservé ; évolutions :

### 11.1 Panneau Result

Colonnes et filtres étendus (section 7.3) ; le filtre texte reste sur les
noms source/target ; « Filter Only Non Determined » reste un filtre de revue.

### 11.2 Panneau Result Info

Rendu des blocs `ClashResultInfo` (section 7.3) avec actions d'affichage
(section 8) par bloc : points, faces, OBB, raycast, zone.

### 11.3 Export BCF

- Nouveau bouton **Export BCF** (groupe Regroup Result ou nouveau groupe
  Export) : exporte les résultats courants filtrés au format BCF, via le
  moteur (`BCF_Subject` / `ResultAnalyze`, à exposer comme requête IPC
  `export_bcf` avec les mêmes références croisées + statuts de revue du
  viewer + viewpoints construits depuis les items d'affichage de la
  section 8).
- Le moteur fournit la charge utile ; l'écriture du fichier se fait côté
  moteur (chemin de sortie fourni par le viewer dans la requête).

## 12. Migration — suppression du port C++

1. Supprimer `src/bonsaiviewer/modules/rule/engine/` (`ObbAbove.{h,cpp}`,
   `Obb.{h,cpp}`) et la conversion de géométrie multi-thread C++ associée.
2. Créer `src/bonsaiviewer/modules/rule/engineproxy/` : `EngineProcess`,
   `EngineClient`, `EngineSettings`, types de messages (une seule définition
   des structs de protocole, sérialisation JSON).
3. `Catalogue.cpp` : remplacer les définitions statiques par un chargement
   depuis la réponse `catalogue` (cache disque optionnel pour un
   démarrage sans moteur).
4. Run Rule / clic droit / double-clic : rerouter vers `EngineClient`.
5. Les interprétations C++ des facettes (Entity facet de
   `SPEC_GeometryRule_UI.md` §5) sont retirées — toutes les sélections
   passent par ifctester dans le moteur (IFC2X3 fallback et `USERDEFINED`
   inclus, supprimés des non-goals).
6. La coloration des paires (rouge/orange) et la bascule vers Clash Filter
   restent côté viewer, alimentées par les références croisées des résultats.

## 13. Limites connues et non-goals

- **Transformations de fédération** : le moteur travaille en coordonnées monde
  du fichier ; les `ModelTransformation` du viewer ne sont pas appliquées.
  Les modèles doivent partager le même référentiel (les clashs intra et
  inter-modèles restent détectés dans cette limite).
- **Fichiers requis sur disque** : le moteur re-ouvre les IFC depuis les
  chemins de session ; un modèle chargé depuis un flux sans chemin n'est pas
  clashable (désactivé pour le run, signalé à l'utilisateur).
- **Statuts de revue** : Valide/ND/Invalide restent en mémoire de session ;
  seul l'export BCF les emporte.
- **Performance** : première requête `run` d'une session inclut le chargement
  IFC côté moteur (géométrie non partagée avec le viewer) ; le moteur peut
  mettre en cache les fichiers ouverts entre les jobs d'une même session
  (invalidation par chemin + mtime).
- **Multi-run concurrent** : un job à la fois (files côté viewer).
- **Licence/distribution** : le découplage par processus maintient le paquet
  moteur distribué séparément ; l'utilisateur installe l'environnement
  lui-même. La licence d'IfcClash_Plus reste à confirmer (question ouverte).

## 14. Prérequis côté moteur (IfcClash_Plus)

Le catalogue complet suppose les finalisations suivantes (référence :
`Rapport.md`) — elles conditionnent les entrées correspondantes du registre :

- fixer `RuleCheckOneObject.run_actor` (crash si acteur défini) et
  `run_exception` (`Same_Building/Storey/Space` → TypeError) ;
- corriger les règles en coordonnées locales (LateralSurface,
  ProjectedSurface) et les conventions Above/Below (Rapport.md §2) ;
- stabiliser ObbHigh (axes permutés par OCCT), ObbLength, Ray_Check,
  OBB_Front_And_Back ;
- ajouter les préfiltres spatiaux aux 5 règles qui en manquent ;
- implémenter `to_dict()` sur `ClashResult`/`ClashResultInfo` conformément à
  `doc/specification_ResultData.md` (section 4) ;
- écrire `ifcclash_plus/ipc.py` (boucle JSON Lines) et
  `ifcclash_plus/registry.py` (registre de la section 5) ;
- `Grouping` : les modes stubs (PartOf, Material, Classification, Closeness)
  sont soit implémentés, soit retirés du registre (le viewer n'affiche que ce
  que le moteur déclare).

## 15. Plan de mise en œuvre

| Phase | Contenu | Critère de sortie |
|---|---|---|
| 1 — Moteur IPC | `ipc.py`, `registry.py`, `hello`/`catalogue`/`run` sur un RuleFile minimal ; `to_dict` des résultats | la CLI Python exécute un RuleFile via IPC et renvoie des résultats JSON validés par un test |
| 2 — Viewer proxy | `engineproxy/` (QProcess, protocole, settings + diagnostic), Catalogue dynamique ; OBB_Above rejoué via le moteur ; suppression du port C++ | parité fonctionnelle avec l'existant (run, couleurs, résultats dans Clash Filter) |
| 3 — Catalogue complet | toutes les règles du registre dans la Rule Library + éditeurs `kind` ; slots Select (facette/règle) ; persistance Save/Load round-trip CLI | un RuleFile sérialisé dans le viewer s'exécute à l'identique dans la CLI |
| 4 — Avancé | exceptions, grouping + criticité + acteur, must-check, activation de dossiers, cascade SelectRule | round-trip complet `serialization.py` des configurations avancées |
| 5 — Affichage & BCF | contrat d'affichage (display input/result, blocs info), export BCF | display OBB_Above visuellement équivalent à l'overlay actuel ; export BCF relisible |

## 16. Tests et validation

- **Moteur** (pytest, un fichier à la fois, display tests exclusés — cf.
  `agent.md`) : round-trip IPC (requête → JSON → re-désérialisation),
  contrat `to_dict` par règle sur les maquettes de calibration
  `IFC_Test_Model/` (paires attendues connues), registre exhaustif
  (une entrée par classe de `Rules.py`, détecté par introspection).
- **Viewer** : test du proxy contre un moteur stub (processus Python factice
  rejouant des messages enregistrés) — pas de dépendance à l'environnement
  conda dans les tests C++ ; test d'intégration manuel avec le vrai moteur.
- **Contractuel** : un fichier `.ifcrule.json` produit par le viewer et
  exécuté par la CLI doit donner les mêmes paires que le run viewer
  (échantillon : OBB_Above, Intersection, ClearanceForDoors).

## 17. Contraintes de contribution (AGENTS.md du dépôt IfcOpenShell)

- **Licence** : `src/bonsaiviewer/` est du code bibliothèque →
  **LGPL-3.0-or-later**. L'architecture par processus externe (section 2)
  maintient le découplage : aucun lien binaire, aucune inclusion de code
  IfcClash_Plus dans le viewer ; le paquet moteur est distribué séparément.
  La licence d'IfcClash_Plus reste à confirmer (question ouverte).
- **Marquage AI** : tout nouveau fichier généré (C++ du proxy, `ipc.py`,
  `registry.py`, tests) porte le commentaire « generated with the assistance
  of an AI coding tool » ; les commits modifiant du code existant le notent
  dans le corps du message.
- **Style** : C++17 minimum, clang-format + clang-tidy (config du dépôt) pour
  le viewer ; ruff format + ruff, 120 colonnes, pour le code Python.
- **PR** : une fonctionnalité par PR, commits petits et autonomes — le plan en
  5 phases (section 15) délimite les découpes naturelles (une phase = un ou
  plusieurs PR).
- **Tests** : les changements de code testables viennent avec leurs tests
  (section 16) ; un PR de la phase 2 (proxy) inclut le moteur stub de test.
