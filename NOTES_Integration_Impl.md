# Notes de reprise — Intégration IfcClash_Plus dans BonsaiViewer

Document de travail (ne pas committer). Écrit à l'arrêt de la session du
2026-10-15. Objectif : permettre de reprendre l'implémentation là où elle
s'est arrêtée. Références : `SPEC_IfcClash_Plus_Integration.md` (la spec
d'intégration), `SPEC_GeometryRule_UI.md`, `SPEC_Clash_Filter_UI.md`.

## 1. Décisions actées (session précédente)

- Moteur Python en **processus externe + IPC JSON Lines** (pas d'interpréteur
  embarqué).
- Environnement Python **détecté/configuré** par l'utilisateur (Settings >
  General : Rule Engine Interpreter + Rule Engine Script).
- Port C++ (`modules/rule/engine/ObbAbove.{h,cpp}`, `Obb.{h,cpp}`) :
  **supprimé** (git rm). Toute la détection passe par le moteur Python.
- Périmètre : **catalogue complet** (~28 règles), chemins de session pour les
  modèles, contrat d'affichage IPC, tout l'avancé en spec (exceptions,
  grouping/criticité/acteur, must-check, cascade SelectRule, BCF).

## 2. État des deux dépôts

### IfcClash_Plus (moteur Python) — FAIT et TESTÉ

Nouveaux fichiers :
- `ifcclash_plus/registry.py` — le registre : 27 règles + 6 facettes IDS,
  familles/slots/params (name, kind, default, unit, doc). Source de vérité
  partagée avec le viewer (réponse `catalogue`).
- `ifcclash_plus/result_export.py` — cross-references d'éléments
  (model/step_id/global_id/name/ifc_type) + `result_to_dict` (ClashResult
  + ClashResultInfo.to_dict, gère OneObject/TwoObjects/Complex).
- `ifcclash_plus/display_export.py` — DisplayItem → JSON : tuples légers
  (edge/face/axis), Bnd_OBB → {center, axes, half_sizes}, TopoDS_Shape →
  arêtes discrétisées (EDGE_DEFLECTION = 0.05).
- `ifcclash_plus/ipc.py` — le serveur JSON Lines. Requêtes : hello,
  catalogue, run, preview, select, cancel (poll entre règles), shutdown.
  Événements : ready, progress, rule_start, rule_result, display, log,
  run_done. Cache des IFC ouverts par (path, mtime). Les états
  Display_Input/Display_Result sont remappés sur Final + export display
  (aucune fenêtre OCC ouverte).

Modifiés :
- `ifcclash_plus/serialization.py` — `serialize_facet`/`deserialize_facet`
  réécrits : tous les paramètres réels des facettes (y compris cardinality,
  instructions — l'ancien code plantait sur Classification/Material via
  `facet.name` inexistant) + blocs de restriction
  `{"restriction": "Enumeration|Pattern|Bounds|Length", "values": [...]}`
  dans les deux sens (helpers `_serialize_facet_param` /
  `_deserialize_facet_param`). Rétro-compatible (clés absentes → défauts).

IMPORTANT : le paquet s'importe en **flat layout** (`from RuleClass import
...`). Lancer en script : `python /path/to/ifcclash_plus/ipc.py` (le répertoire
est sys.path[0]). `python -m ifcclash_plus.ipc` est PIÉGEUX (__init__.py crée
un double espace de noms).

Tests moteur validés (env :
`/home/jocelin/Documents/07 - Programmation/IfcOpenshell/Test_IfcOpenshell/.conda/bin/python`,
Python 3.14.3, ifcopenshell 0.8.4.post1, pythonocc 7.9.3, ifctester 0.8.4) :
- hello/catalogue OK (versions remontées).
- run OBB_Above (IFCFURNISHINGELEMENT vs IFCSLAB, tolerance 0.85) sur
  Ifc_Model/Ifc2x3_Duplex_Architecture.ifc → **8 paires**, cross-refs + OBB
  (labels zone/object) exportés.
- select Entity IFCDOOR → **14 éléments**.
- preview → **61 items OBB** (display contract).
- Commande de test type :
  `cat test.jsonl | <python> ifcclash_plus/ipc.py` avec stdin maintenu ouvert
  (à l'EOF le moteur annule le run en cours : comportement voulu, QProcess
  garde stdin ouvert en prod).

### IfcOpenShell (viewer C++) — ÉCRIT, COMPILATION EN COURS

Nouveaux fichiers (`src/bonsaiviewer/modules/rule/engineproxy/`) :
- `Messages.{h,cpp}` — ElementRef, EngineResult (avec info brut +
  measure nominale), DisplayItem (edge/face/axis/obb/segments) + parse JSON.
- `EngineClient.{h,cpp}` — QProcess + JSON Lines : ensureStarted (avec
  settings AppSettings), requestCatalogue/Run(preview)/Select, signaux
  engineReady/engineFailed/engineLog/catalogueReady/progress/ruleResult/
  displayItemsReady/runFinished/selectFinished. Un job à la fois.
- `RuleFileJson.{h,cpp}` — arbre ↔ JSON **format exact de serialization.py**
  (règles, dossiers, slots : facette → SelectFacet, règle → SelectRule
  imbriquée ; mapping des noms d'attributs de facettes
  predefinedType→predefined_type, propertySet→property_set, baseName→base_name,
  dataType→data_type). Bloc `viewer` {label, description} ajouté aux nœuds
  (clés inconnues ignorées par la CLI → round-trip garanti). Fonctions :
  ruleFileToJson, ruleItemToJson, scopeToJson(tree, root), facetSelectToJson,
  ruleFileFromJson.

Modifiés :
- `src/ifcviewer/AppSettings.{h,cpp}` — réglages `ruleengine/interpreter` et
  `ruleengine/script` (+ signaux). Vides par défaut = fonctions moteur
  désactivées.
- `modules/rule/Catalogue.{h,cpp}` — réécrit : RulePropertyType étendu
  (Text, Float, NullableFloat, Int, Bool, Enum, StringList, Direction),
  RulePropertyDef {name, type, default, nullable, enum_values, doc, unit},
  RuleDefinition {name (clé moteur), label (affichage), family, description,
  properties, slot_names}. Catalogue **statique complet** (miroir de
  registry.py) + `setCatalogueFromEngine()` (remplacement dynamique) +
  `resolveRuleDefinition()` (label↔name) + rôle `kNodeIdRole` (uuid).
- `modules/rule/RulePanel.{h,cpp}` — Save/Load émettent
  `saveRequested`/`loadRequested` (fini le « Not Working ») ; API de
  construction pour le Load (createRuleFileWithId, createFolderWithId,
  addRuleInstance, fillSlot) ; openRuleFile() ; résolution label→définition ;
  le presse-papier copie kNodeIdRole.
- `modules/rule/RuleLibraryPanel.{h,cpp}` — sections construites depuis le
  catalogue (familles dans l'ordre du registre) + `rebuildSections()`.
- `modules/rule/RuleConfigPanel.cpp` — éditeurs pour les nouveaux kinds
  (NullableFloat/Int nullable = champ texte vide = absent, Enum = combo
  éditable, StringList = une valeur/ligne, Direction = 3 spinbox) ;
  FacetValueEditor (restrictions complex) conservé tel quel.
- `modules/rule/ResultPanel.{h,cpp}` — ResultRow : -tolerance,
  +rule_type/measure/criticity/actor ; colonnes Rule et Measure ajoutées
  (8 colonnes, les statuts Valide/ND/Invalide décalés à 5-6-7).
- `modules/rule/ResultInfoPanel.cpp` — Measure/Criticity/Actor à la place
  de Tolerance.
- `MainWindow.{h,cpp}` — tout le run passe par EngineClient :
  setupEngineConnections (dans le constructeur), runGeometryRules(root,
  preview) sérialise via scopeToJson + chemins des modèles de session
  (loader->filePath, skippés si vides), collectEngineResults (mapping model
  index→session id + ElementRegistry → objects, accumulate rows + objets
  rouges/orange), finishGeometryRuleRun (couleurs, setResults, bascule
  Clash Filter), showEngineDisplay (overlays : zones magenta pointillé,
  OBB jaune, autres formes), showFacetSelection + Result (select IPC, cyan),
  saveRuleFile/loadRuleFile (.ifcrule.json). rule_worker_/thread supprimés.
- `modules/settings/Dialog.{h,cpp}` — champs Rule Engine Interpreter /
  Rule Engine Script dans l'onglet General.
- `src/bonsaiviewer/CMakeLists.txt` — engine/ retiré, engineproxy/ ajouté,
  lien `geometry_kernel_opencascade` retiré de BonsaiViewer.

## 3. Build — COMPILÉ AVEC SUCCÈS (2026-10-15)

- `build-qt612/` avait perdu son CMakeCache → reconfiguré :
  `cmake -S cmake -B build-qt612 -DCMAKE_BUILD_TYPE=Release
  -DBUILD_SHARED_LIBS=OFF -DQT_VERSION=6
  -DQt6_DIR=$HOME/Qt/6.12.0/gcc_64/lib/cmake/Qt6`
  (reconfigure = flags.make régénérés = full rebuild, c'est normal).
- **BonsaiViewer compile : [100%] Built target BonsaiViewer**
  (build-qt612/bonsaiviewer/BonsaiViewer, ~5.9 Mo). Logs :
  /tmp/bv_build5.log (zéro erreur).
- Erreurs corrigées en route (si elles réapparaissent, ce sont des
  classiques) :
  1. `row.source_session_model_id` au lieu de `result.…` (MainWindow) ;
  2. signaux saveRequested/loadRequested déplacés de RuleTreeWidget vers
     RulePanel ;
  3. `staticCatalogue()` déclaré/fini après les helpers + déclaration
     anticipée dans Catalogue.cpp (ordre de déclaration du namespace
     anonyme) ;
  4. **`slots` est une macro Qt** : ne JAMAIS nommer une variable/paramètre
     `slots` (renommés en slot_names / select_keys) ;
  5. RuleFileJson.h : forward-declare le VRAI RuleTreeWidget
     (`namespace bonsaiviewer::modules::rule { class RuleTreeWidget; }`),
     pas un `class RuleTreeWidget` local qui crée une classe ambiguë.
- Rebuild incrémental : `make -C build-qt612 BonsaiViewer -j6`
  (**-j6 demandé par l'utilisateur**). NE PAS supprimer
  build-qt612/CMakeCache.txt.

## 4. Comment tester une fois compilé

1. Lancer BonsaiViewer (build-qt612/bonsaiviewer/BonsaiViewer).
2. Settings > General :
   - Rule Engine Interpreter :
     `/home/jocelin/Documents/07 - Programmation/IfcOpenshell/Test_IfcOpenshell/.conda/bin/python`
   - Rule Engine Script :
     `/home/jocelin/Documents/05 - Programmation/IfcClash_Plus/ifcclash_plus/ipc.py`
3. Charger un modèle IFC, onglet GeometryRule : créer un RuleFile, glisser
   OBB Above + Entity Facets (IFCFURNISHINGELEMENT en source, IFCSLAB en
   target), Run Rule → les résultats arrivent dans Clash Filter (colonnes
   Rule/Measure), sources rouges/cibles orange. « Afficher le display
   Input » → zones/OBB en wireframe. Clic droit facette → sélection cyan.
4. Save/Load : .ifcrule.json lisible par la CLI
   (`load_from_json` de serialization.py) et vice-versa.
5. La Rule Library doit afficher les 4 familles (OneObjectRule,
   TwoObjectRule, ComplexRule, IDS Facet) avec le catalogue complet.

## 5. Validation de la session du 2026-10-15 (reprise)

- **Build revalidé** : `make -C build-qt612 BonsaiViewer -j6` →
  [100%] Built target BonsaiViewer, zéro erreur, rien à recompiler.
- **Contrat IPC validé de bout en bout** (driver JSON Lines gardant stdin
  ouvert — à l'EOF le moteur annule, il faut un driver type QProcess) :
  - `hello` → versions (ifcclash_plus 0.3.0, ifcopenshell 0.8.4.post1,
    ifctester 0.8.4, OCC 7.9.0) ; `catalogue` → 4 familles, 27 règles,
    6 facettes ;
  - `run` OBB_Above (IFCFURNISHINGELEMENT vs IFCSLAB, tolérance 0.85) sur
    Ifc_Model/Ifc2x3_Duplex_Architecture.ifc → **8 paires** avec cross-refs
    (model/step_id/global_id/name/ifc_type) et bloc info obbs
    (labels zone/object) ;
  - `preview` → **61 items OBB** ; `select` Entity IFCDOOR → **14 éléments**.
- **Round-trip viewer ↔ CLI validé** : un `.ifcrule.json` au format exact
  de `RuleFileJson.cpp` (RuleFile + viewer blocks + folder + OBB_Above avec
  facettes Entity + SelectRule imbriquée dans un slot + restriction
  Enumeration) est chargé par `load_from_json` (params, slots, restriction
  restaurés) puis exécuté via IPC → 8 paires (règle du dossier) + 20 paires
  (règle avec SelectRule), node_ids préservés (le viewer peut mapper les
  résultats sur ses nœuds).
- **Bug moteur corrigé** (`ifcclash_plus/ipc.py`) : le tuple `progress` de
  `_run_items` était remballé `(done, percent)` mais dépaqueté
  `(done, total)` → pourcentages absurdes dès la 2e règle (100, 2, 150…).
  Normalisé en `(done, total)` partout (règles, dossiers skippés, run_done).
  Revalidé : 50 puis 100, run_done percent=100.
- **Ruff appliqué** aux fichiers moteur (ipc.py, registry.py,
  result_export.py, display_export.py, serialization.py) : sélection
  IfcOpenShell (E9, UP, I, F401, FA, preview), 92 fixes + format 120
  colonnes (3 fichiers reformattés). Moteur revalidé après (run/preview/
  select identiques). Ruff 0.17.1 installé dans l'env conda de test.
- **Revue proxy** : `EngineClient` envoie bien la charge utile plate
  (`rule_file`/`list_ifc_path` à la racine du message — PAS de clé `job`),
  la réponse `catalogue` porte bien `rules` + `facets`, `Messages.cpp`
  parse le bloc `measure` de `ClashResultInfo.to_dict`
  (`{name: {value}}`) et les cross-refs à l'identique.
- **Bug viewer corrigé** : `setupEngineConnections()` était défini mais
  jamais appelé depuis le constructeur → `engine_client_` nul → Run Rule /
  facettes silencieux. Appel ajouté après `setupRibbon()`.
- **Évolutions UI (SPEC mise à jour par l'utilisateur)** : « Supprimer » au
  clic droit de tous les éléments de l'arbre (l'élément et ses fils ;
  supprimer le RuleFile vide l'arbre) ; « Afficher la sélection en lançant
  l'entity » vide la vue (`visibleObjectIds` → tous masqués) puis montre
  uniquement les éléments de la facette (fin de la coloration cyan) ;
  une IDS Facet ne peut atterrir que dans un slot (drop externe refusé
  ailleurs, drag interne limité au remplissage/remplacement de slots).

## 6. Reste à faire

- **Test manuel GUI end-to-end** (§4) : lancer le viewer et rejouer le
  scénario complet — seule la partie non automatisable restante.
- Phase 4 de la spec (avancé) : UI des exceptions/grouping/criticité/acteur/
  must-check/activation de dossiers (le moteur les exécute déjà si le JSON
  les porte — la serialization.py les sérialise — mais le viewer ne sait pas
  encore les éditer : ils partent absents). Le protocole est prêt.
- Phase 5 : export BCF (requete IPC à ajouter côté moteur + bouton viewer) ;
  les blocs info (points/faces/raycast/zone) sont transportés (result.info
  brut) mais Result Info ne les rend pas encore.
- Regrouper par criticité/règle dans Clash Filter (les colonnes existent).
- Moteur (prérechants Rapport.md §5) : run_actor crash, Same_Building/
  Storey/Space TypeError, règles ⚠️ (LateralSurface/ProjectedSurface coords
  locales, Above/Below, Ray_Check, OBB_Front_And_Back, ObbHigh axes) —
  l'IPC les fait déjà survivre par règle (log + run continue).
- Tests : moteur stub Python pour les tests C++ (phase 2 de la spec),
  round-trip contractuel viewer↔CLI automatisé (échantillon OBB_Above,
  Intersection, ClearanceForDoors).

## 7. Pièges connus / décisions de conception

- Le viewer envoie `list_ifc_path` = chemins des modèles de session
  (loader->filePath) ; l'index modèle des résultats est la position dans
  cette liste (run_model_ids_). Les modèles sans chemin sont skippés
  (comptés dans le message de fin).
- Un slot accepte une facette (→ SelectFacet) ou une règle (→ SelectRule
  imbriquée par valeur). Pas d'entrée « Select » explicite dans la
  bibliothèque (simplification par rapport à la spec §10.1).
- Les paramètres du catalogue portent les noms canoniques du moteur
  (tolerance, volume_min, predefinedType...) : la map de valeurs de
  l'instance sérialise directement ; seuls les attributs de facettes sont
  traduits (cf. RuleFileJson.cpp facetParamKey).
- « viewer » block JSON : extension viewer (label/description), ignorée par
  la CLI — round-trip Save/Load/CLI garanti.
- Annulation : `cancel` est lu entre deux règles (poll stdin non bloquant) ;
  EOF stdin = annulation (QProcess garde stdin ouvert en prod).
- Règle avec état Display_Input/Display_Result sérialisée : le moteur la
  joue en Final et exporte ses display_items → le viewer n'affiche pas les
  résultats de ces règles (displayRequested dans ipc.py).
- -j6 demandé par l'utilisateur pour make. Les erreurs « Processus arrêté
  signal terminated » dans les vieux logs = cc1plus tués lors de
  l'interruption du -j12 initial ; cibles recompilées au run suivant.
